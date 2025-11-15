#pragma once

#include "PluginManager.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class PluginListComponent final : public juce::Component,
                                  private juce::ListBoxModel,
                                  private juce::ChangeListener
{
public:
    using PluginSelectionCallback = std::function<void(const juce::PluginDescription&)>;

    explicit PluginListComponent(PluginManager& manager);
    ~PluginListComponent() override;

    void setSelectionCallback(PluginSelectionCallback callback);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    int getNumRows() override;
    void paintListBoxItem(int rowNumber,
                          juce::Graphics& g,
                          int width,
                          int height,
                          bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent&) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;
    juce::String getTooltipForRow(int row) override;

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void refreshList();

    PluginManager& pluginManager;
    juce::ListBox listBox;
    juce::TextButton rescanButton {"Rescan"};
    juce::TextButton addDirectoryButton {"Add Directory"};
    PluginSelectionCallback onSelection;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginListComponent)
};

