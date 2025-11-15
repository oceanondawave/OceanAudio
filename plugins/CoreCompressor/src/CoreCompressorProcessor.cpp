#include "CoreCompressorProcessor.h"
#include "CoreCompressorEditor.h"

namespace
{
constexpr const char* kParamThreshold = "threshold";
constexpr const char* kParamRatio = "ratio";
constexpr const char* kParamAttack = "attack";
constexpr const char* kParamRelease = "release";

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(kParamThreshold,
                                                           "Threshold (dB)",
                                                           juce::NormalisableRange<float>(-60.0F, 0.0F),
                                                           -24.0F));
    layout.add(std::make_unique<juce::AudioParameterFloat>(kParamRatio,
                                                           "Ratio",
                                                           juce::NormalisableRange<float>(1.0F, 20.0F, 0.1F),
                                                           4.0F));
    layout.add(std::make_unique<juce::AudioParameterFloat>(kParamAttack,
                                                           "Attack (ms)",
                                                           juce::NormalisableRange<float>(1.0F, 100.0F),
                                                           10.0F));
    layout.add(std::make_unique<juce::AudioParameterFloat>(kParamRelease,
                                                           "Release (ms)",
                                                           juce::NormalisableRange<float>(5.0F, 500.0F),
                                                           50.0F));
    return layout;
}
} // namespace

CoreCompressorProcessor::CoreCompressorProcessor()
    : juce::AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "STATE", createParameterLayout())
{
}

CoreCompressorProcessor::~CoreCompressorProcessor() = default;

void CoreCompressorProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);

    compressor.setSampleRate(sampleRate);
}

void CoreCompressorProcessor::releaseResources()
{
}

bool CoreCompressorProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto layout = layouts.getMainOutputChannelSet();
    return layout == juce::AudioChannelSet::mono() || layout == juce::AudioChannelSet::stereo();
}

void CoreCompressorProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    compressor.setThreshold(parameters.getRawParameterValue(kParamThreshold)->load());
    compressor.setRatio(parameters.getRawParameterValue(kParamRatio)->load());
    compressor.setAttack(parameters.getRawParameterValue(kParamAttack)->load());
    compressor.setRelease(parameters.getRawParameterValue(kParamRelease)->load());

    juce::dsp::AudioBlock<float> block(buffer);
    compressor.process(juce::dsp::ProcessContextReplacing<float>(block));
}

juce::AudioProcessorEditor* CoreCompressorProcessor::createEditor()
{
    return new CoreCompressorEditor(*this);
}

bool CoreCompressorProcessor::hasEditor() const
{
    return true;
}

const juce::String CoreCompressorProcessor::getName() const
{
    return "OceanAudio Core Compressor";
}

bool CoreCompressorProcessor::acceptsMidi() const
{
    return false;
}

bool CoreCompressorProcessor::producesMidi() const
{
    return false;
}

bool CoreCompressorProcessor::isMidiEffect() const
{
    return false;
}

double CoreCompressorProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int CoreCompressorProcessor::getNumPrograms()
{
    return 1;
}

int CoreCompressorProcessor::getCurrentProgram()
{
    return 0;
}

void CoreCompressorProcessor::setCurrentProgram(int)
{
}

const juce::String CoreCompressorProcessor::getProgramName(int)
{
    return "Default";
}

void CoreCompressorProcessor::changeProgramName(int, const juce::String&)
{
}

void CoreCompressorProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = parameters.copyState())
    {
        if (auto xml = state.createXml())
        {
            copyXmlToBinary(*xml, destData);
        }
    }
}

void CoreCompressorProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary(data, sizeInBytes))
    {
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessorValueTreeState& CoreCompressorProcessor::getValueTreeState()
{
    return parameters;
}

