#pragma once

#include <cstdint>
#include <atomic>

namespace oceanaudio
{
struct SharedAudioRingBufferHeader
{
    static constexpr std::uint32_t kMagic = 0x4F415342; // 'OASB'
    static constexpr std::uint32_t kVersion = 1;

    std::uint32_t magic = kMagic;
    std::uint32_t version = kVersion;
    std::uint32_t channels = 0;
    std::uint32_t sampleRate = 0;
    std::uint32_t frameCapacity = 0;
    std::uint32_t framesPerBlock = 0;
    std::atomic<std::uint32_t> writePosition {0};
    std::atomic<std::uint32_t> readPosition {0};
    std::atomic<std::uint32_t> framesAvailable {0};
    std::atomic<std::uint32_t> overruns {0};
    std::atomic<std::uint32_t> underruns {0};

    [[nodiscard]] std::uint32_t bytesPerFrame() const noexcept
    {
        return channels * sizeof(float);
    }

    [[nodiscard]] std::uint32_t bufferSizeBytes() const noexcept
    {
        return frameCapacity * bytesPerFrame();
    }
};
} // namespace oceanaudio

