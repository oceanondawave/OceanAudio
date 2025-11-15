#pragma once

#include <OceanAudio/BridgeSharedMemory.h>

#include <Windows.h>

#include <cstdint>
#include <string>
#include <vector>

class BridgeConsumer
{
public:
    BridgeConsumer();
    ~BridgeConsumer();

    BridgeConsumer(const BridgeConsumer&) = delete;
    BridgeConsumer& operator=(const BridgeConsumer&) = delete;

    bool open(const std::wstring& mappingName,
              const std::wstring& readyEventName,
              const std::wstring& consumedEventName);
    void close();

    [[nodiscard]] bool isOpen() const noexcept;

    bool waitForData(DWORD timeoutMs) const;
    bool readAvailableFrames(std::vector<float>& frameBuffer, std::uint32_t& framesRead);

    struct Statistics
    {
        std::uint64_t totalFramesRead = 0;
        std::uint64_t underruns = 0;
    };

    Statistics getStatistics() const noexcept;

private:
    void advanceReadPointer(std::uint32_t frames);

    HANDLE mappingHandle;
    HANDLE audioReadyEvent;
    HANDLE audioConsumedEvent;
    oceanaudio::SharedAudioRingBufferHeader* header;
    Statistics stats;
};

