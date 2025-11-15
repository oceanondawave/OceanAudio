#include "PluginManager.h"

namespace
{
class PluginScanThread final : public juce::Thread
{
public:
    explicit PluginScanThread(PluginManager& ownerToUse)
        : juce::Thread("Plugin Scan Thread"),
          owner(ownerToUse)
    {
    }

    void run() override
    {
        owner.performScan();
    }

private:
    PluginManager& owner;
};

juce::File getAppDataDirectory()
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("OceanAudio");
    dir.createDirectory();
    return dir;
}

juce::File getDeadMansPedalDefaultFile()
{
    auto dir = getAppDataDirectory();
    auto pedal = dir.getChildFile("DeadMansPedal.xml");
    if (!pedal.exists())
    {
        pedal.create();
    }
    return pedal;
}

juce::FileSearchPath getDefaultVST3SearchPath()
{
    juce::FileSearchPath path;
#if JUCE_WINDOWS
    auto commonFiles = juce::File::getSpecialLocation(juce::File::commonApplicationDataDirectory)
                           .getChildFile("VST3");
    if (commonFiles.exists())
    {
        path.add(commonFiles);
    }

    auto programFiles = juce::File::getSpecialLocation(juce::File::globalApplicationsDirectory)
                            .getChildFile("Common Files")
                            .getChildFile("VST3");
    if (programFiles.exists())
    {
        path.add(programFiles);
    }

    auto programFilesX86 = juce::File::getSpecialLocation(juce::File::globalApplicationsDirectory)
                                .getParentDirectory()
                                .getChildFile("Common Files (x86)")
                                .getChildFile("VST3");
    if (programFilesX86.exists())
    {
        path.add(programFilesX86);
    }
#endif

    auto bundled = juce::File::getSpecialLocation(juce::File::commonApplicationDataDirectory)
                       .getChildFile("OceanAudio")
                       .getChildFile("Plugins");
    if (bundled.exists())
    {
        path.add(bundled);
    }

    auto userPlugins = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                           .getChildFile("OceanAudio")
                           .getChildFile("Plugins");
    if (userPlugins.exists())
    {
        path.add(userPlugins);
    }

    return path;
}
} // namespace

PluginManager::PluginManager()
    : deadMansPedalFile(getDeadMansPedalDefaultFile())
{
    formatManager.addDefaultFormats();
    knownPluginList.addChangeListener(this);
}

PluginManager::~PluginManager()
{
    shutdown();
    knownPluginList.removeChangeListener(this);
}

void PluginManager::initialise()
{
    loadCustomDirectories();
    startScan();
}

void PluginManager::shutdown()
{
    stopScan();
    activeScanner.reset();
}

void PluginManager::rescanDefaultDirectories()
{
    stopScan();
    knownPluginList.clear();
    startScan();
}

void PluginManager::addCustomDirectory(const juce::File& directory)
{
    if (directory.exists() && directory.isDirectory())
    {
        if (!customDirectories.contains(directory))
        {
            customDirectories.add(directory);
            saveCustomDirectories();
        }
        rescanDefaultDirectories();
    }
}

void PluginManager::removeCustomDirectory(const juce::File& directory)
{
    if (customDirectories.removeAllInstancesOf(directory) > 0)
    {
        saveCustomDirectories();
        rescanDefaultDirectories();
    }
}

const juce::KnownPluginList& PluginManager::getKnownPluginList() const noexcept
{
    return knownPluginList;
}

juce::KnownPluginList& PluginManager::getKnownPluginList() noexcept
{
    return knownPluginList;
}

std::unique_ptr<juce::AudioPluginInstance> PluginManager::createPluginInstance(const juce::PluginDescription& description,
                                                                               double sampleRate,
                                                                               int blockSize,
                                                                               juce::String& errorMessage) const
{
    std::unique_ptr<juce::AudioPluginInstance> instance;

    if (auto* format = formatManager.findFormatForDescription(description))
    {
        instance.reset(formatManager.createPluginInstance(description, sampleRate, blockSize, errorMessage));
    }
    else
    {
        errorMessage = "Format not available for plugin";
    }

    if (instance != nullptr)
    {
        instance->setRateAndBufferSizeDetails(sampleRate, blockSize);
    }

    return instance;
}

juce::File PluginManager::getDeadMansPedalFile() const
{
    return deadMansPedalFile;
}

void PluginManager::changeListenerCallback(juce::ChangeBroadcaster*)
{
    sendChangeMessage();
}

void PluginManager::startScan()
{
    stopScan();
    scanThread = std::make_unique<PluginScanThread>(*this);
    scanThread->startThread();
}

void PluginManager::stopScan()
{
    if (scanThread != nullptr)
    {
        scanThread->stopThread(2000);
        scanThread.reset();
    }
}

void PluginManager::performScan()
{
    juce::String pluginName;
    auto deadMans = getDeadMansPedalFile();
    deadMans.create();

    const auto searchPath = getDefaultSearchPath();

    auto* currentThread = juce::Thread::getCurrentThread();

    for (int formatIndex = 0; formatIndex < formatManager.getNumFormats(); ++formatIndex)
    {
        if (currentThread != nullptr && currentThread->threadShouldExit())
        {
            break;
        }

        auto* format = formatManager.getFormat(formatIndex);
        if (format == nullptr)
        {
            continue;
        }

        juce::PluginDirectoryScanner scanner(knownPluginList,
                                             *format,
                                             searchPath,
                                             false,
                                             deadMans,
                                             pluginName);

        bool keepScanning = true;
        while (keepScanning)
        {
            if (currentThread != nullptr && currentThread->threadShouldExit())
            {
                return;
            }

            const bool allowAsync = true;
            const bool reentrant = false;
            keepScanning = scanner.scanNextFile(allowAsync, pluginName);
        }
    }

    sendChangeMessage();
}

juce::FileSearchPath PluginManager::getDefaultSearchPath() const
{
    juce::FileSearchPath path = getDefaultVST3SearchPath();

    for (const auto& directory : customDirectories)
    {
        if (directory.exists())
        {
            path.add(directory);
        }
    }

    return path;
}

const juce::Array<juce::File>& PluginManager::getCustomDirectories() const noexcept
{
    return customDirectories;
}

void PluginManager::loadCustomDirectories()
{
    customDirectories.clear();

    auto file = getCustomDirectoriesFile();
    if (!file.existsAsFile())
    {
        return;
    }

    juce::var json = juce::JSON::parse(file);
    if (!json.isArray())
    {
        return;
    }

    if (auto* array = json.getArray())
    {
        for (const auto& entry : *array)
        {
            auto path = entry.toString();
            if (path.isNotEmpty())
            {
                juce::File dir(path);
                if (dir.isDirectory())
                {
                    customDirectories.addIfNotAlreadyThere(dir);
                }
            }
        }
    }
}

void PluginManager::saveCustomDirectories() const
{
    juce::Array<juce::var> dirs;
    for (const auto& dir : customDirectories)
    {
        dirs.add(dir.getFullPathName());
    }

    juce::var json(dirs);
    auto file = getCustomDirectoriesFile();
    file.getParentDirectory().createDirectory();
    file.replaceWithText(juce::JSON::toString(json, true));
}

juce::File PluginManager::getCustomDirectoriesFile() const
{
    return getAppDataDirectory().getChildFile("PluginDirectories.json");
}

