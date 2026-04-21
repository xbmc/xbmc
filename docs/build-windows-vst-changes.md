# Windows Build: VST Addon Integration Changes

## Overview

Two new workflow steps were added to `.github/workflows/build-windows.yaml` so that both VST addons are compiled and included in the release ZIP for Windows x86 MSVC builds.

---

## New Steps

### 1. `Build audiodsp.vsthost addon`

**Position:** After the existing "Build addons" step, before "Collect build output into staging directory".

This step compiles the `audiodsp.vsthost` binary addon from the `kodi-audiodsp-vsthost/` source tree using a standalone CMake build (separate from the main Kodi CMake tree):

- Calls `vcvars32.bat` via `vswhere.exe` to set up the MSVC x86 toolchain.
- Configures with `Visual Studio 17 2022 / Win32`, pointing at the Kodi ADSP SDK headers from `addons/kodi-addon-dev-kit/include/kodi`.
- Builds the `audiodsp.vsthost` CMake target (the addon DLL).
- Optionally builds `vstscanner` if the target produces `vstscanner.exe`.
- Installs the build output to `vsthost-install/` in the workspace root.

### 2. `Stage VST addons into staging`

**Position:** After "Collect build output into staging directory", before "Sanitize and verify DLL provenance in staging".

This PowerShell step merges both VST addons into the `staging/addons/` directory that is later zipped for release:

- **`audiodsp.vsthost`** — copies `audiodsp.vsthost.dll` (and `vstscanner.exe` if present) from `vsthost-install/` into `staging/addons/audiodsp.vsthost/`. Emits a `::warning::` annotation if the DLL is missing rather than failing the build outright.
- **`plugin.audio.vstmanager`** — this is a pure-Python/script addon; the entire source tree at `addons/plugin.audio.vstmanager/` is recursively copied into `staging/addons/plugin.audio.vstmanager/`.

---

## Directory Layout After Staging

```
staging/
  kodi.exe
  *.dll               ← Kodi runtime DLLs
  addons/
    audiodsp.vsthost/
      audiodsp.vsthost.dll
      vstscanner.exe  (if built)
    plugin.audio.vstmanager/
      addon.xml
      *.py
      ...
    <other addons built by the main addon step>
  system/
  media/
  userdata/
  sounds/
```

---

## Dependencies / Prerequisites

| Requirement | Notes |
|---|---|
| `kodi-audiodsp-vsthost/` directory | Must exist in the repo root; contains the VST host CMake project |
| `addons/plugin.audio.vstmanager/` directory | Must exist; pure-script addon, no compilation required |
| Visual Studio 2022 (or compatible) | Detected automatically via `vswhere.exe` |
| CMake ≥ 3.16 | Already required by the main Kodi build |

---

## Rollback

To revert these changes, remove the two steps (`Build audiodsp.vsthost addon` and `Stage VST addons into staging`) from `.github/workflows/build-windows.yaml`. No other workflow steps are modified.
