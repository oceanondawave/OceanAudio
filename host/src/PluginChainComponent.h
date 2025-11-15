#pragma once

#include "AudioEngine.h"

#include <juce_gui_basics/juce_gui_basics.h>

class PluginChainComponent final : public juce::Component,
                                   private juce::ReorderableListBoxModel
{
public:
    explicit PluginChainComponent(AudioEngine& engine);
    ~PluginChainComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void refresh();

private:
    int getNumRows() override;
    void paintListBoxItem(int rowNumber,
                          juce::Graphics& g,
                          int width,
                          int height,
                          bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent&) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;
    void moveRows(int startIndex, int rowsToMove, int newIndex) override;

    void removeSelectedPlugin();
    void moveSelectedPlugin(int delta);

    AudioEngine& audioEngine;
    juce::ReorderableListBox listBox;
    juce::TextButton removeButton {"Remove"};
    juce::TextButton moveUpButton {"Move Up"};
    juce::TextButton moveDownButton {"Move Down"};
    juce::Label chainLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginChainComponent)
};

