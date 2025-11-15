# Driver Build & Integration Guide

This document summarises the steps required to bring up the OceanAudio virtual
microphone driver using the Windows Driver Kit (WDK) and Visual Studio.

## Prerequisites

- **Visual Studio 2022** with Desktop development (C++) workload.
- **Windows Driver Kit (WDK) 10** that matches your target Windows SDK version.
- **EV code signing certificate** (required for production-signed drivers).
- Developer machine set to **Test Signing** mode during early development.

## Repository Layout

```
driver/
├── service/        # UMDF bridge service (CMake target: OceanAudioBridgeService)
└── sys/            # AVStream virtual microphone driver placeholders
    ├── include/    # Shared driver headers
    ├── src/        # Miniport implementation (to be generated from WDK template)
    └── OceanAudioVirtualMic.inf  # Driver package manifest (template)
```

## Bringing Up the Driver Project

1. **Create the AVStream Project**
   - Launch Visual Studio with WDK integration.
   - `File > New > Project > Windows Driver > WDM > AVStream Audio Sample`.
   - Name the project `OceanAudioVirtualMic`, target the `driver/sys` directory.
   - Enable KMDF (default) and select the Windows version you plan to support.

2. **Project Configuration**
   - Set the target platform to `x64`.
   - Enable `/Zp1` structure packing if required by your audio structures.
   - Add `shared/include` to the project include path so the driver can access
     shared headers (`BridgeSharedMemory.h`, forthcoming `BridgeProtocol.h`).
   - Replace sample code with the OceanAudio-specific miniport logic (see TODOs
     in `driver/sys/src` placeholders).

3. **INF & Catalog**
   - Update `OceanAudioVirtualMic.inf` with:
     - Correct provider, class, and signature details.
     - Hardware ID (e.g. `ROOT\OCEANAUDIO_VIRTUALMIC`).
     - Reference to the UMDF service (added via co-installers or custom
       install steps).
   - During development, use test certificates to sign the catalog (`.cat`).

4. **Build & Deploy**
   - Build the driver from Visual Studio (use `Build > Build Solution`).
   - Use `pnputil /add-driver Output\OceanAudioVirtualMic.inf /install` to
     install on the test machine (test-signing must be enabled).
   - Verify the virtual microphone endpoint appears in the Sound control panel
     under Recording devices.

## Driver ↔ Service Bridge

The driver uses an AVStream miniport that exposes a capture pin backed by a
shared ring buffer filled by the UMDF bridge service.

1. **Shared Memory Channel**
   - Created by the host (`BridgeClient`) under the name
     `Global\OceanAudio_AudioRing`.
   - Header layout defined in `shared/include/OceanAudio/BridgeSharedMemory.h`.
   - Driver will expose IOCTLs (e.g. `IOCTL_OCEANAUDIO_QUERY_RING`) to map the
     buffer into kernel space or negotiate user-mode access.

2. **UMDF Bridge Service**
   - Builds via CMake target `OceanAudioBridgeService`.
   - Currently able to run in console mode (`--console`) to drain the ring
     buffer.
   - TODO: implement the call-out to the driver (IOCTL write) so frames pulled
     from shared memory are written to the capture pin buffer submitted by the
     audio engine (see service TODO markers).

3. **Handshake Flow**
   - Service attaches to driver via device interface GUID (to be defined).
   - Service maps the ring buffer (or sends `IOCTL` commands) to deliver audio
     frames.
   - Driver signals completion events back to service when buffers are consumed.
   - Current implementation contains placeholder IOCTL (`SubmitFrames`) and
     device enumeration logic; replace the stub once the AVStream driver exposes
     the final control codes.

## Next Steps / TODOs

- [ ] Generate WDK AVStream project scaffolding inside `driver/sys`.
- [ ] Define IOCTL contract (`shared/include/OceanAudio/BridgeProtocol.h`) for
      service ↔ driver communication.
- [ ] Implement UMDF service device discovery & IOCTL loop.
- [ ] Flesh out driver miniport (state machine, format negotiation, DMA buffers).
- [ ] Provide automated packaging (WiX) that installs both driver & service
      with proper signing.

Refer back to this document as milestones are completed; update the TODO list
as features land.

