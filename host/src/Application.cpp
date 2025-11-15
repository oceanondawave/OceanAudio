#include "Application.h"
#include "MainWindow.h"

#include <juce_events/juce_events.h>

namespace
{
constexpr const char* kAppName = "OceanAudio";
constexpr const char* kAppVersion = "0.1.0";
} // namespace

OceanAudioApplication::OceanAudioApplication() = default;

OceanAudioApplication::~OceanAudioApplication() = default;

const juce::String OceanAudioApplication::getApplicationName()
{
    return kAppName;
}

const juce::String OceanAudioApplication::getApplicationVersion()
{
    return kAppVersion;
}

bool OceanAudioApplication::moreThanOneInstanceAllowed()
{
    return false;
}

void OceanAudioApplication::initialise(const juce::String&)
{
    mainWindow = std::make_unique<MainWindow>(getApplicationName());
    mainWindow->centreWithSize(1200, 720);
    mainWindow->setVisible(true);
}

void OceanAudioApplication::shutdown()
{
    mainWindow.reset();
}

void OceanAudioApplication::systemRequestedQuit()
{
    quit();
}

void OceanAudioApplication::anotherInstanceStarted(const juce::String&)
{
    JUCEApplication::anotherInstanceStarted({});
}

