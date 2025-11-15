#pragma once

#include "CoreEQProcessor.h"

#include <juce_gui_extra/juce_gui_extra.h>

class CoreEQEditor final : public juce::AudioProcessorEditor
{
public:
    explicit CoreEQEditor(CoreEQProcessor& processor);
    ~CoreEQEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    CoreEQProcessor& processorRef;
    juce::Slider lowShelfSlider;
    juce::Slider highShelfSlider;
    juce::Label lowShelfLabel;
    juce::Label highShelfLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment lowShelfAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment highShelfAttachment;
};

