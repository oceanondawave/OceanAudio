#include "PresetManager.h"

namespace
{
juce::File getPresetDirectory()
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("OceanAudio")
                   .getChildFile("Presets");
    dir.createDirectory();
    return dir;
}

juce::File getFactoryPresetFile()
{
    auto executableDir = juce::File::getSpecialLocation(juce::File::currentApplicationFile).getParentDirectory();
    auto presetFile = executableDir.getChildFile("presets").getChildFile("FactoryPresets.json");

    if (!presetFile.existsAsFile())
    {
        presetFile = juce::File::getSpecialLocation(juce::File::currentWorkingDirectory)
                         .getChildFile("host")
                         .getChildFile("resources")
                         .getChildFile("presets")
                         .getChildFile("FactoryPresets.json");
    }

    return presetFile;
}

juce::File getUserPresetFilePath()
{
    return getPresetDirectory().getChildFile("UserPresets.json");
}
} // namespace

PresetManager::PresetManager()
{
    loadFactoryPresets();
    loadUserPresets();
}

void PresetManager::loadFactoryPresets()
{
    auto factoryFile = getFactoryPresetFile();
    if (!factoryFile.existsAsFile())
    {
        return;
    }

    juce::var json = juce::JSON::parse(factoryFile);
    if (json.isVoid() || !json.isObject())
    {
        return;
    }

    if (auto* presetsArray = json.getProperty("presets", {}).getArray())
    {
        for (const auto& presetVar : *presetsArray)
        {
            auto chain = parsePreset(presetVar);
            if (!chain.plugins.isEmpty())
            {
                chain.isFactory = true;
                presets.add(std::move(chain));
            }
        }
    }
}

void PresetManager::loadUserPresets()
{
    auto userFile = getUserPresetFile();
    if (!userFile.existsAsFile())
    {
        return;
    }

    juce::var json = juce::JSON::parse(userFile);
    if (json.isVoid() || !json.isObject())
    {
        return;
    }

    if (auto* presetsArray = json.getProperty("presets", {}).getArray())
    {
        for (const auto& presetVar : *presetsArray)
        {
            auto chain = parsePreset(presetVar);
            if (!chain.plugins.isEmpty())
            {
                chain.isFactory = false;
                presets.add(std::move(chain));
            }
        }
    }
}

const juce::Array<PresetManager::ChainPreset>& PresetManager::getPresets() const noexcept
{
    return presets;
}

bool PresetManager::saveUserPreset(const ChainPreset& preset)
{
    auto userPresets = readUserPresetsFromFile();
    auto presetCopy = preset;
    presetCopy.isFactory = false;
    userPresets.add(std::move(presetCopy));

    if (!writeUserPresetsToFile(userPresets))
    {
        return false;
    }

    replaceUserPresets(userPresets);
    return true;
}

bool PresetManager::removeUserPreset(int index)
{
    auto userPresets = readUserPresetsFromFile();
    if (!juce::isPositiveAndBelow(index, userPresets.size()))
    {
        return false;
    }

    userPresets.remove(index);

    if (!writeUserPresetsToFile(userPresets))
    {
        return false;
    }

    replaceUserPresets(userPresets);
    return true;
}

bool PresetManager::updateUserPreset(int index, const ChainPreset& preset)
{
    auto userPresets = readUserPresetsFromFile();
    if (!juce::isPositiveAndBelow(index, userPresets.size()))
    {
        return false;
    }

    auto presetCopy = preset;
    presetCopy.isFactory = false;
    userPresets.set(index, std::move(presetCopy));

    if (!writeUserPresetsToFile(userPresets))
    {
        return false;
    }

    replaceUserPresets(userPresets);
    return true;
}

PresetManager::ChainPreset PresetManager::parsePreset(const juce::var& presetVar) const
{
    ChainPreset chain;
    if (!presetVar.isObject())
    {
        return chain;
    }

    chain.name = presetVar.getProperty("name", "Untitled");

    if (auto* pluginsArray = presetVar.getProperty("plugins", {}).getArray())
    {
        for (const auto& pluginVar : *pluginsArray)
        {
            if (!pluginVar.isObject())
            {
                continue;
            }

            PluginPreset plugin;
            plugin.pluginId = pluginVar.getProperty("id", "").toString();
            plugin.pluginName = pluginVar.getProperty("name", plugin.pluginId).toString();

            if (plugin.pluginId.isEmpty())
            {
                continue;
            }

            if (pluginVar.hasProperty("stateBase64"))
            {
                auto base64 = pluginVar.getProperty("stateBase64", juce::String()).toString();
                juce::MemoryBlock decoded;
                juce::Base64::convertFromBase64(decoded, base64);
                plugin.state = decoded;
            }
            chain.plugins.add(plugin);
        }
    }

    return chain;
}

juce::File PresetManager::getUserPresetFile() const
{
    return getUserPresetFilePath();
}

juce::Array<PresetManager::ChainPreset> PresetManager::readUserPresetsFromFile() const
{
    juce::Array<ChainPreset> userPresets;
    auto userFile = getUserPresetFile();
    if (!userFile.existsAsFile())
    {
        return userPresets;
    }

    juce::var json = juce::JSON::parse(userFile);
    if (!json.isObject())
    {
        return userPresets;
    }

    if (auto* presetsArray = json.getProperty("presets", {}).getArray())
    {
        for (const auto& presetVar : *presetsArray)
        {
            auto chain = parsePreset(presetVar);
            if (!chain.plugins.isEmpty())
            {
                chain.isFactory = false;
                userPresets.add(std::move(chain));
            }
        }
    }

    return userPresets;
}

bool PresetManager::writeUserPresetsToFile(const juce::Array<ChainPreset>& userPresets) const
{
    juce::DynamicObject::Ptr root = new juce::DynamicObject();
    juce::Array<juce::var> presetsVar;

    for (const auto& chain : userPresets)
    {
        juce::DynamicObject::Ptr presetObj = new juce::DynamicObject();
        presetObj->setProperty("name", chain.name);
        juce::Array<juce::var> pluginsArray;

        for (const auto& plugin : chain.plugins)
        {
            juce::DynamicObject::Ptr pluginObj = new juce::DynamicObject();
            pluginObj->setProperty("id", plugin.pluginId);
            pluginObj->setProperty("name", plugin.pluginName);
            pluginObj->setProperty("stateBase64", juce::Base64::toBase64(plugin.state.getData(), plugin.state.getSize()));
            pluginsArray.add(juce::var(pluginObj.get()));
        }

        presetObj->setProperty("plugins", juce::var(pluginsArray));
        presetsVar.add(juce::var(presetObj.get()));
    }

    root->setProperty("presets", juce::var(presetsVar));

    juce::var finalVar(root.get());
    auto jsonText = juce::JSON::toString(finalVar, true);
    return getUserPresetFile().replaceWithText(jsonText);
}

void PresetManager::replaceUserPresets(const juce::Array<ChainPreset>& userPresets)
{
    juce::Array<ChainPreset> updated;
    for (const auto& preset : presets)
    {
        if (preset.isFactory)
        {
            updated.add(preset);
        }
    }

    for (const auto& preset : userPresets)
    {
        updated.add(preset);
    }

    presets = updated;
}

