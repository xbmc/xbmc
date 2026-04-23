# Regression 1 — Boot Hang on Skins Using Startup.xml ReplaceWindow

**Repository:** OrganizerRo/xbmc  
**Date:** 2024  
**Severity:** Critical — complete boot deadlock; Kodi does not reach the home screen.

---

## 1. Root Cause Analysis

### The Deferral Bug

A PR added early-boot deferral logic to `CApplication::ExecuteXBMCAction` guarded by `m_bInitializing == true`. When a skin's `Startup.xml` `<onload>` fires `ReplaceWindow(Home)` or `ActivateWindow(Home)` during initialisation, that message is deferred instead of executed. As a result:

1. The `CGUIWindowManager` never receives the window-activate message.
2. `OnDeinitWindow` is never called on the splash/startup window.
3. `GUI_MSG_UI_READY` is therefore never dispatched to `CApplication`.
4. `CApplication::Initialize()` spins waiting for `GUI_MSG_UI_READY` → **deadlock**.

```cpp
// Application.cpp — BUGGY version
bool CApplication::ExecuteXBMCAction(std::string actionStr, const CGUIListItemPtr &item)
{
  if (m_bInitializing) {
    m_deferredActions.push_back(actionStr);  // ← defers ReplaceWindow(Home)
    return true;                             // ← splash window never deactivated
  }
  // ...
}
```

### The Boot-Ready Signal Chain

```
CApplication::Initialize()
  → CGUIWindowManager::ActivateWindow(WINDOW_STARTUP_ANIM)
  → Startup.xml <onload> runs
      → ExecuteXBMCAction("ReplaceWindow(Home)")   ← DEFERRED → hang
  → [waits for GUI_MSG_UI_READY]
       ↑ never arrives because ReplaceWindow never fires
```

The deferred queue is only flushed _after_ `m_bInitializing` is set to `false` (line 3759), but that assignment only happens _after_ `GUI_MSG_UI_READY` is received — creating a circular dependency.

### Key File:Line References

| Location | Significance |
|----------|-------------|
| `xbmc/Application.h:433` | `bool m_bInitializing = true;` |
| `xbmc/Application.cpp:3759` | `m_bInitializing = false;` — only reached after `GUI_MSG_UI_READY` |
| `xbmc/Application.cpp:3956–3958` | `GUI_MSG_EXECUTE` handler calling `ExecuteXBMCAction` |
| `xbmc/Application.cpp:3964–4013` | Full `ExecuteXBMCAction` body |
| `xbmc/guilib/GUIWindowManager.cpp` | `ActivateWindow` / `ReplaceWindow` — must fire for `UI_READY` |
| `addons/skin.estouchy/xml/Startup.xml` | Example skin using `<onload>ReplaceWindow(Home)</onload>` |

---

## 2. Exact Call Path

```
CApplication::Initialize()
  → GUIWindowManager.ActivateWindow(WINDOW_STARTUP_ANIM)
      → CGUIWindow::OnInitWindow()
          → CGUIAction::ExecuteActions()   [Startup.xml <onload>]
              → CGUIMessage(GUI_MSG_EXECUTE, ..., "ReplaceWindow(Home)")
                  → CApplication::OnMessage()
                      → ExecuteXBMCAction("ReplaceWindow(Home)")
                          → if (m_bInitializing)        ← TRUE
                              m_deferredActions.push_back(...)  ← DEFERS
                              return true               ← boom
  → [spin-wait for GUI_MSG_UI_READY]   ← NEVER ARRIVES
```

---

## 3. Why Only Certain Skins Are Affected

Skins that use a `Startup.xml` with `<onload>ReplaceWindow(...)</onload>` or `ActivateWindow(...)` rely on that action completing synchronously before the init sequence continues. Skins that transition to Home via a different mechanism (e.g., a timed animation that calls `ActivateWindow` after `m_bInitializing` is already `false`) are not affected.

Known affected patterns:
- `<onload>ReplaceWindow(Home)</onload>` in `Startup.xml`
- `<onload>ActivateWindow(Home)</onload>` in `Startup.xml`
- Any `RunScript(...)` or `PlayMedia(...)` call in `Startup.xml` `<onload>` that itself depends on a window being active

---

## 4. Candidate Fix Approaches

### Approach A — Bypass Deferral for Window-Navigation Builtins (Recommended)

Check whether `actionStr` begins with `ActivateWindow(`, `ReplaceWindow(`, or `FullscreenWindow(`. If so, execute immediately regardless of `m_bInitializing`.

```cpp
bool CApplication::ExecuteXBMCAction(std::string actionStr, const CGUIListItemPtr &item)
{
  if (m_bInitializing)
  {
    // Window navigation is required for boot to complete — never defer.
    static const std::vector<std::string> bootCritical = {
      "ActivateWindow(", "ReplaceWindow(", "FullscreenWindow("
    };
    bool isCritical = std::any_of(bootCritical.begin(), bootCritical.end(),
      [&](const std::string& prefix){ return StringUtils::StartsWithNoCase(actionStr, prefix); });
    if (!isCritical)
    {
      m_deferredActions.push_back({actionStr, item});
      return true;
    }
    // fall through to immediate execution
  }
  // ... rest of function
}
```

**Pros:**
- Minimal, targeted change.
- Does not affect any non-window-navigation actions.
- Boot sequence completes normally.

**Cons:**
- Requires maintaining a list of "boot-critical" prefixes.
- Skin authors could invent unusual action strings that alias window navigation.

---

### Approach B — Never Defer `GUI_MSG_EXECUTE` from `CGUIWindow::OnInitWindow`

Pass a flag through the message chain indicating the message originates from a window's `<onload>` handler. In `ExecuteXBMCAction`, skip deferral when this flag is set.

**Pros:** Context-aware; catches all `<onload>` actions.  
**Cons:** Requires threading a flag through `CGUIMessage`, `CGUIAction`, and `CGUIWindow` — wider blast radius.

---

### Approach C — Process Deferral Queue Inline Before the `GUI_MSG_UI_READY` Wait

Before entering the spin-wait for `GUI_MSG_UI_READY`, flush the deferred queue. Repeat until `m_bInitializing` is false or max iterations reached.

**Cons:** Convoluted; the flush can itself enqueue more items; still doesn't guarantee `GUI_MSG_UI_READY` is sent.

---

## 5. Recommended Fix — Approach A

### Detailed Code Changes

#### `xbmc/Application.cpp` — `ExecuteXBMCAction`

```diff
 bool CApplication::ExecuteXBMCAction(std::string actionStr, const CGUIListItemPtr &item)
 {
   if (m_bInitializing)
   {
+    // Window navigation actions must execute immediately during boot;
+    // deferring them prevents GUI_MSG_UI_READY from ever being sent.
+    static const std::array<std::string_view, 3> bootCritical = {
+        "ActivateWindow(", "ReplaceWindow(", "FullscreenWindow("
+    };
+    bool isCritical = std::any_of(bootCritical.begin(), bootCritical.end(),
+        [&](std::string_view prefix)
+        { return StringUtils::StartsWithNoCase(actionStr, std::string(prefix)); });
+    if (!isCritical)
+    {
       m_deferredActions.push_back(actionStr);
       return true;
+    }
   }
   // ... rest unchanged
```

No changes to `Application.h` are required for this fix alone (though R3 fix recommends upgrading `m_deferredActions` to `std::deque<DeferredAction>`; these two fixes compose cleanly).

---

## 6. Edge Cases

### 6a. `ActivateWindow` With Parameters
`ActivateWindow(MyVideoNav,/path/to/dir)` — the prefix check catches this correctly since it tests the start of the string.

### 6b. Skin Uses `RunScript` in Startup That Opens a Window
If `RunScript(script.launcher.py)` internally calls `ActivateWindow`, that call happens from within Python on a separate thread — already outside the `m_bInitializing` guard by the time the Python thread fires. Not affected.

### 6c. Alias `xbmc.activatewindow` Built-in
Some action strings arrive after lower-casing or alias expansion. `StringUtils::StartsWithNoCase` handles case variation. Aliases resolved via `CBuiltins` happen after this deferral check and so are not relevant.

### 6d. `FullscreenWindow` During PVR Startup
`FullscreenWindow` transitions to the fullscreen player. If a PVR skin fires this from `Startup.xml`, the same deadlock occurs. The fix covers this via the `FullscreenWindow(` prefix.

---

## 7. Test Scenarios

### T1 — Skin With `ReplaceWindow(Home)` in `Startup.xml`
1. Set `m_bInitializing = true`.
2. Fire `GUI_MSG_EXECUTE` with `"ReplaceWindow(Home)"`.
3. Verify action is **not** deferred — executes immediately.
4. Verify `GUI_MSG_UI_READY` is subsequently sent.
5. Verify `m_bInitializing` transitions to `false`.

### T2 — Normal Action Is Still Deferred
1. Set `m_bInitializing = true`.
2. Fire `GUI_MSG_EXECUTE` with `"PlayMedia(smb://nas/movie.mkv)"`.
3. Verify action IS deferred.
4. Set `m_bInitializing = false`, flush.
5. Verify `PlayMedia` executes at flush time.

### T3 — Skin Without Startup.xml (No Regression Introduced)
1. Boot with default Estouchy skin (no `Startup.xml` window-nav).
2. Verify boot completes normally and deferred actions flush as before.

### T4 — `ActivateWindow` With Path Parameter During Boot
1. Fire `GUI_MSG_EXECUTE` with `"ActivateWindow(Home,/path)"`.
2. Verify not deferred.
3. Verify correct window + path activated.

---

## 8. Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Bootcritical prefix list incomplete | Low | High | Audit all `CBuiltins` window-navigation handlers; add any missing prefixes |
| Immediate execution crashes if window manager not ready | Low | Medium | Window manager is initialised before `Startup.xml` runs; safe |
| Fix interacts badly with R3 `DeferredAction` struct | None | None | R1 fix is independent; struct upgrade composes cleanly |
| Case-sensitivity of action strings | Low | Low | `StartsWithNoCase` handles all variations |

### Overall Risk
**Low.** The change adds a small allow-list bypass; all other deferral behaviour is unchanged. It directly restores the original boot contract.
