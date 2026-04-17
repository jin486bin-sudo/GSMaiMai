#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class TapePluginProcessor : public juce::AudioProcessor
{
public:
    TapePluginProcessor();
    ~TapePluginProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "GSMaiMai"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.2; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Varispeed ring buffer: pitch AND tempo couple like real tape.
    // NO baseline latency reported (setLatencySamples(0)). Read head starts at
    // write head → zero delay when speed=0. Slowing grows the gap (stretch);
    // output naturally drifts behind timeline — that IS the tape effect.
    // STOP button resyncs.
    struct Varispeed
    {
        std::vector<float> buffer;
        int size = 0;
        long long writeCount = 0;
        double readCount = 0.0;

        void prepare(double sr)
        {
            size = (int)(sr * 30.0); // 30-second buffer: MAX SLOW lasts ~60 sec
            buffer.assign(size, 0.0f);
            writeCount = 0;
            readCount = 0.0; // no baseline gap
        }

        float process(float in, float ratio)
        {
            buffer[(int)(writeCount % size)] = in;

            double maxRead = (double)writeCount - 1.0;
            double minRead = (double)writeCount - (double)(size - 2);
            if (readCount > maxRead) readCount = maxRead;
            if (readCount < minRead) readCount = minRead;

            long long i = (long long)std::floor(readCount);
            float f = (float)(readCount - (double)i);
            int idx0 = (int)(((i % size) + size) % size);
            int idx1 = (int)((((i + 1) % size) + size) % size);
            float out = buffer[idx0] * (1.0f - f) + buffer[idx1] * f;

            readCount += ratio;
            ++writeCount;
            return out;
        }

        // For bypass decision: how far behind live is the read head?
        double currentGap() const noexcept
        {
            return (double)writeCount - 1.0 - readCount;
        }

        // Keep writing input so the buffer stays fresh during bypass,
        // without consuming the read pointer.
        void writeOnly(float in)
        {
            buffer[(int)(writeCount % size)] = in;
            readCount = (double)writeCount; // keep gap at zero
            ++writeCount;
        }

        void resync()
        {
            readCount = (double)writeCount - 1.0;
        }
    };

    std::array<Varispeed, 2> shifters;

    float wowPhase = 0.0f;
    float flutterPhase = 0.0f;

    juce::dsp::StateVariableTPTFilter<float> lowpass;
    juce::dsp::StateVariableTPTFilter<float> highpass;

    std::array<float, 2> dcX1 { 0.0f, 0.0f };
    std::array<float, 2> dcY1 { 0.0f, 0.0f };

    // Rate-limited smoothing: full-range jumps glide (whoosh), small steps are snappy
    float speedCurrent = 0.0f;
    float speedRatePerSecond = 20.0f; // units/sec; full ±1 jump = 100ms glide

    juce::Random rng;

public:
    void resyncTape() { for (auto& s : shifters) s.resync(); }
    bool isEffectExhausted() const
    {
        float spd = *apvts.getRawParameterValue("speed");
        if (spd > 0.01f  && shifters[0].currentGap() < 2.0)  return true;  // FAST caught up
        if (spd < -0.01f && shifters[0].currentGap() > (double)(shifters[0].size - 100)) return true; // SLOW filled buffer
        return false;
    }

private:
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TapePluginProcessor)
};
