#pragma once

#include <OceanAudio/BridgeSharedMemory.h>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

class BridgeClient
{
public:
    BridgeClient();
    ~BridgeClient();

    void connect();
    void disconnect();
    void sendAudio(const float* const* samples, int numChannels, int numSamples);
    void setFormat(int sampleRate, int bufferSize, int channels);
    bool isConnected() const;

    struct Statistics
    {
        int sampleRate = 0;
        int channels = 0;
        int bufferSize = 0;
        int droppedBlocks = 0;
        int queuedFrames = 0;
    };

    Statistics getStatistics() const;

#if !JUCE_WINDOWS
    bool popPendingBlock(juce::AudioBuffer<float>& buffer, int& validSamples);
#endif

private:
#if !JUCE_WINDOWS
    void ensureBuffer(int channels, int capacitySamples);
#endif

#if JUCE_WINDOWS
    void ensureSharedMemory(int channels, int sampleRate, int framesPerBlock);
    void destroySharedMemory();
    bool writeToSharedMemory(const float* const* samples, int numChannels, int numSamples);

    struct SharedMemoryHandles
    {
        void* mappingHandle = nullptr;
        void* audioReadyEvent = nullptr;
        void* audioConsumedEvent = nullptr;
        oceanaudio::SharedAudioRingBufferHeader* header = nullptr;
        std::size_t mappedSizeBytes = 0;
    };

    SharedMemoryHandles sharedMemory;
#else
    juce::AudioBuffer<float> transferBuffer;
    juce::AbstractFifo fifo;

    struct PendingBlock
    {
        int offset = 0;
        int size = 0;
    };

    juce::Array<PendingBlock> pendingBlocks;
    int fifoCapacity;
#endif

    juce::CriticalSection lock;
    Statistics stats;
    bool connected;
};

