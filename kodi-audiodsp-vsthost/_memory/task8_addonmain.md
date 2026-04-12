---
name: Task 8 — Kodi ADSP Addon Entry Point (addon_main)
description: Per-stream ADDON_HANDLE pattern, GetProc helper, capability flags, function name divergence (MenuHook vs CallMenuHook), config path derivation, return-type corrections
type: project
---

## Files created

| File | Purpose |
|------|---------|
| `src/addon_main.h`   | Minimal header — includes `kodi_adsp_dll.h` which declares all required C functions |
| `src/addon_main.cpp` | Full implementation of every Kodi ADSP callback function |

---

## Per-stream state pattern: ADDON_HANDLE->dataAddress

Kodi passes an `ADDON_HANDLE` (i.e. `ADDON_HANDLE_STRUCT*`) to every stream
callback.  The struct has three fields:

```c
struct ADDON_HANDLE_STRUCT {
    void *callerAddress;   // owned by Kodi
    void *dataAddress;     // addon may write a pointer here
    int   dataIdentifier;  // addon may write a small integer here
};
```

`dataAddress` is `void*` and is therefore the correct field to store a
`DSPProcessor*` for the lifetime of a stream.  `dataIdentifier` is an `int`
and MUST NOT be used for pointer storage.

In `StreamCreate` the addon allocates a `DSPProcessor`, initialises it, and
writes the pointer:

```cpp
handle->dataAddress = proc;   // void* — safe for any pointer
```

In `StreamDestroy` the addon retrieves, shuts down, and frees it:

```cpp
auto* proc = static_cast<DSPProcessor*>(handle->dataAddress);
proc->streamDestroy();
delete proc;
handle->dataAddress = nullptr;
```

---

## GetProc helper

```cpp
static DSPProcessor* GetProc(const ADDON_HANDLE handle) {
    return static_cast<DSPProcessor*>(handle->dataAddress);
}
```

Used in all stream callbacks that need to reach the per-stream processor.
Avoids repeated casts throughout the file.

---

## Capability flags

Only master processing is enabled.  All other flags are explicitly set to
`false` via `memset` + assignment:

```cpp
pCapabilities->bSupportsMasterProcess  = true;
// all others remain false
```

Kodi will only call master-processing callbacks (`MasterProcess`,
`MasterProcessGetDelay`, `MasterProcessGetOutChannels`,
`MasterProcessGetStreamInfoString`, `MasterProcessSetMode`,
`MasterProcessNeededSamplesize`) for this addon.

---

## Function name divergence: MenuHook vs CallMenuHook

The `AudioDSP` struct field is named `MenuHook`.  However, `get_addon()` wires
it like this:

```cpp
pDSP->MenuHook = CallMenuHook;   // from kodi_adsp_dll.h
```

Therefore the implementation function MUST be named `CallMenuHook`, not
`MenuHook`.  The declaration in `kodi_adsp_dll.h` also uses `CallMenuHook`.
This is the only name divergence between the `AudioDSP` struct fields and the
implementation function names.

---

## ADDON_GetSettings signature correction

`xbmc_addon_dll.h` declares:

```cpp
unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet);
```

The return type is `unsigned int` (count of settings), NOT `ADDON_STATUS`.
There is no `int* entries` out-parameter.  The task description prototype
differed; the actual header was followed.

---

## MasterProcessGetDelay return type

The `AudioDSP` struct declares:

```cpp
float (__cdecl* MasterProcessGetDelay)(const ADDON_HANDLE);
```

The Kodi API expects a `float` (time in seconds).  `DSPProcessor::masterProcessGetDelay()`
returns an `int` (latency in samples).  The implementation casts to `float`:

```cpp
return static_cast<float>(GetProc(handle)->masterProcessGetDelay());
```

Note: the value returned is sample-count cast to float, which is technically
not seconds.  A future improvement could divide by `getSampleRate()` to return
true seconds.  The cast is correct for compilation; the semantic interpretation
depends on how Kodi uses the value.

---

## AE_DSP_PROPERTIES struct name

The props struct passed to `ADDON_Create` is `AE_DSP_PROPERTIES` (defined in
`kodi_adsp_types.h`).  It has two fields:

```cpp
typedef struct AE_DSP_PROPERTIES {
    const char* strUserPath;    // path to user profile directory
    const char* strAddonPath;   // path to addon installation directory
} AE_DSP_PROPERTIES;
```

`strUserPath` is stored in `g_addonDataPath` and used as the directory for
`chain.json`.

---

## Config path derivation

```cpp
std::string cfgPath = g_addonDataPath + "/chain.json";
proc->setConfigPath(cfgPath);
```

`g_addonDataPath` is set once at addon load from `AE_DSP_PROPERTIES::strUserPath`.
All streams share the same `chain.json` path.  This is appropriate because a
single chain configuration is expected to apply to all streams simultaneously.

---

## Global state

```cpp
static std::string g_addonDataPath;
```

One global, set in `ADDON_Create`.  There is no per-addon-instance state
beyond this; all per-stream state lives in `DSPProcessor` objects referenced
via `ADDON_HANDLE::dataAddress`.

---

## Stub stategies for unsupported processing stages

| Stage | Strategy |
|-------|----------|
| `InputProcess` | Return `false` |
| `InputResampleProcess` | Return `samples` (pass-through count) |
| `InputResampleSampleRate` | Return `0` |
| `InputResampleGetDelay` | Return `0.0f` |
| `PreProcess` | Return `samples` |
| `PreProcessGetDelay` | Return `0.0f` |
| `PostProcess` | Return `samples` |
| `PostProcessGetDelay` | Return `0.0f` |
| `OutputResampleProcess` | Return `samples` |
| `OutputResampleSampleRate` | Return `0` |
| `OutputResampleGetDelay` | Return `0.0f` |
| All `*NeededSamplesize` | Return `0` (no buffer size change) |

`StreamIsModeSupported` returns `AE_DSP_ERROR_NO_ERROR` only for
`AE_DSP_MODE_TYPE_MASTER_PROCESS`; all other mode types get
`AE_DSP_ERROR_IGNORE_ME`.
