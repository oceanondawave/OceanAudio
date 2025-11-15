#include "CoreGateProcessor.h"
#include "CoreGateEditor.h"

namespace
{
constexpr const char* kParamThreshold = "threshold";
constexpr const char* kParamHold = "hold";
constexpr const char* kParamAttack = "attack";
constexpr const char* kParamRelease = "release";

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(kParamThreshold,
                                                           "Threshold (dB)",
                                                           juce::NormalisableRange<float>(-80.0F, 0.0F),
                                                           -40.0F));
    layout.add(std::make_unique<juce::AudioParameterFloat>(kParamHold,
                                                           "Hold (ms)",
                                                           juce::NormalisableRange<float>(0.0F, 500.0F),
                                                           50.0F));
    layout.add(std::make_unique<juce::AudioParameterFloat>(kParamAttack,
                                                           "Attack (ms)",
                                                           juce::NormalisableRange<float>(1.0F, 100.0F),
                                                           5.0F));
    layout.add(std::make_unique<juce::AudioParameterFloat>(kParamRelease,
                                                           "Release (ms)",
                                                           juce::NormalisableRange<float>(10.0F, 500.0F),
                                                           100.0F));
    return layout;
}
} // namespace

CoreGateProcessor::CoreGateProcessor()
    : juce::AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "STATE", createParameterLayout())
{
}

CoreGateProcessor::~CoreGateProcessor() = default;

void CoreGateProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    gate.setSampleRate(sampleRate);
}

void CoreGateProcessor::releaseResources()
{
}

bool CoreGateProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto layout = layouts.getMainOutputChannelSet();
    return layout == juce::AudioChannelSet::mono() || layout == juce::AudioChannelSet::stereo();
}

void CoreGateProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    gate.setThreshold(parameters.getRawParameterValue(kParamThreshold)->load());
    gate.setHoldTime(parameters.getRawParameterValue(kParamHold)->load());
    gate.setAttackTime(parameters.getRawParameterValue(kParamAttack)->load());
    gate.setReleaseTime(parameters.getRawParameterValue(kParamRelease)->load());

    juce::dsp::AudioBlock<float> block(buffer);
    gate.process(juce::dsp::ProcessContextReplacing<float>(block));
}

juce::AudioProcessorEditor* CoreGateProcessor::createEditor()
{
    return new CoreGateEditor(*this);
}

bool CoreGateProcessor::hasEditor() const
{
    return true;
}

const juce::String CoreGateProcessor::getName() const
{
    return "OceanAudio Core Gate";
}

bool CoreGateProcessor::acceptsMidi() const
{
    return false;
}

bool CoreGateProcessor::producesMidi() const
{
    return false;
}

bool CoreGateProcessor::isMidiEffect() const
{
    return false;
}

double CoreGateProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int CoreGateProcessor::getNumPrograms()
{
    return 1;
}

int CoreGateProcessor::getCurrentProgram()
{
    return 0;
}

void CoreGateProcessor::setCurrentProgram(int)
{
}

const juce::String CoreGateProcessor::getProgramName(int)
{
    return "Default";
}

void CoreGateProcessor::changeProgramName(int, const juce::String&)
{
}

void CoreGateProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = parameters.copyState())
    {
        if (auto xml = state.createXml())
        {
            copyXmlToBinary(*xml, destData);
        }
    }
}

void CoreGateProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary(data, sizeInBytes))
    {
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessorValueTreeState& CoreGateProcessor::getValueTreeState()
{
    return parameters;
}

