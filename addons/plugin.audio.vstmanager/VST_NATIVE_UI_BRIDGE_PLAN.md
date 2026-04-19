# VST Native UI Bridge — Design & Implementation Plan

## Overview

This document describes the architecture for bridging native VST plugin editor
windows (HWND-based) from the C++ `audiodsp.vsthost` ADSP addon to the Python
`plugin.audio.vstmanager` UI addon.

**Problem:** Python cannot directly create or manage Win32 HWND windows, yet
VST2 and VST3 plugins require a parent HWND to render their native editor GUIs.

**Solution:** A named-pipe IPC bridge. The C++ addon hosts a background pipe
server that listens for commands from the Python addon. When the Python addon
wants to show a VST editor, it sends a command over the named pipe. The C++
side creates a top-level Win32 window, embeds the VST editor inside it, and
runs a message pump. The window includes a close button (the standard Win32
title-bar "X" button). When the window is closed (by the user clicking X),
the C++ side tears down the editor and notifies Python.

---

## Architecture

```
┌─────────────────────────┐         Named Pipe          ┌──────────────────────────┐
│  Python: VST Manager    │  ──── JSON commands ──────>  │  C++: audiodsp.vsthost   │
│  (plugin.audio.         │                              │  EditorBridge            │
│   vstmanager)           │  <──── JSON responses ─────  │  ├─ pipe server thread   │
│                         │                              │  ├─ Win32 editor window  │
│  editor_bridge.py       │                              │  └─ message pump         │
│  ├─ open_editor(path)   │                              │                          │
│  ├─ close_editor(path)  │                              │  IVSTPlugin              │
│  └─ is_editor_open()    │                              │  ├─ hasEditor()          │
│                         │                              │  ├─ openEditor(hwnd)     │
│  default.py             │                              │  ├─ closeEditor()        │
│  └─ show_vst_ui()       │                              │  ├─ getEditorSize()      │
│      now launches       │                              │  └─ idleEditor()         │
│      native editor      │                              │                          │
└─────────────────────────┘                              └──────────────────────────┘
```

---

## Named Pipe Protocol

**Pipe name:** `\\.\pipe\kodi_vsthost_editor`

**Message format:** Newline-delimited JSON, one message per line.

### Commands (Python → C++)

```json
{"cmd": "open", "path": "C:\\VST\\MyPlugin.dll"}
{"cmd": "close", "path": "C:\\VST\\MyPlugin.dll"}
{"cmd": "close_all"}
{"cmd": "ping"}
```

### Responses (C++ → Python)

```json
{"status": "ok", "cmd": "open", "path": "C:\\VST\\MyPlugin.dll", "hasEditor": true}
{"status": "ok", "cmd": "close", "path": "C:\\VST\\MyPlugin.dll"}
{"status": "error", "cmd": "open", "path": "C:\\VST\\MyPlugin.dll", "error": "Plugin not in chain"}
{"status": "ok", "cmd": "ping"}
{"status": "ok", "cmd": "closed_by_user", "path": "C:\\VST\\MyPlugin.dll"}
```

---

## C++ Changes

### 1. IVSTPlugin — Editor Interface Extensions

Add virtual methods to the abstract base class:

```cpp
virtual bool hasEditor() const = 0;
virtual bool openEditor(void* parentWindow) = 0;
virtual void closeEditor() = 0;
virtual bool getEditorSize(int& width, int& height) const = 0;
virtual void idleEditor() = 0;
```

### 2. VSTPlugin2 — VST2 Editor Implementation

- `hasEditor()` → checks `m_effect->flags & effFlagsHasEditor`
- `openEditor(hwnd)` → `dispatcher(effEditOpen, 0, 0, hwnd, 0)`
- `closeEditor()` → `dispatcher(effEditClose, 0, 0, nullptr, 0)`
- `getEditorSize()` → `dispatcher(effEditGetRect, ...)` reads `ERect`
- `idleEditor()` → `dispatcher(effEditIdle, 0, 0, nullptr, 0)`

Requires adding `ERect` struct to `aeffectx.h`.

### 3. VSTPlugin3 — VST3 Editor Implementation

- Uses `IEditController::createView("editor")` to get `IPlugView`
- `openEditor()` → `IPlugView::attached(hwnd, "HWND")`
- `closeEditor()` → `IPlugView::removed()`
- `getEditorSize()` → `IPlugView::getSize()`
- `idleEditor()` → no-op (VST3 editors are self-pumping via COM)

### 4. EditorBridge — New Class

**File:** `src/bridge/EditorBridge.h`, `src/bridge/EditorBridge.cpp`

Responsibilities:
- Runs a named pipe server on a background thread
- Parses JSON commands, looks up plugins in DSPChain
- Creates a top-level Win32 window with WS_OVERLAPPEDWINDOW style (includes X button)
- Embeds the VST editor as a child HWND
- Runs `SetTimer()` for 30ms editor idle calls
- On WM_CLOSE, calls `closeEditor()`, destroys window, sends notification to Python
- Thread-safe: commands processed on the window thread via `PostMessage`

### 5. ERect Struct

Added to `aeffectx.h`:

```c
struct ERect {
    short top;
    short left;
    short bottom;
    short right;
};
```

---

## Python Changes

### 1. editor_bridge.py — Named Pipe Client

**File:** `resources/lib/editor_bridge.py`

- `open_editor(path)` → connects to named pipe, sends `{"cmd":"open","path":"..."}`
- `close_editor(path)` → sends `{"cmd":"close","path":"..."}`
- Timeout and error handling for when the C++ addon is not running

### 2. default.py — Updated show_vst_ui()

When user clicks a `-` prefixed plugin:
1. Try to open native editor via `editor_bridge.open_editor(path)`
2. If the plugin has no editor (`hasEditor: false`), fall back to the metadata dialog
3. If the pipe is unreachable (C++ addon not loaded), fall back to metadata dialog

---

## Editor Window Lifecycle

```
1. Python sends: {"cmd": "open", "path": "..."}
2. C++ finds plugin in DSPChain by path
3. C++ checks plugin.hasEditor()
4. If yes:
   a. Creates HWND with RegisterClassEx + CreateWindowExW
   b. Calls plugin.getEditorSize() → sizes the window
   c. Calls plugin.openEditor(hwnd) → VST renders inside
   d. ShowWindow(SW_SHOW) — window appears with title bar + X button
   e. SetTimer(30ms) → periodic idleEditor() calls
   f. Responds: {"status":"ok","cmd":"open","hasEditor":true}
5. User interacts with VST editor natively
6. User clicks X button:
   a. WM_CLOSE handler calls plugin.closeEditor()
   b. DestroyWindow()
   c. Sends: {"status":"ok","cmd":"closed_by_user","path":"..."}
7. OR Python sends: {"cmd": "close", "path": "..."}
   a. Same teardown as step 6
   b. Responds: {"status":"ok","cmd":"close"}
```

---

## Threading Model

- **Pipe server thread:** Reads commands, posts them to the window thread
- **Window/UI thread:** Creates/destroys windows, runs Win32 message pump,
  calls VST editor methods (openEditor/closeEditor/idle are all UI-thread calls)
- **Audio thread:** Unaffected — never touches editor methods

The window thread is created by `EditorBridge::start()` and joins in
`EditorBridge::stop()`. All VST editor calls happen on this thread to satisfy
VST2/VST3 threading requirements (editor operations must occur on the same
thread that created the parent window).

---

## Error Handling

| Scenario | Behavior |
|----------|----------|
| C++ addon not loaded | Python falls back to metadata dialog |
| Pipe connection refused | Python falls back to metadata dialog |
| Plugin not in chain | C++ responds with error; Python shows notification |
| Plugin has no editor | C++ responds `hasEditor: false`; Python shows metadata dialog |
| Plugin editor crashes | SEH catches crash; window destroyed; error response sent |
| User closes window | Normal teardown; notification sent to Python |

---

## File Summary

### New C++ Files
| File | Purpose |
|------|---------|
| `src/bridge/EditorBridge.h` | Named pipe server + Win32 editor window manager |
| `src/bridge/EditorBridge.cpp` | Implementation |

### Modified C++ Files
| File | Changes |
|------|---------|
| `src/plugin/IVSTPlugin.h` | Add `hasEditor()`, `openEditor()`, `closeEditor()`, `getEditorSize()`, `idleEditor()` |
| `src/vst2/VSTPlugin2.h` | Declare editor method overrides |
| `src/vst2/VSTPlugin2.cpp` | Implement VST2 editor via dispatcher opcodes |
| `src/vst3/VSTPlugin3.h` | Declare editor method overrides, add `IPlugView` member |
| `src/vst3/VSTPlugin3.cpp` | Implement VST3 editor via `IPlugView` |
| `src/vst2/vestige/aeffectx.h` | Add `ERect` struct |
| `src/addon_main.cpp` | Start/stop `EditorBridge` in `ADDON_Create`/`ADDON_Destroy` |
| `CMakeLists.txt` | Add bridge source files |

### New Python Files
| File | Purpose |
|------|---------|
| `resources/lib/editor_bridge.py` | Named pipe client for opening/closing native VST editors |

### Modified Python Files
| File | Changes |
|------|---------|
| `default.py` | `show_vst_ui()` calls `editor_bridge.open_editor()` with fallback to metadata dialog |
