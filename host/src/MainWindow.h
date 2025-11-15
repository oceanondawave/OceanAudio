#include <memory>

class PluginListComponent;
class PluginChainComponent;
#pragma once

#include "AudioEngine.h"

#include <juce_gui_basics/juce_gui_basics.h>

class MainWindow final : public juce::DocumentWindow
{
public:
    explicit MainWindow(const juce::String& name);
    ~MainWindow() override;

    void closeButtonPressed() override;

private:
    class RootComponent final : public juce::Component,
                                private juce::Timer,
                                private juce::ListBoxModel
    {
    public:
        explicit RootComponent(AudioEngine& engine);
        ~RootComponent() override;

        void resized() override;

    private:
        void timerCallback() override;
        int getNumRows() override;
        void paintListBoxItem(int rowNumber,
                              juce::Graphics& g,
                              int width,
                              int height,
                              bool rowIsSelected) override;
        void listBoxItemClicked(int row, const juce::MouseEvent&) override;
        void updatePresetList();
        void updatePresetButtons();
        bool isFactoryPreset(int row) const;
        int getUserPresetIndex(int row) const;

        AudioEngine& audioEngine;
        juce::Label statusLabel;
        juce::TextButton openPrefsButton;
        std::unique_ptr<class PluginListComponent> pluginListComponent;
        std::unique_ptr<class PluginChainComponent> pluginChainComponent;
        juce::Label presetLabel;
        juce::ListBox presetListBox;
        juce::TextButton savePresetButton {"Save Preset"};
        juce::TextButton renamePresetButton {"Rename Preset"};
        juce::TextButton deletePresetButton {"Delete Preset"};
        int lastPresetCount = -1;
        bool ignorePresetSelectionChange = false;
    };

    AudioEngine audioEngine;
};

