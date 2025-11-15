#pragma once

#include "CoreCompressorProcessor.h"

class CoreCompressorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit CoreCompressorEditor(CoreCompressorProcessor& processor);
    ~CoreCompressorEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    CoreCompressorProcessor& processorRef;
    juce::Slider thresholdSlider;
    juce::Slider ratioSlider;
    juce::Slider attackSlider;
    juce::Slider releaseSlider;
    juce::Label thresholdLabel;
    juce::Label ratioLabel;
    juce::Label attackLabel;
    juce::Label releaseLabel;

    juce::AudioProcessorValueTreeState::SliderAttachment thresholdAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment ratioAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment attackAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment releaseAttachment;
};

