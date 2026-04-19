# VST Manager Addon — Design Plan

## Overview

**Addon Name:** VST Manager
**Addon ID:** `plugin.audio.vstmanager`
**Type:** Python plugin source (`xbmc.python.pluginsource`)
**Target:** Kodi 18.x Leia (Python 2.7 compatible)
**License:** GPL-2.0-or-later

The VST Manager is a Kodi UI addon that provides a user-friendly interface for managing VST audio plugins in the Kodi audio processing chain. It works alongside the existing C++ `audiodsp.vsthost` ADSP addon, which handles the actual VST plugin loading, audio processing, and real-time chain execution.

---

## Architecture

### Communication Model

The Python UI addon communicates with the C++ ADSP addon through **shared JSON configuration files** stored in the `audiodsp.vsthost` addon data directory (`special://profile/addon_data/audiodsp.vsthost/`):

| File | Purpose | Read/Write |
|------|---------|------------|
| `chain.json` | Active plugin chain configuration | Read + Write |
| `plugin_cache.json` | Cached VST scan results (from `vstscanner.exe`) | Read only |
| `settings.json` | Search paths, scanner location | Read + Write |

### Data Flow

```
User → VST Manager (Python) → chain.json → audiodsp.vsthost (C++) → Audio Chain
                             ↑
                    plugin_cache.json ← vstscanner.exe
```

---

## File Structure

```
addons/plugin.audio.vstmanager/
├── addon.xml                      # Addon descriptor
├── default.py                     # Main entry point (plugin source)
├── VST_MANAGER_PLAN.md            # This plan document
└── resources/
    ├── settings.xml               # Settings page (VST directory)
    └── lib/
        ├── __init__.py            # Package init
        ├── chain_manager.py       # Chain JSON read/write operations
        └── plugin_scanner.py      # VST plugin discovery and cache reading
```

---

## Settings Page

The addon settings page allows users to specify the directory containing VST plugins. Two input methods are provided:

1. **Folder browser** (`type="folder"`) — Opens Kodi's native directory selector
2. **Text input** (`type="text"`) — Allows typing/pasting a path directly

The text input takes precedence over the folder browser when both are configured. This accommodates different user preferences and input devices (remote, keyboard, touch).

---

## Main Page — VST Listing

When the user opens the addon, a directory listing displays all discovered VST plugins with status indicators:

### Display Format
- **`+ Plugin Name`** — Available to add (not in chain)
- **`- Plugin Name`** — Already active in the audio chain

### Plugin Discovery Priority
1. **Plugin cache** (`plugin_cache.json`) — Rich metadata (name, vendor, format, parameter count) from the out-of-process scanner
2. **Directory scan** — Fallback for uncached plugins; enumerates `.dll` (VST2) and `.vst3` (VST3) files

### Interactions

| State | Single Click | Right-Click / Long-Press |
|-------|-------------|--------------------------|
| `+ Available` | Confirmation dialog → Add to chain → Show VST UI | N/A |
| `- In Chain` | Show VST UI | Context menu → "Remove from audio chain" → Confirmation dialog → Remove |

---

## VST UI Dialog

When a user views a VST plugin's UI (via click on a chain plugin, or after adding one), a dialog displays:

- **Plugin name** (heading)
- **Vendor** name
- **Format** (VST2/VST3) and **parameter count**
- **Status** (Active in audio chain)

> **Note:** Native VST editor windows (HWND-based on Windows) cannot be launched from Python. The dialog shows plugin metadata from the scan cache. Full native editor integration would require extending the C++ ADSP addon with a notification bridge. The parameter count is shown to indicate the plugin's complexity.

---

## Error Handling

### Failed Plugin Loading
Any VST plugin that fails to load or causes issues is **quietly removed** from the audio chain:
- The `add_plugin()` method wraps chain modification in try/except
- On failure, the plugin entry is removed from `chain.json`
- A brief notification is shown (non-intrusive)

### Missing Files
- Plugins that no longer exist on disk are automatically cleaned from the chain during listing
- Missing `chain.json` or `plugin_cache.json` files are handled gracefully with empty defaults

### Missing VST Directory
- If no VST directory is configured, the user is prompted to set one in settings

---

## JSON File Formats

### chain.json
```json
{
  "version": 1,
  "sampleRate": 44100.0,
  "numChannels": 2,
  "plugins": [
    {
      "path": "C:\\VST\\MyPlugin.dll",
      "format": "vst2",
      "bypassed": false,
      "state": ""
    }
  ]
}
```

### plugin_cache.json
```json
{
  "plugins": [
    {
      "path": "C:\\VST\\MyPlugin.dll",
      "format": "vst2",
      "name": "My Plugin",
      "vendor": "Plugin Corp",
      "numParams": 12,
      "numInputs": 2,
      "numOutputs": 2,
      "hasChunk": true,
      "error": null
    }
  ]
}
```

---

## Design Decisions

1. **File-based IPC** — Using JSON files for communication between the Python UI and C++ ADSP addon avoids complex IPC mechanisms and works reliably across Kodi restarts.

2. **Plugin source type** — Chosen over `xbmc.service` or `xbmc.script` because it provides native directory listing support with context menus, sorting, and Kodi navigation integration.

3. **Dual directory input** — Both folder browser and text input are offered because Kodi's folder browser (`type="folder"`) doesn't allow direct text editing, while `type="text"` doesn't offer browsing.

4. **Python 2.7 compatibility** — Kodi 18.x Leia uses Python 2.7 as its default interpreter. All code uses `from __future__ import unicode_literals` and compatible imports.

5. **Quiet failure removal** — VST plugins that fail are silently removed to prevent chain corruption and avoid user confusion with error cascades.

---

## Limitations

- **No native VST editor windows** — Python cannot launch HWND-based VST editor UIs. The dialog shows metadata only.
- **No real-time parameter control** — Parameter adjustment requires a C++ bridge that exports parameter state to a shared file or socket.
- **Scanner dependency** — Rich metadata requires `vstscanner.exe` to have been run by the C++ ADSP addon. Without it, only basic file information is available.
- **Single directory** — The settings page supports one VST directory. Multiple directories can be configured through the `settings.json` file directly.
