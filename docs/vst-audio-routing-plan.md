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
  → MixSounds + Deamplify                        ← existing
  → CActiveAEDSP::MasterProcess(out)             ← NEW: in-place VST chain
      └─ if m_dspFailed or not planar → passthrough
      └─ __try { DSP_MasterProcess } __except → set m_dspFailed, passthrough
  → m_sinkBuffers (sink resampler)
  → CActiveAESink → WASAPI / DirectSound
```

---

## Implementation plan

---

### 1 — Virtual provider addon
**File:** `addons/xbmc.audiodsp/addon.xml`

Create the built-in virtual addon that advertises `xbmc.audiodsp 0.1.8`.
Without this the addon manager will never satisfy the `<import>` dependency
declared in `kodi-audiodsp-vsthost/resources/addon.xml`.  
Pattern to follow: `addons/xbmc.core/addon.xml`.

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
**New files:**  
`xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAEDSP.h`  
`xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAEDSP.cpp`

A single lightweight class `CActiveAEDSP`:

| Method | What it does |
|--------|--------------|
| `Init()` | Queries `CAddonMgr` for the first enabled `ADDON_AUDIODSP` addon, loads its DLL via `DllAddon`, calls `get_addon()` to fill the `AudioDSP` function-pointer table, then calls `ADDON_Create()` passing `AE_DSP_PROPERTIES{strUserPath}` |
| `StreamCreate(AEAudioFormat)` | Allocates an `ADDON_HANDLE_STRUCT`, fills `AE_DSP_SETTINGS`, calls `StreamCreate` → `StreamIsModeSupported(MASTER_PROCESS)` → `StreamInitialize`. Clears `m_dspFailed`. |
| `MasterProcess(CSampleBuffer*)` | **Planar-only guard:** if `out->pkt->planes <= 1` (interleaved format), passthrough. If `m_dspFailed`, passthrough. Otherwise cast `pkt->data` to `float**` and call `DSP_MasterProcess` inside a Windows SEH `__try/__except` block. On exception: set `m_dspFailed = true`, log error, passthrough, start recovery timer. |
| `StreamDestroy()` | Calls `StreamDestroy`, frees the handle. |
| `Deinit()` | Calls `ADDON_Destroy`, unloads DLL. |
| `OnAEResume()` | Called when AE returns from `SUSPEND` (sleep/wake). Calls `StreamDestroy()` then `StreamCreate()` to refresh sample-rate/block-size state. If `m_dspFailed`, calls `TryRecover()` immediately. |
| `TryRecover()` | Calls `Deinit()` then `Init()`. If successful, calls `StreamCreate()` with the current format and clears `m_dspFailed`. If it fails, re-arms the recovery timer. |
| `IsActive() const` | Returns `m_initialized && !m_dspFailed`. |

**Latency fix:**  
`MasterProcessGetDelay` returns latency in *samples* but Kodi expects
*seconds*.  Convert:
```cpp
latencySeconds = (float)DSP_MasterProcessGetDelay(handle) / (float)m_streamSampleRate;
```

**Crash recovery timer:**  
`m_recoveryTimer` fires 30 s after `m_dspFailed` is set, calling
`TryRecover()` once.  If recovery fails the timer re-arms.  This limits
impact from a permanently-crashed plugin while automatically restoring
processing once the cause is resolved.

> **Why no `libKODI_adsp` host callbacks are needed:**  
> `addon_main.cpp` makes **zero** callbacks into Kodi.  `ADDON_Create` only
> reads `AE_DSP_PROPERTIES::strUserPath`.  All processing is self-contained.

---

### 5 — Hook in `ActiveAE.cpp`
**File:** `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAE.cpp`  
**Header:** `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAE.h`

- Add member `CActiveAEDSP m_dsp` to `CActiveAE`.
- Call `m_dsp.Init()` at the end of `CActiveAE::Start()`.
- Call `m_dsp.Deinit()` in `CActiveAE::Dispose()`.
- In `CActiveAE::CreateStream(...)`, after stream creation, call
  `m_dsp.StreamCreate(stream->m_format)`.
- In `CActiveAE::DiscardStream(...)`, before deleting the stream object,
  call `m_dsp.StreamDestroy()`.
- In `RunMixStage()`, after `Deamplify` produces `out` and **before**
  `m_sinkBuffers->m_inputSamples.push_back(out)`, insert:

```cpp
if (out && m_dsp.IsActive() && m_mode != MODE_RAW)
    m_dsp.MasterProcess(out);
```

- At the end of the `AE_TOP_CONFIGURED_SUSPEND` → `INIT` transition (after
  `Configure()` succeeds on wake), call `m_dsp.OnAEResume()`.

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
`StreamCreate` addon call is made — the DSP addon's stream handle becomes
stale.

**Fix:**  
After `Configure()` succeeds inside the `AE_TOP_CONFIGURED_SUSPEND`/`INIT`
branch in `ActiveAE.cpp`, call `m_dsp.OnAEResume()` (defined in §4).
`OnAEResume()` tears down the stale stream handle and re-creates it with the
current format, keeping DSP processing valid after wake.

**Named-pipe robustness during sleep:**  
The pipe server loop calls blocking `ConnectNamedPipe()`.  Win32 may cancel
this on sleep with `ERROR_BROKEN_PIPE` / `ERROR_OPERATION_ABORTED`.  The
existing loop already handles errors via `continue`, which re-creates the
pipe.  No change needed; the pipe is reconstructed within one loop iteration
after wake.

---

### 8 — VST crash recovery  *(new)*

#### Layer 1 — Per-plugin SEH guard in `DSPChain::process()`

**File:** `kodi-audiodsp-vsthost/src/dsp/DSPChain.cpp`

Add a standalone SEH wrapper (must be a free function — MSVC prohibits mixing
`__try/__except` with C++ destructors in the same scope):

```cpp
static int callPluginProcessSafe(IVSTPlugin* p, float** in, float** out, int samples)
{
    int result = 0;
    __try {
        result = p->process(in, out, samples);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        result = 0;
    }
    return result;
}
```

Replace the direct `slot.plugin->process(...)` call in the processing loop
with `callPluginProcessSafe(...)`.  On exception:
- `result == 0` is already handled by copying input to output (passthrough).
- Additionally call `slot.plugin->setBypassed(true)` to skip the crashed
  plugin in all subsequent blocks with zero SEH overhead.
- Log the crash with the plugin path.

#### Layer 2 — Addon-level SEH guard in `CActiveAEDSP::MasterProcess()`

```cpp
__try {
    m_dspFuncs.MasterProcess(m_handle, planes, planes,
                              (unsigned int)buf->pkt->nb_samples);
} __except (EXCEPTION_EXECUTE_HANDLER) {
    CLog::Log(LOGERROR, "CActiveAEDSP: vsthost crashed in MasterProcess, "
                         "bypassing DSP");
    m_dspFailed = true;
    m_recoveryTimer.Start(RECOVERY_DELAY_MS);
    // buf already holds the unmodified audio — passthrough continues
}
```

#### Layer 3 — `m_dspFailed` bypass and auto-recovery

- `CActiveAEDSP` holds `std::atomic<bool> m_dspFailed{false}`.
- While `m_dspFailed` is set, `MasterProcess()` is a zero-cost passthrough —
  Kodi audio continues uninterrupted.
- `m_recoveryTimer` fires 30 s later calling `TryRecover()`:
  - `Deinit()` unloads the DLL safely (no audio processing during unload).
  - `Init()` reloads the DLL and calls `ADDON_Create` again.
  - On success: `StreamCreate()` with the current format, clear `m_dspFailed`.
  - On failure: set `m_dspFailed = true`, re-arm timer.
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

```xml
<addon optional="true">xbmc.audiodsp</addon>
```

---

## Summary of all gaps addressed

| # | Issue | Fix |
|---|-------|-----|
| A | `AddonBuilder::Build()` missing AUDIODSP case → `nullptr` | §3 |
| B | CMakeLists path wrong (no `ActiveAE/CMakeLists.txt`) | §9 |
| C | Internal format may be `AE_FMT_FLOAT` (interleaved) | §4 MasterProcess planar guard |
| D | `MasterProcessGetDelay` returns samples, Kodi expects seconds | §4 latency fix |
| E | Pipe only alive during streaming (dies on idle-stop / sleep) | §6 |
| F | DSP stream handle stale after sleep/wake + format change | §7 |
| G | VST plugin crash terminates audio render thread | §8 layer 1 |
| H | Whole addon crash terminates audio render thread | §8 layer 2 |
| I | No automatic recovery after crash | §8 layer 3 |
| J | `EditorBridge::m_chain` dangling after `StreamDestroy` | §6 `setChain(nullptr)` |

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
| `CActiveAE::RunMixStage()` injection point | `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAE.cpp:2255` |
| `m_sinkBuffers->m_inputSamples.push_back(out)` | `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAE.cpp:2263` |
| `DSPChain::process()` | `kodi-audiodsp-vsthost/src/dsp/DSPChain.h` |
| `EditorBridge` pipe name | `kodi-audiodsp-vsthost/src/bridge/EditorBridge.h:92` |
| `CActiveAE::Suspend()` / `Resume()` | `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAE.cpp:2806-2830` |
| `CPowerManager::OnSleep()` / `OnWake()` | `xbmc/powermanagement/PowerManager.cpp:170-219` |
| `CAddonDll::Create(ADDON_TYPE, void*, void*)` | `xbmc/addons/binary-addons/AddonDll.cpp:184` |
