---
name: Task 4 — VSTPlugin2 implementation
description: VST2 plugin wrapper — implementation decisions, gotchas, and public API surface
type: project
---

## Files Created

| File | Purpose |
|------|---------|
| `src/vst2/VSTPlugin2.h`   | Class declaration — inherits IVSTPlugin, declares all private members |
| `src/vst2/VSTPlugin2.cpp` | Full implementation of the VST2 host wrapper |

## Dependencies

| Header | Role |
|--------|------|
| `src/plugin/IVSTPlugin.h`      | Pure abstract base class |
| `src/vst2/vestige/aeffectx.h`  | VST2 binary ABI (opcodes, AEffect struct, VSTPluginMainProc) |
| `src/vst2/vst2_types.h`        | VstInt32, VstIntPtr, VstProcessLevel enums (included via aeffectx.h) |
| `src/util/ParamQueue.h`        | RingBuffer<T,N> template + ParamChange2 struct |
| `<windows.h>`                  | HMODULE, LoadLibraryW, GetProcAddress, FreeLibrary, SEH |

## Class Layout

```
VSTPlugin2 : public IVSTPlugin
  Private state:
    HMODULE  m_hModule          — Windows DLL handle
    AEffect* m_effect           — plugin object returned by VSTPluginMain
    string   m_path/name/vendor
    double   m_sampleRate       — current configured sample rate
    int      m_blockSize        — max frames per process() call
    int      m_numChannels      — host-side channel count
    bool     m_loaded/m_active
    RingBuffer<ParamChange2,256> m_paramQueue
    vector<vector<float>> m_inputBufs, m_outputBufs   — channel scratch buffers
    vector<float*>        m_inputPtrs, m_outputPtrs   — raw pointer arrays for processReplacing
```

## load() Sequence

1. `LoadLibraryW` (UTF-8 path converted with MultiByteToWideChar)
2. `GetProcAddress("VSTPluginMain")` → fallback to `"main"`
3. `callPluginMainSafe(proc)` — SEH-guarded
4. Validate `effect->magic == kEffectMagic`
5. `effect->user = this` — stores host context
6. `dispatcher(effOpen, ...)`
7. `dispatcher(effSetSampleRate, 0, 0, nullptr, (float)sampleRate)` — **opt, not value**
8. `dispatcher(effSetBlockSize, 0, blockSize, nullptr, 0.0f)` — value
9. `dispatcher(effMainsChanged, 0, 1, ...)` — resume
10. `dispatcher(effGetEffectName, ...)` → fallback to `filesystem::path::stem`
11. `dispatcher(effGetVendorString, ...)`
12. `allocateScratchBuffers()`
13. Set `m_loaded = m_active = true`

## Critical Implementation Gotchas

### 1. effSetSampleRate uses `opt`, NOT `value`

The VST2 spec passes the sample rate in the **float `opt`** argument of the
dispatcher call.  Passing it as `VstIntPtr value` is silently ignored by all
real plugins.

```cpp
// WRONG — common mistake
dispatcher(fx, effSetSampleRate, 0, (VstIntPtr)44100, nullptr, 0.0f);

// CORRECT
dispatcher(fx, effSetSampleRate, 0, 0, nullptr, (float)sampleRate);
```

### 2. effGetParamName — allocate 64 bytes, not 8

The VST2 spec says parameter name strings fit in 8 bytes (`char[8]`).
In practice nearly every plugin writes beyond this limit.  All calls to
`effGetParamName`, `effGetParamLabel`, and `effGetParamDisplay` use a
`char[64]` buffer to avoid stack corruption.

### 3. SEH and C++ destructors cannot coexist in the same function

MSVC emits warning C4509 (error in /W4 builds) if a function contains both
a `__try`/`__except` block and a local C++ object with a non-trivial
destructor.  Solution: isolate the `__try` block in its own function
(`callPluginMainSafe`) that holds only a plain pointer — no destructors in
scope at all.

### 4. AEffect::user is written by the HOST, not the plugin

After `VSTPluginMain` returns `AEffect*`, the host immediately sets
`effect->user = this`.  The static trampoline `staticAudioMaster` then
recovers the `VSTPlugin2*` via `static_cast<VSTPlugin2*>(effect->user)`.

### 5. effSetParameter uses the `opt` (float) argument for the value

When draining the parameter queue the value is passed via the `opt` float
argument of dispatcher, consistent with the VST2 ABI for `effSetParameter`.

## audioMaster() Handled Opcodes

| Opcode | Response |
|--------|----------|
| `audioMasterVersion`                | 2400 |
| `audioMasterGetSampleRate`          | (VstIntPtr)m_sampleRate |
| `audioMasterGetBlockSize`           | m_blockSize |
| `audioMasterGetCurrentProcessLevel` | kVstProcessLevelRealtime (2) |
| `audioMasterGetVendorString`        | "Kodi VST Host" |
| `audioMasterGetProductString`       | "Kodi AudioDSP" |
| `audioMasterGetVendorVersion`       | 1000 |
| `audioMasterCanDo`                  | 0 (no MIDI, no time info) |
| all others                          | 0 |

## process() Behaviour

1. `drainParamQueue()` — apply any pending parameter changes
2. Bypass check — if `!m_loaded || m_bypassed`, copy in→out and return
3. Clamp input channel count to `min(pluginIns, m_numChannels)`
4. Zero-fill any excess plugin input channels
5. Call `effect->processReplacing(effect, inputPtrs, outputPtrs, samples)`
6. Copy `min(pluginOuts, m_numChannels)` output channels to caller
7. Mono fold-down: if `pluginOuts == 1 && m_numChannels >= 2`, copy out[0]→out[1]

## State Persistence Format

Matches the contract defined in task3_interface.md:

| First byte | Remaining bytes | When used |
|------------|-----------------|-----------|
| `'C'`      | raw chunk blob  | `effFlagsProgramChunks` is set in `effect->flags` |
| `'P'`      | numParams × float (little-endian native) | fallback parameter dump |

## Thread Safety

| Method | Permitted thread |
|--------|-----------------|
| `load()` / `unload()` / `reinitialize()` | Settings/stream thread only |
| `process()` | Audio render thread only |
| `setParameter()` | Any thread — pushes to RingBuffer |
| `getParameter()` | Any thread (approximate, display use only) |
| `saveState()` / `loadState()` | Settings thread — never overlaps process() |

## Public API Surface (methods beyond IVSTPlugin defaults)

`VSTPlugin2` adds no public methods beyond those declared in `IVSTPlugin`.
The constructor takes a single `const std::string& path` argument — the
absolute filesystem path to the VST2 `.dll`.

```cpp
auto plugin = std::make_unique<VSTPlugin2>("C:/VST/MyEffect.dll");
if (plugin->load(48000.0, 512, 2)) {
    // ready to call process()
}
```
