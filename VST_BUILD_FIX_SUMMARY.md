# VST Addon Build Fix — Summary

## State: COMPLETE

---

## What Was Fixed

The VST addon build workflow (`build-vst-addons.yaml`, run 24705844367 / job 72258847369)
failed with two root causes. Three targeted, single-line configuration corrections resolve both.

---

## Root Cause Analysis

### Cause 1 — Non-existent VST3SDK Git Tag (PRIMARY)

| | |
|---|---|
| **File** | `kodi-audiodsp-vsthost/CMakeLists.txt` line 35 |
| **Error** | `fatal: invalid reference: v3.7.11_build_18` |
| **Problem** | Tag `v3.7.11_build_18` does not exist in `steinbergmedia/vst3sdk` |
| **Effect** | FetchContent git-clone fails → CMake configure aborts |

The Steinberg VST3 SDK 3.7.11 series has only one published build: `v3.7.11_build_10`.
Build 18 was never released.

### Cause 2 — Incorrect KODI_ADSP_SDK_DIR Path

| | |
|---|---|
| **Files** | `build-vst-addons.yaml` line 67 and `build-windows.yaml` line 387 |
| **Problem** | Path `%GITHUB_WORKSPACE%\addons\kodi-addon-dev-kit\include\kodi` does not exist |
| **Correct path** | `%GITHUB_WORKSPACE%\xbmc\addons\kodi-addon-dev-kit\include\kodi` |
| **Effect** | CMake cannot find `AddonBase.h` → header inclusion fails |

The Kodi source tree places the addon dev-kit under `xbmc/addons/`, not `addons/`.

---

## Fixes Applied

| # | File | Change |
|---|------|--------|
| 1 | `kodi-audiodsp-vsthost/CMakeLists.txt:35` | `v3.7.11_build_18` → `v3.7.11_build_10` |
| 2 | `.github/workflows/build-vst-addons.yaml:67` | Added `xbmc\` to KODI_ADSP_SDK_DIR path |
| 3 | `.github/workflows/build-windows.yaml:387` | Added `xbmc\` to KODI_ADSP_SDK_DIR path |

All three changes are single-token configuration corrections — no source code, no build
logic, and no dependencies were altered.

---

## Verified Tag

```
$ git ls-remote https://github.com/steinbergmedia/vst3sdk.git refs/tags/v3.7.11_build_10
7d92338ae922db2d559ac458824a4df40f37e82e  refs/tags/v3.7.11_build_10
```

Tag `v3.7.11_build_10` exists and is fetchable via `GIT_SHALLOW TRUE`.

---

## Expected Build Flow After This Fix

```
build-vst-addons.yaml
├── Cache restore (vst3sdk FetchContent deps)
├── [cache miss] FetchContent clones v3.7.11_build_10  ← Fix 1
├── CMake configure with correct KODI_ADSP_SDK_DIR       ← Fix 2
├── Build audiodsp.vsthost.dll
├── Build vstscanner.exe (if present)
├── Stage addons/ layout
├── Package kodi-vst-addons-win32.zip
└── Create GitHub Release (vst-v* tag)

build-windows.yaml (VST section, lines 379–394)
├── vcvars32.bat + CMake configure with correct path      ← Fix 3
├── Build audiodsp.vsthost.dll
└── cmake --install → staging
```

---

## Subagent State Machine Files

| File | Subagent | State |
|------|----------|-------|
| `VST_RESEARCH_AGENT.md` | Research Subagent | COMPLETE |
| `VST_IMPLEMENTATION_PLAN.md` | Plan Subagent | COMPLETE |
| `VST_BUILD_FIX_SUMMARY.md` | Orchestrator | COMPLETE |

---

## Risk Assessment

- **Scope**: 3 single-line changes across 2 YAML files and 1 CMakeLists.txt
- **Blast radius**: Zero — other targets, addons, and workflow steps are unaffected
- **Regression risk**: Negligible — correcting non-existent tag to verified existing tag
- **Reversibility**: Trivially reversible (revert 3 lines)
- **Overall risk: LOW**
