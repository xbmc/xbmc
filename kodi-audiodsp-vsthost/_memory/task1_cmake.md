---
name: Task 1 — CMake build system
description: CMakeLists.txt structure, target names, include paths, SDK locations
type: project
---

## Files created

| File | Description |
|------|-------------|
| `CMakeLists.txt` | Root build file — all targets, FetchContent, include paths |
| `.gitmodules` | Git submodule reference for `deps/vst3sdk` (created separately) |

## CMake target names

| Target | Type | Purpose |
|--------|------|---------|
| `vst3_hosting` | STATIC (or INTERFACE fallback) | Compiles the 13 VST3 SDK hosting source files |
| `audiodsp.vsthost` | SHARED | Main Kodi ADSP addon shared library |
| `vstscanner` | EXECUTABLE | Standalone VST plugin scanner; conditionally added when `scanner/vstscanner.cpp` exists |

## Key CMake variables

| Variable | Default | Purpose |
|----------|---------|---------|
| `KODI_ADSP_SDK_DIR` | `C:/src/xbmc/xbmc/addons/kodi-addon-dev-kit/include/kodi` | Path to the Kodi ADSP SDK headers; override with `-DKODI_ADSP_SDK_DIR=...` |
| `VST3SDK_DIR` | Set automatically by FetchContent or local override | Absolute path to the root of the VST3 SDK tree; available to all targets |
| `VST3_HOSTING_SOURCES` | List of 13 `.cpp` files | Full paths to the VST3 hosting subset sources; populated after SDK is available |

## Include path structure

```
audiodsp.vsthost (PRIVATE)
  ├── ${CMAKE_SOURCE_DIR}/src          — plugin source tree headers
  ├── ${VST3SDK_DIR}                   — VST3 SDK root (base/, pluginterfaces/, public.sdk/)
  └── ${KODI_ADSP_SDK_DIR}            — Kodi ADSP addon dev-kit headers

vst3_hosting (PUBLIC → inherited by consumers)
  └── ${VST3SDK_DIR}

vstscanner (PRIVATE)
  ├── ${CMAKE_SOURCE_DIR}/src
  └── ${VST3SDK_DIR}
```

## VST3 SDK acquisition strategy

1. **Local override** — if `deps/vst3sdk/CMakeLists.txt` exists (git submodule populated),
   it is used directly.  Set by `.gitmodules` with:
   ```
   [submodule "deps/vst3sdk"]
       path = deps/vst3sdk
       url  = https://github.com/steinbergmedia/vst3sdk.git
       branch = main
   ```
2. **FetchContent fallback** — downloads `v3.7.11_build_18` with only the three
   required sub-repos (`base`, `pluginterfaces`, `public.sdk`).

## VST3 hosting sources compiled into `vst3_hosting`

```
public.sdk/source/vst/hosting/module.cpp
public.sdk/source/vst/hosting/module_win32.cpp
public.sdk/source/vst/hosting/plugprovider.cpp
public.sdk/source/vst/hosting/pluginterfacesupport.cpp
public.sdk/source/vst/hosting/hostclasses.cpp
public.sdk/source/vst/hosting/processdata.cpp
public.sdk/source/vst/hosting/parameterchanges.cpp
public.sdk/source/vst/hosting/eventlist.cpp
public.sdk/source/vst/hosting/connectionproxy.cpp
base/source/fstring.cpp
base/source/updatehandler.cpp
pluginterfaces/base/funknown.cpp
pluginterfaces/base/conststringtable.cpp
```

## Windows-specific settings

- `audiodsp.vsthost` has `WINDOWS_EXPORT_ALL_SYMBOLS ON` so the DLL exports
  without an explicit `.def` file or `__declspec(dllexport)` annotations.
- All targets define `WIN32`, `UNICODE`, `_UNICODE` on Windows.
- The addon shared library has `PREFIX ""` to prevent the `lib` prefix on all
  platforms.

## Notes for later tasks

- `src/` uses `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` — adding new `.cpp`
  files to any subdirectory of `src/` will be picked up automatically on the
  next CMake reconfigure.
- `vstscanner` is conditionally added; Task N that creates `scanner/vstscanner.cpp`
  will automatically activate the target on the next configure.
- The `KODI_ADSP_SDK_DIR` cache variable can be overridden from the command line:
  `cmake -DKODI_ADSP_SDK_DIR=/path/to/sdk ..`
