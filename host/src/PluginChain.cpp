#include "PluginChain.h"

namespace
{
constexpr int kNumChannels = 2;
}

PluginChain::PluginChain()
    : graph(std::make_unique<juce::AudioProcessorGraph>()),
      inputNode(),
      outputNode()
{
}

PluginChain::~PluginChain() = default;

juce::AudioProcessor* PluginChain::getProcessor()
{
    return graph.get();
}

void PluginChain::initialiseDefaultChain()
{
    graph->clear();
    pluginNodes.clear();
    pluginNames.clear();
    pluginIdentifiers.clear();

    inputNode = addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
        juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode));

    outputNode = addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
        juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));

    rebuildConnections();
}

bool PluginChain::addPlugin(std::unique_ptr<juce::AudioProcessor> processor,
                            const juce::String& name,
                            const juce::String& identifier)
{
    if (processor == nullptr)
    {
        return false;
    }

    auto node = addNode(std::move(processor));
    if (node == NodeID())
    {
        return false;
    }

    pluginNodes.add(node);
    pluginNames.add(name);
    pluginIdentifiers.add(identifier);
    rebuildConnections();
    return true;
}

bool PluginChain::removePlugin(size_t index)
{
    if (!juce::isPositiveAndBelow(static_cast<int>(index), pluginNodes.size()))
    {
        return false;
    }

    const auto nodeId = pluginNodes[static_cast<int>(index)];

    if (auto* node = graph->getNodeForId(nodeId))
    {
        graph->removeNode(nodeId);
    }

    pluginNodes.remove(static_cast<int>(index));
    pluginNames.remove(static_cast<int>(index));
    pluginIdentifiers.remove(static_cast<int>(index));
    rebuildConnections();
    return true;
}

bool PluginChain::movePlugin(size_t index, int delta)
{
    if (delta == 0 || pluginNodes.size() <= 1)
    {
        return false;
    }

    const int currentIndex = static_cast<int>(index);
    const int targetIndex = juce::jlimit(0, pluginNodes.size() - 1, currentIndex + delta);

    if (!juce::isPositiveAndBelow(currentIndex, pluginNodes.size()) || currentIndex == targetIndex)
    {
        return false;
    }

    pluginNodes.swap(currentIndex, targetIndex);
    pluginNames.swap(currentIndex, targetIndex);
    pluginIdentifiers.swap(currentIndex, targetIndex);
    rebuildConnections();
    return true;
}

juce::StringArray PluginChain::getPluginNames() const
{
    return pluginNames;
}

juce::StringArray PluginChain::getPluginIdentifiers() const
{
    return pluginIdentifiers;
}

void PluginChain::forEachPlugin(const std::function<void(juce::AudioProcessor&,
                                                         const juce::String&,
                                                         const juce::String&)>& visitor) const
{
    if (visitor == nullptr)
    {
        return;
    }

    for (int i = 0; i < pluginNodes.size(); ++i)
    {
        if (auto* node = graph->getNodeForId(pluginNodes[i]))
        {
            if (auto* processor = node->getProcessor())
            {
                visitor(*processor, pluginNames[i], pluginIdentifiers[i]);
            }
        }
    }
}

PluginChain::NodeID PluginChain::addNode(std::unique_ptr<juce::AudioProcessor> processor)
{
    auto* node = graph->addNode(std::move(processor));
    return node != nullptr ? node->nodeID : PluginChain::NodeID();
}

void PluginChain::connect(NodeID sourceNode,
                          int sourceChannel,
                          NodeID destinationNode,
                          int destinationChannel)
{
    graph->addConnection({{sourceNode, sourceChannel}, {destinationNode, destinationChannel}});
}

void PluginChain::rebuildConnections()
{
    if (inputNode == NodeID() || outputNode == NodeID())
    {
        return;
    }

    graph->clearConnections();

    auto addStereoConnections = [this](NodeID source, NodeID dest)
    {
        for (int channel = 0; channel < kNumChannels; ++channel)
        {
            connect(source, channel, dest, channel);
        }
    };

    NodeID previousNode = inputNode;

    for (const auto nodeId : pluginNodes)
    {
        addStereoConnections(previousNode, nodeId);
        previousNode = nodeId;
    }

    addStereoConnections(previousNode, outputNode);
}

