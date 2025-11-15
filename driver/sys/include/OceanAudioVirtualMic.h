#pragma once

#include <ntddk.h>
#include <wdf.h>

#include <ks.h>
#include <ksmedia.h>

#include <OceanAudio/BridgeSharedMemory.h>
#include <OceanAudio/BridgeProtocol.h>

// Placeholder declarations for the AVStream miniport implementation. The real
// driver should provide structures for device context, pin descriptors, and
// DMA buffer management. This header exists so the UMDF service and other
// components can reference common enums / GUIDs.

// TODO: define the KS category GUID once the driver project is generated.
// DEFINE_GUIDSTRUCT("00000000-0000-0000-0000-000000000000", KSCATEGORY_OCEANAUDIO);

// Device context storing the shared memory state negotiated with the user-mode
// bridge service. Fill in with real fields when the WDK project is created.
typedef struct _OCEANAUDIO_DEVICE_CONTEXT
{
    PVOID SharedBuffer;
    SIZE_T SharedBufferSize;
    oceanaudio::SharedAudioRingBufferHeader Header;
} OCEANAUDIO_DEVICE_CONTEXT, *POCEANAUDIO_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(OCEANAUDIO_DEVICE_CONTEXT, OceanAudioGetContext);

