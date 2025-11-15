#pragma once

#include "BridgeClient.h"
#include "PluginChain.h"
#include "PluginManager.h"
#include "PresetManager.h"

#include <juce_audio_utils/juce_audio_utils.h>

class AudioEngine final : private juce::AudioIODeviceCallback
{
public:
    AudioEngine();
    ~AudioEngine() override;

    void openDeviceSettings();
    juce::String getStatusText() const;

    void prepareForVirtualOutput();
    PluginManager& getPluginManager();

    bool addPlugin(const juce::PluginDescription& description, juce::String& errorMessage);
    void removePlugin(size_t index);
    void movePlugin(size_t index, int delta);
    juce::StringArray getLoadedPluginNames() const;

    const juce::Array<PresetManager::ChainPreset>& getPresets() const;
    bool applyPreset(const PresetManager::ChainPreset& preset, juce::String& errorMessage);
    bool saveCurrentChainAsPreset(const juce::String& presetName, juce::String& errorMessage);
    bool removeUserPreset(int userIndex, juce::String& errorMessage);
    bool updateUserPreset(int userIndex, const PresetManager::ChainPreset& preset, juce::String& errorMessage);

private:
    PresetManager::ChainPreset createPresetFromCurrentChain(const juce::String& presetName) const;

    void audioDeviceIOCallback(const float* const* inputChannelData,
                               int numInputChannels,
                               float* const* outputChannelData,
                               int numOutputChannels,
                               int numSamples) override;
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    juce::AudioDeviceManager deviceManager;
    juce::AudioProcessorPlayer processorPlayer;
    PluginChain pluginChain;
    PluginManager pluginManager;
    PresetManager presetManager;
    BridgeClient bridgeClient;
    juce::String lastStatus;
};

