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

    // True varispeed: pitch AND tempo coupled like real tape.
    // Read pointer lags write by baselineGap samples → FAST has runway to eat into.
    // Host PDC compensates for this baseline latency automatically.
    struct Varispeed
    {
        std::vector<float> buffer;
        int size = 0;
        int baselineGap = 0;
        long long writeCount = 0;
        double readCount = 0.0;

        void prepare(double sr)
        {
            size = (int)(sr * 4.0);              // 4-second capture buffer
            baselineGap = (int)(sr * 0.5);        // 500ms lead → ~1s of 2x fast runway
            buffer.assign(size, 0.0f);
            writeCount = 0;
            readCount = -(double)baselineGap;    // start with baseline gap
        }

        float process(float in, float ratio)
        {
            buffer[(int)(writeCount % size)] = in;

            // Clamp read position: must stay behind write head and within buffer
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

        // Snap back to baseline lead. Used when STOP pressed.
        void resync()
        {
            readCount = (double)writeCount - (double)baselineGap;
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

private:
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TapePluginProcessor)
};
