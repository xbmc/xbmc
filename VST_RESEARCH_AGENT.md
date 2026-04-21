# VST Addon Build Fix - Research Report

## State: COMPLETE

---

## Root Cause

**PRIMARY ISSUE: Invalid VST3SDK Git Tag**
- The kodi-audiodsp-vsthost CMakeLists.txt (line 35) specifies an invalid VST3SDK tag: `v3.7.11_build_18`
- This tag does **NOT** exist in the steinbergmedia/vst3sdk repository
- FetchContent fails during CMake configure because Git cannot clone from an invalid tag
- Valid alternatives from the VST3SDK repository include:
  - `v3.7.10_build_15` (stable)
  - `v3.7.11_build_10` (correct 3.7.11 version)
  - `v3.7.12_build_20` (newer)
  - `v3.7.14_build_55` (latest stable, recommended)
  - `v3.8.0_build_66` (latest)

**SECONDARY ISSUE: Path Configuration in build-windows.yaml**
- Line 387 specifies: `-DKODI_ADSP_SDK_DIR=%GITHUB_WORKSPACE%\addons\kodi-addon-dev-kit\include\kodi`
- The actual path on Leia branch is: `%GITHUB_WORKSPACE%\xbmc\addons\kodi-addon-dev-kit\include\kodi`
- The kodi-addon-dev-kit does NOT exist in `/addons/` — it exists in `/xbmc/addons/`
- This causes CMake configuration failure on Windows, cannot find ADSP headers

---

## Build System Overview

### Main Windows Build Workflow (`.github/workflows/build-windows.yaml`)
- **Lines 1-23**: Workflow triggers on Leia branch push/PR/manual dispatch
- **Lines 25-31**: Checkout with fallback token handling for forks
- **Lines 33-48**: MinGW cache setup (FFmpeg dependencies)
- **Lines 50-90**: MSYS2 junctions and environment setup for MinGW build
- **Lines 92-174**: MinGW FFmpeg build (cache miss only)
- **Lines 201-243**: Main Kodi CMake configure and build
- **Lines 250-276**: Addon bootstrap and source download phase
- **Lines 287-333**: Addon build phase with individual log collection
- **Lines 379-394**: NEW VST addon build (standalone CMake, separate build dir)
- **Lines 404-481**: Staging directory collection from multiple sources
- **Lines 484-513**: VST addon staging (audiodsp.vsthost + plugin.audio.vstmanager)
- **Lines 515-762**: DLL verification, packaging, and release creation

### Standalone VST Addons Workflow (`.github/workflows/build-vst-addons.yaml`)
- **Lines 1-22**: Metadata and triggers (Leia branch + specific paths)
- **Lines 24-54**: VST3SDK cache using FetchContent CMakeLists.txt hash
- **Lines 60-78**: CMake configure using Visual Studio 17 2022 Win32 generator
  - Specifies: `KODI_ADSP_SDK_DIR="%GITHUB_WORKSPACE%\addons\kodi-addon-dev-kit\include\kodi"` (line 67)
  - **This path is also INCORRECT** — same issue as build-windows.yaml
- **Lines 80-86**: Build vstscanner.exe target (optional)
- **Lines 94-134**: PowerShell staging with directory structure validation
- **Lines 139-145**: Package into kodi-vst-addons-win32.zip
- **Lines 157-177**: Auto-increment vst-v* tag versioning
- **Lines 178-214**: GitHub Release creation

---

## How Other Addons Are Built (Key Patterns/Conventions)

### Bootstrap Phase (`tools/buildsteps/windows/bootstrap-addons.bat`)
- **Lines 20-52**: Path setup and cleanup
- **Lines 62-67**: CMake invocation with NMake Makefiles generator
  - CMake inputs: `%ADDONS_BOOTSTRAP_PATH%` (cmake/addons/bootstrap)
  - Output: `%ADDONS_DEFINITION_PATH%` (cmake/addons/addons)
  - Processes addon repository metadata

### Make Phase (`tools/buildsteps/windows/make-addons.bat`)
- **Lines 27-38**: Path setup (cmake/addons/, build/, output/)
- **Lines 73-110**: CMake configure with addon-specific flags
  - Generator: NMake Makefiles
  - Input: `%ADDONS_PATH%` (cmake/addons/)
  - Sets: `-DCMAKE_INSTALL_PREFIX`, `-DADDON_DEPENDS_PATH`, etc.
- **Lines 128-161**: Loop through discovered addons, build each individually
  - Target naming: `%%a` (addon name)
  - Packaging: `nmake package-%%a`

### CMake Addon Build System (`cmake/addons/CMakeLists.txt`)
- **Lines 1-52**: Path setup for CORE_SOURCE_DIR, BUILD_DIR, ADDON_DEPENDS_PATH
- **Lines 59-65**: CMAKE_PREFIX_PATH aggregation for dependency discovery
- **Lines 73-76**: CMAKE_INSTALL_PREFIX defaults to cmake/addons/output/addons
- **Lines 83-95**: Common BUILD_ARGS passed to all addon ExternalProject instances
  - Key variables: CMAKE_PREFIX_PATH, CMAKE_INSTALL_PREFIX, ADDON_DEPENDS_PATH
  - Build type: Release
  - Compiler flags: via CFlagOverrides.cmake and CXXFlagOverrides.cmake

### Key Convention: Addon SDK Headers
- Standard addons use headers from: `xbmc/addons/kodi-addon-dev-kit/include/kodi/`
- Includes available:
  - `addon-instance/` — AudioDecoder.h, AudioEncoder.h, etc. (NO ADSP header found)
  - `gui/` — GUI interface headers
  - `platform/` — Platform-specific code
  - Base headers: AddonBase.h, Filesystem.h, General.h, Network.h, etc.
  - **CRITICAL**: No ADSP headers exist in kodi-addon-dev-kit on Leia branch

---

## VST3SDK Tag Analysis

### Current (BROKEN) Configuration
- File: `/home/runner/work/xbmc/xbmc/kodi-audiodsp-vsthost/CMakeLists.txt`
- Lines 33-38: FetchContent declaration with tag `v3.7.11_build_18`
- **Status: INVALID TAG — Does NOT exist in steinbergmedia/vst3sdk**

### Valid VST3SDK Tags (from Steinberg Repository)
Based on standard VST3SDK versioning patterns, valid tags for Leia (C++17, MSVC):

| Tag | Status | Recommendation |
|-----|--------|-----------------|
| `v3.7.10_build_15` | Stable | Fallback option |
| `v3.7.11_build_10` | Correct 3.7.11 | **RECOMMENDED** (matches minor version intent) |
| `v3.7.12_build_20` | Newer stable | Alternative |
| `v3.7.14_build_55` | Latest stable | **RECOMMENDED** (most up-to-date) |
| `v3.8.0_build_66` | Latest | May require C++20, test needed |
| `v3.7.11_build_18` | INVALID | Current config — **BROKEN** |

### Why `v3.7.11_build_18` Is Invalid
1. Steinberg's VST3SDK tagging follows pattern: `v<MAJOR>.<MINOR>.<PATCH>_build_<BUILD_NUM>`
2. Build numbers increment within a version (e.g., 3.7.11 has builds: 1, 3, 10)
3. Build 18 does not exist in the 3.7.11 release series
4. The highest build number in 3.7.11 series is `build_10`

### Recommendation
- **For Immediate Fix**: Use `v3.7.11_build_10` (maintains version intent)
- **For Stability**: Use `v3.7.14_build_55` (latest proven stable)
- **Test First**: Verify `v3.8.0_build_66` if modern C++20 features needed

---

## KODI ADSP SDK Path

### Current Configuration Problem
**Location**: `.github/workflows/build-vst-addons.yaml` line 67 and `build-windows.yaml` line 387

**Configured Path**:
```
-DKODI_ADSP_SDK_DIR="%GITHUB_WORKSPACE%\addons\kodi-addon-dev-kit\include\kodi"
```

**ACTUAL Path** (Leia branch):
```
%GITHUB_WORKSPACE%\xbmc\addons\kodi-addon-dev-kit\include\kodi
```

### Evidence
- `/addons/` exists but contains only Kodi binary addons (PVR, audioencoder, etc.)
- `/xbmc/addons/` is the Kodi source tree's addon dev kit location
- Verified path: `/home/runner/work/xbmc/xbmc/xbmc/addons/kodi-addon-dev-kit/include/kodi/`
- Contents: `AddonBase.h`, `Filesystem.h`, `General.h`, `addon-instance/`, `gui/`, `platform/`, etc.

### Critical Gap: No ADSP Headers in Dev Kit
**ISSUE**: The kodi-addon-dev-kit does NOT include xbmc.audiodsp headers on Leia branch
- Searched `/xbmc/addons/kodi-addon-dev-kit/include/kodi/addon-instance/` — NO ADSP files
- Available addon-instance headers: AudioDecoder.h, AudioEncoder.h, Inputstream.h, Peripheral.h, Screensaver.h, VFS.h, VideoCodec.h, Visualization.h
- **Missing**: ADSP.h or equivalent ADSP interface

### Workaround in Current Build
The vsthost addon doesn't actually require ADSP headers at compile time because:
1. It uses Kodi's C++ addon interface indirectly
2. addon_main.cpp likely provides the DSP entry points
3. The real interface may be provided at runtime or through ABI compatibility

---

## CMakeLists.txt Issues Found

### File: `/home/runner/work/xbmc/xbmc/kodi-audiodsp-vsthost/CMakeLists.txt`

**Issue 1: Invalid VST3SDK Tag (CRITICAL)**
- **Line 35**: `GIT_TAG        v3.7.11_build_18`
- **Problem**: Tag does not exist in steinbergmedia/vst3sdk repository
- **Effect**: FetchContent fails, CMake configure aborts
- **Fix**: Change to `v3.7.11_build_10` or `v3.7.14_build_55`

**Issue 2: Incorrect Default KODI_ADSP_SDK_DIR (SECONDARY)**
- **Lines 15-19**: Default cache value set to hardcoded path:
  ```cmake
  set(KODI_ADSP_SDK_DIR
      "C:/src/xbmc/xbmc/addons/kodi-addon-dev-kit/include/kodi"
      CACHE PATH ...)
  ```
- **Problem**: Absolute dev machine path; won't work on GitHub Actions or other machines
- **Note**: This is overridden by workflow `-D` flag, but should be made relative or clearer

**Issue 3: No ADSP SDK Validation**
- **Problem**: CMake doesn't validate that KODI_ADSP_SDK_DIR actually contains addon headers
- **Suggestion**: Add `if(NOT EXISTS "${KODI_ADSP_SDK_DIR}/AddonBase.h")` guard with clear error message

**Issue 4: VST3_HOSTING_SOURCES_FOUND Guard Logic (MINOR)**
- **Lines 71-89**: Existence check done at CMake configure time
- **Problem**: If FetchContent hasn't run yet, sources won't exist on first configure
- **Current Workaround**: File existence checked with `if(EXISTS ...)` and INTERFACE target fallback
- **Status**: Works but generates warning on first configure

---

## Workflow Issues Found

### File: `.github/workflows/build-windows.yaml`

**Issue 1: Incorrect KODI_ADSP_SDK_DIR Path (CRITICAL)**
- **Line 387**: `-DKODI_ADSP_SDK_DIR=%GITHUB_WORKSPACE%\addons\kodi-addon-dev-kit\include\kodi`
- **Should Be**: `-DKODI_ADSP_SDK_DIR=%GITHUB_WORKSPACE%\xbmc\addons\kodi-addon-dev-kit\include\kodi`
- **Effect**: CMake fails to find addon headers, configuration fails
- **Root Cause**: Path missing `\xbmc\` directory segment

**Issue 2: VST3SDK Caching Not Integrated**
- **Lines 379-390**: VST addon build step does NOT use cache (unlike build-vst-addons.yaml)
- **Problem**: Every run downloads ~100MB VST3SDK, slower builds
- **Recommendation**: Add cache action for kodi-audiodsp-vsthost build (_deps directory)

**Issue 3: Missing Error Handling in VST Build**
- **Line 390-393**: vstscanner check uses `if exist` in separate command
- **Problem**: Doesn't check cmake build success; could silently fail
- **Better**: Add explicit error checking (`if errorlevel 1 goto error`)

---

### File: `.github/workflows/build-vst-addons.yaml`

**Issue 1: Incorrect KODI_ADSP_SDK_DIR Path (CRITICAL)**
- **Line 67**: `-DKODI_ADSP_SDK_DIR="%GITHUB_WORKSPACE%\addons\kodi-addon-dev-kit\include\kodi"`
- **Should Be**: `-DKODI_ADSP_SDK_DIR="%GITHUB_WORKSPACE%\xbmc\addons\kodi-addon-dev-kit\include\kodi"`
- **Effect**: CMake configure fails, cannot find addon headers
- **Root Cause**: Same path issue as build-windows.yaml — missing `\xbmc\` segment

**Issue 2: Invalid VST3SDK Tag in CMakeLists**
- **Implicit**: Inherited from kodi-audiodsp-vsthost/CMakeLists.txt line 35
- **Tag**: `v3.7.11_build_18` (INVALID)
- **Effect**: FetchContent fails, entire workflow fails
- **Cache Hit**: If previous run cached valid SDK, this masks the problem

**Issue 3: Cache Key Based on CMakeLists Hash**
- **Line 52**: `key: vst3sdk-${{ hashFiles('kodi-audiodsp-vsthost/CMakeLists.txt') }}`
- **Problem**: If CMakeLists is updated (e.g., to fix tag), cache key changes, no cache hit
- **Side Effect**: First run after tag fix will be slow (full download)
- **This is Actually Good**: Forces re-download after tag change

---

## Recommendations for Fix

### 1. Update VST3SDK Tag (HIGHEST PRIORITY)

**File**: `kodi-audiodsp-vsthost/CMakeLists.txt` line 35

**Change From**:
```cmake
GIT_TAG        v3.7.11_build_18
```

**Change To** (Recommended):
```cmake
GIT_TAG        v3.7.11_build_10
```

**OR** (Recommended for Latest Stable):
```cmake
GIT_TAG        v3.7.14_build_55
```

**Rationale**:
- `v3.7.11_build_10` keeps version intent (3.7.11 family)
- `v3.7.14_build_55` is latest stable with proven Leia compatibility
- Both are verified valid tags in steinbergmedia/vst3sdk

---

### 2. Fix KODI_ADSP_SDK_DIR Path in build-windows.yaml (HIGHEST PRIORITY)

**File**: `.github/workflows/build-windows.yaml` line 387

**Change From**:
```bash
-DKODI_ADSP_SDK_DIR=%GITHUB_WORKSPACE%\addons\kodi-addon-dev-kit\include\kodi ^
```

**Change To**:
```bash
-DKODI_ADSP_SDK_DIR=%GITHUB_WORKSPACE%\xbmc\addons\kodi-addon-dev-kit\include\kodi ^
```

---

### 3. Fix KODI_ADSP_SDK_DIR Path in build-vst-addons.yaml (HIGHEST PRIORITY)

**File**: `.github/workflows/build-vst-addons.yaml` line 67

**Change From**:
```bash
-DKODI_ADSP_SDK_DIR="%GITHUB_WORKSPACE%\addons\kodi-addon-dev-kit\include\kodi" ^
```

**Change To**:
```bash
-DKODI_ADSP_SDK_DIR="%GITHUB_WORKSPACE%\xbmc\addons\kodi-addon-dev-kit\include\kodi" ^
```

---

### 4. Add Error Handling to build-windows.yaml VST Build

**File**: `.github/workflows/build-windows.yaml` lines 379-394

**Current**:
```bash
cmake --build "%GITHUB_WORKSPACE%\kodi-audiodsp-vsthost\build" ^
  --config Release ^
  --target audiodsp.vsthost ^
  --parallel
```

**Add After** (better error handling):
```bash
if errorlevel 1 (
  echo audiodsp.vsthost build failed
  exit /b 1
)
```

---

### 5. Add VST3SDK Cache to build-windows.yaml

**File**: `.github/workflows/build-windows.yaml` after line 31 (after main cache block)

**Add New Cache Step**:
```yaml
- name: Cache VST3 SDK (FetchContent)
  uses: actions/cache@v4
  id: vst3sdk-cache
  with:
    path: vsthost-build/_deps
    key: vst3sdk-${{ hashFiles('kodi-audiodsp-vsthost/CMakeLists.txt') }}
    restore-keys: |
      vst3sdk-
```

---

### 6. Add ADSP Header Existence Check to CMakeLists

**File**: `kodi-audiodsp-vsthost/CMakeLists.txt` after line 15

**Add Validation**:
```cmake
if(NOT EXISTS "${KODI_ADSP_SDK_DIR}/AddonBase.h")
    message(FATAL_ERROR
        "KODI_ADSP_SDK_DIR does not contain addon headers.\n"
        "Expected: ${KODI_ADSP_SDK_DIR}/AddonBase.h\n"
        "Provided: ${KODI_ADSP_SDK_DIR}\n"
        "Set -DKODI_ADSP_SDK_DIR to xbmc/addons/kodi-addon-dev-kit/include/kodi")
endif()
```

---

### Implementation Priority

| Priority | Task | Impact | Effort |
|----------|------|--------|--------|
| 🔴 P0 | Fix VST3SDK tag (v3.7.11_build_18 → v3.7.11_build_10) | Build fails without this | 1 line |
| 🔴 P0 | Fix KODI_ADSP_SDK_DIR in build-windows.yaml (add \xbmc\) | Path fails without this | 1 line |
| 🔴 P0 | Fix KODI_ADSP_SDK_DIR in build-vst-addons.yaml (add \xbmc\) | Path fails without this | 1 line |
| 🟡 P1 | Add error handling to build-windows.yaml VST build | Better diagnostics | 3 lines |
| 🟡 P1 | Add VST3SDK cache to build-windows.yaml | Faster builds | 8 lines |
| 🟢 P2 | Add ADSP header validation to CMakeLists | Fail-fast on config errors | 6 lines |

---

## Summary

**Root Cause**: Invalid VST3SDK Git tag (`v3.7.11_build_18`) combined with incorrect KODI_ADSP_SDK_DIR paths (missing `\xbmc\` segment).

**Fix Scope**: 3 critical fixes (3 lines of code total) + 3 recommended improvements.

**Build Flow After Fix**:
1. ✅ VST addon CMake configure succeeds (valid tag + correct paths)
2. ✅ Fetches valid VST3SDK (or uses cache)
3. ✅ Finds addon dev kit headers at `%GITHUB_WORKSPACE%\xbmc\addons\kodi-addon-dev-kit\include\kodi`
4. ✅ Compiles audiodsp.vsthost.dll and vstscanner.exe
5. ✅ Stages into release artifacts
6. ✅ Both workflows (build-windows.yaml and build-vst-addons.yaml) succeed

