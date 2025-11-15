#include "BridgeConsumer.h"

#include <Windows.h>
#include <SetupAPI.h>
#include <cfgmgr32.h>
#include <objbase.h>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <cstring>

#include <OceanAudio/BridgeProtocol.h>

namespace
{
constexpr wchar_t kServiceName[] = L"OceanAudioBridgeService";
constexpr wchar_t kMappingName[] = L"Global\\OceanAudio_AudioRing";
constexpr wchar_t kReadyEventName[] = L"Global\\OceanAudio_AudioReady";
constexpr wchar_t kConsumedEventName[] = L"Global\\OceanAudio_AudioConsumed";
constexpr wchar_t kDeviceInterfaceId[] = oceanaudio::kDeviceInterfaceId;

SERVICE_STATUS_HANDLE g_statusHandle = nullptr;
HANDLE g_stopEvent = nullptr;
BridgeConsumer g_consumer;
HANDLE g_driverHandle = INVALID_HANDLE_VALUE;
GUID getBridgeInterfaceGuid()
{
    GUID guid {};
    if (FAILED(CLSIDFromString(kDeviceInterfaceId, &guid)))
    {
        return GUID {};
    }
    return guid;
}

void closeDriverHandle(HANDLE& handle)
{
    if (handle != nullptr && handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(handle);
    }
    handle = INVALID_HANDLE_VALUE;
}

HANDLE openDriverInterface()
{
#if !defined(_WIN32)
    return INVALID_HANDLE_VALUE;
#else
    GUID interfaceGuid = getBridgeInterfaceGuid();
    if (interfaceGuid == GUID {})
    {
        OutputDebugStringW(L"[OceanAudioBridgeService] Invalid device interface GUID.\n");
        return INVALID_HANDLE_VALUE;
    }

    HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(&interfaceGuid,
                                                  nullptr,
                                                  nullptr,
                                                  DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (deviceInfoSet == INVALID_HANDLE_VALUE)
    {
        return INVALID_HANDLE_VALUE;
    }

    SP_DEVICE_INTERFACE_DATA interfaceData;
    interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    HANDLE deviceHandle = INVALID_HANDLE_VALUE;

    for (DWORD index = 0; SetupDiEnumDeviceInterfaces(deviceInfoSet, nullptr, &interfaceGuid, index, &interfaceData); ++index)
    {
        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetailW(deviceInfoSet, &interfaceData, nullptr, 0, &requiredSize, nullptr);
        if (requiredSize == 0)
        {
            continue;
        }

        std::vector<wchar_t> buffer(requiredSize / sizeof(wchar_t) + 1);
        auto* detailData = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(buffer.data());
        detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

        if (!SetupDiGetDeviceInterfaceDetailW(deviceInfoSet, &interfaceData, detailData, requiredSize, nullptr, nullptr))
        {
            continue;
        }

        deviceHandle = CreateFileW(detailData->DevicePath,
                                   GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   nullptr,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   nullptr);
        if (deviceHandle != INVALID_HANDLE_VALUE)
        {
            break;
        }
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return deviceHandle;
#endif
}

bool submitFramesToDriver(HANDLE driverHandle, const std::vector<float>& buffer, std::uint32_t frames)
{
#if !defined(_WIN32)
    (void)driverHandle;
    (void)buffer;
    (void)frames;
    return false;
#else
    if (driverHandle == INVALID_HANDLE_VALUE || frames == 0 || buffer.empty())
    {
        return false;
    }

    const std::size_t payloadBytes = buffer.size() * sizeof(float);
    const std::size_t totalBytes = sizeof(oceanaudio::BridgeAudioPacket) + payloadBytes;

    std::vector<std::uint8_t> ioBuffer(totalBytes);
    auto* packet = reinterpret_cast<oceanaudio::BridgeAudioPacket*>(ioBuffer.data());
    packet->framesWritten = frames;
    packet->reserved = 0;
    std::memcpy(ioBuffer.data() + sizeof(oceanaudio::BridgeAudioPacket),
                buffer.data(),
                payloadBytes);

    DWORD bytesReturned = 0;
    const auto ioctlCode = oceanaudio::BridgeIoctlCode(oceanaudio::BridgeIoctl::SubmitFrames);
    const BOOL result = DeviceIoControl(driverHandle,
                                        ioctlCode,
                                        ioBuffer.data(),
                                        static_cast<DWORD>(ioBuffer.size()),
                                        nullptr,
                                        0,
                                        &bytesReturned,
                                        nullptr);

    if (result == FALSE)
    {
        const auto error = GetLastError();
        if (error != ERROR_NOT_SUPPORTED)
        {
            OutputDebugStringW(L"[OceanAudioBridgeService] DeviceIoControl failed when submitting frames.\n");
        }
        return false;
    }

    return true;
#endif
}

void reportServiceStatus(DWORD currentState, DWORD win32ExitCode, DWORD waitHint)
{
    SERVICE_STATUS status {};
    status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    status.dwCurrentState = currentState;

    if (currentState == SERVICE_START_PENDING)
    {
        status.dwControlsAccepted = 0;
    }
    else
    {
        status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    }

    status.dwWin32ExitCode = win32ExitCode;
    status.dwWaitHint = waitHint;
    SetServiceStatus(g_statusHandle, &status);
}

void WINAPI serviceControlHandler(DWORD controlCode)
{
    switch (controlCode)
    {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            if (g_stopEvent != nullptr)
            {
                SetEvent(g_stopEvent);
            }
            reportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
            break;
        default:
            break;
    }
}

void processAudioStream(HANDLE stopEvent)
{
    constexpr int kMaxAttempts = 50;
    int attempts = 0;
    while (!g_consumer.open(kMappingName, kReadyEventName, kConsumedEventName))
    {
        if (WaitForSingleObject(stopEvent, 100) != WAIT_TIMEOUT)
        {
            return;
        }

        if (++attempts >= kMaxAttempts)
        {
            OutputDebugStringW(L"[OceanAudioBridgeService] Failed to open shared mapping after multiple attempts.\n");
            return;
        }
    }

    OutputDebugStringW(L"[OceanAudioBridgeService] Shared audio mapping opened.\n");

    g_driverHandle = openDriverInterface();
    if (g_driverHandle == INVALID_HANDLE_VALUE)
    {
        OutputDebugStringW(L"[OceanAudioBridgeService] Driver handle unavailable; will continue without IOCTL forwarding.\n");
    }

    std::vector<float> buffer;
    buffer.reserve(48000);

    while (WaitForSingleObject(stopEvent, 0) == WAIT_TIMEOUT)
    {
        if (!g_consumer.waitForData(10))
        {
            continue;
        }

        std::uint32_t framesRead = 0;
        if (g_consumer.readAvailableFrames(buffer, framesRead))
        {
            submitFramesToDriver(g_driverHandle, buffer, framesRead);
        }
        else
        {
            // No frames available; sleep briefly to avoid busy waiting.
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    g_consumer.close();
    closeDriverHandle(g_driverHandle);
}

void WINAPI serviceMain(DWORD, LPWSTR*)
{
    g_statusHandle = RegisterServiceCtrlHandlerW(kServiceName, serviceControlHandler);
    if (g_statusHandle == nullptr)
    {
        return;
    }

    reportServiceStatus(SERVICE_START_PENDING, NO_ERROR, 0);

    g_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (g_stopEvent == nullptr)
    {
        reportServiceStatus(SERVICE_STOPPED, GetLastError(), 0);
        return;
    }

    reportServiceStatus(SERVICE_RUNNING, NO_ERROR, 0);
    processAudioStream(g_stopEvent);

    if (g_stopEvent != nullptr)
    {
        CloseHandle(g_stopEvent);
        g_stopEvent = nullptr;
    }

    reportServiceStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

int runConsoleMode()
{
    std::wcout << L"[OceanAudioBridgeService] Console mode\n";
    HANDLE stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

    std::thread worker([&]()
    {
        processAudioStream(stopEvent);
    });

    std::wcout << L"Press ENTER to exit...\n";
    std::wstring line;
    std::getline(std::wcin, line);

    SetEvent(stopEvent);
    if (worker.joinable())
    {
        worker.join();
    }
    CloseHandle(stopEvent);
    return 0;
}
} // namespace

int wmain(int argc, wchar_t* argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        if (_wcsicmp(argv[i], L"--console") == 0)
        {
            return runConsoleMode();
        }
    }

    SERVICE_TABLE_ENTRYW serviceTable[] = {
        {const_cast<LPWSTR>(kServiceName), serviceMain},
        {nullptr, nullptr},
    };

    if (!StartServiceCtrlDispatcherW(serviceTable))
    {
        return GetLastError();
    }

    return 0;
}

