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

    // Dual-head crossfading ring-buffer pitch shifter.
    // Zero latency, no baseline gap → automation aligns perfectly with timeline.
    // Tape tempo change isn't simulated (physically impossible in a realtime plugin
    // without adding latency that breaks DAW sync) — only pitch shift + tape color.
    struct PitchShifter
    {
        std::vector<float> buffer;
        int size = 0;
        int writeIdx = 0;
        float read1 = 0.0f;
        float read2 = 0.0f;

        void prepare(double sr)
        {
            size = (int)(sr * 0.08);  // 80ms grain buffer
            buffer.assign(size, 0.0f);
            writeIdx = 0;
            read1 = 0.0f;
            read2 = size * 0.5f;
        }

        float process(float in, float ratio)
        {
            buffer[writeIdx] = in;

            auto interp = [&](float pos) {
                while (pos < 0) pos += size;
                while (pos >= size) pos -= size;
                int i = (int)pos;
                float f = pos - (float)i;
                int j = (i + 1) % size;
                return buffer[i] * (1.0f - f) + buffer[j] * f;
            };

            auto dist = [&](float pos) {
                float d = (float)writeIdx - pos;
                while (d < 0) d += size;
                while (d >= size) d -= size;
                return d;
            };

            auto fade = [&](float d) {
                float phase = d / (float)size * juce::MathConstants<float>::pi;
                return std::sin(phase);
            };

            float s1 = interp(read1) * fade(dist(read1));
            float s2 = interp(read2) * fade(dist(read2));

            read1 += ratio;
            read2 += ratio;
            while (read1 >= size) read1 -= size;
            while (read2 >= size) read2 -= size;
            while (read1 < 0) read1 += size;
            while (read2 < 0) read2 += size;

            writeIdx = (writeIdx + 1) % size;
            return s1 + s2;
        }

        void resync() {} // no-op; kept for API compatibility
    };

    std::array<PitchShifter, 2> shifters;

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
