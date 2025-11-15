#include "CoreGateEditor.h"

namespace
{
constexpr int kEditorWidth = 420;
constexpr int kEditorHeight = 240;
constexpr const char* kParamThreshold = "threshold";
constexpr const char* kParamHold = "hold";
constexpr const char* kParamAttack = "attack";
constexpr const char* kParamRelease = "release";
}

CoreGateEditor::CoreGateEditor(CoreGateProcessor& processor)
    : juce::AudioProcessorEditor(processor),
      processorRef(processor),
      thresholdAttachment(processorRef.getValueTreeState(), kParamThreshold, thresholdSlider),
      holdAttachment(processorRef.getValueTreeState(), kParamHold, holdSlider),
      attackAttachment(processorRef.getValueTreeState(), kParamAttack, attackSlider),
      releaseAttachment(processorRef.getValueTreeState(), kParamRelease, releaseSlider)
{
    setSize(kEditorWidth, kEditorHeight);

    auto configureSlider = [](juce::Slider& slider, const juce::String& suffix)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextValueSuffix(suffix);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    };

    configureSlider(thresholdSlider, " dB");
    configureSlider(holdSlider, " ms");
    configureSlider(attackSlider, " ms");
    configureSlider(releaseSlider, " ms");

    thresholdLabel.setText("Threshold", juce::dontSendNotification);
    holdLabel.setText("Hold", juce::dontSendNotification);
    attackLabel.setText("Attack", juce::dontSendNotification);
    releaseLabel.setText("Release", juce::dontSendNotification);

    for (auto* slider : {&thresholdSlider, &holdSlider, &attackSlider, &releaseSlider})
    {
        addAndMakeVisible(slider);
    }

    for (auto* label : {&thresholdLabel, &holdLabel, &attackLabel, &releaseLabel})
    {
        label->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
    }
}

CoreGateEditor::~CoreGateEditor() = default;

void CoreGateEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkslategrey);
}

void CoreGateEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    auto labelHeight = 24;
    auto sliderHeight = 100;

    auto row = area.removeFromTop(labelHeight);
    thresholdLabel.setBounds(row.removeFromLeft(row.getWidth() / 4));
    holdLabel.setBounds(row.removeFromLeft(row.getWidth() / 3));
    attackLabel.setBounds(row.removeFromLeft(row.getWidth() / 2));
    releaseLabel.setBounds(row);

    area.removeFromTop(10);
    auto sliderRow = area.removeFromTop(sliderHeight);

    auto quarterWidth = sliderRow.getWidth() / 4;
    thresholdSlider.setBounds(sliderRow.removeFromLeft(quarterWidth));
    holdSlider.setBounds(sliderRow.removeFromLeft(quarterWidth));
    attackSlider.setBounds(sliderRow.removeFromLeft(quarterWidth));
    releaseSlider.setBounds(sliderRow);
}

