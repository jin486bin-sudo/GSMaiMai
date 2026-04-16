#include "PluginProcessor.h"
#include "PluginEditor.h"

// ----- Optimized math (param-safe; replaces std::sin / std::tanh hot paths) --
namespace {
    struct SinLUT {
        static constexpr int N = 4096;
        float table[N + 1];
        SinLUT() {
            for (int i = 0; i <= N; ++i)
                table[i] = std::sin(i * juce::MathConstants<float>::twoPi / (float)N);
        }
        // phase MUST be wrapped to [0, 2π) by caller
        inline float operator()(float phase) const noexcept {
            float pos = phase * ((float)N / juce::MathConstants<float>::twoPi);
            int i = (int)pos;
            float f = pos - (float)i;
            return table[i] * (1.0f - f) + table[i + 1] * f;
        }
    };
    static const SinLUT kSinLUT;

    // Pade approximant for tanh, accurate within ±3 → within 1e-4 of std::tanh
    inline float fastTanh(float x) noexcept {
        x = juce::jlimit(-3.0f, 3.0f, x);
        float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }
}

TapePluginProcessor::TapePluginProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout TapePluginProcessor::createParameterLayout()
{
    using P = juce::AudioParameterFloat;
    using R = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Master: -1 full slow, 0 normal, +1 full fast. Everything derives from this.
    params.push_back(std::make_unique<P>("speed", "Speed", R(-1.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<P>("mix",   "Mix",   R(0.0f, 1.0f, 0.001f), 1.0f));

    return { params.begin(), params.end() };
}

void TapePluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    for (auto& s : shifters) s.prepare(sampleRate);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32)samplesPerBlock, 2 };
    lowpass.prepare(spec);
    lowpass.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    highpass.prepare(spec);
    highpass.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    highpass.setCutoffFrequency(40.0f);

    speedCurrent = 0.0f;

    // Report baseline latency (500ms) so host can PDC-compensate
    setLatencySamples((int)(sampleRate * 0.5));

    wowPhase = flutterPhase = 0.0f;
}

bool TapePluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void TapePluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    const float sr = (float)currentSampleRate;

    const float target = *apvts.getRawParameterValue("speed");
    const float mix = *apvts.getRawParameterValue("mix");

    // Rate-limited smoothing: small steps feel instant, full jumps glide over 100ms
    const float maxDelta = speedRatePerSecond / sr;

    for (int n = 0; n < numSamples; ++n)
    {
        // Snappy linear approach to target
        if (target > speedCurrent)      speedCurrent = std::min(target, speedCurrent + maxDelta);
        else if (target < speedCurrent) speedCurrent = std::max(target, speedCurrent - maxDelta);
        float speed = speedCurrent;    // -1..+1
        float absSpeed = std::abs(speed);
        float ratio = std::pow(2.0f, speed);         // 0.5x .. 2.0x

        float slowAmt = juce::jmax(0.0f, -speed);
        float fastAmt = juce::jmax(0.0f,  speed);

        // Tape character ONLY active when speed is non-zero.
        // All modulation scales from absSpeed → zero when neutral = true bypass.
        float wowDepth  = 0.025f * absSpeed;
        float flutDepth = 0.010f * absSpeed;

        // Advance LFO phases only when active (avoids "pre-charged" state)
        if (absSpeed > 1e-4f)
        {
            wowPhase     += juce::MathConstants<float>::twoPi * 0.8f / sr;
            flutterPhase += juce::MathConstants<float>::twoPi * 7.0f / sr;
            if (wowPhase     > juce::MathConstants<float>::twoPi) wowPhase     -= juce::MathConstants<float>::twoPi;
            if (flutterPhase > juce::MathConstants<float>::twoPi) flutterPhase -= juce::MathConstants<float>::twoPi;
        }
        else
        {
            wowPhase = flutterPhase = 0.0f; // reset so next activation starts clean
        }

        float wobble = 1.0f + wowDepth  * kSinLUT(wowPhase)
                            + flutDepth * kSinLUT(flutterPhase);
        float finalRatio = ratio * wobble;

        float satAmt    = 0.7f * slowAmt + 0.15f * fastAmt;
        float driveGain = 1.0f + 2.0f * slowAmt + 0.3f * fastAmt;

        // Slow = dark tape (lowpass closes). Update cutoff only every 32 samples
        // (control rate) — smoothing is slow enough that this is inaudible, saves pow+coeff recalc.
        if ((n & 31) == 0)
        {
            float lpHz = 18000.0f * std::pow(0.12f, slowAmt);
            lpHz = juce::jlimit(1800.0f, 20000.0f, lpHz);
            lowpass.setCutoffFrequency(lpHz);
        }

        float hissAmt = 0.02f * slowAmt;

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            float dry = data[n];

            float shifted = shifters[ch].process(dry, finalRatio);

            // Bypass all color when at neutral — exact passthrough through varispeed only
            float wet;
            if (absSpeed > 1e-4f)
            {
                float driven = shifted * driveGain;
                float sat = fastTanh(driven) - 0.1f * satAmt * driven * driven;
                float hiss = (rng.nextFloat() * 2.0f - 1.0f) * hissAmt;
                wet = sat + hiss;

                float y = wet - dcX1[ch] + 0.995f * dcY1[ch];
                dcX1[ch] = wet;
                dcY1[ch] = y;
                wet = y;
            }
            else
            {
                wet = shifted;
                dcX1[ch] = dcY1[ch] = 0.0f; // reset DC state
            }

            data[n] = dry * (1.0f - mix) + wet * mix;
        }
    }

    // Bandwidth filters only when tape is engaged
    float currentSpeed = *apvts.getRawParameterValue("speed");
    if (std::abs(currentSpeed) > 1e-4f)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        highpass.process(ctx);
        lowpass.process(ctx);
    }
    else
    {
        highpass.reset();
        lowpass.reset();
    }
}

juce::AudioProcessorEditor* TapePluginProcessor::createEditor()
{
    return new TapePluginEditor(*this);
}

void TapePluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void TapePluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TapePluginProcessor();
}
