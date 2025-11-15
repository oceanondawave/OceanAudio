#include "PluginListComponent.h"

namespace
{
constexpr int kHeaderHeight = 32;
}

PluginListComponent::PluginListComponent(PluginManager& manager)
    : pluginManager(manager)
{
    listBox.setModel(this);
    listBox.setRowHeight(26);
    addAndMakeVisible(listBox);

    addAndMakeVisible(rescanButton);
    rescanButton.onClick = [this]()
    {
        pluginManager.rescanDefaultDirectories();
    };

    addAndMakeVisible(addDirectoryButton);
    addDirectoryButton.onClick = [this]()
    {
        juce::FileChooser chooser("Select VST3 directory",
                                  juce::File::getSpecialLocation(juce::File::userHomeDirectory));
        if (chooser.browseForDirectory())
        {
            pluginManager.addCustomDirectory(chooser.getResult());
        }
    };

    pluginManager.addChangeListener(this);
    refreshList();
}

PluginListComponent::~PluginListComponent()
{
    pluginManager.removeChangeListener(this);
    listBox.setModel(nullptr);
}

void PluginListComponent::setSelectionCallback(PluginSelectionCallback callback)
{
    onSelection = std::move(callback);
}

void PluginListComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey.darker(0.3F));
    g.setColour(juce::Colours::white);
    g.setFont(16.0F);
    g.drawText("Available Plugins",
               12,
               4,
               getWidth() - 24,
               24,
               juce::Justification::centredLeft);
}

void PluginListComponent::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop(24); // header label space

    auto controls = area.removeFromTop(kHeaderHeight);
    rescanButton.setBounds(controls.removeFromRight(120).reduced(4));
    addDirectoryButton.setBounds(controls.removeFromRight(140).reduced(4));

    listBox.setBounds(area);
}

int PluginListComponent::getNumRows()
{
    return pluginManager.getKnownPluginList().getNumTypes();
}

void PluginListComponent::paintListBoxItem(int rowNumber,
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
        g.fillAll(juce::Colours::darkgrey.withAlpha(0.3F));
    }

    g.setColour(juce::Colours::white);
    g.setFont(14.0F);

    if (auto* type = pluginManager.getKnownPluginList().getType(rowNumber))
    {
        g.drawText(type->name, 12, 0, width - 24, height, juce::Justification::centredLeft);
    }
}

void PluginListComponent::listBoxItemClicked(int row, const juce::MouseEvent&)
{
    listBox.selectRow(row);
}

void PluginListComponent::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    if (auto* type = pluginManager.getKnownPluginList().getType(row))
    {
        if (onSelection != nullptr)
        {
            onSelection(*type);
        }
    }
}

juce::String PluginListComponent::getTooltipForRow(int row)
{
    if (auto* type = pluginManager.getKnownPluginList().getType(row))
    {
        return type->name + " (" + type->pluginFormatName + ")";
    }

    return {};
}

void PluginListComponent::changeListenerCallback(juce::ChangeBroadcaster*)
{
    refreshList();
}

void PluginListComponent::refreshList()
{
    listBox.updateContent();
    listBox.repaint();
}

