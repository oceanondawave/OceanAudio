#pragma once

#include <juce_core/juce_core.h>

class PresetManager
{
public:
    struct PluginPreset
    {
        juce::String pluginId;
        juce::String pluginName;
        juce::MemoryBlock state;
    };

    struct ChainPreset
    {
        juce::String name;
        juce::Array<PluginPreset> plugins;
    bool isFactory = false;
    };

    PresetManager();
    ~PresetManager() = default;

    void loadFactoryPresets();
    void loadUserPresets();

    const juce::Array<ChainPreset>& getPresets() const noexcept;
    bool saveUserPreset(const ChainPreset& preset);
    bool removeUserPreset(int index);
    bool updateUserPreset(int index, const ChainPreset& preset);

private:
    ChainPreset parsePreset(const juce::var& presetVar) const;
    juce::File getUserPresetFile() const;
    juce::Array<ChainPreset> readUserPresetsFromFile() const;
    bool writeUserPresetsToFile(const juce::Array<ChainPreset>& userPresets) const;
    void replaceUserPresets(const juce::Array<ChainPreset>& userPresets);

    juce::Array<ChainPreset> presets;
};

