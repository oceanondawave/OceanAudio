#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class MainWindow;

class OceanAudioApplication final : public juce::JUCEApplication
{
public:
    OceanAudioApplication();
    ~OceanAudioApplication() override;

    const juce::String getApplicationName() override;
    const juce::String getApplicationVersion() override;
    bool moreThanOneInstanceAllowed() override;

    void initialise(const juce::String& commandLineParameters) override;
    void shutdown() override;

    void systemRequestedQuit() override;
    void anotherInstanceStarted(const juce::String& commandLineParameters) override;

private:
    std::unique_ptr<MainWindow> mainWindow;
};

