#include "MainWindow.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include "PluginChainComponent.h"
#include "PluginListComponent.h"

namespace
{
constexpr int kStatusTimerMs = 500;
}

MainWindow::MainWindow(const juce::String& name)
    : juce::DocumentWindow(name,
                           juce::Desktop::getInstance().getDefaultLookAndFeel()
                               .findColour(juce::ResizableWindow::backgroundColourId),
                           juce::DocumentWindow::allButtons),
      audioEngine()
{
    setContentOwned(new RootComponent(audioEngine), true);
    setUsingNativeTitleBar(true);
    setResizable(true, true);
}

MainWindow::~MainWindow() = default;

void MainWindow::closeButtonPressed()
{
    JUCEApplication::getInstance()->systemRequestedQuit();
}

MainWindow::RootComponent::RootComponent(AudioEngine& engine)
    : audioEngine(engine)
{
    addAndMakeVisible(statusLabel);
    statusLabel.setJustificationType(juce::Justification::centredLeft);
    statusLabel.setFont(juce::Font(16.0F));

    addAndMakeVisible(openPrefsButton);
    openPrefsButton.setButtonText("Audio Preferences");
    openPrefsButton.onClick = [this]()
    {
        audioEngine.openDeviceSettings();
    };

    pluginListComponent = std::make_unique<PluginListComponent>(audioEngine.getPluginManager());
    pluginListComponent->setSelectionCallback([this](const juce::PluginDescription& description)
    {
        juce::String error;
        if (audioEngine.addPlugin(description, error))
        {
            if (pluginChainComponent != nullptr)
            {
                pluginChainComponent->refresh();
            }
        }
        else if (error.isNotEmpty())
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                   "Plugin Load Failed",
                                                   error);
        }
    });
    addAndMakeVisible(*pluginListComponent);

    pluginChainComponent = std::make_unique<PluginChainComponent>(audioEngine);
    addAndMakeVisible(*pluginChainComponent);

    presetListBox.setModel(this);
    presetListBox.setRowHeight(24);
    presetListBox.setClickingTogglesRowSelection(true);
    addAndMakeVisible(presetLabel);
    presetLabel.setText("Presets", juce::dontSendNotification);
    presetLabel.setJustificationType(juce::Justification::centredLeft);
    presetLabel.setFont(juce::Font(15.0F));

    addAndMakeVisible(presetListBox);
    addAndMakeVisible(savePresetButton);
    addAndMakeVisible(renamePresetButton);
    addAndMakeVisible(deletePresetButton);

    savePresetButton.onClick = [this]()
    {
        juce::AlertWindow dialog("Save Preset",
                                 "Enter a name for the current plugin chain:",
                                 juce::AlertWindow::NoIcon);
        dialog.addTextEditor("name", "", "Preset Name:");
        dialog.addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
        dialog.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

        if (dialog.runModalLoop() == 1)
        {
            const auto name = dialog.getTextEditorContents("name").trim();
            if (name.isEmpty())
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                       "Invalid Preset Name",
                                                       "Preset name cannot be empty.");
                return;
            }

            juce::String error;
            if (!audioEngine.saveCurrentChainAsPreset(name, error))
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                       "Preset Save Failed",
                                                       error.isNotEmpty() ? error
                                                                           : "Unable to save preset.");
            }
            else
            {
                updatePresetList();
                const int lastIndex = audioEngine.getPresets().size() - 1;
                if (lastIndex >= 0)
                {
                    juce::ScopedValueSetter<bool> guard(ignorePresetSelectionChange, true);
                    presetListBox.selectRow(lastIndex);
                }
                updatePresetButtons();
            }
        }
    };

    renamePresetButton.onClick = [this]()
    {
        const auto selected = presetListBox.getSelectedRow();
        if (!juce::isPositiveAndBelow(selected, audioEngine.getPresets().size()))
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                   "Rename Preset",
                                                   "Select a preset to rename.");
            return;
        }

        if (isFactoryPreset(selected))
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                   "Rename Preset",
                                                   "Factory presets cannot be renamed.");
            return;
        }

        const auto& preset = audioEngine.getPresets().getReference(selected);

        juce::AlertWindow dialog("Rename Preset",
                                 "Enter a new name for this preset:",
                                 juce::AlertWindow::NoIcon);
        dialog.addTextEditor("name", preset.name, "Preset Name:");
        dialog.addButton("Rename", 1, juce::KeyPress(juce::KeyPress::returnKey));
        dialog.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

        if (dialog.runModalLoop() == 1)
        {
            const auto name = dialog.getTextEditorContents("name").trim();
            if (name.isEmpty())
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                       "Invalid Preset Name",
                                                       "Preset name cannot be empty.");
                return;
            }

            const int userIndex = getUserPresetIndex(selected);
            if (userIndex < 0)
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                       "Rename Preset",
                                                       "Unable to locate preset entry.");
                return;
            }

            PresetManager::ChainPreset updatedPreset = audioEngine.getPresets().getReference(selected);
            updatedPreset.name = name;

            juce::String error;
            if (!audioEngine.updateUserPreset(userIndex, updatedPreset, error))
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                       "Preset Rename Failed",
                                                       error.isNotEmpty() ? error : "Unable to rename preset.");
                return;
            }

            updatePresetList();
            updatePresetButtons();
        }
    };

    deletePresetButton.onClick = [this]()
    {
        const auto selected = presetListBox.getSelectedRow();
        if (!juce::isPositiveAndBelow(selected, audioEngine.getPresets().size()))
        {
            return;
        }

        if (isFactoryPreset(selected))
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                   "Delete Preset",
                                                   "Factory presets cannot be deleted.");
            return;
        }

        if (!juce::AlertWindow::showOkCancelBox(juce::AlertWindow::WarningIcon,
                                                "Delete Preset",
                                                "Are you sure you want to delete this preset?",
                                                "Delete",
                                                "Cancel"))
        {
            return;
        }

        const int userIndex = getUserPresetIndex(selected);
        if (userIndex < 0)
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                   "Preset Delete Failed",
                                                   "Unable to locate preset entry.");
            return;
        }
        juce::String error;
        if (!audioEngine.removeUserPreset(userIndex, error))
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                   "Preset Delete Failed",
                                                   error.isNotEmpty() ? error : "Unable to delete preset.");
            return;
        }

        presetListBox.deselectAllRows();
        updatePresetList();
        updatePresetButtons();
    };

    updatePresetList();
    updatePresetButtons();

    startTimer(kStatusTimerMs);
}

MainWindow::RootComponent::~RootComponent()
{
    stopTimer();
    presetListBox.setModel(nullptr);
}

void MainWindow::RootComponent::resized()
{
    auto area = getLocalBounds().reduced(16);
    statusLabel.setBounds(area.removeFromTop(24));
    area.removeFromTop(12);
    openPrefsButton.setBounds(area.removeFromTop(32).removeFromLeft(200));

    area.removeFromTop(12);
    auto contentArea = area;
    auto left = contentArea.removeFromLeft(contentArea.getWidth() * 0.45F);
    auto middle = contentArea.removeFromLeft(contentArea.getWidth() * 0.55F);
    auto right = contentArea;

    left.removeFromRight(8);
    middle.removeFromRight(8);

    if (pluginListComponent != nullptr)
    {
        pluginListComponent->setBounds(left);
    }

    if (pluginChainComponent != nullptr)
    {
        pluginChainComponent->setBounds(middle);
    }

    presetLabel.setBounds(right.removeFromTop(24));
    savePresetButton.setBounds(right.removeFromTop(32).reduced(0, 4));
    renamePresetButton.setBounds(right.removeFromTop(32).reduced(0, 4));
    deletePresetButton.setBounds(right.removeFromTop(32).reduced(0, 4));
    presetListBox.setBounds(right);
}

int MainWindow::RootComponent::getNumRows()
{
    return audioEngine.getPresets().size();
}

void MainWindow::RootComponent::paintListBoxItem(int rowNumber,
                                                 juce::Graphics& g,
                                                 int width,
                                                 int height,
                                                 bool rowIsSelected)
{
    if (rowIsSelected)
    {
        g.fillAll(juce::Colours::cadetblue.withAlpha(0.6F));
    }
    else if (rowNumber % 2 == 0)
    {
        g.fillAll(juce::Colours::darkgrey.withAlpha(0.2F));
    }

    g.setFont(14.0F);

    const auto& presets = audioEngine.getPresets();
    if (juce::isPositiveAndBelow(rowNumber, presets.size()))
    {
        const auto textColour = presets[rowNumber].isFactory ? juce::Colours::lightgrey : juce::Colours::white;
        g.setColour(textColour);
        g.drawText(presets.getReference(rowNumber).name,
                   8,
                   0,
                   width - 16,
                   height,
                   juce::Justification::centredLeft);
    }
}

void MainWindow::RootComponent::timerCallback()
{
    statusLabel.setText(audioEngine.getStatusText(), juce::dontSendNotification);
    if (pluginChainComponent != nullptr)
    {
        pluginChainComponent->refresh();
    }

    updatePresetList();
}

void MainWindow::RootComponent::listBoxItemClicked(int row, const juce::MouseEvent&)
{
    if (ignorePresetSelectionChange)
    {
        return;
    }

    if (juce::isPositiveAndBelow(row, audioEngine.getPresets().size()))
    {
        juce::String error;
        if (!audioEngine.applyPreset(audioEngine.getPresets().getReference(row), error)
            && error.isNotEmpty())
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                   "Preset Load Failed",
                                                   error);
        }
        else if (pluginChainComponent != nullptr)
        {
            pluginChainComponent->refresh();
        }
    }

    updatePresetButtons();
}

void MainWindow::RootComponent::updatePresetList()
{
    const int presetCount = audioEngine.getPresets().size();
    const int previousSelection = presetListBox.getSelectedRow();

    if (presetCount != lastPresetCount)
    {
        lastPresetCount = presetCount;
        const int newSelection = juce::jlimit(-1, presetCount - 1, previousSelection);

        {
            juce::ScopedValueSetter<bool> guard(ignorePresetSelectionChange, true);
            presetListBox.updateContent();

            if (presetCount == 0 || newSelection < 0)
            {
                presetListBox.deselectAllRows();
            }
            else
            {
                presetListBox.selectRow(newSelection);
            }
        }
    }

    presetListBox.repaint();
    updatePresetButtons();
}

void MainWindow::RootComponent::updatePresetButtons()
{
    const int selected = presetListBox.getSelectedRow();
    const bool canDelete = juce::isPositiveAndBelow(selected, audioEngine.getPresets().size())
        && !isFactoryPreset(selected);
    const bool canRename = canDelete;

    deletePresetButton.setEnabled(canDelete);
    renamePresetButton.setEnabled(canRename);
}

bool MainWindow::RootComponent::isFactoryPreset(int row) const
{
    const auto& presets = audioEngine.getPresets();
    if (!juce::isPositiveAndBelow(row, presets.size()))
    {
        return false;
    }

    return presets[row].isFactory;
}

int MainWindow::RootComponent::getUserPresetIndex(int row) const
{
    const auto& presets = audioEngine.getPresets();
    int userIndex = 0;

    for (int i = 0; i < presets.size(); ++i)
    {
        if (!presets[i].isFactory)
        {
            if (i == row)
            {
                return userIndex;
            }

            ++userIndex;
        }
    }

    return -1;
}

