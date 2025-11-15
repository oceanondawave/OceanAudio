#pragma once

#include "CoreGateProcessor.h"

class CoreGateEditor final : public juce::AudioProcessorEditor
{
public:
    explicit CoreGateEditor(CoreGateProcessor& processor);
    ~CoreGateEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    CoreGateProcessor& processorRef;
    juce::Slider thresholdSlider;
    juce::Slider holdSlider;
    juce::Slider attackSlider;
    juce::Slider releaseSlider;
    juce::Label thresholdLabel;
    juce::Label holdLabel;
    juce::Label attackLabel;
    juce::Label releaseLabel;

    juce::AudioProcessorValueTreeState::SliderAttachment thresholdAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment holdAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment attackAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment releaseAttachment;
};

