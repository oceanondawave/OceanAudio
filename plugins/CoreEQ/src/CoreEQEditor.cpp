#include "CoreEQEditor.h"

namespace
{
constexpr int kEditorWidth = 360;
constexpr int kEditorHeight = 180;
constexpr const char* kParamLowShelfGainId = "lowShelfGain";
constexpr const char* kParamHighShelfGainId = "highShelfGain";
}

CoreEQEditor::CoreEQEditor(CoreEQProcessor& processor)
    : juce::AudioProcessorEditor(processor),
      processorRef(processor),
      lowShelfAttachment(processorRef.getValueTreeState(), kParamLowShelfGainId, lowShelfSlider),
      highShelfAttachment(processorRef.getValueTreeState(), kParamHighShelfGainId, highShelfSlider)
{
    setSize(kEditorWidth, kEditorHeight);

    for (auto* slider : {&lowShelfSlider, &highShelfSlider})
    {
        addAndMakeVisible(slider);
        slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        slider->setRange(-12.0, 12.0, 0.1);
    }

    lowShelfLabel.setText("Low Shelf (dB)", juce::dontSendNotification);
    highShelfLabel.setText("High Shelf (dB)", juce::dontSendNotification);

    for (auto* label : {&lowShelfLabel, &highShelfLabel})
    {
        addAndMakeVisible(label);
        label->setJustificationType(juce::Justification::centred);
    }
}

CoreEQEditor::~CoreEQEditor() = default;

void CoreEQEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
}

void CoreEQEditor::resized()
{
    auto area = getLocalBounds().reduced(20);

    auto topRow = area.removeFromTop(60);
    lowShelfLabel.setBounds(topRow.removeFromLeft(topRow.getWidth() / 2));
    highShelfLabel.setBounds(topRow);

    area.removeFromTop(10);
    auto sliderRow = area.removeFromTop(80);
    lowShelfSlider.setBounds(sliderRow.removeFromLeft(sliderRow.getWidth() / 2).withTrimmedBottom(10));
    highShelfSlider.setBounds(sliderRow.withTrimmedBottom(10));
}

