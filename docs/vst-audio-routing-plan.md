# VST Audio Routing Plan

## Goal

Route all Kodi audio output through the `audiodsp.vsthost` VST chain before it
reaches the output device (WASAPI / DirectSound on Windows).

---

## Audio data flow after this change

```
Kodi stream decoder
  → ActiveAE per-stream resampler / processing buffers
  → ActiveAE mixer  (planar float, m_internalFormat)
  → MixSounds + Deamplify                    ← existing
  → CActiveAEDSP::MasterProcess(out)         ← NEW: in-place VST chain
  → m_sinkBuffers (sink resampler)
  → CActiveAESink → WASAPI / DirectSound
```

---

## Implementation plan

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
- Add the `{"xbmc.audiodsp", ADDON_AUDIODSP, ...}` row to the
  extension-point table in `AddonInfo.cpp` so the addon manager can
  discover, install and enable ADSP addons.

---

### 3 — Minimal ADSP DLL loader  
**New files:**  
`xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAEDSP.h`  
`xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAEDSP.cpp`

A single lightweight class `CActiveAEDSP`:

| Method | What it does |
|--------|--------------|
| `Init()` | Queries `CAddonMgr` for the first enabled `ADDON_AUDIODSP` addon, loads its DLL via `DllAddon`, calls `get_addon()` to fill the `AudioDSP` function-pointer table, then calls `ADDON_Create()` passing `AE_DSP_PROPERTIES{strUserPath}` |
| `StreamCreate(AEAudioFormat)` | Allocates an `ADDON_HANDLE_STRUCT`, fills `AE_DSP_SETTINGS` from the format, calls `StreamCreate` → `StreamIsModeSupported(MASTER_PROCESS)` → `StreamInitialize` |
| `MasterProcess(CSampleBuffer*)` | Casts `buf->pkt->data` (already planar float) to `float**` and calls `DSP_MasterProcess` in-place |
| `StreamDestroy()` | Calls `StreamDestroy`, frees the handle |
| `Deinit()` | Calls `ADDON_Destroy`, unloads DLL |

The include path for `kodi_adsp_types.h` / `kodi_adsp_dll.h` is already
present at `kodi-audiodsp-vsthost/include/kodi-legacy-adsp/`; it only needs
to be added to the ActiveAE `CMakeLists.txt`.

> **Why no `libKODI_adsp` host callbacks are needed:**  
> `addon_main.cpp` makes **zero** callbacks into Kodi.  `ADDON_Create` only
> reads `AE_DSP_PROPERTIES::strUserPath`.  All processing is self-contained in
> `DSPChain`.

---

### 4 — Hook in `ActiveAE.cpp`  
**File:** `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAE.cpp`  
**Header:** `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAE.h`

Add a member `CActiveAEDSP m_dsp` to `CActiveAE`.

In `CActiveAE::RunMixStage()`, after the mix + `MixSounds` / `Deamplify`
block produces `out` (around line 2255), and **before**
`m_sinkBuffers->m_inputSamples.push_back(out)`, insert:

```cpp
if (out && m_dsp.IsActive())
    m_dsp.MasterProcess(out);
```

On stream creation / destruction (the existing `CreateStream` / `FreeStream`
paths), call `m_dsp.StreamCreate(format)` / `m_dsp.StreamDestroy()`.

---

### 5 — CMakeLists for ActiveAE  
**File:** `xbmc/cores/AudioEngine/Engines/ActiveAE/CMakeLists.txt`

- Add `ActiveAEDSP.cpp` to the `SOURCES` list.
- Add the `kodi-legacy-adsp` include directory.

---

### 6 — Addon manifest  
**File:** `system/addon-manifest.xml`

```xml
<addon optional="true">xbmc.audiodsp</addon>
```

---

## What this does NOT require

- Restoring `ActiveAEDSPModeFactory`, `ActiveAEDSPProcessing`,
  `ActiveAEDSPDatabase`, and the other ~15 Krypton ADSP files.
- Implementing `libKODI_adsp` host callbacks (the addon makes none).
- Changing the `ADDON_TYPE` enum in `versions.h` (the DLL is loaded
  directly via `DllAddon::GetAddon()`).
- Modifying `audiodsp.vsthost` at all.

---

## Internal format compatibility

ActiveAE's internal mixing format is planar 32-bit float
(`AE_FMT_FLOATP`).  `CSampleBuffer::pkt->data` is a `uint8_t**` array of
float planes.  `IVSTPlugin::process()` / `DSPChain::process()` both accept
`float** in, float** out, int samples` — a direct cast with no format
conversion needed.

---

## Key source references

| Symbol | File |
|--------|------|
| `struct AudioDSP` (addon function table) | `kodi-audiodsp-vsthost/include/kodi-legacy-adsp/kodi_adsp_types.h:477` |
| `get_addon()` export | `kodi-audiodsp-vsthost/include/kodi-legacy-adsp/kodi_adsp_dll.h:536` |
| `CActiveAE::RunMixStage()` injection point | `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAE.cpp:2255` |
| `m_sinkBuffers->m_inputSamples.push_back(out)` | `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAE.cpp:2263` |
| `DSPChain::process()` | `kodi-audiodsp-vsthost/src/dsp/DSPChain.h` |
| `CAddonDll::Create(ADDON_TYPE, void*, void*)` | `xbmc/addons/binary-addons/AddonDll.cpp:184` |
