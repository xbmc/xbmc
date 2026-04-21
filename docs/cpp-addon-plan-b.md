# Plan B: First-class Precompiled Binary C/C++ Addon Support

## Overview

This plan covers adding first-class support for precompiled C/C++ binary addons as a
plugin-source type, distinct from Python. The approach reuses the mature `CAddonDll`
infrastructure already used by all other native addon types (PVR, VFS, game, audio
encoder/decoder) rather than requiring a C/C++ compiler on the target device.

This is the **recommended approach** because it:

- Requires far fewer changes than a compile-at-runtime approach.
- Reuses the battle-tested `CAddonDll` + `BinaryAddonManager` loading mechanism.
- Is consistent with how PVR clients, VFS addons, game clients, audio
  encoders/decoders, and inputstream addons already work.
- Does not require a compiler toolchain on the target device.
- Provides a stable ABI via the existing C function-pointer table (`AddonGlobalInterface`).

---

## Files to Change

| File | Change |
|---|---|
| `xbmc/addons/AddonInfo.cpp` | Add new type mappings for `kodi.cpp.*` extension points |
| `xbmc/addons/IAddon.h` | Add new `TYPE` enum values |
| `xbmc/addons/AddonManager.cpp` | Register factory for new type → `CAddonDll` |
| `xbmc/addons/PluginSource.cpp` | Route native plugin-source addons via `CAddonDll` instead of `CScriptInvocationManager` |
| `xbmc/addons/kodi-addon-dev-kit/include/kodi/kodi_cpp_plugin_dll.h` | **New** — C/C++ plugin entry-point contract |

---

## Detailed Steps

### 1. New extension points in `AddonInfo.cpp`

Add two entries to the `types[]` table in `xbmc/addons/AddonInfo.cpp`:

```cpp
{"kodi.cpp.pluginsource", ADDON_CPP_PLUGIN, 24005, ""},
{"kodi.cpp.script",       ADDON_CPP_SCRIPT,  24009, ""},
```

These mirror the existing `xbmc.python.pluginsource` / `xbmc.python.script` entries
but signal a precompiled binary rather than a Python script.

### 2. New `TYPE` enum values in `IAddon.h`

In the `ADDON::TYPE` enum add:

```cpp
ADDON_CPP_PLUGIN,   ///< kodi.cpp.pluginsource — precompiled C/C++ plugin
ADDON_CPP_SCRIPT,   ///< kodi.cpp.script        — precompiled C/C++ script/executable
```

### 3. Factory registration in `AddonManager.cpp`

In `CAddonMgr::AddonFromProps()` (or the equivalent type→factory dispatch), add cases
for `ADDON_CPP_PLUGIN` and `ADDON_CPP_SCRIPT` that construct a `CAddonDll` (or a thin
`CCppPluginSource` subclass of `CAddonDll`), exactly as `ADDON_PVRDLL`,
`ADDON_AUDIODECODER`, etc. do today.

### 4. Plugin-source routing in `PluginSource.cpp`

`CPluginSource` currently sends all plugin invocations through
`CScriptInvocationManager` (which only knows about `.py` files). Add a branch:

```cpp
if (Type() == ADDON_CPP_PLUGIN || Type() == ADDON_CPP_SCRIPT)
{
    // Load as a binary DLL and call the exported entry point
    auto dll = std::static_pointer_cast<CAddonDll>(shared_from_this());
    dll->Create(ADDON_CPP_PLUGIN, &funcTable, &info);
}
else
{
    // Existing Python path via CScriptInvocationManager
    ...
}
```

### 5. New SDK header: `kodi_cpp_plugin_dll.h`

Create `xbmc/addons/kodi-addon-dev-kit/include/kodi/kodi_cpp_plugin_dll.h` defining
the required entry points that every C/C++ plugin DLL must export:

```c
#pragma once
#include "kodi/AddonBase.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Called by Kodi to run the plugin.
 *
 * Equivalent to a Python plugin's default.py execution.
 * argv[0] = plugin base URL  (e.g. "plugin://myaddon/")
 * argv[1] = addon handle     (decimal string)
 * argv[2] = query string     (e.g. "?action=list")
 *
 * @param argc Argument count (always 3 for plugin-source invocations)
 * @param argv Argument vector
 * @return 0 on success, non-zero on failure
 */
ADDONCREATOR(int) kodi_cpp_plugin_main(int argc, char** argv);

#ifdef __cplusplus
}
#endif
```

Addons use the existing `AddonGlobalInterface` callback table (exposed via
`AddonBase.h`) to call back into Kodi for logging, settings, filesystem access,
list-item creation, etc.

### 6. Example `addon.xml` for a C/C++ plugin

```xml
<?xml version="1.0" encoding="UTF-8"?>
<addon id="plugin.example.cpp"
       name="Example C++ Plugin"
       version="1.0.0"
       provider-name="YourName">
  <requires>
    <import addon="xbmc.core" version="0.0.1"/>
  </requires>
  <extension point="kodi.cpp.pluginsource" provides="video">
    <!-- no <provides> children needed; binary is self-contained -->
  </extension>
  <extension point="xbmc.addon.metadata">
    <summary lang="en_GB">An example C++ plugin</summary>
    <description lang="en_GB">Demonstrates the kodi.cpp.pluginsource extension point.</description>
    <platform>all</platform>
  </extension>
</addon>
```

The addon ships a platform-specific shared library (`plugin.example.cpp.so` on Linux,
`plugin.example.cpp.dll` on Windows, `plugin.example.cpp.dylib` on macOS) alongside
`addon.xml`. Kodi locates the library using the same `DllAddon`/`CAddonDll::LibPath()`
logic used for all other binary addons.

---

## Why Not Approach A (Compile-at-Runtime)?

Approach A — shipping `.c`/`.cpp` sources and compiling them on the fly — was
considered but rejected for this plan because:

1. **No compiler on the target device.** Embedded targets (Raspberry Pi, Android, etc.)
   typically do not have `gcc`/`clang` available at runtime.
2. **Security surface.** Arbitrary source compilation introduces significant sandboxing
   and code-injection concerns.
3. **Complexity.** It requires a new `ILanguageInvocationHandler` + `ILanguageInvoker`
   implementation, CMake toolchain detection, and management of a compilation
   subprocess — far more code than Approach B.
4. **Already covered.** The `CAddonDll` mechanism already does everything Approach A
   would produce (a loaded shared library with an entry point), but without the runtime
   compilation step.
