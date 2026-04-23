# VST Audio Routing Plan

## Goal

Route all Kodi audio through the `audiodsp.vsthost` VST chain before it
reaches the output device (WASAPI / DirectSound on Windows).  The named-pipe
IPC server must remain live for the full lifetime of the addon — not just
during active playback.  A VST plugin crash or a vsthost crash must not drop
audio; Kodi must bypass the vsthost automatically and keep playing.

---

## Audio data flow after this change

```
Kodi stream decoder
  → ActiveAE per-stream resampler / processing buffers
  → ActiveAE mixer  (planar float, m_internalFormat AE_FMT_FLOATP)
  → MixSounds + Deamplify                        ← existing (GUI sounds, volume)
  → CActiveAEDSP::MasterProcess(out)             ← NEW: in-place VST chain
      └─ if m_dspFailed → passthrough (audio unmodified)
      └─ planar (planes > 1): cast pkt->data to float** and call DSP in-place
      └─ interleaved (planes == 1): deinterleave → DSP → reinterleave
      └─ __try/__except wraps DSP call → set m_dspFailed on crash, passthrough
  → m_sinkBuffers (sink resampler)
  → CActiveAESink → WASAPI / DirectSound
```

---

## Implementation plan

---

### 1 — Virtual provider addon
**File:** `addons/xbmc.audiodsp/addon.xml`

The file already exists and **is correctly configured** — `version="0.1.8"` and
`<backwards-compatibility abi="0.1.8"/>` are already set, matching the
`<import addon="xbmc.audiodsp" version="0.1.8"/>` declaration in
`kodi-audiodsp-vsthost/resources/addon.xml` and the `KODI_AE_DSP_API_VERSION`
macro in `kodi_adsp_types.h`.  No changes are needed here.

---

### 2 — Addon type registration
**Files:** `xbmc/addons/AddonInfo.h`, `xbmc/addons/AddonInfo.cpp`

- Add `ADDON_AUDIODSP` to the `TYPE` enum in `AddonInfo.h`.
- Add `{"xbmc.audiodsp", ADDON_AUDIODSP, ...}` to the extension-point table
  in `AddonInfo.cpp` so the addon manager can discover and enable ADSP addons.

---

### 3 — AddonBuilder wiring  *(gap from previous plan)*
**File:** `xbmc/addons/AddonBuilder.cpp`

Add a case for `ADDON_AUDIODSP` in **both** `Build()` and `FromProps()` switch
statements, returning `std::make_shared<CAddonDll>(...)`.  Without this,
`Build()` falls through to `default` and returns `nullptr`, making the addon
invisible to the manager.

---

### 4 — Minimal ADSP DLL loader
**Files (already created):**  
`xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAEDSP.h`  
`xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAEDSP.cpp`

A single lightweight class `CActiveAEDSP`.

#### Public interface

| Method | Thread | What it does |
|--------|--------|--------------|
| `Init()` | AE setup (`Start`) | Queries `CAddonMgr` for the first enabled `ADDON_AUDIODSP` addon, loads its DLL via `DllAddon`, calls `get_addon()` to fill the `AudioDSP` function-pointer table, then calls `ADDON_Create()` passing `AE_DSP_PROPERTIES{strUserPath, strAddonPath}`. |
| `Deinit()` | AE setup (`Dispose`) | Calls private `StreamDestroy()`, then `ADDON_Destroy`, unloads DLL. |
| `OnConfigure(AEAudioFormat)` | AE worker (`Configure`) | Called at the end of `CActiveAE::Configure()` whenever the output format changes. If the format has changed (or no stream is active), calls private `StreamDestroy()` then `StreamCreate()`. This is also the correct hook for post-sleep refresh (see §7). |
| `MasterProcess(CSampleBuffer*)` | AE render (`RunStages`) | If `m_dspFailed` or no stream is active: silent passthrough. Otherwise casts `pkt->data` to `float**` and calls the addon's `MasterProcess` inside a Windows SEH `__try/__except` guard. On exception: sets `m_dspFailed = true` and returns — the buffer is unmodified so audio continues. Handles both planar (`planes > 1`) and interleaved (`planes == 1`) formats. |
| `IsActive() const` | any | Returns `m_initialized && !m_dspFailed`. |

#### Private helpers

`StreamCreate(AEAudioFormat)` and `StreamDestroy()` are **private** helpers
called only by `OnConfigure()` and `Deinit()`.  They are not exposed on the
public interface and are not called directly from `ActiveAE.cpp`.

`StreamCreate` allocates `ADDON_HANDLE_STRUCT`, fills `AE_DSP_SETTINGS`, calls
`StreamCreate` → `StreamIsModeSupported(MASTER_PROCESS)` →
`MasterProcessSetMode` → `StreamInitialize`.  It also allocates scratch buffers
for the interleaved de/re-interleave path.

**Latency note (already fixed in addon):**  
`MasterProcessGetDelay` returns latency in *samples*.  `addon_main.cpp`
divides by the current sample rate before returning, so Kodi receives seconds.
No conversion is needed in `CActiveAEDSP`.

**Crash recovery timer (future work):**  
`m_dspFailed` provides immediate bypass (zero-cost passthrough).  A future
`TryRecover()` method would call `Deinit()` then `Init()` on a background
timer (≈ 30 s after failure) and clear `m_dspFailed` on success.  This is
**not yet implemented**; audio simply continues without DSP until Kodi is
restarted or the addon is re-enabled from the addon manager.

> **Why no `libKODI_adsp` host callbacks are needed:**  
> `addon_main.cpp` makes **zero** callbacks into Kodi.  `ADDON_Create` only
> reads `AE_DSP_PROPERTIES::strUserPath`.  All processing is self-contained.

---

### 5 — Hook in `ActiveAE.cpp`
**File:** `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAE.cpp`  
**Header:** `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAE.h`

- Add member `CActiveAEDSP m_dsp` to `CActiveAE`.
- Call `m_dsp.Init()` near the top of `CActiveAE::Start()`, **after**
  `Create()` (which starts the AE worker thread) and **before** the
  `SendOutMessageSync(INIT)` handshake.  This ensures the addon DLL is loaded
  and its named pipe is live before AE begins processing.
- Call `m_dsp.Deinit()` in `CActiveAE::Dispose()`, after `m_sink.Dispose()`.
- At the **end of `CActiveAE::Configure()`** (after sink buffers are set up and
  all format decisions are final), call:

```cpp
m_dsp.OnConfigure(m_internalFormat);
```

  This single hook covers both the initial stream-start case and any
  subsequent format reconfigure (sample-rate change, channel-count change,
  sleep/wake resume) — see §7.  No per-stream `StreamCreate` /
  `StreamDestroy` calls from `CreateStream()` or `DiscardStream()` are
  needed: the DSP stream is bound to the **mixer output format**, not to
  individual `CActiveAEStream` objects.

- In `RunStages()`, after `MixSounds` and `Deamplify` and **before**
  `m_sinkBuffers->m_inputSamples.push_back(out)`, insert:

```cpp
m_dsp.MasterProcess(out);
```

---

### 6 — Named pipe persistence  *(new)*

**Root cause:** The current code starts `g_editorBridge` in `StreamCreate`
and stops it in `StreamDestroy`.  Because `CPowerManager::OnSleep()` calls
`g_application.StopPlaying()` (which fires `StreamDestroy`) before
`CActiveAE::Suspend()`, the pipe dies on every sleep/idle-stop even though
the addon DLL remains loaded.

**Fix — `EditorBridge` (`kodi-audiodsp-vsthost/src/bridge/`):**

Add a new method:
```cpp
void setChain(DSPChain* chain);
```
Implementation: acquire `m_editorMutex`, assign `m_chain = chain`.  Safe to
call at any time while the pipe server is running.

**Fix — `addon_main.cpp`:**

| Lifecycle point | Old behaviour | New behaviour |
|-----------------|---------------|---------------|
| `ADDON_Create()` | nothing | `g_editorBridge.start(nullptr)` — pipe live immediately, no chain yet |
| `StreamCreate()` | `g_editorBridge.start(&proc->getChain())` | `g_editorBridge.setChain(&proc->getChain())` |
| `StreamDestroy()` | `g_editorBridge.stop()` | `g_editorBridge.setChain(nullptr)` — pipe keeps running |
| `ADDON_Destroy()` | `g_editorBridge.stop()` | unchanged — only stops on full addon shutdown |

**Fix — `EditorBridge::processCommand()`:**

`open`/`close` commands that arrive while `m_chain == nullptr` return:
```json
{"status":"error","error":"No active audio chain — Kodi is not playing"}
```

---

### 7 — Sleep / wake survival  *(new)*

**What already works:**  
`CPowerManager::OnSleep()` calls `CActiveAE::Suspend()` → state
`AE_TOP_CONFIGURED_SUSPEND`.  The DSP DLL is not unloaded.  The named-pipe
thread survives because it is now started in `ADDON_Create` (§6).

On `CPowerManager::OnWake()`, `CActiveAE::Resume()` sends `INIT` →
`Configure()`.  Streams already in `m_streams` are not re-created, so no new
`StreamCreate` addon call is made.  Without corrective action the DSP addon's
stream handle would have stale sample-rate / block-size state.

**Fix (already implemented via §5):**  
`Configure()` calls `m_dsp.OnConfigure(m_internalFormat)` at its end
**regardless of how the reconfigure was triggered** — initial start, format
change, or sleep/wake resume.  `OnConfigure` detects the format change (or
the absence of an active stream) and calls private `StreamDestroy()` +
`StreamCreate()` to refresh the addon's stream handle with the current format.

No separate `OnAEResume()` method is required; the existing `OnConfigure`
hook is sufficient because `Configure()` is always called before the AE
worker re-enters its processing states after wake.

**Named-pipe robustness during sleep:**  
The pipe server loop calls blocking `ConnectNamedPipe()`.  Win32 may cancel
this on sleep with `ERROR_BROKEN_PIPE` / `ERROR_OPERATION_ABORTED`.  The
existing loop already handles errors via `continue`, which re-creates the
pipe.  No change needed; the pipe is reconstructed within one loop iteration
after wake.

---

### 8 — VST crash recovery  *(new)*

#### Layer 1 — Per-plugin SEH guard in `DSPChain::process()` *(scoped for current work — VST2 only)*

**File:** `kodi-audiodsp-vsthost/src/dsp/DSPChain.cpp`  
**Scope:** VST2 only.  VST3 sources are excluded from the current build
(`CMakeLists.txt` explicitly filters out `src/vst3/*.cpp`).  When VST3 support
is added in a future phase this same guard will apply automatically, because the
crash-isolation path dispatches through the `IVSTPlugin::process()` virtual.

> **Status: not yet implemented.**  The current `DSPChain::process()` at lines
> 158–197 calls `slot.plugin->process(...)` directly with no SEH protection.
> A crash inside a misbehaving VST2 plugin's `processReplacing` function
> propagates as an unhandled SEH exception through the entire addon, where it is
> eventually caught by the addon-level guard in `CActiveAEDSP::MasterProcess()`
> (Layer 2), which sets `m_dspFailed = true` and disables **all** DSP for the
> session.  Layer 1 adds a logging step before allowing that propagation to
> reach Layer 2.

##### Logging strategy: why `fprintf` and not `CLog`, and when logging is safe

On the **normal audio render path** (every 5–20 ms block) all logging is
prohibited.  `CLog::Log` acquires a mutex, may allocate heap memory, and may
issue kernel I/O — any of which can stall the render thread long enough to
cause an audible underrun.  This restriction applies to the hot path only.

Inside an `__except` handler (or SEH filter expression), the render thread has
already been interrupted by a structured exception — the timing constraint no
longer applies, and logging is not just permissible but essential for
diagnosis.  Log freely in any `__except` body or SEH filter.

Additionally, Layer 1 lives inside the **addon DLL** (`DSPChain.cpp`), which
does not link against Kodi.  `CLog` is a Kodi-side symbol and is not
accessible from the addon.  `std::fprintf(stderr, ...)` is the correct choice
here: it is available in any C runtime context, is safe to call during stack
unwinding, and its output appears in Kodi's debug log on Windows (stderr is
captured by the parent process).

##### Problem

When plugin slot `N` raises a structured exception inside `processReplacing()`:

1. The exception escapes `slot.plugin->process()` in `DSPChain::process()`.
2. MSVC's stack unwinder propagates it past `DSPChain::process()`.
3. It escapes `DSPProcessor::masterProcess()` and `MasterProcess()` in the addon.
4. The addon-level `__try/__except` in `CActiveAEDSP::MasterProcess()` catches
   it, sets `m_dspFailed = true`, and disables all DSP.

Layer 1 **does not prevent** this outcome.  Its role is to **log the offending
plugin** before allowing the exception to propagate to Layer 2, so the crash
report identifies *which* plugin caused the failure.

##### Design decision — escalate, not isolate

Per-plugin isolation (bypassing only the crashed slot while letting the rest of
the chain continue) is a tempting approach but is **not adopted here**.  A VST2
`processReplacing` crash frequently implies heap or stack corruption in the
plugin's address space.  Continuing to call other plugins in the same process
after such corruption risks silent data-corruption in subsequent plugins'
output.  The safer and more predictable outcome is to disable all DSP
immediately — the same outcome Layer 2 already provides — while Layer 1
contributes the critical piece Layer 2 cannot: **the identity of the offending
plugin**.

##### Implementation

**Constraint — MSVC SEH scope rule:**
MSVC (C4509) prohibits `__try/__except` in any function scope that also contains
C++ objects with non-trivial destructors.  `DSPChain::process()` holds
`std::vector<float*>*` locals on the stack, making it ineligible.  The pattern
is identical to `VSTPlugin2::callPluginMainSafe()` (`VSTPlugin2.cpp:44–53`),
which wraps the plugin entry-point call for the same reason.

**Step 1 — Add two standalone free functions immediately above `DSPChain::process()`:**

```cpp
// SEH filter: runs during stack walk, before any unwind destructors.
// Logs the offending plugin path and SEH exception code, then signals that
// the exception should propagate to the next enclosing __try (Layer 2 in
// CActiveAEDSP::MasterProcess).
// Must be a free function — MSVC C4509 prohibits __try/__except in any scope
// that contains C++ objects with non-trivial destructors.
static int logSlotCrashAndEscalate(EXCEPTION_POINTERS* ep, const char* pluginPath)
{
    std::fprintf(stderr,
        "[DSPChain] plugin crashed (SEH 0x%08X) — escalating to disable all DSP: %s\n",
        static_cast<unsigned>(ep->ExceptionRecord->ExceptionCode),
        pluginPath ? pluginPath : "<unknown>");
    return EXCEPTION_CONTINUE_SEARCH;   // propagate → caught by Layer 2
}

// Wrapper that applies the per-slot SEH filter and then re-raises.
// The __except body is never entered; the filter always returns CONTINUE_SEARCH.
// pluginPath must be captured from host-heap before the __try (see below).
static int callSlotProcessSafe(IVSTPlugin* p, float** in, float** out,
                                int samples, const char* pluginPath)
{
    int result = 0;
    __try {
        result = p->process(in, out, samples);
    } __except (logSlotCrashAndEscalate(GetExceptionInformation(), pluginPath)) {
        // Never reached — filter always returns EXCEPTION_CONTINUE_SEARCH.
    }
    return result;
}
```

**Step 2 — Replace the direct `process()` call in the processing loop
(line 182), pre-capturing the plugin path before the `__try`:**

```cpp
// BEFORE:
const int processed = slot.plugin->process(current->data(), next->data(), safeSamples);

// AFTER:
// Capture path as plain C string before the __try.  slot.plugin's wrapper
// object lives on the host heap (unaffected by a plugin-side crash), but
// capturing the pointer before the __try eliminates any method-call risk
// after a potential corruption event.
const char* pluginPath = slot.plugin->getPath().c_str();
const int processed = callSlotProcessSafe(
    slot.plugin.get(), current->data(), next->data(), safeSamples, pluginPath);
```

No other change to `DSPChain::process()` is needed.  The existing
`if (processed == 0)` block already copies `*current` to `*next` (passthrough
for the current block).  The filter returns `EXCEPTION_CONTINUE_SEARCH`, so
the exception continues unwinding through `DSPChain::process()`, past
`DSPProcessor::masterProcess()`, and is caught by the Layer 2
`__try/__except` in `CActiveAEDSP::MasterProcess()`, which sets
`m_dspFailed = true` and disables all DSP.

**Why `getPath().c_str()` is safe to capture before the `__try`:**  
`IVSTPlugin::getPath()` returns `const std::string&` stored in the
`VSTPlugin2` wrapper object on the **host heap** — not inside the crashed
plugin DLL.  For the typical crash scenario (access violation or stack
overflow inside `processReplacing`), the host heap is intact.  Capturing
`c_str()` before the `__try` removes any residual risk of a method call after
a hypothetical corruption event.

##### Interaction with Layers 2 and 3

| Layer | Triggers when | Outcome |
|-------|---------------|---------|
| 1 — `callSlotProcessSafe` filter | Single VST2 plugin raises SEH exception | Plugin path + SEH code logged to `stderr`; `EXCEPTION_CONTINUE_SEARCH` returned — exception escalates to Layer 2 |
| 2 — `CActiveAEDSP::MasterProcess()` SEH | Exception propagated from Layer 1 (or any crash past Layer 1) | `m_dspFailed = true`; all DSP disabled; `CLog::Log(LOGERROR, ...)` emitted on Kodi side; audio passes through unmodified |
| 3 — `m_dspFailed` bypass + recovery timer | `m_dspFailed` set by Layer 2 | Immediate zero-cost Kodi-level passthrough; auto-recovery timer (future work) |

The combined log output for a single plugin crash is therefore:
1. **Addon-side** (`stderr`): `[DSPChain] plugin crashed (SEH 0xC0000005) — escalating to disable all DSP: C:\VST\MyPlugin.dll`
2. **Kodi-side** (`CLog LOGERROR`): `CActiveAEDSP::MasterProcess — add-on crashed; disabling DSP`

##### Acceptance criteria

- A VST2 plugin that raises an SEH exception inside `processReplacing()`
  causes all DSP to be disabled (same outcome as a crash that was never
  wrapped by Layer 1).
- The crash emits a `[DSPChain] plugin crashed (SEH 0x...)` line to `stderr`
  identifying the offending plugin's path and the SEH exception code, exactly
  once per crash event.
- `m_dspFailed` is set to `true` by Layer 2 as a result of the escalation.
- No per-plugin bypass state is written; `m_dspFailed` is the sole disable
  mechanism.
- Audio continues unmodified (Kodi-level passthrough) immediately after the
  crash block.

#### Layer 2 — Addon-level SEH guard in `CActiveAEDSP::MasterProcess()` *(implemented)*

```cpp
__try {
    m_funcs->MasterProcess(m_handle, planes, planes,
                            (unsigned int)buf->pkt->nb_samples);
} __except (EXCEPTION_EXECUTE_HANDLER) {
    CLog::Log(LOGERROR, "CActiveAEDSP::MasterProcess — add-on crashed; disabling DSP");
    m_dspFailed = true;
    // buf already holds the unmodified audio — passthrough continues
}
```

#### Layer 3 — `m_dspFailed` bypass and auto-recovery

- `CActiveAEDSP` holds `std::atomic<bool> m_dspFailed{false}` *(implemented)*.
- While `m_dspFailed` is set, `MasterProcess()` is a zero-cost passthrough —
  Kodi audio continues uninterrupted *(implemented)*.
- **Auto-recovery timer (future work):** a `TryRecover()` method would fire
  ≈ 30 s after `m_dspFailed` is set:
  - `Deinit()` unloads the DLL safely (no audio processing during unload).
  - `Init()` reloads the DLL and calls `ADDON_Create` again.
  - On success: `OnConfigure()` with the current format, clear `m_dspFailed`.
  - On failure: re-arm timer.
  
  Until this is implemented, DSP remains disabled until Kodi is restarted or
  the addon is re-enabled manually from the addon manager.
- Optionally post a Kodi notification toast: "VST addon recovered" /
  "VST addon failed — audio bypassed".

---

### 9 — CMakeLists update  *(corrects wrong path from previous plan)*

**File:** `xbmc/cores/AudioEngine/CMakeLists.txt`  
*(There is no separate `ActiveAE/CMakeLists.txt` in this tree — all AE sources
are registered in the parent `CMakeLists.txt`.)*

- Add `Engines/ActiveAE/ActiveAEDSP.cpp` to the `SOURCES` list.
- Add `${CMAKE_SOURCE_DIR}/kodi-audiodsp-vsthost/include/kodi-legacy-adsp`
  to the include directories.

---

### 10 — Addon manifest
**File:** `system/addon-manifest.xml`

The `xbmc.audiodsp` virtual-provider entry is already present and registered as
**required** (no `optional` attribute):

```xml
<addon>xbmc.audiodsp</addon>
```

This is intentional: `xbmc.audiodsp` ships no DLL — it is a virtual
extension-point addon.  It must always be present so that `audiodsp.vsthost`
can declare its `<import>` dependency and have it resolved by the addon manager.
Making it `optional="true"` would allow the manager to consider
`audiodsp.vsthost` enabled even when the virtual provider is absent, causing a
failed `MeetsVersion` check at runtime and silently preventing audio routing.

---

## Summary of all gaps addressed

| # | Issue | Fix | Status |
|---|-------|-----|--------|
| A | `AddonBuilder::Build()` missing AUDIODSP case → `nullptr` | §3 | ✅ done |
| B | CMakeLists path wrong (no `ActiveAE/CMakeLists.txt`) | §9 | ✅ done |
| C | Internal format may be `AE_FMT_FLOAT` (interleaved) | §4 MasterProcess planar guard | ✅ done |
| D | `MasterProcessGetDelay` returns samples, Kodi expects seconds | Fixed in addon `addon_main.cpp` | ✅ done |
| E | Pipe only alive during streaming (dies on idle-stop / sleep) | §6 | ✅ done |
| F | DSP stream handle stale after sleep/wake + format change | §7 (`OnConfigure` in `Configure()`) | ✅ done |
| G | VST plugin crash propagates through entire render thread | §8 layer 2 (addon-level SEH) | ✅ done |
| H | Whole addon crash terminates audio render thread | §8 layer 2 SEH → `m_dspFailed` passthrough | ✅ done |
| I | No automatic recovery after crash | §8 layer 3 recovery timer | ⏳ future work |
| J | `EditorBridge::m_chain` dangling after `StreamDestroy` | §6 `setChain(nullptr)` | ✅ done |
| K | `addons/xbmc.audiodsp/addon.xml` version 0.1.0 ≠ required 0.1.8 | §1 — bumped to 0.1.8 | ✅ done |
| L | Per-plugin crash logging before all-DSP disable in `DSPChain::process()` (VST2) | §8 layer 1 `callSlotProcessSafe` filter + `logSlotCrashAndEscalate` | 🔧 current work |

---

## What this does NOT require

- Restoring `ActiveAEDSPModeFactory`, `ActiveAEDSPProcessing`,
  `ActiveAEDSPDatabase`, and the other ~15 Krypton ADSP files.
- Implementing `libKODI_adsp` host callbacks (the addon makes none).
- Changing the `ADDON_TYPE` enum in `versions.h`.
- Modifying `audiodsp.vsthost` audio processing logic (except `EditorBridge`
  lifecycle and SEH crash guards).

---

## Internal format compatibility

ActiveAE's internal mixing format is planar 32-bit float (`AE_FMT_FLOATP`).
`CSampleBuffer::pkt->data` is a `uint8_t**` array of float planes.
`IVSTPlugin::process()` / `DSPChain::process()` both accept
`float** in, float** out, int samples` — a direct cast with no conversion
needed, provided the planar guard in `MasterProcess()` passes.

---

## Key source references

| Symbol | File |
|--------|------|
| `struct AudioDSP` (addon function table) | `kodi-audiodsp-vsthost/include/kodi-legacy-adsp/kodi_adsp_types.h:477` |
| `get_addon()` export | `kodi-audiodsp-vsthost/include/kodi-legacy-adsp/kodi_adsp_dll.h:536` |
| `CActiveAEDSP` public interface | `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAEDSP.h:44-108` |
| `CActiveAEDSP::Init()` / `Deinit()` / `OnConfigure()` | `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAEDSP.cpp:63-186` |
| `CActiveAEDSP::MasterProcess()` | `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAEDSP.cpp:275-342` |
| `m_dsp.Init()` call site | `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAE.cpp:2630` |
| `m_dsp.Deinit()` call site | `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAE.cpp:299` |
| `m_dsp.OnConfigure()` call site | `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAE.cpp:1414` |
| `CActiveAE::RunStages()` DSP injection point | `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAE.cpp:2245` |
| `m_sinkBuffers->m_inputSamples.push_back(out)` | `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAE.cpp:2272` |
| `DSPChain::process()` — processing loop to wrap | `kodi-audiodsp-vsthost/src/dsp/DSPChain.cpp:158-197` |
| `callSlotProcessSafe` + `logSlotCrashAndEscalate` (to be added before line 158) | `kodi-audiodsp-vsthost/src/dsp/DSPChain.cpp` |
| `VSTPlugin2::callPluginMainSafe()` — SEH pattern reference | `kodi-audiodsp-vsthost/src/vst2/VSTPlugin2.cpp:44-53` |
| `EditorBridge` pipe name | `kodi-audiodsp-vsthost/src/bridge/EditorBridge.h:96` |
| `EditorBridge::setChain()` | `kodi-audiodsp-vsthost/src/bridge/EditorBridge.cpp:95-99` |
| `ADDON_Create` / `ADDON_Destroy` lifecycle | `kodi-audiodsp-vsthost/src/addon_main.cpp:39-60` |
| `CActiveAE::Suspend()` / `Resume()` | `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAE.cpp:2816-2844` |
| `AE_TOP_CONFIGURED_SUSPEND → INIT` branch | `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAE.cpp:826-860` |
| `CPowerManager::OnSleep()` / `OnWake()` | `xbmc/powermanagement/PowerManager.cpp:170-219` |
| `CAddonDll::Create(ADDON_TYPE, void*, void*)` | `xbmc/addons/binary-addons/AddonDll.cpp:184` |
| Virtual provider addon | `addons/xbmc.audiodsp/addon.xml` (version 0.1.8, abi 0.1.8) |
