#include "PluginChainComponent.h"

namespace
{
constexpr int kControlHeight = 32;
}

PluginChainComponent::PluginChainComponent(AudioEngine& engine)
    : audioEngine(engine),
      listBox("PluginChain", this)
{
    listBox.setRowHeight(28);
    addAndMakeVisible(listBox);

    chainLabel.setText("Plugin Chain", juce::dontSendNotification);
    chainLabel.setJustificationType(juce::Justification::centredLeft);
    chainLabel.setFont(juce::Font(16.0F));
    addAndMakeVisible(chainLabel);

    removeButton.onClick = [this]()
    {
        removeSelectedPlugin();
    };
    addAndMakeVisible(removeButton);

    moveUpButton.onClick = [this]()
    {
        moveSelectedPlugin(-1);
    };
    addAndMakeVisible(moveUpButton);

    moveDownButton.onClick = [this]()
    {
        moveSelectedPlugin(1);
    };
    addAndMakeVisible(moveDownButton);
}

void PluginChainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey.darker(0.2F));
}

void PluginChainComponent::resized()
{
    auto area = getLocalBounds().reduced(4);
    auto header = area.removeFromTop(kControlHeight);
    chainLabel.setBounds(header.removeFromLeft(header.getWidth() - 240));

    auto buttonArea = header;
    removeButton.setBounds(buttonArea.removeFromRight(80).reduced(2));
    moveDownButton.setBounds(buttonArea.removeFromRight(80).reduced(2));
    moveUpButton.setBounds(buttonArea.removeFromRight(80).reduced(2));

    listBox.setBounds(area);
}

void PluginChainComponent::refresh()
{
    listBox.updateContent();
    listBox.repaint();
}

int PluginChainComponent::getNumRows()
{
    return static_cast<int>(audioEngine.getLoadedPluginNames().size());
}

void PluginChainComponent::paintListBoxItem(int rowNumber,
                                            juce::Graphics& g,
                                            int width,
                                            int height,
                                            bool rowIsSelected)
{
    if (rowIsSelected)
    {
        g.fillAll(juce::Colours::cornflowerblue.withAlpha(0.6F));
    }
    else if (rowNumber % 2 == 0)
    {
        g.fillAll(juce::Colours::darkgrey.withAlpha(0.2F));
    }

    g.setColour(juce::Colours::white);
    g.setFont(14.0F);

    const auto names = audioEngine.getLoadedPluginNames();
    if (juce::isPositiveAndBelow(rowNumber, names.size()))
    {
        g.drawText(names[rowNumber], 12, 0, width - 24, height, juce::Justification::centredLeft);
    }
}

void PluginChainComponent::listBoxItemClicked(int row, const juce::MouseEvent&)
{
    listBox.selectRow(row);
}

void PluginChainComponent::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    juce::ignoreUnused(row);
    removeSelectedPlugin();
}

void PluginChainComponent::moveRows(int startIndex, int rowsToMove, int newIndex)
{
    const int totalRows = getNumRows();
    if (rowsToMove <= 0 || startIndex < 0 || startIndex >= totalRows)
    {
        return;
    }

    if (rowsToMove > 1)
    {
        // Move blocks sequentially.
        for (int i = 0; i < rowsToMove; ++i)
        {
            moveRows(startIndex + i, 1, newIndex + i);
        }
        return;
    }

    if (newIndex > startIndex)
    {
        --newIndex;
    }

    const int clampedNewIndex = juce::jlimit(0, totalRows - 1, newIndex);
    const int delta = clampedNewIndex - startIndex;
    if (delta == 0)
    {
        return;
    }

    audioEngine.movePlugin(static_cast<size_t>(startIndex), delta);
    refresh();
    listBox.selectRow(clampedNewIndex);
}

void PluginChainComponent::removeSelectedPlugin()
{
    const auto selected = listBox.getSelectedRow();
    if (selected < 0)
    {
        return;
    }

    audioEngine.removePlugin(static_cast<size_t>(selected));
    refresh();
}

void PluginChainComponent::moveSelectedPlugin(int delta)
{
    const auto selected = listBox.getSelectedRow();
    if (selected < 0)
    {
        return;
    }

    audioEngine.movePlugin(static_cast<size_t>(selected), delta);
    const auto newIndex = juce::jlimit(0, getNumRows() - 1, selected + delta);
    refresh();
    listBox.selectRow(newIndex);
}

