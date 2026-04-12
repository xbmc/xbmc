---
name: Task 6 — Out-of-process VST plugin scanner
description: vstscanner.cpp design, JSON output format, SEH pattern, invocation
type: project
---

## File created

| File | Description |
|------|-------------|
| `scanner/vstscanner.cpp` | Standalone Windows console EXE — scans VST2/VST3 plugins and emits NDJSON |

## CMake activation

The `vstscanner` executable target is defined in `CMakeLists.txt` with a
conditional guard:

```cmake
if(EXISTS "${CMAKE_SOURCE_DIR}/scanner/vstscanner.cpp")
    add_executable(vstscanner ...)
```

Creating `scanner/vstscanner.cpp` (this task) automatically activates the
target on the next `cmake` configure.  No CMakeLists.txt edits are required.

## Purpose

A crash-safe, out-of-process tool used by the Kodi ADSP addon to populate its
plugin database without risking the host process.  The addon spawns
`vstscanner.exe`, reads NDJSON from its stdout pipe, and parses the results.

Running in a separate process means that even a hard crash (segfault, stack
overflow) inside a plugin's `VSTPluginMain` only kills the scanner, not Kodi.

## Invocation

```
# Scan all default Windows VST directories (including registry path):
vstscanner.exe

# Scan one or more specific directories:
vstscanner.exe "C:\Program Files\VSTPlugins" "D:\MyPlugins"

# Capture output for offline analysis:
vstscanner.exe > plugins.ndjson
```

## NDJSON output format

One JSON object per line (Newline-Delimited JSON / NDJSON).  All strings are
UTF-8.  Backslashes and double-quotes in string values are escaped.

```json
{"path":"C:\\plugins\\myeq.dll","format":"vst2","name":"MyEQ","vendor":"Acme","numParams":8,"numInputs":2,"numOutputs":2,"hasChunk":false,"error":null}
{"path":"C:\\plugins\\delay.vst3","format":"vst3","name":"StereoDelay","vendor":"Widgets","numParams":0,"numInputs":0,"numOutputs":0,"hasChunk":false,"error":null}
{"path":"C:\\plugins\\bad.dll","format":"unknown","name":"","vendor":"","numParams":0,"numInputs":0,"numOutputs":0,"hasChunk":false,"error":"load failed (error 126)"}
```

### Field definitions

| Field | Type | Notes |
|-------|------|-------|
| `path` | string | Absolute path to the file that was scanned |
| `format` | string | `"vst2"`, `"vst3"`, or `"unknown"` |
| `name` | string | Plugin name from `effGetEffectName` (VST2) or classInfo.name (VST3) |
| `vendor` | string | Vendor string from `effGetVendorString` (VST2) or factoryInfo.vendor (VST3) |
| `numParams` | int | VST2: `AEffect::numParams`; VST3: always 0 (requires instantiation) |
| `numInputs` | int | VST2: `AEffect::numInputs`; VST3: always 0 |
| `numOutputs` | int | VST2: `AEffect::numOutputs`; VST3: always 0 |
| `hasChunk` | bool | VST2: `effFlagsProgramChunks` set in `AEffect::flags`; VST3: always false |
| `error` | string\|null | `null` on success; human-readable error message on failure |

## SEH containment pattern

The key design constraint is that MSVC does not allow C++ objects with
destructors (`std::string`, etc.) in the same function as `__try/__except`
blocks.  The solution is to isolate every SEH-guarded call into its own
separate, non-inline function:

```cpp
// SEPARATE FUNCTION — no C++ objects here
static AEffect* callVST2MainSafe(VESTENTRYPROC proc) {
    AEffect* result = nullptr;
    __try {
        result = proc(dummyMaster);   // plugin entry point — may crash
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        result = nullptr;             // contain any structured exception
    }
    return result;
}

// CALLING FUNCTION — may use std::string freely
static PluginInfo scanVST2(const std::string& path) {
    ...
    AEffect* effect = callVST2MainSafe(proc);   // SEH contained inside
    if (!effect || effect->magic != kEffectMagic) {
        info.error = "invalid AEffect";         // std::string safe here
    }
    ...
}
```

The same pattern applies to `dispatchSafe()` (wraps `effect->dispatcher`)
and the raw VST3 factory calls in the fallback path.

## Detection algorithm

For `.dll` files:

1. `LoadLibraryA` the file.
2. Check `GetProcAddress(hMod, "GetPluginFactory")` → VST3.
3. Check `GetProcAddress(hMod, "VSTPluginMain")` → VST2.
4. Check `GetProcAddress(hMod, "main")` → VST2 (pre-2.4 fallback).
5. Otherwise → "unknown", skip.
6. `FreeLibrary`, then call the appropriate scan function (which re-loads).

For `.vst3` paths (file or directory bundle): always VST3, skip detection.

## VST3 scanning — two-tier approach

### Tier 1 (preferred): VST3 SDK hosting library

When `VST3SDK_AVAILABLE` is defined (i.e. the `vst3_hosting` static lib is
linked in), uses `VST3::Hosting::Module::create()` and iterates
`factory.classInfos()` looking for `category == kVstAudioEffectClass`.

### Tier 2 (fallback): raw COM-style vtable walk

When the full SDK is not linked (e.g. a minimal build), the scanner manually
loads the DLL, resolves `GetPluginFactory`, and reads the binary
`IPluginFactory` vtable directly.  This avoids any SDK dependency while still
extracting name and vendor.

The raw vtable layout used:

```
vtable[3] = getFactoryInfo(self, PFactoryInfo*) → tresult
vtable[4] = countClasses(self) → int32
vtable[5] = getClassInfo(self, index, PClassInfo*) → tresult
```

`PClassInfo` fields used: `category[32]` (check == "Audio Module Class"),
`name[64]` (plugin display name).
`PFactoryInfo` fields used: `vendor[64]`.

numParams / numInputs / numOutputs are **not** populated for VST3 without
full instantiation (which requires a complete host session context).

## File enumeration

`std::filesystem::recursive_directory_iterator` with
`skip_permission_denied` option.  Collected extensions (case-insensitive):

- `.dll` — regular files only (VST2 candidates)
- `.vst3` — files or directories; `disable_recursion_pending()` called on
  directories to avoid descending inside the bundle

## Default search paths (no arguments)

```
C:\Program Files\VSTPlugins
C:\Program Files\Steinberg\VSTPlugins
C:\Program Files\Common Files\VST3
C:\Program Files\Common Files\Steinberg\VST3
HKLM\SOFTWARE\VST\VSTPluginsPath  (registry, if present)
```

## JSON encoding

Implemented manually in `toJson()` — no external library.  All 7 JSON
string-escape sequences handled: `\"`, `\\`, `\n`, `\r`, `\t`, and `\uXXXX`
for control characters below 0x20.  Output is flushed after every line so
that a parent process reading the stdout pipe receives results incrementally.

## Known limitations

- VST3 `numParams`, `numInputs`, `numOutputs` are always 0 in the scan output
  (would require full audio processor instantiation).
- 32-bit plugins cannot be loaded by a 64-bit scanner process (and vice versa).
  The recommended solution is to build separate 32-bit and 64-bit scanner
  binaries and invoke the appropriate one based on the DLL's PE header
  machine type.
- Shell plugins (VST2 `kPlugCategShell`) contain multiple sub-plugins under
  a single DLL.  The scanner reports only the top-level name returned by
  `effGetEffectName` without iterating sub-plugin IDs.
