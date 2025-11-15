#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class PluginManager : private juce::ChangeListener,
                      public juce::ChangeBroadcaster
{
public:
    PluginManager();
    ~PluginManager() override;

    void initialise();
    void shutdown();

    void rescanDefaultDirectories();
    void addCustomDirectory(const juce::File& directory);
    void removeCustomDirectory(const juce::File& directory);

    const juce::KnownPluginList& getKnownPluginList() const noexcept;
    juce::KnownPluginList& getKnownPluginList() noexcept;
    const juce::Array<juce::File>& getCustomDirectories() const noexcept;

    std::unique_ptr<juce::AudioPluginInstance> createPluginInstance(const juce::PluginDescription& description,
                                                                    double sampleRate,
                                                                    int blockSize,
                                                                    juce::String& errorMessage) const;

    juce::File getDeadMansPedalFile() const;

private:
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void startScan();
    void stopScan();
    void performScan();
    juce::FileSearchPath getDefaultSearchPath() const;
    void loadCustomDirectories();
    void saveCustomDirectories() const;
    juce::File getCustomDirectoriesFile() const;

    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPluginList;

    std::unique_ptr<juce::PluginDirectoryScanner> activeScanner;
    std::unique_ptr<juce::Thread> scanThread;
    juce::CriticalSection scanLock;

    juce::File deadMansPedalFile;
    juce::Array<juce::File> customDirectories;
};

