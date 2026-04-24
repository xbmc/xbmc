# VST Addon SEH & Resilience Update Plan — Issue 1

**Repository:** `OrganizerRo/xbmc`  
**Branch:** `copilot/check-vst-audio-routing-implementation`  
**Scope:** Windows-only (`#ifdef TARGET_WINDOWS`) — no changes to non-Windows code paths.

---

## Table of Contents

1. [Background & Current State](#background--current-state)
2. [Issue 1 — No Automatic Restart of a Crashed VST](#issue-1--no-automatic-restart-of-a-crashed-vst)
3. [Issue 2 — No SEH Around the VST Editor UI Thread](#issue-2--no-seh-around-the-vst-editor-ui-thread)
4. [File Change Index](#file-change-index)

---

## Background & Current State

### Audio path

```
CActiveAE::RunStages()                        [ActiveAE.cpp:2238-2245]
  MixSounds()
  Deamplify()
  m_dsp.MasterProcess(out)                   ← VST chain call
    CActiveAEDSP::MasterProcess()            [ActiveAEDSP.cpp:275-342]
      __try { m_funcs->MasterProcess() }
      __except { m_dspFailed = true; }
```

### DSP lifecycle hooks

| Method | Called from | File:Line |
|--------|------------|-----------|
| `Init()` | `CActiveAE::Start()` | `ActiveAE.cpp:2630` |
| `Deinit()` | `CActiveAE::Dispose()` | `ActiveAE.cpp:299` |
| `OnConfigure()` | `CActiveAE::Configure()` end | `ActiveAE.cpp:1414` |
| `MasterProcess()` | `CActiveAE::RunStages()` | `ActiveAE.cpp:2245` |

### EditorBridge topology

```
EditorBridge (global in addon_main.cpp:22)
  ├── m_pipeThread  — pipeServerLoop()  [EditorBridge.cpp:105-147]
  │     reads JSON command, calls processCommand(), writes response
  └── m_uiThread    — uiThreadLoop()   [EditorBridge.cpp:271-340]
        Win32 GetMessageW() pump, handles WM_VSTBRIDGE_OPEN/CLOSE
        doOpenEditor() → effEditOpen, SetTimer(30ms idle)
        WM_TIMER      → effEditIdle
        WM_CLOSE      → effEditClose, DestroyWindow
```

---

## Issue 1 — No Automatic Restart of a Crashed VST

### Problem

When `CActiveAEDSP::MasterProcess()` catches an SEH exception, it sets:

```cpp
// ActiveAEDSP.cpp:298 (planar path)  and  :333 (interleaved path)
m_dspFailed = true;
```

Every subsequent call to `MasterProcess()` returns immediately at line 277:

```cpp
if (!m_streamActive || m_dspFailed.load() || !m_funcs->MasterProcess)
    return;
```

`OnConfigure()` also bails early at line 172:

```cpp
if (!m_initialized || m_dspFailed.load())
    return;
```

`IsActive()` returns `false` at line 350:

```cpp
return m_initialized && !m_dspFailed.load();
```

There is **no path to clear `m_dspFailed` without a full Kodi restart.**

### Is DSP Disable/Re-enable Possible From Within Kodi?

Yes — and it does not require a Kodi restart. The mechanism already exists in the AE control-protocol message pump. `CActiveAEControlProtocol` defines a `RECONFIGURE` signal (see `ActiveAE.h:74`) that causes `CActiveAE` to call `Configure()`, which already calls `m_dsp.OnConfigure()` at line 1414. We extend this by adding a `Reset()` path to `CActiveAEDSP` that fully tears down and reloads the DSP, then hooking that into the `RECONFIGURE` cycle.

### Detailed Plan

#### Step 1 — Add `Reset()` and crash-count tracking to `CActiveAEDSP`

**File:** `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAEDSP.h`

Add three new members inside the `#ifdef TARGET_WINDOWS` block (after line 97 where `m_dspFailed` is declared):

```
// Lines to add after ActiveAEDSP.h:97
std::atomic<bool> m_dspNeedsReset{false};   // set by render thread, cleared by worker thread
std::atomic<int>  m_recoveryAttempts{0};    // number of resets attempted this session
static constexpr int MAX_RECOVERY_ATTEMPTS = 3;
```

Add two new public methods in the header (after `IsActive()` declaration at line 83):

```
// Lines to add after ActiveAEDSP.h:83
/// Returns true if a crash was caught and recovery has not yet been attempted.
bool NeedsReset() const;

/// Tear down and reload the ADSP add-on, then re-initialize the stream.
/// Safe to call from the AE worker thread only (same thread as Init/Configure).
/// Returns false and leaves DSP disabled if max recovery attempts are exceeded.
bool TryReset(const AEAudioFormat& fmt);
```

**File:** `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAEDSP.cpp`

After line 298 (planar `m_dspFailed = true`) add:

```cpp
m_dspNeedsReset = true;
```

After line 333 (interleaved `m_dspFailed = true`) add:

```cpp
m_dspNeedsReset = true;
```

Implement `NeedsReset()` (add after `IsActive()` at ~line 351):

```cpp
bool CActiveAEDSP::NeedsReset() const
{
    return m_dspNeedsReset.load();
}
```

Implement `TryReset()` (add after `NeedsReset()`):

```cpp
bool CActiveAEDSP::TryReset(const AEAudioFormat& fmt)
{
    if (!m_dspFailed.load())
        return true;  // nothing to recover

    if (m_recoveryAttempts.load() >= MAX_RECOVERY_ATTEMPTS)
    {
        CLog::Log(LOGWARNING,
            "CActiveAEDSP::TryReset — max recovery attempts (%d) reached; DSP remains disabled",
            MAX_RECOVERY_ATTEMPTS);
        m_dspNeedsReset = false;
        return false;
    }

    const int attempt = m_recoveryAttempts.fetch_add(1) + 1;
    CLog::Log(LOGINFO, "CActiveAEDSP::TryReset — recovery attempt %d of %d",
              attempt, MAX_RECOVERY_ATTEMPTS);

    // Full teardown
    Deinit();

    // Clear failure flag so Init() and OnConfigure() are not gated
    m_dspFailed   = false;
    m_dspNeedsReset = false;

    // Reload the DLL and restart the addon
    if (!Init())
    {
        CLog::Log(LOGERROR, "CActiveAEDSP::TryReset — Init() failed on attempt %d", attempt);
        m_dspFailed = true;
        return false;
    }

    // Restore the last known stream format
    OnConfigure(fmt);

    CLog::Log(LOGINFO, "CActiveAEDSP::TryReset — DSP restored on attempt %d", attempt);
    return true;
}
```

**Important:** `Deinit()` calls `FreeLibrary()` on the crashed DLL. If the DLL is in a corrupted state, `FreeLibrary()` itself could fault. Wrap the `Deinit()` call inside `TryReset()` in its own `__try/__except`:

```cpp
__try
{
    Deinit();
}
__except(EXCEPTION_EXECUTE_HANDLER)
{
    CLog::Log(LOGERROR, "CActiveAEDSP::TryReset — Deinit() faulted; forcibly clearing state");
    // Manually null out members without calling DLL functions
    m_dll          = nullptr;
    m_streamActive = false;
    m_initialized  = false;
    std::memset(m_funcs, 0, sizeof(AudioDSP));
    m_scratch.clear();
    m_scratchPtrs.clear();
}
```

#### Step 2 — Call `TryReset()` from the AE worker thread

The render thread (`RunStages`) must **not** call `TryReset()` directly because `Init()` calls `LoadLibraryW()` which is far too slow for a real-time audio callback. The recovery must happen on the AE worker thread — the same thread that calls `Configure()`.

**File:** `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAE.cpp`

Locate the AE worker thread's main processing loop (it runs `RunStages()` in a cycle governed by the state machine). Find the section just **before** `RunStages()` is called or just **after** it returns. The call to `RunStages()` is inside the `AE_TOP_CONFIGURED_PLAY` state handler.

Search for the existing call at line 2245:
```cpp
m_dsp.MasterProcess(out);
```

In the outer AE worker thread loop (look for the `while (!m_bStop)` loop that drives the state machine), add a recovery check at the top of the iteration, before any state processing:

```cpp
// ActiveAE.cpp — add after the existing state machine dispatch, at the point
// where the worker thread is between buffer cycles and is safe to call Init().
// Exact insertion point: the idle/timeout branch of the AE_TOP_CONFIGURED_PLAY
// state, where the thread is waiting for messages and not touching audio buffers.

if (m_dsp.NeedsReset())
{
    CLog::Log(LOGINFO, "CActiveAE — DSP crash detected; attempting recovery");
    m_dsp.TryReset(m_internalFormat);
}
```

The safest placement is in the `AE_TOP_CONFIGURED_PLAY` → `TIMEOUT` signal handler, where the thread is idle and no audio buffers are being processed. Search `ActiveAE.cpp` for the `TIMEOUT` case near the `AE_TOP_CONFIGURED_PLAY` state label and insert the check there.

#### Step 3 — Preserve `m_streamFmt` across `Deinit()`

`TryReset()` passes `fmt` in as a parameter, which is sourced from `m_internalFormat` in `CActiveAE`. This is always valid because `m_internalFormat` is set during `Configure()` and is stable while the stream is running. No additional work is needed here.

#### Step 4 — Reset the crash counter on a clean stop

In `CActiveAEDSP::Deinit()` (line 145), reset the recovery counter so that the count is per-plugin-load, not per-session:

```cpp
// ActiveAEDSP.cpp — add at end of Deinit() before the closing brace (~line 163)
m_recoveryAttempts = 0;
m_dspNeedsReset    = false;
```

#### Step 5 — Pipe command to manually trigger DSP reset (optional)

The named pipe (`\\.\pipe\kodi_vsthost_editor`) already handles `open`/`close`/`ping` commands. To allow the Python VST Manager (or any external tool) to manually trigger a DSP reset without waiting for the automatic retry cycle, add a `reset_dsp` command.

**File:** `kodi-audiodsp-vsthost/src/bridge/EditorBridge.cpp`

In `processCommand()` (line 175), add a new branch after the `close_all` handler (~line 264):

```cpp
if (cmd == "reset_dsp")
{
    // The addon itself cannot call back into Kodi's AE layer.
    // All this command can do is clear the addon-side state so that
    // the next StreamCreate (triggered by Kodi's AE reconfigure) finds
    // a clean chain.  The actual DSP re-init is driven by CActiveAEDSP::TryReset().
    // This is useful for the Python UI to confirm the pipe is still alive
    // and to retrieve current recovery status.
    return "{\"status\":\"ok\",\"cmd\":\"reset_dsp\","
           "\"note\":\"DSP recovery is automatic; use ping to verify pipe health\"}";
}
```

> **Note:** The addon DLL has no callback into `CActiveAEDSP` so it cannot directly trigger `TryReset()`. The automatic mechanism in Step 2 is the primary recovery path. This command exists solely as a diagnostic/confirmation endpoint.

#### Step 6 — Thread-safety audit of the recovery sequence

| Action | Thread | Safe? | Rationale |
|--------|--------|-------|-----------|
| Set `m_dspFailed = true` | Render | ✅ | `std::atomic<bool>` |
| Set `m_dspNeedsReset = true` | Render | ✅ | `std::atomic<bool>` |
| Read `m_dsp.NeedsReset()` | AE worker | ✅ | `atomic::load()` |
| `TryReset()` → `Deinit()` → `FreeLibrary()` | AE worker | ✅ | Same thread as `Init()` |
| `TryReset()` → `Init()` → `LoadLibraryW()` | AE worker | ✅ | Same thread as original `Init()` |
| `MasterProcess()` reading `m_dspFailed` | Render | ✅ | `atomic::load()` |
| `TryReset()` writing `m_dspFailed = false` | AE worker | ⚠️ | Render thread could check it concurrently |

The ⚠️ case: the render thread reads `m_dspFailed` at line 277 while the worker thread clears it in `TryReset()`. Because both are `std::atomic<bool>` operations this is safe — no torn read is possible. The worst case is that one audio buffer is silently skipped (passthrough) during the window between the worker clearing the flag and the render thread next checking it.

#### Summary of changes for Issue 1

| File | Line range | Change |
|------|-----------|--------|
| `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAEDSP.h` | After line 97 | Add `m_dspNeedsReset`, `m_recoveryAttempts`, `MAX_RECOVERY_ATTEMPTS` |
| `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAEDSP.h` | After line 83 | Declare `NeedsReset()` and `TryReset()` |
| `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAEDSP.cpp` | Lines 298, 333 | Add `m_dspNeedsReset = true` after each `m_dspFailed = true` |
| `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAEDSP.cpp` | After line 351 | Implement `NeedsReset()` and `TryReset()` with SEH-guarded `Deinit()` |
| `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAEDSP.cpp` | End of `Deinit()` (~line 163) | Reset `m_recoveryAttempts` and `m_dspNeedsReset` |
| `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAE.cpp` | TIMEOUT branch of `AE_TOP_CONFIGURED_PLAY` | Add `if (m_dsp.NeedsReset()) m_dsp.TryReset(m_internalFormat);` |
| `kodi-audiodsp-vsthost/src/bridge/EditorBridge.cpp` | After line ~264 | Add `reset_dsp` pipe command (diagnostic only) |

---

## Issue 2 — No SEH Around the VST Editor UI Thread

### Problem

The entire GUI subsystem of the addon runs on a single Win32 message-pump thread (`m_uiThread`). Three VST dispatcher calls happen on this thread without any exception protection:

| Call site | File:Line | Dispatcher opcode |
|-----------|-----------|-------------------|
| `plugin->openEditor(hwnd)` | `EditorBridge.cpp:420` | `effEditOpen` |
| `p->idleEditor()` in `WM_TIMER` | `EditorBridge.cpp:542` | `effEditIdle` |
| `p->closeEditor()` in `WM_CLOSE` | `EditorBridge.cpp:573` | `effEditClose` |

If any of these calls causes an access violation or other SEH exception, the Win32 structured exception propagates up through the message pump (`GetMessageW`/`DispatchMessageW`), kills `uiThreadLoop()`, and leaves the thread dead. After that:

- `m_uiThreadID` holds a stale value (dead thread ID)
- `PostThreadMessageW(m_uiThreadID, WM_VSTBRIDGE_OPEN, ...)` silently fails (returns FALSE)
- The pipe command returns `{"status":"ok"}` but no window ever appears
- All tracking maps (`m_openEditors`, `m_hwndToPath`) are left in an inconsistent state
- The only recovery is a full Kodi restart

### Two-Layer Plan

#### Layer A — SEH at each VST UI dispatcher callsite (prerequisite for all approaches)

This is a low-risk, high-value change that is required regardless of whether we also implement Layer B. It prevents the UI thread from dying in the first place for most crash scenarios.

**File:** `kodi-audiodsp-vsthost/src/bridge/EditorBridge.cpp`

**Change 1 — `doOpenEditor()`, line 420 (`effEditOpen` call):**

Replace:
```cpp
if (!plugin->openEditor(static_cast<void*>(hwnd)))
```

With an SEH-guarded version. Because MSVC prohibits mixing C++ destructors and `__try` in the same function scope (C4509), extract the guarded VST call into a separate static helper:

```cpp
// New static helper to add near the top of EditorBridge.cpp (~line 15):
static bool SafeOpenEditor(IVSTPlugin* plugin, void* hwnd)
{
    bool result = false;
    __try {
        result = plugin->openEditor(hwnd);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        std::fprintf(stderr, "[EditorBridge] effEditOpen faulted\n");
    }
    return result;
}
```

Then in `doOpenEditor()` replace the call at line 420 with:
```cpp
if (!SafeOpenEditor(plugin, static_cast<void*>(hwnd)))
```

**Change 2 — `handleMessage()`, WM_TIMER handler (~line 541, `effEditIdle` call):**

Similarly extract into a helper:
```cpp
static void SafeIdleEditor(IVSTPlugin* plugin)
{
    __try {
        plugin->idleEditor();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        std::fprintf(stderr, "[EditorBridge] effEditIdle faulted\n");
    }
}
```

Replace the call at line 542 (`p->idleEditor()`) with `SafeIdleEditor(p)`.

If `effEditIdle` keeps faulting we still want to stop the timer for that window. After calling `SafeIdleEditor`, check an out-of-band flag or simply catch repeated failures by tracking a per-HWND idle-fail count (see Layer B section for a cleaner version).

**Change 3 — `handleMessage()`, WM_CLOSE handler (~line 573, `effEditClose` call):**

```cpp
static void SafeCloseEditor(IVSTPlugin* plugin)
{
    __try {
        plugin->closeEditor();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        std::fprintf(stderr, "[EditorBridge] effEditClose faulted\n");
    }
}
```

Replace the call at line 573 (`p->closeEditor()`) with `SafeCloseEditor(p)`.

**Change 4 — Wrap the entire `uiThreadLoop()` message pump in a top-level SEH guard:**

Even with per-call guards in place, an unexpected fault elsewhere in the UI thread (e.g., inside `TranslateMessage` or `DispatchMessageW` dispatching to a broken VST window) could still kill the thread. Add a recovery loop around the pump:

```cpp
// EditorBridge.cpp — inside uiThreadLoop(), after SetEvent(m_uiReadyEvent) (~line 289)
// Wrap the GetMessageW() loop:

constexpr int MAX_UI_RESTARTS = 5;
int uiRestartCount = 0;

restart_pump:
__try
{
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        if (msg.hwnd == nullptr)
        {
            // ... existing switch (msg.message) handler unchanged ...
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}
__except (EXCEPTION_EXECUTE_HANDLER)
{
    ++uiRestartCount;
    std::fprintf(stderr,
        "[EditorBridge] uiThreadLoop() SEH fault #%d; %s\n",
        uiRestartCount,
        (uiRestartCount < MAX_UI_RESTARTS) ? "restarting pump" : "giving up");

    // Clean up tracking state for any editors that were open
    {
        std::lock_guard<std::mutex> lock(m_editorMutex);
        m_openEditors.clear();
        m_hwndToPath.clear();
    }

    if (uiRestartCount < MAX_UI_RESTARTS && m_running.load())
        goto restart_pump;  // Re-enter the pump on the same thread
}
```

> **Note on `goto`:** Using `goto` to restart after an SEH handler is idiomatic in Windows SEH code. The alternative is a `while (m_running) { __try { pump(); } __except {...} }` wrapper function, which is the preferred C++ pattern but requires extracting the pump body into its own function (see `SafePumpLoop()` helper below).

A cleaner version without `goto`: extract the pump into a separate function `void EditorBridge::pumpLoop()` that contains **only** the `GetMessageW` loop (no C++ objects with destructors in scope), and call it from `uiThreadLoop()` inside a `__try/__except` retry loop:

```cpp
// New private method declaration in EditorBridge.h (~line 73):
void pumpLoop();  // contains GetMessageW loop, no C++ objects in scope

// Implementation in EditorBridge.cpp:
void EditorBridge::pumpLoop()
{
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        if (msg.hwnd == nullptr) { /* existing switch */ }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

// uiThreadLoop() after the readiness event (~line 289):
for (int restarts = 0; restarts < MAX_UI_RESTARTS && m_running.load(); ++restarts)
{
    __try { pumpLoop(); }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        std::fprintf(stderr, "[EditorBridge] UI thread pump crash #%d\n", restarts + 1);
        std::lock_guard<std::mutex> lock(m_editorMutex);
        m_openEditors.clear();
        m_hwndToPath.clear();
    }
}
```

After `pumpLoop()` exits normally (WM_QUIT received), the loop exits and `uiThreadLoop()` proceeds to cleanup.

#### Layer B — Per-plugin UI thread isolation (significant but contained change)

This eliminates the shared-thread failure mode: one misbehaving plugin's editor cannot kill the editors for all other plugins. Each plugin's editor runs its own Win32 message pump on its own thread.

**Assessment of scope:** This is a moderate-complexity change — roughly 120–180 lines of new code, touching `EditorBridge.h` and `EditorBridge.cpp` only. It does **not** touch `CActiveAEDSP`, `ActiveAE.cpp`, `addon_main.cpp`, or any Kodi-core files. It is self-contained within the addon.

##### Data model change

**File:** `kodi-audiodsp-vsthost/src/bridge/EditorBridge.h`

Replace the single-thread members (lines 103–120):
```cpp
// REMOVE these single-thread members:
std::thread         m_uiThread;
DWORD               m_uiThreadID = 0;
std::unordered_map<std::string, HWND> m_openEditors;
std::unordered_map<HWND, std::string> m_hwndToPath;
HANDLE m_uiReadyEvent = nullptr;
ATOM m_windowClass = 0;
```

Replace with a per-plugin record:
```cpp
struct PluginEditorThread
{
    std::thread     thread;
    DWORD           threadID  = 0;
    HWND            hwnd      = nullptr;
    std::atomic<bool> alive   = false;
    int             faultCount = 0;
    HANDLE          readyEvent = nullptr;
};

// Guards m_chain and m_editors
mutable std::mutex                                      m_editorMutex;
std::unordered_map<std::string, PluginEditorThread>     m_editors;
ATOM                                                    m_windowClass = 0;
```

##### Thread-per-plugin lifecycle

**`doOpenEditor()`** — instead of posting `WM_VSTBRIDGE_OPEN` to a shared thread, create a new per-plugin thread:

```cpp
void EditorBridge::doOpenEditor(const std::string& pluginPath)
{
    {
        std::lock_guard<std::mutex> lock(m_editorMutex);
        auto it = m_editors.find(pluginPath);
        if (it != m_editors.end() && it->second.alive.load())
        {
            // Already open and healthy — bring to front
            if (it->second.hwnd)
                SetForegroundWindow(it->second.hwnd);
            return;
        }
        // Dead or not yet started — remove stale record if present
        if (it != m_editors.end())
        {
            if (it->second.thread.joinable())
                it->second.thread.detach(); // was already dead; detach stale thread
            m_editors.erase(it);
        }
    }

    // Spawn a new per-plugin UI thread
    auto& slot = m_editors[pluginPath];   // create under lock
    slot.readyEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

    slot.thread = std::thread([this, pluginPath, &slot]()
    {
        pluginUIThreadLoop(pluginPath, slot);
    });

    // Wait for the thread to signal its message pump is ready
    if (slot.readyEvent)
    {
        WaitForSingleObject(slot.readyEvent, 5000);
        CloseHandle(slot.readyEvent);
        slot.readyEvent = nullptr;
    }
}
```

**`pluginUIThreadLoop()`** — new method that hosts a single editor:

```cpp
// New private declaration in EditorBridge.h:
void pluginUIThreadLoop(const std::string& pluginPath, PluginEditorThread& slot);

// Implementation:
void EditorBridge::pluginUIThreadLoop(const std::string& pluginPath,
                                       PluginEditorThread& slot)
{
    slot.threadID = GetCurrentThreadId();
    slot.alive    = true;

    if (slot.readyEvent)
        SetEvent(slot.readyEvent);

    // Create host window, call effEditOpen, start idle timer
    // (same logic as current doOpenEditor() lines 383-456)
    // ...
    HWND hwnd = /* CreateWindowExW(...) */;
    slot.hwnd = hwnd;

    // Per-plugin pump with SEH restart loop
    constexpr int MAX_RESTARTS = 3;
    for (int restarts = 0; restarts < MAX_RESTARTS && m_running.load(); ++restarts)
    {
        __try
        {
            MSG msg;
            while (GetMessageW(&msg, hwnd, 0, 0) > 0)
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            break;  // Clean WM_QUIT exit — don't restart
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ++slot.faultCount;
            std::fprintf(stderr,
                "[EditorBridge] editor crash for '%s' (fault #%d)\n",
                pluginPath.c_str(), slot.faultCount);
        }
    }

    // Cleanup
    {
        std::lock_guard<std::mutex> lock(m_editorMutex);
        m_editors.erase(pluginPath);
    }
    slot.alive = false;
}
```

**`doCloseEditor()`** — post WM_QUIT to the per-plugin thread:

```cpp
void EditorBridge::doCloseEditor(const std::string& pluginPath)
{
    std::lock_guard<std::mutex> lock(m_editorMutex);
    auto it = m_editors.find(pluginPath);
    if (it != m_editors.end() && it->second.alive.load())
        PostThreadMessageW(it->second.threadID, WM_QUIT, 0, 0);
}
```

**`processCommand("open")`** — update the alive check: if the prior thread for this plugin is dead, the next `open` command spawns a fresh one cleanly. No change to the pipe protocol is needed.

##### Window class registration

The shared `ATOM m_windowClass` from the single-thread design must remain valid across all per-plugin threads. Register it once in `start()` before spawning any plugin threads, and unregister it in `stop()` after all plugin threads have been joined. This avoids the current design's issue of registering the class on the UI thread (which ties the class lifetime to that thread).

```cpp
// In start() [EditorBridge.cpp:30], after m_running = true:
WNDCLASSEXW wc{};
wc.cbSize       = sizeof(wc);
wc.lpfnWndProc  = WndProc;
wc.hInstance    = GetModuleHandleW(nullptr);
wc.lpszClassName = L"KodiVSTEditor";
wc.hCursor      = LoadCursorW(nullptr, IDC_ARROW);
wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
m_windowClass = RegisterClassExW(&wc);

// In stop() [EditorBridge.cpp:59], after all threads joined:
if (m_windowClass)
{
    UnregisterClassW(L"KodiVSTEditor", GetModuleHandleW(nullptr));
    m_windowClass = 0;
}
```

##### Pipe command: re-opening a crashed editor

When a plugin editor thread dies due to a crash (after exhausting its restart limit), `slot.alive` becomes `false` and the entry is removed from `m_editors`. The next `open` command from the Python side will find no entry and spawn a fresh thread — so **re-opening after a crash is automatic with no protocol changes.** The Python VST Manager can detect this by sending `open` and observing a successful response (the pipe still works because `m_pipeThread` is entirely separate from all UI threads).

##### Crash isolation matrix

| Scenario | Layer A only (shared thread) | Layer B (per-plugin threads) |
|----------|------------------------------|------------------------------|
| Plugin A editor crashes | All editors killed | Only Plugin A's editor killed |
| Plugin A idle (effEditIdle) crashes | All editors killed | Only Plugin A's editor killed |
| Plugin A close (effEditClose) crashes | All editors killed | Only Plugin A's editor killed |
| Re-open after crash | Requires UI thread restart | Automatic on next `open` command |
| Pipe server affected? | No | No |
| Audio processing affected? | No | No |

---

## File Change Index

### Kodi-core side (Windows ADSP bridge)

| File | Nature of change |
|------|-----------------|
| `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAEDSP.h` | +3 member variables; +2 public method declarations |
| `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAEDSP.cpp` | +2 lines in `MasterProcess()`; +2 new methods (`NeedsReset`, `TryReset`); +2 lines in `Deinit()` |
| `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAE.cpp` | +4 lines in the AE worker thread loop (TIMEOUT/idle branch) |

### Addon side (audiodsp.vsthost)

| File | Nature of change — Layer A | Nature of change — Layer B |
|------|---------------------------|---------------------------|
| `kodi-audiodsp-vsthost/src/bridge/EditorBridge.h` | +1 private method (`pumpLoop`); +`MAX_UI_RESTARTS` constant | Replace single-thread members with `PluginEditorThread` map; +1 method (`pluginUIThreadLoop`) |
| `kodi-audiodsp-vsthost/src/bridge/EditorBridge.cpp` | +3 static SEH helpers; restructure `uiThreadLoop()` pump; +`pumpLoop()` | Rewrite `doOpenEditor`, `doCloseEditor`, `doCloseAll`, `uiThreadLoop`; add `pluginUIThreadLoop`; move window-class registration to `start()`/`stop()` |
| `kodi-audiodsp-vsthost/src/bridge/EditorBridge.cpp` | +`reset_dsp` pipe command (diagnostic) | No additional change |

> **Recommendation:** Implement Layer A first (SEH guards + pump restart) as a standalone, lower-risk change. Review and test before proceeding to Layer B (per-plugin thread isolation), which requires more extensive refactoring of `EditorBridge` but provides much stronger fault isolation.
