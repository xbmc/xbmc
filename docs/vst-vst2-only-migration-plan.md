# VST Addon VST3 Removal Plan (VST/VST2 Only)

## Goal
- Disable VST3 support in the VST addon.
- Keep VST/VST2 support working.
- Remove VST3-specific build, compilation, and workflow steps.

## Orchestrator Role
- Coordinate analysis and implementation.
- Gate edits with baseline and post-change validation.
- Ensure tasks are split and executed by subagents.
- Consolidate final status and export final summary document.

## Subagent Task Breakdown

### Subagent A — Code & Build System (Addon)
- Update `kodi-audiodsp-vsthost/CMakeLists.txt`:
  - Remove VST3 SDK fetch/submodule usage.
  - Remove VST3 hosting library target and include/link wiring.
  - Keep VST2 build path intact.
- Update addon runtime code to stop instantiating/using VST3 code paths.
- Ensure code compiles without VST3 sources linked.

### Subagent B — Scanner & Metadata
- Update `kodi-audiodsp-vsthost/scanner/vstscanner.cpp`:
  - Remove VST3 probing logic.
  - Keep VST2 scanning and reporting.
  - Remove VST3-specific search path/extension handling where required.
- Update addon metadata text to reflect VST/VST2-only support.

### Subagent C — CI/Workflow Cleanup
- Update `.github/workflows/build-vst-addons.yaml`:
  - Remove VST3 SDK cache/validation/build references.
  - Keep addon build/package flow for VST/VST2.
  - Update release notes text to remove VST3 wording.
- Update `.github/workflows/build-windows.yaml`:
  - Remove VST3-specific addon build/staging steps if present.

### Subagent D — Validation
- Run relevant configure/build checks for changed areas.
- Confirm no required VST2 functionality/build wiring regressed.
- Provide concise pass/fail output back to orchestrator.

## Execution Sequence
1. Baseline check before edits.
2. Subagent A applies addon CMake/runtime changes.
3. Subagent B applies scanner/metadata changes.
4. Subagent C applies workflow cleanup.
5. Subagent D runs post-change validation.
6. Orchestrator reviews diffs/results and finalizes summary.

## Validation Criteria
- No VST3 SDK fetch/build references remain in targeted addon/workflows.
- VST2-related code paths remain present and enabled.
- Addon configure/build checks for touched scope complete successfully.
- Workflow YAML remains valid after cleanup.

## Deliverables
- Updated addon source/build/workflow files with VST3 support removed.
- This plan document.
- Final implementation summary markdown in `docs/`.
