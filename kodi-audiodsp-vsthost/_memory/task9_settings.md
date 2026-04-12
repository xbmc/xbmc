---
name: Task 9 — PluginSettings + VSTPluginManager
description: Settings persistence, scanner invocation, plugin cache format, addon.xml extension point, rescan workflow
type: project
---

## Files created

| File | Description |
|------|-------------|
| `src/settings/VSTPluginManager.h`   | Singleton managing out-of-process VST discovery |
| `src/settings/VSTPluginManager.cpp` | scan(), loadCache(), saveCache(), parseJsonLine() |
| `src/settings/PluginSettings.h`     | User preference persistence (search paths, scanner path, active config) |
| `src/settings/PluginSettings.cpp`   | loadSettings(), saveSettings(), rescan(), applyChainConfig() |
| `resources/addon.xml`               | Kodi addon manifest — xbmc.audiodsp extension point |

---

## VSTPluginManager — scanner invocation pattern

`VSTPluginManager::scan()` spawns `vstscanner.exe` using `CreateProcessW` with
an anonymous stdout pipe (`CreatePipe` + `STARTF_USESTDHANDLES`).

```
"vstscanner.exe" ["path1"] ["path2"] ...
```

- Each search directory is passed as a separate quoted argument.
- An empty `searchPaths` vector → no extra arguments → scanner uses its own
  built-in default Windows VST directories (see task6_scanner.md).
- `CREATE_NO_WINDOW` prevents a console window flashing during scans.
- The write end of the pipe is closed in the parent immediately after
  `CreateProcessW` succeeds, so `ReadFile` returns EOF when the child exits.
- Output is read in 4 KiB chunks; lines are extracted via `buffer.find('\n')`.

---

## Plugin cache JSON format (`plugin_cache.json`)

Written by `VSTPluginManager::saveCache()`, read back by `loadCache()`.

```json
{
  "plugins": [
    {"path":"C:\\VST\\MyEQ.dll","format":"vst2","name":"MyEQ","vendor":"Acme",
     "numParams":8,"numInputs":2,"numOutputs":2,"hasChunk":false,"error":null},
    {"path":"C:\\VST3\\Delay.vst3","format":"vst3","name":"StereoDelay","vendor":"Widgets",
     "numParams":0,"numInputs":0,"numOutputs":0,"hasChunk":false,"error":null},
    {"path":"C:\\VST\\bad.dll","format":"unknown","name":"","vendor":"",
     "numParams":0,"numInputs":0,"numOutputs":0,"hasChunk":false,"error":"load failed (error 126)"}
  ]
}
```

### Field definitions

| Field | Type | Notes |
|-------|------|-------|
| `path` | string | Absolute path scanned |
| `format` | string | `"vst2"`, `"vst3"`, or `"unknown"` |
| `name` | string | Plugin display name |
| `vendor` | string | Vendor/manufacturer string |
| `numParams` | int | 0 for VST3 (requires full instantiation) |
| `numInputs` | int | 0 for VST3 |
| `numOutputs` | int | 0 for VST3 |
| `hasChunk` | bool | VST2 chunk flag; always false for VST3 |
| `error` | string\|null | `null` on success; message on failure |

- Entries with `"error": null` are stored in `VSTPluginManager::m_plugins`.
- Entries with a non-null error string are stored in `m_failed`.
- `loadCache()` repopulates both lists from the cache file.

---

## settings.json format

Stored at `{addonDataPath}/settings.json`.  Written by
`PluginSettings::saveSettings()`, read by `loadSettings()`.

```json
{
  "scannerPath": "C:\\path\\to\\addon\\vstscanner.exe",
  "searchPaths": [
    "C:\\Program Files\\VSTPlugins",
    "C:\\Program Files\\Common Files\\VST3"
  ],
  "activeConfig": "default"
}
```

| Key | Type | Default | Notes |
|-----|------|---------|-------|
| `scannerPath` | string | `{dataPath}/vstscanner.exe` | Overridden via `setScannerPath()` |
| `searchPaths` | array of strings | `[]` | Passed as arguments to scanner; empty → scanner uses built-in defaults |
| `activeConfig` | string | `"default"` | Base name of the chain config JSON file to apply on stream start |

All string values use standard JSON escape sequences (`\\`, `\"`, etc.).

---

## addon.xml extension point

```xml
<extension point="xbmc.audiodsp"
           library_win="audiodsp.vsthost.dll">
  <support>
    <modes>
      <mode id="1" type="masterprocess" label="VST Chain" ...>
        <settings>
          <setting id="scan_paths" label="VST Plugin Folders" type="string" default=""/>
        </settings>
      </mode>
    </modes>
  </support>
</extension>
```

- `type="masterprocess"` places the addon in Kodi's master DSP stage.
- `library_win` names the DLL without path (Kodi resolves it from the addon directory).
- The `xbmc.audiodsp` import version `0.1.8` matches `KODI_AE_DSP_API_VERSION` in
  `kodi_adsp_types.h`.
- The `xbmc.addon.metadata` extension targets `platform=windows` because
  `VSTPluginManager` uses Win32 APIs (`CreateProcessW`, `CreatePipe`).

---

## Rescan workflow

```
PluginSettings::rescan(progressCallback)
  └─ VSTPluginManager::instance().scan(m_scannerPath, m_searchPaths, progressCallback)
       ├─ CreateProcessW("vstscanner.exe" [paths...])
       ├─ ReadFile loop — parse NDJSON lines via parseJsonLine()
       │    ├─ error empty → m_plugins.push_back(info)
       │    └─ error present → m_failed.push_back(info)
       └─ WaitForSingleObject / CloseHandle cleanup
  └─ VSTPluginManager::instance().saveCache(dataPath + "/plugin_cache.json")
```

On addon startup, if `plugin_cache.json` exists, `loadCache()` can be called
instead of `rescan()` to restore the previous scan results without spawning
the scanner process.

---

## applyChainConfig() behaviour

- Config name `"default"` → calls `DSPProcessor::loadConfig()` which reads
  `{configPath}/chain.json` (standard DSPProcessor convention).
- Any other name → reads `{dataPath}/{configName}.json` directly and calls
  `DSPChain::deserializeFromJson()`.

The chain JSON format is documented in task7_dspchain.md (version, sampleRate,
numChannels, plugins array with path/format/bypassed/state fields).

---

## Thread safety

| Method | Thread |
|--------|--------|
| `VSTPluginManager::scan()` | Settings thread only — spawns child process |
| `VSTPluginManager::loadCache() / saveCache()` | Settings thread only — file I/O |
| `VSTPluginManager::getPlugins() / getFailedPlugins()` | Read-only after scan; any thread |
| `PluginSettings::loadSettings() / saveSettings()` | Settings thread only |
| `PluginSettings::rescan()` | Settings thread only |
| `PluginSettings::applyChainConfig()` | Settings thread (audio stopped) |
