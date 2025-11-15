# OceanAudio

OceanAudio is a Windows desktop application that captures audio from your inputs, processes the signal through VST3 plugins, and exposes the result as a low-latency virtual microphone.

## Status
- ✅ Architecture defined
- ⏳ Implementation in progress

## Documentation
- `docs/architecture.md` – high-level system design and technology choices.
- `docs/driver_setup.md` – WDK driver scaffolding, build, and deployment notes.

## Roadmap (short-term)
1. Scaffold virtual microphone driver (AVStream) and UMDF bridge service projects.
2. Implement service-side shared-memory consumer and driver buffer handoff.
3. Build plugin chain UI with drag-and-drop routing and plugin directory management.

## Building (Windows)

### Quick Start
1. **Install Prerequisites**
   - Visual Studio 2022 with "Desktop development with C++"
   - CMake 3.24+ (https://cmake.org/download/)
   - Git (https://git-scm.com/download/win)

2. **Download Dependencies** (one-time setup)
   ```bash
   setup_dependencies.bat
   ```
   This downloads JUCE and VST3 SDK to `external/` folder.
   
   *Alternative*: If you have internet access, CMake will auto-download them.

3. **Generate Build Files**
   ```bash
   cmake -S . -B build -G "Visual Studio 17 2022"
   ```

4. **Build**
   ```bash
   cmake --build build --config Release --target OceanAudioHost
   cmake --build build --config Release --target OceanAudioBridgeService
   cmake --build build --config Release --target CoreEQ_VST3
   cmake --build build --config Release --target CoreCompressor_VST3
   cmake --build build --config Release --target CoreGate_VST3
   ```

See `docs/build_windows.md` for detailed instructions and troubleshooting.

## Components
- `host/` – JUCE desktop app with audio engine, plugin chain, and shared-memory bridge client.
- `plugins/` – Core VST3 suite (EQ, Compressor, Gate) compiled via `juce_add_plugin`.
- `driver/service/` – UMDF bridge service scaffold that consumes the shared ring buffer (console and service modes).
- `driver/` – Placeholder for AVStream driver + UMDF bridge service (up next).
- `installer/` – WiX project skeleton.

## UI Overview
- Left panel lists discovered VST3 plugins (bundled + system paths); double-click to insert into the active chain. Use `Add Directory` to index custom plugin folders (persisted between launches).
- Right panel shows the processing order; drag rows or use `Move Up/Move Down` to reorder, double-click (or `Remove`) to drop a plugin.
- Status banner reports audio device state plus shared-memory telemetry (queued/dropped frames).
- Preset column lets you load factory/user chains, save the current setup, and delete user-defined presets.
- Presets and directory lists persist in `%AppData%\OceanAudio` (e.g. `Presets\UserPresets.json`, `PluginDirectories.json`), so custom setups carry across launches.
