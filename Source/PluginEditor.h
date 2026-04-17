#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class TapePluginEditor : public juce::AudioProcessorEditor,
                         private juce::Timer
{
public:
    explicit TapePluginEditor(TapePluginProcessor&);
    ~TapePluginEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void applyStep(int delta);

    TapePluginProcessor& proc;

    juce::Rectangle<int> minBtn, stepDownBtn, stopBtn, stepUpBtn;
    int pressedBtn = -1; // 0=min 1=stepDown 2=stop 3=stepUp
    float reelPhase = 0.0f;
    int holdFrames = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TapePluginEditor)
};
