# VST3 Link Failure — Root Cause & Fix Options

## Background

The `audiodsp.vsthost` Win32 addon fails to link because the `vst3_hosting` static
library is missing three implementation files from its source list.  The linker
reports unresolved externals for `FUnknown::iid` (and other interface `iid` statics)
and `ThreadChecker::create`.

### Confirmed root causes (VST3 SDK tag `v3.7.9_build_61`)

| # | Symbol(s) | Missing file | Notes |
|---|-----------|-------------|-------|
| 1 | `FUnknown::iid`, `IPluginFactory::iid`, `IBStream::iid`, `IPluginBase::iid`, … | `pluginterfaces/base/coreiids.cpp` | Not in the CMake source list at all |
| 2 | `IPlugView::iid`, `IPlugFrame::iid` | `public.sdk/source/common/commoniids.cpp` | Not in the CMake source list at all |
| 3 | `ThreadChecker::create` | `public.sdk/source/common/threadchecker_win32.cpp` | **Wrong path** in CMakeLists.txt — listed as `public.sdk/source/vst/utility/threadchecker_win32.cpp` (that path does not exist); CMake silently drops it via the `if(EXISTS)` guard |

---

## Option A — Fix the CMakeLists.txt (Recommended)

**Three surgical changes to `kodi-audiodsp-vsthost/CMakeLists.txt`.**  
No C++ source changes required.  VST3 support remains fully enabled.

### Change 1 — Add `coreiids.cpp`

```cmake
# in VST3_HOSTING_SOURCES, after pluginterfaces/base/funknown.cpp:
"${VST3SDK_DIR}/pluginterfaces/base/coreiids.cpp"
```

This file defines `DEF_CLASS_IID` for `FUnknown`, `IPluginFactory`, `IPluginFactory2`,
`IPluginFactory3`, `IPluginBase`, `ICloneable`, `IDependent`, `IUpdateHandler`,
`IBStream`, and `ISizeableStream`.

### Change 2 — Add `commoniids.cpp`

```cmake
# in VST3_HOSTING_SOURCES, after commoniids.cpp entry:
"${VST3SDK_DIR}/public.sdk/source/common/commoniids.cpp"
```

This file defines `IPlugView::iid` and `IPlugFrame::iid` (and
`IPlugViewContentScaleSupport::iid`).

### Change 3 — Fix the threadchecker path

```cmake
# WRONG (file does not exist in v3.7.9):
"${VST3SDK_DIR}/public.sdk/source/vst/utility/threadchecker_win32.cpp"

# CORRECT:
"${VST3SDK_DIR}/public.sdk/source/common/threadchecker_win32.cpp"
```

`ThreadChecker::create()` is used by `connectionproxy.cpp` (a member default
initializer: `std::unique_ptr<ThreadChecker> threadChecker {ThreadChecker::create()}`).
Moving the path to `source/common/` — where the file actually lives — resolves the
unresolved-external at link time.

### Why this is the preferred fix

- VST3 is the current industry standard for audio plugins; keeping it enabled
  preserves the most user value.
- The three changes are purely in the build system (CMakeLists.txt).  Zero C++
  source files need touching.
- The `if(EXISTS)` guard in CMakeLists.txt already correctly silences the problem
  during early cmake invocations (before FetchContent runs); after the fix the files
  will be found and compiled on the first real build.

---

## Option B — Comment out VST3, keep VST2/classic only

Use this option if you want a guaranteed-green CI immediately and are willing to
defer VST3 work (e.g., no VST3 plugins to test with right now).

**Goal:** keep the VST3 source files in the repository but exclude them from the
build so they can be re-enabled trivially later.

### Step 1 — CMakeLists.txt: remove the `vst3_hosting` library

Remove (or comment out) the entire `vst3_hosting` static library block (the
`VST3_HOSTING_SOURCES` list, the `add_library(vst3_hosting …)` call, and its
`target_include_directories` / `target_compile_features` / `target_compile_definitions`
blocks).

Remove `vst3_hosting` from `target_link_libraries(audiodsp.vsthost …)` and from
`target_link_libraries(vstscanner …)`.

Remove or guard the `FetchContent` VST3 SDK download block so it is skipped when
VST3 is disabled.

Add a CMake option to make re-enablement easy:
```cmake
option(ENABLE_VST3 "Enable VST3 plugin hosting" OFF)
```

### Step 2 — CMakeLists.txt: exclude VST3 sources from the glob

The main plugin sources are collected with `file(GLOB_RECURSE … src/*.cpp)`.
Add an exclusion for the `vst3/` sub-directory when `ENABLE_VST3` is `OFF`:
```cmake
if(NOT ENABLE_VST3)
    list(FILTER PLUGIN_SOURCES EXCLUDE REGEX ".*/vst3/.*")
endif()
```

### Step 3 — DSPChain.cpp: guard the VST3 include and instantiation

Wrap the VST3 include and the `else` branch of `addPlugin()` in a preprocessor guard:
```cpp
#ifdef ENABLE_VST3
#include "../vst3/VSTPlugin3.h"
#endif

// In addPlugin():
if (format == IVSTPlugin::PluginFormat::VST2)
    slot.plugin = std::make_unique<VSTPlugin2>(path);
#ifdef ENABLE_VST3
else
    slot.plugin = std::make_unique<VSTPlugin3>(path);
#else
else {
    std::fprintf(stderr,
        "[DSPChain] addPlugin: VST3 support not compiled in — treating '%s' as VST2\n",
        path.c_str());
    slot.plugin = std::make_unique<VSTPlugin2>(path);
}
#endif
```

### Step 4 — DSPChain serialization: preserve format value

In `serializeToJson` / `deserializeFromJson`, keep saving/loading the `"vst3"` format
string.  When `ENABLE_VST3` is `OFF`, `deserializeFromJson` maps `"vst3"` entries to
`VSTPlugin2` (with a stderr warning) rather than failing.  This means existing
`chain.json` files continue to load without data loss.

### Step 5 — IVSTPlugin.h: retain the `VST3` enum value

Keep `enum class PluginFormat { VST2, VST3 };` unchanged.  Removing the value would
break `chain.json` round-trips and any code that checks `getFormat()`.

### Step 6 — vstscanner: already guarded

`vstscanner.cpp` already gates VST3 scanning behind `#ifdef VST3SDK_AVAILABLE`.
When the VST3 library target is absent the `target_compile_definitions` that set
`VST3SDK_AVAILABLE` are not reached, so VST3 scanning is automatically disabled —
no source change needed.

### Step 7 — CMakeLists.txt: remove `VST3SDK_AVAILABLE` define

The `if(VST3_HOSTING_SOURCES_FOUND)` block that emits `VST3SDK_AVAILABLE` is already
conditional; removing the whole FetchContent + `vst3_hosting` block makes it
unreachable automatically.

### Trade-offs

| | Option A | Option B |
|---|----------|----------|
| CI green immediately | ✓ (after 3-line fix) | ✓ |
| VST3 plugins supported | ✓ | ✗ (until re-enabled) |
| Code changes | CMakeLists.txt only | CMakeLists.txt + DSPChain.cpp |
| Re-enable effort | N/A | Set `ENABLE_VST3=ON` + revert guards |
| Recommended | **Yes** | Only if deferring VST3 |
