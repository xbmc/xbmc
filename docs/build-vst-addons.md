# Build VST Addons

Standalone GitHub Actions workflow that builds and packages the Kodi VST addons for Windows x86.

## Workflow file

`.github/workflows/build-vst-addons.yaml`

## What it builds

| Artifact | Source | Build |
|---|---|---|
| `audiodsp.vsthost.dll` | `kodi-audiodsp-vsthost/` | CMake → Visual Studio 17 2022, Win32, Release |
| `vstscanner.exe` | `kodi-audiodsp-vsthost/scanner/vstscanner.cpp` | Same CMake project (skipped if file absent) |
| `plugin.audio.vstmanager/` | `addons/plugin.audio.vstmanager/` | No build — copied as-is |

## Triggers

- **Push** to `Leia` branch when files under `kodi-audiodsp-vsthost/`, `addons/plugin.audio.vstmanager/`, or the workflow itself change.
- **Pull request** to `Leia` for the same paths.
- **Manual** (`workflow_dispatch`).

## Zip layout

```
kodi-vst-addons-win32.zip
  addons/
    audiodsp.vsthost/
      audiodsp.vsthost.dll
      vstscanner.exe          (present only when scanner/vstscanner.cpp exists)
    plugin.audio.vstmanager/
      addon.xml
      default.py
      resources/
        settings.xml
        lib/
          ...
```

## Release versioning

Uses the `vst-v*` tag prefix (e.g. `vst-v1.0.0`) so VST releases are independent of the main Kodi build releases (`v*`). The patch component is auto-incremented on each successful push/dispatch to `Leia`.

## Caching

The VST3 SDK fetched via CMake `FetchContent` is cached in `kodi-audiodsp-vsthost/build/_deps` using `actions/cache@v4`. The cache key is a hash of `kodi-audiodsp-vsthost/CMakeLists.txt`; a `vst3sdk-` restore prefix allows a partial hit when only the tag changes.

## CMake configuration

```
cmake -G "Visual Studio 17 2022" -A Win32
  -DCMAKE_BUILD_TYPE=Release
  -DKODI_ADSP_SDK_DIR=<workspace>/addons/kodi-addon-dev-kit/include/kodi
  -DCMAKE_INSTALL_PREFIX=<workspace>/kodi-audiodsp-vsthost/install
  -S kodi-audiodsp-vsthost/
  -B kodi-audiodsp-vsthost/build/
```

The MSVC x86 toolset is selected automatically by the `Win32` platform argument — no need to invoke `vcvars32.bat` manually.

## Secrets

| Secret | Purpose |
|---|---|
| `ORGANIZERRO_PAT` | Preferred token for `gh release create` (allows release creation as OrganizerRo). Falls back to `github.token` if unset, enabling forks and public clones to run builds. |

## Permissions

`permissions: contents: write` — required for `gh release create` to push the release tag and upload assets.
