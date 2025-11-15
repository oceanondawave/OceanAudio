#include <OceanAudio/BridgeProtocol.h>

#include "BridgeConsumer.h"

#include <algorithm>
#include <cstring>

namespace
{
constexpr wchar_t kDefaultMappingName[] = L"Global\\OceanAudio_AudioRing";
constexpr wchar_t kDefaultReadyEventName[] = L"Global\\OceanAudio_AudioReady";
constexpr wchar_t kDefaultConsumedEventName[] = L"Global\\OceanAudio_AudioConsumed";
} // namespace

BridgeConsumer::BridgeConsumer()
    : mappingHandle(nullptr),
      audioReadyEvent(nullptr),
      audioConsumedEvent(nullptr),
      header(nullptr),
      stats()
{
}

BridgeConsumer::~BridgeConsumer()
{
    close();
}

bool BridgeConsumer::open(const std::wstring& mappingName,
                          const std::wstring& readyEventName,
                          const std::wstring& consumedEventName)
{
    close();

    const std::wstring mapping = mappingName.empty() ? std::wstring(kDefaultMappingName) : mappingName;
    const std::wstring ready = readyEventName.empty() ? std::wstring(kDefaultReadyEventName) : readyEventName;
    const std::wstring consumed = consumedEventName.empty() ? std::wstring(kDefaultConsumedEventName) : consumedEventName;

    mappingHandle = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, mapping.c_str());
    if (mappingHandle == nullptr)
    {
        return false;
    }

    audioReadyEvent = OpenEventW(SYNCHRONIZE, FALSE, ready.c_str());
    if (audioReadyEvent == nullptr)
    {
        close();
        return false;
    }

    audioConsumedEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, consumed.c_str());
    if (audioConsumedEvent == nullptr)
    {
        close();
        return false;
    }

    auto* mappedPtr = MapViewOfFile(mappingHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (mappedPtr == nullptr)
    {
        close();
        return false;
    }

    header = static_cast<oceanaudio::SharedAudioRingBufferHeader*>(mappedPtr);
    if (header->magic != oceanaudio::SharedAudioRingBufferHeader::kMagic)
    {
        close();
        return false;
    }

    return true;
}

void BridgeConsumer::close()
{
    if (header != nullptr)
    {
        UnmapViewOfFile(header);
        header = nullptr;
    }

    if (audioReadyEvent != nullptr)
    {
        CloseHandle(audioReadyEvent);
        audioReadyEvent = nullptr;
    }

    if (audioConsumedEvent != nullptr)
    {
        CloseHandle(audioConsumedEvent);
        audioConsumedEvent = nullptr;
    }

    if (mappingHandle != nullptr)
    {
        CloseHandle(mappingHandle);
        mappingHandle = nullptr;
    }
}

bool BridgeConsumer::isOpen() const noexcept
{
    return header != nullptr;
}

bool BridgeConsumer::waitForData(DWORD timeoutMs) const
{
    if (audioReadyEvent == nullptr)
    {
        return false;
    }

    const DWORD result = WaitForSingleObject(audioReadyEvent, timeoutMs);
    return result == WAIT_OBJECT_0;
}

bool BridgeConsumer::readAvailableFrames(std::vector<float>& frameBuffer, std::uint32_t& framesRead)
{
    framesRead = 0;
    if (header == nullptr)
    {
        return false;
    }

    const auto available = header->framesAvailable.load(std::memory_order_acquire);
    if (available == 0)
    {
        header->underruns.fetch_add(1, std::memory_order_relaxed);
        ++stats.underruns;
        return false;
    }

    const auto channels = header->channels;
    if (channels == 0)
    {
        return false;
    }

    const auto capacity = header->frameCapacity;
    const auto framesPerBlock = header->framesPerBlock != 0 ? header->framesPerBlock : available;
    const auto framesToRead = std::min(available, framesPerBlock);

    frameBuffer.resize(static_cast<std::size_t>(framesToRead * channels));

    const auto readPosition = header->readPosition.load(std::memory_order_relaxed);
    auto* payload = reinterpret_cast<const float*>(header + 1);

    const std::uint32_t firstChunk = std::min(framesToRead, capacity - readPosition);
    const std::uint32_t secondChunk = framesToRead - firstChunk;

    const std::size_t frameStride = channels;
    float* destination = frameBuffer.data();

    for (std::uint32_t i = 0; i < firstChunk; ++i)
    {
        const auto frameIndex = readPosition + i;
        const auto offset = frameIndex * frameStride;
        std::memcpy(destination + (i * frameStride), payload + offset, frameStride * sizeof(float));
    }

    for (std::uint32_t i = 0; i < secondChunk; ++i)
    {
        const auto offset = i * frameStride;
        std::memcpy(destination + ((firstChunk + i) * frameStride),
                    payload + offset,
                    frameStride * sizeof(float));
    }

    framesRead = framesToRead;
    advanceReadPointer(framesToRead);
    stats.totalFramesRead += framesToRead;

    if (audioConsumedEvent != nullptr)
    {
        SetEvent(audioConsumedEvent);
    }

    return true;
}

BridgeConsumer::Statistics BridgeConsumer::getStatistics() const noexcept
{
    return stats;
}

void BridgeConsumer::advanceReadPointer(std::uint32_t frames)
{
    const auto capacity = header->frameCapacity;
    auto readPosition = header->readPosition.load(std::memory_order_relaxed);

    readPosition = (readPosition + frames) % capacity;
    header->readPosition.store(readPosition, std::memory_order_release);
    header->framesAvailable.fetch_sub(frames, std::memory_order_release);
}

