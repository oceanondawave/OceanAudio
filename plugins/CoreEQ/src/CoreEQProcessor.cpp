#include "CoreEQProcessor.h"
#include "CoreEQEditor.h"

namespace
{
constexpr const char* kParamLowShelfGainId = "lowShelfGain";
constexpr const char* kParamHighShelfGainId = "highShelfGain";

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(kParamLowShelfGainId,
                                                           "Low Shelf Gain",
                                                           juce::NormalisableRange<float>(-12.0F, 12.0F),
                                                           0.0F));
    layout.add(std::make_unique<juce::AudioParameterFloat>(kParamHighShelfGainId,
                                                           "High Shelf Gain",
                                                           juce::NormalisableRange<float>(-12.0F, 12.0F),
                                                           0.0F));
    return layout;
}
} // namespace

CoreEQProcessor::CoreEQProcessor()
    : juce::AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "STATE", createParameterLayout())
{
}

CoreEQProcessor::~CoreEQProcessor() = default;

void CoreEQProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    lowShelf.reset();
    highShelf.reset();
    lowShelf.prepare(spec);
    highShelf.prepare(spec);
}

void CoreEQProcessor::releaseResources()
{
    lowShelf.reset();
    highShelf.reset();
}

bool CoreEQProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mono = layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
    const auto stereo = layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
    return mono || stereo;
}

void CoreEQProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const auto lowGain = parameters.getRawParameterValue(kParamLowShelfGainId)->load();
    const auto highGain = parameters.getRawParameterValue(kParamHighShelfGainId)->load();

    const auto sampleRate = getSampleRate();
    if (sampleRate <= 0.0)
    {
        return;
    }

    auto lowCoefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, 120.0F, 0.7071F, juce::Decibels::decibelsToGain(lowGain));
    auto highCoefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 8000.0F, 0.7071F, juce::Decibels::decibelsToGain(highGain));

    *lowShelf.state = *lowCoefficients;
    *highShelf.state = *highCoefficients;

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    lowShelf.process(context);
    highShelf.process(context);
}

juce::AudioProcessorEditor* CoreEQProcessor::createEditor()
{
    return new CoreEQEditor(*this);
}

bool CoreEQProcessor::hasEditor() const
{
    return true;
}

const juce::String CoreEQProcessor::getName() const
{
    return "OceanAudio Core EQ";
}

bool CoreEQProcessor::acceptsMidi() const
{
    return false;
}

bool CoreEQProcessor::producesMidi() const
{
    return false;
}

bool CoreEQProcessor::isMidiEffect() const
{
    return false;
}

double CoreEQProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int CoreEQProcessor::getNumPrograms()
{
    return 1;
}

int CoreEQProcessor::getCurrentProgram()
{
    return 0;
}

void CoreEQProcessor::setCurrentProgram(int)
{
}

const juce::String CoreEQProcessor::getProgramName(int)
{
    return "Default";
}

void CoreEQProcessor::changeProgramName(int, const juce::String&)
{
}

void CoreEQProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = parameters.copyState())
    {
        if (auto xml = state.createXml())
        {
            copyXmlToBinary(*xml, destData);
        }
    }
}

void CoreEQProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary(data, sizeInBytes))
    {
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessorValueTreeState& CoreEQProcessor::getValueTreeState()
{
    return parameters;
}

