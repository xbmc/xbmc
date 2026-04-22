# VST Audio Routing Integration — Fix Tracker

## Goal
Make `audiodsp.vsthost` ADSP add-on fully functional:
1. Kodi discovers & loads the add-on as type `ADDON_AUDIODSP`
2. Audio flows: Kodi → ActiveAE mixer → CActiveAEDSP → VST chain → back to ActiveAE → sink (speakers)
3. Named pipe (`\\.\pipe\kodi_vsthost_editor`) stays alive across stream changes so the Python VST Manager can open/close plugin UIs
4. Compiles with no errors on Windows; non-Windows builds are unaffected

---

## Issues Found (from vst-audio-routing-plan.md analysis)

| # | Issue | File | Status |
|---|-------|------|--------|
| 1 | `ADDON_AUDIODSP` missing from TYPE enum | `xbmc/addons/AddonInfo.h` | ✅ Done |
| 2 | No `xbmc.audiodsp` → `ADDON_AUDIODSP` mapping in types table | `xbmc/addons/AddonInfo.cpp` | ✅ Done |
| 3 | `AddonBuilder::Build()` / `FromProps()` has no `ADDON_AUDIODSP` case → returns null | `xbmc/addons/AddonBuilder.cpp` | ✅ Done |
| 4 | No virtual add-on descriptor for `xbmc.audiodsp` provider | `addons/xbmc.audiodsp/addon.xml` | ✅ Done |
| 5 | Add-on not in the mandatory manifest | `system/addon-manifest.xml` | ✅ Done |
| 6 | `CActiveAEDSP` class (Windows-only DSP integration) missing | `ActiveAEDSP.h/.cpp` | ✅ Done |
| 7 | ActiveAE does not call DSP; no Init/Deinit/OnConfigure/MasterProcess hooks | `ActiveAE.h`, `ActiveAE.cpp` | ✅ Done |
| 8 | `ActiveAEDSP.cpp` not in CMake build | `AudioEngine/CMakeLists.txt` | ✅ Done |
| 9 | Named pipe stops when stream ends; restarts on next stream → Python must reconnect | `addon_main.cpp`, `EditorBridge.h/.cpp` | ✅ Done |
| 10 | `__try/__except` crash guard missing around VST process call | `ActiveAEDSP.cpp` (Windows-only) | ✅ Done |
| 11 | Interleaved float buffers bypassed VST processing silently | `ActiveAEDSP.cpp` | ✅ Done |

---

## Design Decisions

### Named pipe persistence
- `ADDON_Create` → `g_editorBridge.start(nullptr)` — start pipe, no chain yet
- `StreamCreate`  → `g_editorBridge.setChain(&proc->getChain())` — attach chain
- `StreamDestroy` → `g_editorBridge.setChain(nullptr)` — detach chain
- `ADDON_Destroy` → `g_editorBridge.stop()` — shutdown pipe

### MasterProcess format handling
- Planar (planes > 1): call DSP directly in-place
- Interleaved (planes == 1): deinterleave → DSP → reinterleave
  Uses per-stream scratch buffers allocated in `StreamCreate`

### DLL loading (bypass CAddonDll::Create)
- `CAddonDll::Create(ADDON_TYPE, ...)` calls `CheckAPIVersion` which expects
  `ADDON_GetTypeVersion` exported from the DLL — our legacy ADSP DLL does NOT export it.
- Instead: downcast to `CAddonDll*`, get `LibPath()`, create `DllAddon` manually,
  call `GetAddon(&m_funcs)` then `Create(nullptr, &props)` directly.

### SEH crash guard
- `__try/__except(EXCEPTION_EXECUTE_HANDLER)` wraps every `MasterProcess` call.
- On crash: sets `m_dspFailed` flag, logs error, and passthrough resumes silently.
- Only compiled on `TARGET_WINDOWS`.

---

## Key Paths
| Purpose | Path |
|---------|------|
| ADSP type enum | `xbmc/addons/AddonInfo.h` |
| Type mapping table | `xbmc/addons/AddonInfo.cpp` |
| Addon builder | `xbmc/addons/AddonBuilder.cpp` |
| Virtual addon descriptor | `addons/xbmc.audiodsp/addon.xml` |
| Manifest | `system/addon-manifest.xml` |
| DSP integration header | `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAEDSP.h` |
| DSP integration impl | `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAEDSP.cpp` |
| AudioEngine CMake | `xbmc/cores/AudioEngine/CMakeLists.txt` |
| ActiveAE main | `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAE.h/.cpp` |
| Addon entry point | `kodi-audiodsp-vsthost/src/addon_main.cpp` |
| Editor bridge | `kodi-audiodsp-vsthost/src/bridge/EditorBridge.h/.cpp` |
| Legacy ADSP headers | `kodi-audiodsp-vsthost/include/kodi-legacy-adsp/` |
