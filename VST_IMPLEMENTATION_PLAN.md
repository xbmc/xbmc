# VST Addon Build Fix — Implementation Plan

## State: COMPLETE

---

## Problem Statement Summary

The VST addon build pipeline fails on two distinct fronts:

1. **CMake FetchContent fails** during `kodi-audiodsp-vsthost` configuration because the specified VST3SDK Git tag (`v3.7.11_build_18`) does not exist in the upstream `steinbergmedia/vst3sdk` repository. Git cannot resolve the tag, causing a fatal clone failure.

2. **CMake cannot find ADSP headers** because both workflow files pass an incorrect `-DKODI_ADSP_SDK_DIR` path. The path omits the `xbmc/` prefix, pointing to a directory that does not exist on disk (`addons/kodi-addon-dev-kit/`) instead of the correct location (`xbmc/addons/kodi-addon-dev-kit/`).

Both issues are small, targeted configuration errors — no logic changes are required.

---

## Confirmed Root Causes

### Root Cause 1 — Non-existent VST3SDK Git Tag

- **File**: `kodi-audiodsp-vsthost/CMakeLists.txt`, line 35
- **Evidence**: Tag `v3.7.11_build_18` was verified absent from `https://github.com/steinbergmedia/vst3sdk` (tags jump from `v3.7.11_build_10` directly to later builds; `_build_18` was never published).
- **Confirmed valid tag**: `v3.7.11_build_10` (commit SHA `7d92338ae922db2d559ac458824a4df40f37e82e`) exists and is fetchable.
- **Failure mode**: `git clone --branch v3.7.11_build_18` exits non-zero → CMake `FetchContent_MakeAvailable` aborts → entire configure step fails.

### Root Cause 2 — Wrong `KODI_ADSP_SDK_DIR` in `build-vst-addons.yaml`

- **File**: `.github/workflows/build-vst-addons.yaml`, line 67
- **Evidence**: The Kodi source tree layout places the addon dev-kit at `xbmc/addons/kodi-addon-dev-kit/`. The `addons/` top-level directory in `GITHUB_WORKSPACE` is the addons build output area, not the SDK source. Passing a non-existent directory causes CMake to fail finding `AddonBase.h` and related ADSP headers.

### Root Cause 3 — Wrong `KODI_ADSP_SDK_DIR` in `build-windows.yaml`

- **File**: `.github/workflows/build-windows.yaml`, line 387
- **Evidence**: Identical path mistake as Root Cause 2 — same `addons\kodi-addon-dev-kit` vs `xbmc\addons\kodi-addon-dev-kit` discrepancy. This is the main integration build workflow; the same CMake configure step fails for the same reason.

---

## Fix 1: VST3SDK Tag — `kodi-audiodsp-vsthost/CMakeLists.txt`

**File**: `kodi-audiodsp-vsthost/CMakeLists.txt`  
**Line**: 35

### Before (broken)
```cmake
    FetchContent_Declare(vst3sdk
        GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
        GIT_TAG        v3.7.11_build_18          # <-- does NOT exist
        GIT_SUBMODULES "base;pluginterfaces;public.sdk"
        GIT_SHALLOW    TRUE
    )
```

### After (fixed)
```cmake
    FetchContent_Declare(vst3sdk
        GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
        GIT_TAG        v3.7.11_build_10          # valid tag, SHA 7d92338ae922db2d559ac458824a4df40f37e82e
        GIT_SUBMODULES "base;pluginterfaces;public.sdk"
        GIT_SHALLOW    TRUE
    )
```

**Change**: Replace `v3.7.11_build_18` → `v3.7.11_build_10` (single-token change on line 35).

---

## Fix 2: `KODI_ADSP_SDK_DIR` Path — `build-vst-addons.yaml`

**File**: `.github/workflows/build-vst-addons.yaml`  
**Line**: 67  
**Step**: "Configure audiodsp.vsthost (CMake)"

### Before (broken)
```yaml
          cmake -G "Visual Studio 17 2022" -A Win32 ^
            -DCMAKE_BUILD_TYPE=Release ^
            -DKODI_ADSP_SDK_DIR="%GITHUB_WORKSPACE%\addons\kodi-addon-dev-kit\include\kodi" ^
            -DCMAKE_INSTALL_PREFIX="%GITHUB_WORKSPACE%\kodi-audiodsp-vsthost\install" ^
            -S "%GITHUB_WORKSPACE%\kodi-audiodsp-vsthost" ^
            -B "%GITHUB_WORKSPACE%\kodi-audiodsp-vsthost\build"
```

### After (fixed)
```yaml
          cmake -G "Visual Studio 17 2022" -A Win32 ^
            -DCMAKE_BUILD_TYPE=Release ^
            -DKODI_ADSP_SDK_DIR="%GITHUB_WORKSPACE%\xbmc\addons\kodi-addon-dev-kit\include\kodi" ^
            -DCMAKE_INSTALL_PREFIX="%GITHUB_WORKSPACE%\kodi-audiodsp-vsthost\install" ^
            -S "%GITHUB_WORKSPACE%\kodi-audiodsp-vsthost" ^
            -B "%GITHUB_WORKSPACE%\kodi-audiodsp-vsthost\build"
```

**Change**: Insert `xbmc\` after `%GITHUB_WORKSPACE%\` in the `KODI_ADSP_SDK_DIR` value on line 67.

---

## Fix 3: `KODI_ADSP_SDK_DIR` Path — `build-windows.yaml`

**File**: `.github/workflows/build-windows.yaml`  
**Line**: 387  
**Step**: VST addon CMake configure block (lines 382–394)

### Before (broken)
```yaml
          cmake -G "Visual Studio 17 2022" -A Win32 ^
                -DCMAKE_BUILD_TYPE=Release ^
                -DKODI_ADSP_SDK_DIR=%GITHUB_WORKSPACE%\addons\kodi-addon-dev-kit\include\kodi ^
                -DCMAKE_INSTALL_PREFIX=%GITHUB_WORKSPACE%\vsthost-install ^
                %GITHUB_WORKSPACE%\kodi-audiodsp-vsthost
```

### After (fixed)
```yaml
          cmake -G "Visual Studio 17 2022" -A Win32 ^
                -DCMAKE_BUILD_TYPE=Release ^
                -DKODI_ADSP_SDK_DIR=%GITHUB_WORKSPACE%\xbmc\addons\kodi-addon-dev-kit\include\kodi ^
                -DCMAKE_INSTALL_PREFIX=%GITHUB_WORKSPACE%\vsthost-install ^
                %GITHUB_WORKSPACE%\kodi-audiodsp-vsthost
```

**Change**: Insert `xbmc\` after `%GITHUB_WORKSPACE%\` in the `KODI_ADSP_SDK_DIR` value on line 387. Note: this block does **not** quote the path (unlike Fix 2), so no quote adjustments are needed.

---

## Why These Fixes Work

| Fix | Mechanism |
|-----|-----------|
| Fix 1 | `GIT_SHALLOW TRUE` + a valid tag lets `git clone --branch v3.7.11_build_10 --depth 1` succeed. FetchContent populates `vst3sdk_SOURCE_DIR` and CMake configure proceeds normally. |
| Fix 2 | CMake receives an existing directory for `KODI_ADSP_SDK_DIR`. The `find_path`/`include_directories` calls in `kodi-audiodsp-vsthost/CMakeLists.txt` resolve `AddonBase.h` and ADSP headers, allowing compilation. |
| Fix 3 | Same mechanism as Fix 2, applied to the main integration workflow so the integrated build also succeeds. |

All three changes are purely configuration corrections — no source code, no build logic, and no dependencies are altered.

---

## Validation Steps

### Verify Fix 1 (tag exists)
```bash
git ls-remote https://github.com/steinbergmedia/vst3sdk.git refs/tags/v3.7.11_build_10
# Expected output:
# 7d92338ae922db2d559ac458824a4df40f37e82e  refs/tags/v3.7.11_build_10
```

### Verify Fix 2 (path is correct in build-vst-addons.yaml)
```bash
grep -n "KODI_ADSP_SDK_DIR" .github/workflows/build-vst-addons.yaml
# Expected: line 67 shows xbmc\addons\kodi-addon-dev-kit\include\kodi
```

### Verify Fix 3 (path is correct in build-windows.yaml)
```bash
grep -n "KODI_ADSP_SDK_DIR" .github/workflows/build-windows.yaml
# Expected: line 387 shows xbmc\addons\kodi-addon-dev-kit\include\kodi
```

### Verify the SDK directory actually exists in the repo
```bash
ls xbmc/addons/kodi-addon-dev-kit/include/kodi/
# Should list AddonBase.h, Filesystem.h, addon-instance/, gui/, etc.
```

### End-to-end workflow validation
After merging the three fixes, trigger `.github/workflows/build-vst-addons.yaml` manually (workflow_dispatch on Leia branch). The pipeline should:
1. Pass "Configure audiodsp.vsthost (CMake)" — no FetchContent failure, no missing headers.
2. Pass "Build audiodsp.vsthost.dll" — DLL compiled successfully.
3. Produce `kodi-vst-addons-win32.zip` artifact.

---

## Expected Build Flow After Fixes

```
build-vst-addons.yaml
├── [cache hit?] Restore vst3sdk from Actions cache
│     └── cache key: hash of kodi-audiodsp-vsthost/CMakeLists.txt
├── [cache miss] FetchContent clones v3.7.11_build_10  ← Fix 1 enables this
│     └── git clone --branch v3.7.11_build_10 --depth 1 steinbergmedia/vst3sdk
├── Configure CMake
│     └── -DKODI_ADSP_SDK_DIR=...xbmc\addons\kodi-addon-dev-kit\include\kodi  ← Fix 2
│           CMake finds AddonBase.h → configure succeeds
├── Build audiodsp.vsthost.dll  ← previously unreachable
├── (optional) Build vstscanner.exe
├── Stage build artifacts
└── Create GitHub Release → kodi-vst-addons-win32.zip

build-windows.yaml  (VST section, lines 379–394)
├── vcvars32.bat environment setup
├── Configure CMake
│     └── -DKODI_ADSP_SDK_DIR=...xbmc\addons\kodi-addon-dev-kit\include\kodi  ← Fix 3
│           CMake finds AddonBase.h → configure succeeds
├── Build audiodsp.vsthost target
├── (optional) Build vstscanner.exe
└── cmake --install → artifacts staged for packaging
```

---

## Risk Assessment

| Dimension | Assessment |
|-----------|------------|
| **Scope** | 3 single-line configuration changes across 2 YAML files and 1 CMakeLists.txt |
| **Blast radius** | Zero — no source code, no build logic, no other targets affected |
| **Regression risk** | Negligible — correcting a non-existent tag to a verified existing tag cannot break other functionality |
| **Reversibility** | Trivially reversible by reverting the three lines |
| **Confidence** | High — all three root causes verified by direct inspection and tag existence checks |

**Overall risk: LOW**

---

## Implementation Order

Apply all three fixes in a single commit (they are independent and non-conflicting):

1. Edit `kodi-audiodsp-vsthost/CMakeLists.txt` line 35: `v3.7.11_build_18` → `v3.7.11_build_10`
2. Edit `.github/workflows/build-vst-addons.yaml` line 67: add `xbmc\` to path
3. Edit `.github/workflows/build-windows.yaml` line 387: add `xbmc\` to path
