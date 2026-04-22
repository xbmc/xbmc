# VST Addon VST3 Removal — Final Summary

## Scope Completed
- Disabled VST3 support in the VST addon code/build path.
- Kept VST/VST2 support active.
- Removed VST3-specific build/workflow steps where applicable.

## Orchestrator/Subagent Execution
- Orchestrator performed coordination, planning, validation, and consolidation.
- Subagent A: addon CMake/runtime cleanup.
- Subagent B: scanner/runtime VST3 path removal and addon-local submodule cleanup.
- Subagent C: workflow cleanup for VST3-specific CI steps.

## Files Updated
- `docs/vst-vst2-only-migration-plan.md` (detailed plan)
- `.github/workflows/build-vst-addons.yaml`
- `kodi-audiodsp-vsthost/CMakeLists.txt`
- `kodi-audiodsp-vsthost/scanner/vstscanner.cpp`
- `kodi-audiodsp-vsthost/src/dsp/DSPChain.cpp`
- `kodi-audiodsp-vsthost/src/dsp/DSPChain.h`
- `kodi-audiodsp-vsthost/src/plugin/IVSTPlugin.h`
- `kodi-audiodsp-vsthost/resources/addon.xml`
- Removed: `kodi-audiodsp-vsthost/.gitmodules` (VST3 SDK submodule metadata)

## Key Outcomes
- VST3 SDK FetchContent/submodule-based build wiring removed from addon CMake.
- VST3 hosting linkage removed.
- VST3 source subtree excluded from addon compilation.
- Runtime plugin chain now rejects non-VST2 formats safely.
- Scanner now enumerates/probes VST2 (`.dll`) only; VST3 probing removed.
- VST3-specific CI steps removed from `build-vst-addons.yaml`.
- `build-windows.yaml` was reviewed; no VST3-specific steps required removal.
- Addon metadata text updated to VST/VST2 wording.

## Validation Performed
- Pre-change configure check: `cmake -S /home/runner/work/xbmc/xbmc/kodi-audiodsp-vsthost -B /tmp/xbmc-vsthost-baseline` (pass; Windows-only build skipped on Linux host).
- Post-change configure check: `cmake -S /home/runner/work/xbmc/xbmc/kodi-audiodsp-vsthost -B /tmp/xbmc-vsthost-postchange` (pass; Windows-only build skipped on Linux host).
- Parallel validation executed (Code Review + CodeQL): no blocking findings.
