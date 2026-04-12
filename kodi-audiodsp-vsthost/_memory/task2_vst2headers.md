---
name: Task 2 — VST2 ABI headers
description: aeffectx.h and vst2_types.h locations, key VST2 gotchas
type: project
---

## Files Created

| File | Purpose |
|------|---------|
| `src/vst2/vst2_types.h` | Platform integer typedefs (`VstInt32`, `VstIntPtr`) and enums for process level and language |
| `src/vst2/vestige/aeffectx.h` | Full VST2 binary ABI header — clean-room GPLv2 (Javier Serrano Polo / LMMS lineage) |

## Include Order

`aeffectx.h` includes `../../vst2/vst2_types.h` — so consumers only need to include `aeffectx.h`.

## VSTCALLBACK Macro

On Windows (`_WIN32`) `VSTCALLBACK` expands to `__cdecl`. This is mandatory: VST2 plugin entry points and all function pointers in `AEffect` use the cdecl calling convention. Getting this wrong causes a crash on first dispatcher call because the stack is not cleaned up correctly.

On all other platforms the macro expands to nothing (the platform default is already correct).

## AEffect Struct — Critical Offset Map

```
0x00  magic              — must be kEffectMagic ('VstP')
0x04  dispatcher         — all host->plugin opcodes go here
0x08  process            — DEPRECATED, skip
0x0C  setParameter
0x10  getParameter
0x14  numPrograms
0x18  numParams
0x1C  numInputs
0x20  numOutputs
0x24  flags              — effFlags* bitmask
0x28  ptr1               — reserved NULL
0x2C  ptr2               — reserved NULL
0x30  initialDelay       — latency in samples
0x34  empty3a            — reserved
0x38  empty3b            — reserved
0x3C  unknown_float      — internal, do not rely on
0x40  ptr3               — plugin-private
0x44  user               — HOST CONTEXT POINTER (see below)
0x48  uniqueID
0x4C  version
0x50  processReplacing   — MAIN AUDIO PATH
0x54  processDoubleReplacing — 64-bit (may be NULL)
0x58  future[56]         — zero padding
```

Total size on 32-bit: 0x58 + 56 = 144 bytes.
Total size on 64-bit: larger due to pointer widening — do NOT hardcode sizes.

## Critical Gotchas

### effSetSampleRate uses `opt`, not `value`

```cpp
// WRONG — common mistake
dispatcher(fx, effSetSampleRate, 0, (VstIntPtr)44100, nullptr, 0.0f);

// CORRECT — sample rate is passed in the float `opt` argument
dispatcher(fx, effSetSampleRate, 0, 0, nullptr, 44100.0f);
```

The `opt` argument is a `float` in the dispatcher signature. Passing the rate as `value` (a `VstIntPtr`) is silently ignored by all real plugins.

### effGetParamName: official 8 chars, allocate 64

The VST2 spec says parameter name strings fit in 8 bytes. In practice nearly every plugin writes more. Always allocate at least 64 bytes for the destination buffer when calling `effGetParamName` (index=param, ptr=buffer).

```cpp
char paramName[64] = {};
dispatcher(fx, effGetParamName, paramIndex, 0, paramName, 0.0f);
```

Same caution applies to `effGetParamLabel` and `effGetParamDisplay`.

### AEffect::user — the host context pointer

`AEffect::user` is **written by the host** (not the plugin). After `VSTPluginMain` returns the `AEffect*`, the host stores its own context pointer in `AEffect::user`:

```cpp
AEffect* fx = VSTPluginMain(hostCallback);
fx->user = static_cast<void*>(this);  // e.g. VSTPlugin2 instance
```

Inside `audioMasterCallback` the host recovers itself:

```cpp
VstIntPtr VSTCALLBACK hostCallback(AEffect* fx, VstInt32 opcode, ...)
{
    auto* plugin = static_cast<VSTPlugin2*>(fx->user);
    // ...
}
```

This is the standard pattern used by VSTPlugin2, Carla, and yabridge.

### VSTPluginMainProc — dual entry-point names

Most VST2 plugins export `VSTPluginMain`. Older plugins (pre-2.4) export `main` or `main_macho` (macOS). When loading, attempt `VSTPluginMain` first, then fall back to `main`.

```cpp
auto mainProc = (VSTPluginMainProc)dlsym(handle, "VSTPluginMain");
if (!mainProc)
    mainProc = (VSTPluginMainProc)dlsym(handle, "main");
```

## License Note

`aeffectx.h` is GPLv2. The kodi-audiodsp-vsthost addon must be distributed under GPLv2 or compatible terms. This is already consistent with Kodi's own license.
