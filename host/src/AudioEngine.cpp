#include "AudioEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

namespace
{
constexpr double kDefaultSampleRate = 48000.0;
constexpr int kDefaultBufferSize = 256;

juce::String createIdentifierString(const juce::PluginDescription& description)
{
    return juce::PluginDescription::createIdentifierString(description.pluginFormatName,
                                                           description.name,
                                                           description.version,
                                                           description.fileOrIdentifier);
}
}

AudioEngine::AudioEngine()
{
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    setup.bufferSize = kDefaultBufferSize;
    setup.sampleRate = kDefaultSampleRate;

    deviceManager.initialiseWithDefaultDevices(2, 2);
    deviceManager.addAudioCallback(this);

    processorPlayer.setProcessor(pluginChain.getProcessor());
    deviceManager.addAudioCallback(&processorPlayer);

    pluginChain.initialiseDefaultChain();
    pluginManager.initialise();
    presetManager.loadFactoryPresets();
    presetManager.loadUserPresets();
    prepareForVirtualOutput();

    lastStatus = "Audio engine initialised";
}

AudioEngine::~AudioEngine()
{
    deviceManager.removeAudioCallback(&processorPlayer);
    deviceManager.removeAudioCallback(this);
    processorPlayer.setProcessor(nullptr);
    pluginManager.shutdown();
}

void AudioEngine::openDeviceSettings()
{
    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = "Audio Device Settings";
    options.dialogBackgroundColour = juce::Colours::black;
    options.escapeKeyTriggersClose = true;
    options.useNativeTitleBar = true;
    options.content.setOwned(new juce::AudioDeviceSelectorComponent(deviceManager,
                                                                    1,
                                                                    2,
                                                                    1,
                                                                    2,
                                                                    true,
                                                                    true,
                                                                    true,
                                                                    false),
                             true);
    options.content->setSize(600, 400);
    options.launchAsync();
}

juce::String AudioEngine::getStatusText() const
{
    return lastStatus;
}

void AudioEngine::prepareForVirtualOutput()
{
    bridgeClient.connect();
}

PluginManager& AudioEngine::getPluginManager()
{
    return pluginManager;
}

bool AudioEngine::addPlugin(const juce::PluginDescription& description, juce::String& errorMessage)
{
    auto* device = deviceManager.getCurrentAudioDevice();
    const double sampleRate = device != nullptr ? device->getCurrentSampleRate() : kDefaultSampleRate;
    const int blockSize = device != nullptr ? device->getCurrentBufferSizeSamples() : kDefaultBufferSize;

    auto instance = pluginManager.createPluginInstance(description, sampleRate, blockSize, errorMessage);
    if (instance == nullptr)
    {
        return false;
    }

    const auto identifier = createIdentifierString(description);

    if (!pluginChain.addPlugin(std::move(instance), description.name, identifier))
    {
        errorMessage = "Unable to add plugin to processing graph";
        return false;
    }

    return true;
}

void AudioEngine::removePlugin(size_t index)
{
    pluginChain.removePlugin(index);
}

void AudioEngine::movePlugin(size_t index, int delta)
{
    pluginChain.movePlugin(index, delta);
}

juce::StringArray AudioEngine::getLoadedPluginNames() const
{
    return pluginChain.getPluginNames();
}

const juce::Array<PresetManager::ChainPreset>& AudioEngine::getPresets() const
{
    return presetManager.getPresets();
}

bool AudioEngine::applyPreset(const PresetManager::ChainPreset& preset, juce::String& errorMessage)
{
    pluginChain.initialiseDefaultChain();

    const auto& knownList = pluginManager.getKnownPluginList();
    auto* device = deviceManager.getCurrentAudioDevice();
    const double sampleRate = device != nullptr ? device->getCurrentSampleRate() : kDefaultSampleRate;
    const int blockSize = device != nullptr ? device->getCurrentBufferSizeSamples() : kDefaultBufferSize;

    for (const auto& pluginPreset : preset.plugins)
    {
        auto type = knownList.getTypeForIdentifierString(pluginPreset.pluginId);
        if (type == nullptr)
        {
            for (int i = 0; i < knownList.getNumTypes(); ++i)
            {
                if (auto* candidate = knownList.getType(i))
                {
                    if (candidate->name == pluginPreset.pluginName)
                    {
                        type = candidate;
                        break;
                    }
                }
            }
        }

        if (type == nullptr)
        {
            const auto label = pluginPreset.pluginName.isNotEmpty() ? pluginPreset.pluginName : pluginPreset.pluginId;
            errorMessage = "Preset references unknown plugin: " + label;
            return false;
        }

        auto instance = pluginManager.createPluginInstance(*type, sampleRate, blockSize, errorMessage);
        if (instance == nullptr)
        {
            return false;
        }

        if (pluginPreset.state.getSize() > 0)
        {
            instance->setStateInformation(pluginPreset.state.getData(),
                                          static_cast<int>(pluginPreset.state.getSize()));
        }

        const auto identifier = createIdentifierString(*type);

        if (!pluginChain.addPlugin(std::move(instance), type->name, identifier))
        {
            errorMessage = "Failed to insert plugin into chain";
            return false;
        }
    }

    return true;
}

bool AudioEngine::saveCurrentChainAsPreset(const juce::String& presetName, juce::String& errorMessage)
{
    const auto trimmedName = presetName.trim();
    if (trimmedName.isEmpty())
    {
        errorMessage = "Preset name cannot be empty.";
        return false;
    }

    auto preset = createPresetFromCurrentChain(trimmedName);
    if (preset.plugins.isEmpty())
    {
        errorMessage = "No plugins in the current chain to save.";
        return false;
    }

    if (!presetManager.saveUserPreset(preset))
    {
        errorMessage = "Failed to write preset to disk.";
        return false;
    }

    return true;
}

bool AudioEngine::removeUserPreset(int userIndex, juce::String& errorMessage)
{
    if (userIndex < 0)
    {
        errorMessage = "Invalid preset index.";
        return false;
    }

    if (!presetManager.removeUserPreset(userIndex))
    {
        errorMessage = "Unable to delete preset.";
        return false;
    }

    return true;
}

bool AudioEngine::updateUserPreset(int userIndex, const PresetManager::ChainPreset& preset, juce::String& errorMessage)
{
    if (userIndex < 0)
    {
        errorMessage = "Invalid preset index.";
        return false;
    }

    if (!presetManager.updateUserPreset(userIndex, preset))
    {
        errorMessage = "Unable to update preset.";
        return false;
    }

    return true;
}

PresetManager::ChainPreset AudioEngine::createPresetFromCurrentChain(const juce::String& presetName) const
{
    PresetManager::ChainPreset preset;
    preset.name = presetName;
    preset.isFactory = false;

    pluginChain.forEachPlugin([&preset](juce::AudioProcessor& processor,
                                        const juce::String& pluginName,
                                        const juce::String& identifier)
    {
        PresetManager::PluginPreset pluginState;
        pluginState.pluginId = identifier;
        pluginState.pluginName = pluginName;

        juce::MemoryBlock state;
        processor.getStateInformation(state);
        pluginState.state = state;

        preset.plugins.add(pluginState);
    });

    return preset;
}

void AudioEngine::audioDeviceIOCallback(const float* const* inputChannelData,
                                        int numInputChannels,
                                        float* const* outputChannelData,
                                        int numOutputChannels,
                                        int numSamples)
{
    juce::ignoreUnused(outputChannelData, numOutputChannels);

    bridgeClient.sendAudio(inputChannelData, numInputChannels, numSamples);

    auto* device = deviceManager.getCurrentAudioDevice();
    const auto sampleRate = device != nullptr ? device->getCurrentSampleRate() : 0.0;

    const auto stats = bridgeClient.getStatistics();
    lastStatus = juce::String::formatted("Streaming %d samples @ %0.1f Hz (queued frames: %d, dropped blocks: %d)",
                                         numSamples,
                                         sampleRate,
                                         stats.queuedFrames,
                                         stats.droppedBlocks);
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (device == nullptr)
    {
        lastStatus = "Audio device unavailable";
        return;
    }

    bridgeClient.setFormat(static_cast<int>(device->getCurrentSampleRate()),
                           device->getCurrentBufferSizeSamples(),
                           device->getActiveInputChannels().countNumberOfSetBits());

    lastStatus = "Audio device started";
}

void AudioEngine::audioDeviceStopped()
{
    lastStatus = "Audio device stopped";
}

