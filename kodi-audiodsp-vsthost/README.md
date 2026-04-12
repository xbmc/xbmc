# audiodsp.vsthost

A Kodi Audio DSP addon that lets you load and chain **VST2** and **VST3** audio effect plugins into Kodi's master audio processing stage. Audio passes through your configured plugin chain in real time during playback.

## Features

- Host VST2 (`.dll`) and VST3 (`.vst3`) effect plugins
- Chain multiple plugins in a user-defined order
- Per-plugin bypass control
- Plugin state/preset persistence (saved as base64 in the chain config)
- Out-of-process plugin scanning via `vstscanner.exe` (crashes in a bad plugin won't take down Kodi)
- Latency reporting for A/V sync compensation

## Building

Requires CMake 3.16+ and a C++17 compiler (MSVC on Windows).

```bash
cmake -S . -B build -DKODI_ADSP_SDK_DIR="C:/src/xbmc/xbmc/addons/kodi-addon-dev-kit/include/kodi"
cmake --build build --config Release
```

The VST3 SDK is fetched automatically via FetchContent, or you can place it in `deps/vst3sdk/`.

## Configuration

All configuration lives in the addon's **data directory**:

```
%APPDATA%\Kodi\userdata\addon_data\audiodsp.vsthost\
```

There are two files you need to set up:

### 1. `settings.json` -- where your VSTs live

Create `settings.json` in the addon data directory with the following format:

```json
{
  "scannerPath": "C:\\Kodi\\addons\\audiodsp.vsthost\\vstscanner.exe",
  "searchPaths": [
    "C:\\Program Files\\VSTPlugins",
    "C:\\Program Files\\Common Files\\VST3"
  ],
  "activeConfig": "default"
}
```

| Field | Description |
|---|---|
| `scannerPath` | Absolute path to `vstscanner.exe`. Defaults to the addon data directory if omitted. |
| `searchPaths` | Array of directories to scan for VST2 `.dll` and VST3 `.vst3` files. |
| `activeConfig` | Name of the chain config to load (see below). Defaults to `"default"`. |

### 2. `chain.json` -- your VST processing chain

This file defines which plugins are loaded, in what order, whether each is bypassed, and their saved state. It is read automatically when Kodi starts an audio stream.

The file is produced by `DSPChain::serializeToJson()` and consumed by `deserializeFromJson()`. The structure looks like:

```json
{
  "plugins": [
    {
      "path": "C:\\VSTPlugins\\MyEQ.dll",
      "format": "vst2",
      "bypassed": false,
      "state": "<base64-encoded plugin state>"
    },
    {
      "path": "C:\\Program Files\\Common Files\\VST3\\Compressor.vst3",
      "format": "vst3",
      "bypassed": false,
      "state": "<base64-encoded plugin state>"
    }
  ]
}
```

| Field | Description |
|---|---|
| `path` | Absolute path to the VST plugin file on disk. |
| `format` | `"vst2"` or `"vst3"`. |
| `bypassed` | `true` to skip this plugin during processing. |
| `state` | Base64-encoded plugin state (presets/parameters). Optional -- omit to use the plugin's defaults. |

Plugins are processed in array order: audio flows through the first plugin, then the second, and so on.

If `activeConfig` in `settings.json` is set to something other than `"default"`, the addon looks for `{activeConfig}.json` instead of `chain.json`.

### Scanning for plugins

The addon uses `vstscanner.exe` to discover available plugins out-of-process. To trigger a scan, call `PluginSettings::rescan()` from code. The scanner walks every directory in `searchPaths`, probes each plugin, and writes the results to `plugin_cache.json` in the addon data directory. This cache is used by the settings UI to present available plugins.

## How it works at runtime

1. Kodi starts an audio stream and calls `StreamCreate`
2. A `DSPProcessor` is created and reads `chain.json` from the addon data directory
3. Each VST plugin in the chain is loaded and initialized with the stream's sample rate, channel count, and block size
4. On every audio block, `MasterProcess` passes audio through the chain sequentially using ping-pong buffers
5. When playback stops, `StreamDestroy` unloads all plugins

## Current limitations

- **No runtime UI.** There is currently no in-Kodi interface for browsing, adding, removing, or reordering VST plugins. All configuration is done by editing the JSON files described above.
- **Changes require a stream restart.** If you edit `chain.json` while Kodi is playing, the changes won't take effect until the next audio stream starts (stop and restart playback, or restart Kodi).
- **Windows only.** The addon currently builds and runs on Windows (`audiodsp.vsthost.dll`). Linux/macOS support would require platform-specific plugin loading in the VST2/VST3 implementations and the scanner.
- **No plugin GUI.** VST plugin editor windows are not displayed. Parameters can only be set programmatically or via saved state.

## License

GPL-2.0-or-later
