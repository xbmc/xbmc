# VST Host Addon — Comprehensive Issue Analysis

> **Source files analysed:** `kodi-audiodsp-vsthost/` (CMakeLists.txt, all `.cpp`/`.h` under `src/`, `scanner/vstscanner.cpp`), `.github/workflows/build-vst-addons.yaml`, `.gitmodules`  
> **Date:** 2026-04-22

---

## Summary Table

| # | Severity | Category | Short Description | File(s) |
|---|----------|----------|-------------------|---------|
| 1 | 🔴 Build | Linker | `baseiids.cpp` missing → `IString::iid` unresolved | `CMakeLists.txt` |
| 2 | 🔴 Crash | Runtime UB | Dangling `inputParameterChanges` in VSTPlugin3 on bypass | `VSTPlugin3.cpp` |
| 3 | 🔴 Crash | Runtime | Buffer overflow in `DSPChain::process` (samples > blockSize) | `DSPChain.cpp` |
| 4 | 🔴 Crash | Runtime | `processReplacing` null dereference on old VST2 plugins | `VSTPlugin2.cpp` |
| 5 | 🔴 Crash | Runtime | Dangling `m_chain` in `EditorBridge` after `StreamDestroy` | `addon_main.cpp` |
| 6 | 🔴 Crash | Race | Data race on `m_chain` between UI thread and `stop()` | `EditorBridge.cpp` |
| 7 | 🟠 Bug | Functional | `MasterProcessGetDelay` returns samples, Kodi expects seconds | `addon_main.cpp` |
| 8 | 🟠 Bug | Functional | VST3 editor always opens 640×480; `getEditorSize` never works | `EditorBridge.cpp`, `VSTPlugin3.cpp` |
| 9 | 🟠 Bug | Functional | `IComponent` ↔ `IEditController` not connected via `IConnectionPoint` | `VSTPlugin3.cpp` |
| 10 | 🟠 Bug | Real-time | Heap alloc in audio thread during parameter delivery | `VSTPlugin3.cpp` |
| 11 | 🟡 Build | Workflow | `KODI_ADSP_SDK_DIR` path wrong in workflow | `build-vst-addons.yaml` |
| 12 | 🟡 Build | Config | `.gitmodules` `main` branch vs CMakeLists.txt tag `v3.7.9_build_61` | `.gitmodules` |
| 13 | 🟡 Quality | Code | JSON utilities duplicated in 4 files with subtle differences | `DSPChain.cpp`, `EditorBridge.cpp`, `VSTPluginManager.cpp`, `PluginSettings.cpp` |
| 14 | 🟡 Quality | Performance | `VSTPlugin3::hasEditor()` creates a view on every call | `VSTPlugin3.cpp` |
| 15 | 🟡 Quality | Logic | `ChainPlugin::bypassed` and `IVSTPlugin::m_bypassed` can desync | `DSPChain.h`, `DSPChain.cpp` |
| 16 | 🟡 Quality | Cleanup | Unnecessary `<windows.h>` in `addon_main.cpp` | `addon_main.cpp` |
| 17 | 🟡 Quality | Robustness | `DSPChain` ignores `process()` return value; silent corruption | `DSPChain.cpp` |

---

## 🔴 Category 1 — Build-Blocking / Linker Errors

### Issue 1 — `base/source/baseiids.cpp` missing from VST3_HOSTING_SOURCES

**File:** `kodi-audiodsp-vsthost/CMakeLists.txt`

`fstring.cpp` (line 76) is compiled and it calls into Steinberg base infrastructure that requires `IString::iid` to be defined. That definition lives in `vst3_base/source/baseiids.cpp`, which is **not listed** anywhere in `VST3_HOSTING_SOURCES`. The linker fails with `unresolved external symbol Steinberg::IString::iid`.

**Fix:** Add `"${VST3SDK_DIR}/base/source/baseiids.cpp"` to `VST3_HOSTING_SOURCES`.

---

## 🔴 Category 2 — Runtime Crash / Undefined Behavior

### Issue 2 — Dangling pointer in `VSTPlugin3::process()` when bypass is active

**File:** `kodi-audiodsp-vsthost/src/vst3/VSTPlugin3.cpp`

When `m_bypassed` is true, the code creates a stack-local `blockParamChanges`, potentially stores a pointer to it in `m_processData.inputParameterChanges`, then returns without clearing that pointer. The next `process()` call reads a dangling stack pointer.

```cpp
// BEFORE (broken):
Steinberg::Vst::ParameterChanges blockParamChanges;
drainParamQueue(blockParamChanges);
if (blockParamChanges.getParameterCount() > 0)
    m_processData.inputParameterChanges = &blockParamChanges;  // stored

if (m_bypassed) { ... return samples; }   // returns WITHOUT clearing

// line 363 — NEVER REACHED on bypass path:
m_processData.inputParameterChanges = nullptr;
```

**Fix:** Move `m_processData.inputParameterChanges = nullptr` to the top of `process()`, before the param drain.

---

### Issue 3 — Buffer overflow in `DSPChain::process()` if `samples > m_blockSize`

**File:** `kodi-audiodsp-vsthost/src/dsp/DSPChain.cpp`

Ping-pong scratch buffers are allocated for exactly `m_blockSize` samples. `process()` uses the caller-supplied `samples` count with no guard:

```cpp
std::memcpy(m_ptrA[ch], in[ch], static_cast<size_t>(samples) * sizeof(float));
```

If Kodi calls with `samples > m_blockSize` this is heap corruption.

**Fix:** Add `const int n = std::min(samples, m_blockSize);` and use `n` for all copies inside `process()`.

---

### Issue 4 — `m_effect->processReplacing` null dereference for old VST2 plugins

**File:** `kodi-audiodsp-vsthost/src/vst2/VSTPlugin2.cpp`

VST2 plugins older than VST2.1 may have a null `processReplacing` pointer. There is no null check before the call:

```cpp
m_effect->processReplacing(m_effect, m_inputPtrs.data(), m_outputPtrs.data(), samples);
```

**Fix:** Check `m_effect->processReplacing != nullptr` before calling; if null, either fall back to the legacy `m_effect->process` accumulating callback or pass-through.

---

### Issue 5 — `EditorBridge` holds a dangling `m_chain` pointer after `StreamDestroy`

**File:** `kodi-audiodsp-vsthost/src/addon_main.cpp`

The editor bridge is started with a pointer to the chain of the first-created stream processor. When that stream is destroyed (`StreamDestroy`), the processor is `delete`d, but `g_editorBridge.m_chain` still points into the freed object.

**Fix:** In `StreamDestroy`, call `g_editorBridge.stop()` when the being-destroyed processor is the bridge's current chain owner, then restart the bridge with the next active processor (or leave it stopped if none).

---

### Issue 6 — Data race on `EditorBridge::m_chain` between UI thread and `stop()`

**File:** `kodi-audiodsp-vsthost/src/bridge/EditorBridge.cpp`

`stop()` sets `m_chain = nullptr` on the calling thread while the UI thread may simultaneously be inside `handleMessage()`, `doOpenEditor()`, or `doCloseEditor()` — all of which read `m_chain` without any lock.

**Fix:** Protect `m_chain` with `m_editorMutex` (or a dedicated `m_chainMutex`), ensuring both the setter in `stop()` and all readers hold the lock.

---

## 🟠 Category 3 — Functional / Semantic Bugs

### Issue 7 — `MasterProcessGetDelay` returns sample count, not seconds

**File:** `kodi-audiodsp-vsthost/src/addon_main.cpp`

```cpp
return static_cast<float>(GetProc(handle)->masterProcessGetDelay());
```

`masterProcessGetDelay()` returns total latency in *samples*. The Kodi ADSP API documents this return value as **seconds**. At 44100 Hz, a 512-sample delay would be reported as 512.0 seconds rather than ~0.012 seconds.

**Fix:**
```cpp
auto* proc = GetProc(handle);
return static_cast<float>(proc->masterProcessGetDelay())
       / static_cast<float>(proc->getSampleRate());
```

---

### Issue 8 — VST3 editor always opens at 640×480; `getEditorSize` never works

**Files:** `kodi-audiodsp-vsthost/src/bridge/EditorBridge.cpp`, `kodi-audiodsp-vsthost/src/vst3/VSTPlugin3.cpp`

`doOpenEditor()` calls `plugin->getEditorSize()` **before** `plugin->openEditor()`. For VST3, `getEditorSize()` requires `m_plugView != nullptr` (only set by `openEditor()`), so it always returns `false`. The window always opens at the hardcoded 640×480 fallback.

**Fix:** Call `openEditor(hwnd)` first, then call `getEditorSize()` and resize the host window to match the view's reported size.

---

### Issue 9 — `IComponent` ↔ `IEditController` not connected via `IConnectionPoint`

**File:** `kodi-audiodsp-vsthost/src/vst3/VSTPlugin3.cpp`

The VST3 spec requires that when the component and controller are separate objects (non-single-component plugins), the host must connect them via `IConnectionPoint::connect()`. This handshake is never performed, so parameter notifications and preset changes between the DSP side and GUI side are broken for many VST3 plugins.

**Fix:** After obtaining both `m_component` and `m_controller`, query each for `IConnectionPoint` and call `connect()` on both (passing the other as the peer), as described in the VST3 Hosting Guide.

---

### Issue 10 — Real-time heap allocation in `VSTPlugin3::process()` on parameter changes

**File:** `kodi-audiodsp-vsthost/src/vst3/VSTPlugin3.cpp`

`process()` creates a stack-local `Steinberg::Vst::ParameterChanges` and calls `addParameterData()` on it. Internally this uses `std::vector`, so `addParameterData()` may call `malloc` when new parameter queues are needed. This violates the no-allocation constraint on the audio thread and risks audio dropouts.

**Fix:** Pre-allocate a `ParameterChanges`-like object as a member of `VSTPlugin3` (or use a custom static-capacity parameter change container) that is reset each block without heap allocation.

---

## 🟡 Category 4 — Build / Workflow Issues

### Issue 11 — `KODI_ADSP_SDK_DIR` path incorrect in workflow

**File:** `.github/workflows/build-vst-addons.yaml`

```yaml
-DKODI_ADSP_SDK_DIR="%GITHUB_WORKSPACE%\xbmc\addons\kodi-addon-dev-kit\include\kodi"
```

`GITHUB_WORKSPACE` is the repository root, so this resolves to `…\xbmc\xbmc\addons\…` which does not exist.

**Fix:** Change to:
```yaml
-DKODI_ADSP_SDK_DIR="%GITHUB_WORKSPACE%\addons\kodi-addon-dev-kit\include\kodi"
```

---

### Issue 12 — `.gitmodules` branch conflicts with CMakeLists.txt tag

**File:** `.gitmodules`

`.gitmodules` pins the VST3 SDK submodule to `branch = main`, but `CMakeLists.txt` specifies `GIT_TAG v3.7.9_build_61` for FetchContent. Developers using `git submodule update` get a potentially incompatible newer SDK version.

**Fix:** Set `.gitmodules` to the exact tagged commit that matches `v3.7.9_build_61`, or pin to `branch = v3.7.9_build_61`.

---

## 🟡 Category 5 — Code Quality / Minor Issues

### Issue 13 — JSON utility functions duplicated across four files

**Files:** `DSPChain.cpp`, `EditorBridge.cpp`, `VSTPluginManager.cpp`, `PluginSettings.cpp`

`jsonEscape`, `extractStringField`, and `extractBoolField` are independently re-implemented in each file, with subtle behavioural differences (e.g. whitespace handling, `\u` escape support). This risks parse inconsistencies when one component reads JSON written by another.

**Fix:** Extract a shared `src/util/JsonUtil.h` / `JsonUtil.cpp` with a single canonical implementation and update all four files to include it.

---

### Issue 14 — `VSTPlugin3::hasEditor()` creates and releases a view on every call

**File:** `kodi-audiodsp-vsthost/src/vst3/VSTPlugin3.cpp`

Every call to `hasEditor()` (when no view is cached) calls `m_controller->createView("editor")` and immediately releases it. This is called repeatedly by `EditorBridge` when scanning the chain and can trigger expensive side-effects in some plugins.

**Fix:** Add a `bool m_hasEditor = false` member, populated once during `load()`, and return it from `hasEditor()`.

---

### Issue 15 — `ChainPlugin::bypassed` and `IVSTPlugin::m_bypassed` can desync

**Files:** `kodi-audiodsp-vsthost/src/dsp/DSPChain.h`, `kodi-audiodsp-vsthost/src/dsp/DSPChain.cpp`

`ChainPlugin::bypassed` is a separate field from `IVSTPlugin::m_bypassed` (set via `setBypassed()`). Both are written independently; if they diverge, latency reporting (`getTotalLatencySamples` reads `slot.bypassed`) and actual processing bypass (`IVSTPlugin::process` reads `m_bypassed`) become inconsistent.

**Fix:** Remove `ChainPlugin::bypassed` and derive bypass state exclusively from `slot.plugin->isBypassed()` everywhere.

---

### Issue 16 — Unnecessary `<windows.h>` include in `addon_main.cpp`

**File:** `kodi-audiodsp-vsthost/src/addon_main.cpp`

`addon_main.cpp` includes `<windows.h>` at the top level but none of the functions defined in the file use any Windows API types directly. This is an unnecessary dependency that increases compile time and macro pollution.

**Fix:** Remove the `#include <windows.h>` line from `addon_main.cpp`.

---

### Issue 17 — `DSPChain` ignores `process()` return value

**Files:** `kodi-audiodsp-vsthost/src/dsp/DSPChain.cpp`, `kodi-audiodsp-vsthost/src/vst3/VSTPlugin3.cpp`

`VSTPlugin3::process()` returns `0` when the plugin is not loaded. `DSPChain::process()` calls `slot.plugin->process()` and unconditionally swaps ping-pong buffers, ignoring the return. If a plugin is unloaded mid-session, that slot silently emits an un-swapped buffer to the next stage, corrupting the audio without any diagnostic.

**Fix:** Check the return value from `slot.plugin->process()`. If `0` is returned (unloaded), copy the current input to the current output (passthrough) instead of swapping to a potentially stale buffer.

---

## Known Limitations (not fixable in isolation)

1. **VST3 `numParams` always 0 in scanner** — requires full audio processor instantiation, which risks crashes. By design.
2. **32/64-bit bitness mismatch** — a 32-bit scanner cannot probe 64-bit plugins and vice versa.
3. **VST2 shell plugins** — only the top-level plugin name is reported; sub-plugin IDs are not iterated.
