#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <functional>

class PluginChain
{
public:
    PluginChain();
    ~PluginChain();

    juce::AudioProcessor* getProcessor();
    void initialiseDefaultChain();
    bool addPlugin(std::unique_ptr<juce::AudioProcessor> processor,
                   const juce::String& name,
                   const juce::String& identifier);
    bool removePlugin(size_t index);
    bool movePlugin(size_t index, int delta);
    juce::StringArray getPluginNames() const;
    juce::StringArray getPluginIdentifiers() const;

    void forEachPlugin(const std::function<void(juce::AudioProcessor&,
                                               const juce::String& name,
                                               const juce::String& identifier)>& visitor) const;

private:
    using NodeID = juce::AudioProcessorGraph::NodeID;

    NodeID addNode(std::unique_ptr<juce::AudioProcessor> processor);
    void connect(NodeID sourceNode,
                 int sourceChannel,
                 NodeID destinationNode,
                 int destinationChannel);
    void rebuildConnections();

    std::unique_ptr<juce::AudioProcessorGraph> graph;
    NodeID inputNode;
    NodeID outputNode;
    juce::Array<NodeID> pluginNodes;
    juce::StringArray pluginNames;
    juce::StringArray pluginIdentifiers;
};

