# OceanAudio Virtual Microphone Driver (AVStream)

This directory will contain the kernel-mode AVStream miniport driver that exposes the virtual microphone capture endpoint. The project is intentionally not built by the default CMake configuration because it requires the Windows Driver Kit (WDK) toolchain.

## Planned Layout
- `OceanAudioVirtualMic.vcxproj` – Visual Studio WDK driver project (to be generated via the WDK templates).
- `OceanAudioVirtualMic.inf` – Driver installation INF.
- `OceanAudioVirtualMic.idl` – Optional AVStream topology descriptions.
- `include/` – Shared driver headers.
- `src/` – Miniport implementation files.

## Build Notes
1. Install the latest Windows Driver Kit (WDK) alongside Visual Studio 2022.
2. Generate the driver project using the “AVStream Virtual Audio Device” template.
3. Update the project to reference the shared `OceanAudio_AudioRing` header for buffer negotiation with the UMDF bridge service.
4. Add a custom MSBuild step (or CMake `add_custom_target`) to invoke the WDK build once the project files are committed.

