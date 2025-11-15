# OceanAudio Technical Architecture

## High-Level Overview
OceanAudio is a high-performance Windows desktop application that captures audio from physical or virtual input devices, processes the signal through a user-configurable chain of VST plugins, and exposes the processed stream via a low-latency virtual microphone. The system is divided into three major subsystems:

1. **Audio Host Application (User Mode, C++/JUCE)** – Handles device enumeration, plugin hosting, realtime audio processing, and user interface.
2. **Virtual Audio Device (Kernel & User Mode, WDM/AVStream Driver)** – Exposes a virtual capture endpoint and relies on a companion user-mode service for buffer exchange with the host.
3. **Support Services** – Installation, update, telemetry, and crash reporting helpers.

Performance, stability, and low-latency operation on commodity hardware are first-class goals. Realtime-safe programming practices, efficient memory usage, and asynchronous UI separation are enforced throughout.

## Technology Choices
- **Language & Tooling:** Modern C++20 for the core host and driver components. CMake-based builds with presets for MSVC (Visual Studio 2022) and LLVM/Clang. Automatic formatting via clang-format and static analysis with clang-tidy.
- **UI & Audio Host Framework:** [JUCE](https://juce.com/) for its mature VST3 hosting, cross-platform audio abstractions, and GPU-accelerated UI rendering on Windows. Audio processing runs on JUCE’s `AudioProcessorGraph`.
- **Plugin Support:** VST3 exclusively (Steinberg SDK). VST2 is deprecated/licensing-restricted and will not be bundled. CLAP support can be evaluated later.
- **Virtual Device:** Custom WDM/AVStream capture driver modeled after Microsoft’s SwapAPO sample, ensuring a signed driver package. A user-mode audio engine (UMDF service) bridges the driver with the host via shared ring buffers.
- **Audio Backend:** WASAPI exclusive mode for minimal latency, with optional ASIO support when available on the target system.
- **Installer:** WiX Toolset to bundle the host app, driver, and required redistributables. Custom actions handle driver installation via `pnputil`.

## Subsystems

### 1. Audio Host Application
- **Modules**
  - `AudioEngine`: Manages WASAPI/ASIO devices, buffer scheduling, and sample rate negotiation.
  - `PluginManager`: Maintains a `KnownPluginList`, scans bundled/system VST3 directories (plus user-added folders persisted in `%AppData%\OceanAudio\PluginDirectories.json`), and instantiates plugins on demand.
  - `PluginChain`: Wraps JUCE `AudioProcessorGraph`, supports multi-slot routing, parameter automation, and preset storage.
  - `SessionManager`: Handles user profiles, stored chains, and integration with default bundled plugins.
  - `UIModule`: JUCE-based UI with live level meters, plugin chain editor, virtual I/O routing panel.
  - `PresetManager`: Loads factory/user chain presets (JSON), captures current chains, and persists user-created presets (`%AppData%\OceanAudio\Presets\UserPresets.json`).
  - `BridgeClient`: Communicates with the virtual driver service using shared memory + event handles; streams processed audio frames.
    - Allocates a global named file mapping (`OceanAudio_AudioRing`) with lock-free read/write pointers stored in a shared header and signals readiness through Win32 events.
- **Realtime Guarantees**
  - Lock-free queues for audio callbacks.
  - Avoid dynamic allocation in the realtime path.
  - SIMD optimizations (AVX2/SSE2) where applicable.

### 2. Virtual Audio Device
- **Kernel Driver (`OceanAudioVirtualMic`)**
  - AVStream miniport presenting a capture endpoint.
  - Loopback support to monitor outgoing audio.
  - Uses DMA-like circular buffer shared with user mode.
  - Signed with EV certificate during release builds.
  - Build instructions tracked in `docs/driver_setup.md` (WDK project scaffolding, INF template, signing workflow).
- **User-Mode Service (`OceanAudioBridgeService`)**
  - Runs as a Windows service under LocalSystem.
  - Receives processed audio buffers from the host via named shared memory (`Global\OceanAudio_AudioRing`).
  - Current scaffold maps the shared buffer, drains frames, and exposes console mode for debugging.
  - Next milestone: issue IOCTLs to `OceanAudioVirtualMic` (device interface `kDeviceInterfaceId`) and forward frames into the driver capture pin.
  - Provides format negotiation with the host (sample rate, channel layout).
  - Offers health monitoring and reconnection logic.

### 3. Installer & Tooling
- WiX bundle packaging host executable, service, driver (.inf, .cat, .sys), and prerequisites (VC Runtime, WDK redistributables).
- Post-install tasks: register service, install driver using `pnputil`, configure default plugin directory.

## Default Plugin Strategy
- Bundle the client-provided VST3 plugins inside the installer under `%ProgramFiles%\OceanAudio\Plugins`.
- Plugins auto-register on first launch; metadata (name, vendor, category) cached for fast enumeration.
- Users can add or remove external VST3 directories via the UI; rescans happen asynchronously to avoid blocking the audio thread.
- Baseline internal plugin suite (Core EQ, Core Compressor, Core Gate) is implemented as JUCE `juce_add_plugin` modules and compiled alongside the main build for automated testing. Their binaries ship as part of the default collection.

## Build & Repository Structure
```
/OceanAudio
├── CMakeLists.txt
├── external/         # JUCE submodule, VST3 SDK, third-party libs
├── host/
│   ├── CMakeLists.txt
│   ├── src/
│   └── resources/
├── driver/
│   ├── CMakeLists.txt
│   ├── sys/
│   └── service/
├── plugins/
│   ├── CoreEQ/
│   ├── CoreCompressor/
│   └── CoreGate/
├── installer/
│   ├── Product.wxs
│   └── Bundle.wxs
└── docs/
    └── architecture.md
```

## Next Steps
1. Add automated tests for bridge buffering and plugin parameter defaults using JUCE unit test harness.
2. Scaffold the virtual microphone driver (AVStream miniport) and corresponding UMDF bridge service.
3. Implement the bridge service consumer that maps `OceanAudio_AudioRing`, drains audio, and forwards it to the driver.
4. Implement plugin chain UI/editor with drag-and-drop reordering and preset management.
5. Integrate WiX build scripts for automated packaging.


