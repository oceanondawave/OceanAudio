#include "BridgeClient.h"

#include <cstring>

#if JUCE_WINDOWS
    #define NOMINMAX
    #include <windows.h>
#endif

namespace
{
#if JUCE_WINDOWS
constexpr wchar_t kMappingName[] = L"Global\\OceanAudio_AudioRing";
constexpr wchar_t kAudioReadyEventName[] = L"Global\\OceanAudio_AudioReady";
constexpr wchar_t kAudioConsumedEventName[] = L"Global\\OceanAudio_AudioConsumed";
#endif

constexpr int kMinCapacityMultiplier = 16;
constexpr int kMaxCapacitySamples = 1 << 19; // 524,288 frames
} // namespace

BridgeClient::BridgeClient()
    : connected(false)
#if !JUCE_WINDOWS
    , fifo(1024)
    , fifoCapacity(1024)
#endif
{
}

BridgeClient::~BridgeClient()
{
    disconnect();
}

void BridgeClient::connect()
{
    const juce::ScopedLock guard(lock);
    stats.droppedBlocks = 0;
    stats.queuedFrames = 0;
#if JUCE_WINDOWS
    if (sharedMemory.header == nullptr && stats.channels > 0 && stats.bufferSize > 0)
    {
        ensureSharedMemory(stats.channels, stats.sampleRate, stats.bufferSize);
    }
#else
    fifo.reset();
    pendingBlocks.clearQuick();
    ensureBuffer(stats.channels > 0 ? stats.channels : 2, fifoCapacity);
#endif
    connected = true;
}

void BridgeClient::disconnect()
{
    const juce::ScopedLock guard(lock);
    connected = false;
#if JUCE_WINDOWS
    destroySharedMemory();
#else
    pendingBlocks.clearQuick();
    fifo.reset();
    transferBuffer.setSize(0, 0, false, false, true);
#endif
}

#if !JUCE_WINDOWS
void BridgeClient::ensureBuffer(int channels, int capacitySamples)
{
    if (channels <= 0)
    {
        channels = 2;
    }

    if (transferBuffer.getNumChannels() != channels || transferBuffer.getNumSamples() < capacitySamples)
    {
        transferBuffer.setSize(channels,
                               juce::jmax(capacitySamples, transferBuffer.getNumSamples()),
                               false,
                               false,
                               true);
    }
}
#endif

void BridgeClient::sendAudio(const float* const* samples, int numChannels, int numSamples)
{
    if (samples == nullptr || numChannels <= 0 || numSamples <= 0)
    {
        return;
    }

    const juce::ScopedLock guard(lock);
    if (!connected)
    {
        return;
    }

#if JUCE_WINDOWS
    if (!writeToSharedMemory(samples, numChannels, numSamples))
    {
        ++stats.droppedBlocks;
    }
#else
    ensureBuffer(numChannels, fifoCapacity);

    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    if (!fifo.write(numSamples, start1, size1, start2, size2))
    {
        ++stats.droppedBlocks;
        return;
    }

    for (int channel = 0; channel < numChannels; ++channel)
    {
        if (size1 > 0)
        {
            transferBuffer.copyFrom(channel, start1, samples[channel], size1);
        }
        if (size2 > 0)
        {
            transferBuffer.copyFrom(channel, start2, samples[channel] + size1, size2);
        }
    }

    if (size1 > 0)
    {
        pendingBlocks.add({start1, size1});
        stats.queuedFrames += size1;
    }
    if (size2 > 0)
    {
        pendingBlocks.add({start2, size2});
        stats.queuedFrames += size2;
    }
#endif
}

void BridgeClient::setFormat(int sampleRate, int bufferSize, int channels)
{
    const juce::ScopedLock guard(lock);
    stats.sampleRate = sampleRate;
    stats.bufferSize = bufferSize;
    stats.channels = channels;

#if JUCE_WINDOWS
    if (connected && channels > 0 && bufferSize > 0)
    {
        ensureSharedMemory(channels, sampleRate, bufferSize);
    }
#else
    fifoCapacity = juce::nextPowerOfTwo(bufferSize * kMinCapacityMultiplier);
    fifoCapacity = juce::jlimit(bufferSize * kMinCapacityMultiplier, kMaxCapacitySamples, fifoCapacity);
    fifo.setTotalSize(fifoCapacity);
    pendingBlocks.clearQuick();
    fifo.reset();
    ensureBuffer(channels, fifoCapacity);
#endif
}

bool BridgeClient::isConnected() const
{
    const juce::ScopedLock guard(lock);
    return connected;
}

BridgeClient::Statistics BridgeClient::getStatistics() const
{
    const juce::ScopedLock guard(lock);
    return stats;
}

#if !JUCE_WINDOWS
bool BridgeClient::popPendingBlock(juce::AudioBuffer<float>& buffer, int& validSamples)
{
    const juce::ScopedLock guard(lock);
    if (pendingBlocks.isEmpty())
    {
        return false;
    }

    const auto pending = pendingBlocks.removeAndReturn(0);
    validSamples = pending.size;

    if (buffer.getNumChannels() != transferBuffer.getNumChannels()
        || buffer.getNumSamples() < pending.size)
    {
        buffer.setSize(transferBuffer.getNumChannels(), pending.size, false, false, true);
    }

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        buffer.copyFrom(channel, 0, transferBuffer, channel, pending.offset, pending.size);
    }

    fifo.finishedRead(pending.size);
    stats.queuedFrames = juce::jmax(0, stats.queuedFrames - pending.size);
    return true;
}
#endif

#if JUCE_WINDOWS
void BridgeClient::ensureSharedMemory(int channels, int sampleRate, int framesPerBlock)
{
    jassert(channels > 0);
    jassert(framesPerBlock > 0);

    const int desiredCapacity = juce::nextPowerOfTwo(framesPerBlock * kMinCapacityMultiplier);
    const int capacity = juce::jlimit(framesPerBlock * kMinCapacityMultiplier, kMaxCapacitySamples, desiredCapacity);
    const std::size_t requiredBytes = sizeof(oceanaudio::SharedAudioRingBufferHeader)
        + static_cast<std::size_t>(capacity) * static_cast<std::size_t>(channels) * sizeof(float);

    if (sharedMemory.header != nullptr
        && sharedMemory.header->channels == static_cast<std::uint32_t>(channels)
        && sharedMemory.header->frameCapacity == static_cast<std::uint32_t>(capacity))
    {
        sharedMemory.header->sampleRate = static_cast<std::uint32_t>(sampleRate);
        sharedMemory.header->framesPerBlock = static_cast<std::uint32_t>(framesPerBlock);
        return;
    }

    destroySharedMemory();

    HANDLE mappingHandle = CreateFileMappingW(INVALID_HANDLE_VALUE,
                                              nullptr,
                                              PAGE_READWRITE,
                                              0,
                                              static_cast<DWORD>(requiredBytes),
                                              kMappingName);
    if (mappingHandle == nullptr)
    {
        jassertfalse;
        return;
    }

    const bool newlyCreated = GetLastError() != ERROR_ALREADY_EXISTS;

    auto* header = static_cast<oceanaudio::SharedAudioRingBufferHeader*>(
        MapViewOfFile(mappingHandle, FILE_MAP_ALL_ACCESS, 0, 0, requiredBytes));
    if (header == nullptr)
    {
        CloseHandle(mappingHandle);
        jassertfalse;
        return;
    }

    sharedMemory.mappingHandle = mappingHandle;
    sharedMemory.header = header;
    sharedMemory.mappedSizeBytes = requiredBytes;

    if (newlyCreated
        || header->magic != oceanaudio::SharedAudioRingBufferHeader::kMagic
        || header->version != oceanaudio::SharedAudioRingBufferHeader::kVersion)
    {
        std::memset(header, 0, requiredBytes);
        header->magic = oceanaudio::SharedAudioRingBufferHeader::kMagic;
        header->version = oceanaudio::SharedAudioRingBufferHeader::kVersion;
    }

    header->channels = static_cast<std::uint32_t>(channels);
    header->sampleRate = static_cast<std::uint32_t>(sampleRate);
    header->frameCapacity = static_cast<std::uint32_t>(capacity);
    header->framesPerBlock = static_cast<std::uint32_t>(framesPerBlock);
    header->writePosition.store(0, std::memory_order_release);
    header->readPosition.store(0, std::memory_order_release);
    header->framesAvailable.store(0, std::memory_order_release);

    sharedMemory.audioReadyEvent = CreateEventW(nullptr, FALSE, FALSE, kAudioReadyEventName);
    sharedMemory.audioConsumedEvent = CreateEventW(nullptr, FALSE, TRUE, kAudioConsumedEventName);
}

void BridgeClient::destroySharedMemory()
{
    if (sharedMemory.header != nullptr)
    {
        UnmapViewOfFile(sharedMemory.header);
        sharedMemory.header = nullptr;
    }

    if (sharedMemory.mappingHandle != nullptr)
    {
        CloseHandle(static_cast<HANDLE>(sharedMemory.mappingHandle));
        sharedMemory.mappingHandle = nullptr;
    }

    if (sharedMemory.audioReadyEvent != nullptr)
    {
        CloseHandle(static_cast<HANDLE>(sharedMemory.audioReadyEvent));
        sharedMemory.audioReadyEvent = nullptr;
    }

    if (sharedMemory.audioConsumedEvent != nullptr)
    {
        CloseHandle(static_cast<HANDLE>(sharedMemory.audioConsumedEvent));
        sharedMemory.audioConsumedEvent = nullptr;
    }

    sharedMemory.mappedSizeBytes = 0;
}

bool BridgeClient::writeToSharedMemory(const float* const* samples, int numChannels, int numSamples)
{
    auto* header = sharedMemory.header;
    if (header == nullptr)
    {
        return false;
    }

    const int channels = static_cast<int>(header->channels);
    if (channels <= 0)
    {
        return false;
    }

    const int capacity = static_cast<int>(header->frameCapacity);
    const int available = static_cast<int>(header->framesAvailable.load(std::memory_order_acquire));
    if (numSamples > capacity || available + numSamples > capacity)
    {
        header->overruns.fetch_add(1, std::memory_order_relaxed);
        stats.queuedFrames = capacity;
        return false;
    }

    const int writePosition = static_cast<int>(header->writePosition.load(std::memory_order_relaxed));
    float* payload = reinterpret_cast<float*>(header + 1);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const int frameIndex = (writePosition + sample) % capacity;
        const int frameOffset = frameIndex * channels;
        for (int channel = 0; channel < channels; ++channel)
        {
            const int sourceChannel = channel < numChannels ? channel : (numChannels - 1);
            payload[frameOffset + channel] = samples[sourceChannel][sample];
        }
    }

    const int newWritePosition = (writePosition + numSamples) % capacity;
    header->writePosition.store(static_cast<std::uint32_t>(newWritePosition), std::memory_order_release);
    header->framesAvailable.fetch_add(static_cast<std::uint32_t>(numSamples), std::memory_order_release);
    stats.queuedFrames = static_cast<int>(header->framesAvailable.load(std::memory_order_relaxed));

    if (sharedMemory.audioReadyEvent != nullptr)
    {
        SetEvent(static_cast<HANDLE>(sharedMemory.audioReadyEvent));
    }

    return true;
}
#endif

