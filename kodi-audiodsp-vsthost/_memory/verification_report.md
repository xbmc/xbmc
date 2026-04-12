# VST Host Addon — Verification Report

## Files Verified

| File | Status |
|------|--------|
| `CMakeLists.txt` | Fixed — added `VST3SDK_AVAILABLE` define |
| `src/addon_main.cpp` | Fixed — config path double-append bug |
| `src/addon_main.h` | OK |
| `src/plugin/IVSTPlugin.h` | OK |
| `src/util/ParamQueue.h` | OK |
| `src/vst2/VSTPlugin2.h` | OK |
| `src/vst2/VSTPlugin2.cpp` | Fixed — `effSetParameter` → direct function call |
| `src/vst2/vestige/aeffectx.h` | OK |
| `src/vst2/vst2_types.h` | OK |
| `src/vst3/VSTPlugin3.h` | Fixed — added `ParameterChanges` param to drainParamQueue |
| `src/vst3/VSTPlugin3.cpp` | Fixed — parameter delivery bug; updated drainParamQueue |
| `src/vst3/VSTHostContext.h` | OK |
| `src/vst3/VSTHostContext.cpp` | OK |
| `src/dsp/DSPChain.h` | OK |
| `src/dsp/DSPChain.cpp` | OK |
| `src/dsp/DSPProcessor.h` | OK |
| `src/dsp/DSPProcessor.cpp` | OK |
| `src/settings/VSTPluginManager.h` | OK |
| `src/settings/VSTPluginManager.cpp` | OK |
| `src/settings/PluginSettings.h` | OK |
| `src/settings/PluginSettings.cpp` | OK |
| `scanner/vstscanner.cpp` | Fixed — SEH+C++ mixing; LoadLibrary SEH; VST3SDK_AVAILABLE |
| `resources/addon.xml` | OK |

---

## Issues Found and Fixed

### Issue 1 — `DSPProcessor::loadConfig()` / `saveConfig()` double-appended `/chain.json`

**File:** `src/addon_main.cpp`, line 170 (original)

**Description:** `addon_main.cpp` called `proc->setConfigPath(g_addonDataPath + "/chain.json")`,
passing a complete file path. `DSPProcessor::loadConfig()` and `saveConfig()` then appended
`"/chain.json"` again, producing a path like `C:/user/data/chain.json/chain.json`.

**Fix:** Changed `addon_main.cpp` to call `proc->setConfigPath(g_addonDataPath)` — pass the
directory, not the file path. `DSPProcessor` already appends the filename internally.

---

### Issue 2 — `VSTPlugin2::drainParamQueue()` used non-existent opcode `effSetParameter`

**File:** `src/vst2/VSTPlugin2.cpp`, `drainParamQueue()` and `loadState()`

**Description:** The code called `m_effect->dispatcher(m_effect, effSetParameter, ...)` but
`effSetParameter` is not defined anywhere in `aeffectx.h` (it is not a valid VST2 dispatcher
opcode). The VST2 ABI for setting a parameter is a direct function-pointer call:
`effect->setParameter(effect, index, value)`.

**Fix:** Replaced both occurrences of the dispatcher call with
`m_effect->setParameter(m_effect, index, value)`.

---

### Issue 3 — `VSTPlugin3::drainParamQueue()` parameters never delivered to processor

**File:** `src/vst3/VSTPlugin3.cpp`, `drainParamQueue()`

**Description:** The original implementation created a `ParameterChanges paramChanges` local
variable inside the loop, set `m_processData.inputParameterChanges = &paramChanges`, but then
the `paramChanges` object went out of scope at the end of each loop iteration. After the loop,
`process()` immediately cleared `inputParameterChanges = nullptr`. Parameters were enqueued
but never actually delivered to the processor.

**Fix:** Changed `drainParamQueue()` to accept a `Steinberg::Vst::ParameterChanges&` parameter.
`process()` now allocates a stack-local `blockParamChanges`, passes it to `drainParamQueue()`,
then sets `m_processData.inputParameterChanges = &blockParamChanges` before calling
`m_processor->process()`. The stack-local is automatically destroyed at the end of `process()`,
cleanly resetting state for the next block.

---

### Issue 4 — `VST3SDK_AVAILABLE` preprocessor macro never defined

**File:** `CMakeLists.txt`

**Description:** The `vstscanner.cpp` conditionally compiles a full VST3 SDK path gated on
`#ifdef VST3SDK_AVAILABLE`, but this macro was never defined anywhere in the build system —
not even when the `vst3_hosting` library sources were present. The scanner always took the
raw vtable fallback path.

**Fix:** Added to `CMakeLists.txt`:
```cmake
if(VST3_HOSTING_SOURCES_FOUND)
    target_compile_definitions(vstscanner PRIVATE VST3SDK_AVAILABLE)
endif()
```

---

### Issue 5 — `vstscanner.cpp` SEH mixed with C++ destructors (MSVC C4509)

**Files:** `scanner/vstscanner.cpp`

**Description:** The `scanVST3` fallback path had `__try/__except` blocks in the same function
scope as C++ objects with destructors (`PluginInfo` contains `std::string` members). MSVC
forbids this combination and emits error C4509. Similarly, `scanFile` had a `LoadLibraryA`
wrapped in `__try/__except` alongside `PluginInfo info`.

**Fix 1:** Added a new `loadLibrarySafe()` function that wraps `LoadLibraryA` in a standalone
SEH-only function with no C++ locals. Used in both `scanVST2` and `scanFile`.

**Fix 2:** Extracted the entire VST3 fallback body into `scanVST3Raw()` — a pure-POD function
that returns a `Vst3RawResult` struct (char arrays, no std::string). All the SEH-guarded COM
vtable calls live exclusively in this function. The `scanVST3` caller then builds `std::string`
results from the returned POD struct, keeping the C++ strings safely outside any SEH scope.

---

## Issues Found but NOT Fixed (require manual attention)

### Latency reporting in seconds vs. samples

**File:** `src/addon_main.cpp`, `MasterProcessGetDelay()`

`DSPProcessor::masterProcessGetDelay()` returns an `int` (sample count).
`MasterProcessGetDelay()` casts this to `float` and returns it. However, the Kodi ADSP API
documentation says this value should be in **seconds**. The current implementation returns
sample count as a float, which is only correct if Kodi interprets it as samples rather than
seconds. Correct implementation would be:
```cpp
return static_cast<float>(GetProc(handle)->masterProcessGetDelay())
       / static_cast<float>(GetProc(handle)->getSampleRate());
```
This is a semantic correctness issue, not a compilation issue. Left as-is to avoid changing
runtime behavior without understanding how this specific Kodi version interprets the value.

### VSTPlugin3 `ParameterChanges` allocation on audio thread

When there are pending parameter changes, `process()` allocates a stack-local
`Steinberg::Vst::ParameterChanges blockParamChanges`. The VST3 SDK `ParameterChanges`
class internally uses `std::vector`, so `addParameterData()` may heap-allocate. This is
a potential real-time safety issue. For production use, a pre-allocated pool or a
lock-free queue that drains directly into process data should be used.

### VST3 `numParams` always 0 in scanner

By design — documented in `task6_scanner.md`. VST3 parameter count requires full plugin
instantiation which risks crashes in the scanner process.

---

## API Compliance

All Kodi ADSP C function signatures were verified against the actual SDK headers
(`kodi_adsp_dll.h`, `xbmc_addon_dll.h`). Findings:

| Function | Header expects | Implementation | Status |
|----------|---------------|----------------|--------|
| `ADDON_Create` | `ADDON_STATUS (void*, void*)` | Matches | OK |
| `ADDON_Destroy` | `void ()` | Matches | OK |
| `ADDON_GetStatus` | `ADDON_STATUS ()` | Matches | OK |
| `ADDON_HasSettings` | `bool ()` | Matches | OK |
| `ADDON_GetSettings` | `unsigned int (ADDON_StructSetting***)` | Matches | OK |
| `ADDON_SetSetting` | `ADDON_STATUS (const char*, const void*)` | Matches | OK |
| `ADDON_Stop` | `void ()` | Matches | OK |
| `ADDON_FreeSettings` | `void ()` | Matches | OK |
| `GetAudioDSPAPIVersion` | `const char* ()` | Matches | OK |
| `GetMinimumAudioDSPAPIVersion` | `const char* ()` | Matches | OK |
| `GetGUIAPIVersion` | `const char* ()` | Matches | OK |
| `GetMinimumGUIAPIVersion` | `const char* ()` | Matches | OK |
| `GetAddonCapabilities` | `AE_DSP_ERROR (AE_DSP_ADDON_CAPABILITIES*)` | Matches | OK |
| `GetDSPName` | `const char* ()` | Matches | OK |
| `GetDSPVersion` | `const char* ()` | Matches | OK |
| `CallMenuHook` | `AE_DSP_ERROR (const AE_DSP_MENUHOOK&, const AE_DSP_MENUHOOK_DATA&)` | Matches | OK |
| `StreamCreate` | `AE_DSP_ERROR (const AE_DSP_SETTINGS*, const AE_DSP_STREAM_PROPERTIES*, ADDON_HANDLE)` | Matches | OK |
| `StreamDestroy` | `AE_DSP_ERROR (const ADDON_HANDLE)` | Matches | OK |
| `StreamIsModeSupported` | `AE_DSP_ERROR (const ADDON_HANDLE, AE_DSP_MODE_TYPE, unsigned int, int)` | Matches | OK |
| `StreamInitialize` | `AE_DSP_ERROR (const ADDON_HANDLE, const AE_DSP_SETTINGS*)` | Matches | OK |
| `InputProcess` | `bool (const ADDON_HANDLE, const float**, unsigned int)` | Matches | OK |
| `MasterProcessSetMode` | `AE_DSP_ERROR (const ADDON_HANDLE, AE_DSP_STREAMTYPE, unsigned int, int)` | Matches | OK |
| `MasterProcessNeededSamplesize` | `unsigned int (const ADDON_HANDLE)` | Matches | OK |
| `MasterProcessGetDelay` | `float (const ADDON_HANDLE)` | Matches (value semantics TBD) | OK |
| `MasterProcessGetOutChannels` | `int (const ADDON_HANDLE, unsigned long&)` | Matches | OK |
| `MasterProcess` | `unsigned int (const ADDON_HANDLE, float**, float**, unsigned int)` | Matches | OK |
| `MasterProcessGetStreamInfoString` | `const char* (const ADDON_HANDLE)` | Matches | OK |
| `ADDON_Announce` | Not in SDK headers | Implemented (no-op) | OK — extra, harmless |

Note: `ADDON_Announce` is not declared in any SDK header. It was implemented as a stub
for forward compatibility. Because `WINDOWS_EXPORT_ALL_SYMBOLS` is set, it will be exported,
which is harmless.

---

## Known Limitations

1. VST3 `numParams`, `numInputs`, `numOutputs` are always 0 in scanner output —
   requires full audio processor instantiation to determine.
2. 32-bit plugins cannot be scanned by a 64-bit scanner (and vice versa).
   Bitness mismatch results in `LoadLibrary` failures.
3. VST2 shell plugins (kPlugCategShell) — only the top-level plugin name is reported;
   sub-plugin IDs are not iterated.
4. `MasterProcessGetDelay` returns sample count as float, not time in seconds.
   May cause incorrect A/V sync in Kodi if Kodi expects seconds.
5. `VSTPlugin3::drainParamQueue()` may heap-allocate on the audio thread when parameters
   are pending (via `ParameterChanges::addParameterData`).

---

## Build Instructions

```
cd C:/src/xbmc/kodi-audiodsp-vsthost

# Option A: with VST3 SDK as git submodule
git submodule update --init --recursive
cmake -B build -DCMAKE_BUILD_TYPE=Release -DKODI_ADSP_SDK_DIR=C:/src/xbmc/xbmc/addons/kodi-addon-dev-kit/include/kodi
cmake --build build --config Release

# Option B: let FetchContent download the SDK (requires internet access)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DKODI_ADSP_SDK_DIR=C:/src/xbmc/xbmc/addons/kodi-addon-dev-kit/include/kodi
cmake --build build --config Release
```

Output files:
- `build/Release/audiodsp.vsthost.dll` — Kodi ADSP addon
- `build/Release/vstscanner.exe` — out-of-process plugin scanner

---

## Summary

Six bugs were identified and fixed:

1. **Config path double-append** (`addon_main.cpp`) — would have caused chain.json to never
   load or save at runtime.
2. **`effSetParameter` undefined opcode** (`VSTPlugin2.cpp`) — would have caused a compile
   error. VST2 parameter setting uses `effect->setParameter()` directly, not a dispatcher call.
3. **VST3 parameter delivery broken** (`VSTPlugin3.cpp`) — parameters from the ring buffer
   were enqueued but silently discarded without being delivered to the processor.
4. **`VST3SDK_AVAILABLE` never defined** (`CMakeLists.txt`) — the scanner always took the
   raw vtable fallback path even when the full SDK was available.
5. **SEH + C++ destructor mixing in `scanVST3`** (`vstscanner.cpp`) — MSVC C4509 violation;
   fixed by extracting all SEH-guarded work into a pure-POD helper function.
6. **Unguarded `LoadLibraryA` in `scanVST2` and `scanFile`** (`vstscanner.cpp`) — now uses
   the `loadLibrarySafe()` SEH wrapper for crash containment consistency.

The codebase is otherwise well-structured with correct VST2 ABI usage (effSetSampleRate via
`opt`, 64-byte param name buffers, callPluginMainSafe SEH isolation), correct Kodi ADSP API
compliance, and a sound ping-pong buffer design in `DSPChain`.
