# Kodi Crash: PlayMedia Called Before PlayerCoreFactory Is Ready

## Summary

Kodi crashes during startup when a skin `<onload>` action (such as `PlayMedia`) fires
synchronously **before** `CPlayerCoreFactory` has been created by `CServiceManager::InitStageThree()`.
The null-pointer dereference inside `CServiceManager::GetPlayerCoreFactory()` produces a hard CRT
abort with no user-visible error message.

---

## Affected Files

| File | Role |
|------|------|
| `xbmc/Application.cpp` | Main init sequence; ordering of `ActivateWindow` vs `InitStageThree` |
| `xbmc/ServiceManager.cpp` | `InitStageThree` creates `m_playerCoreFactory`; `GetPlayerCoreFactory()` dereferences it |
| `xbmc/ServiceBroker.cpp` | `GetPlayerCoreFactory()` forwards to `g_application.m_ServiceManager->GetPlayerCoreFactory()` |
| `xbmc/ServiceBroker.h` | Declares `IsServiceManagerUp()` (checks `init_level == 3`) |
| `xbmc/guilib/GUIWindow.cpp` | `OnInitWindow()` → `RunLoadActions()` fires `onload` actions synchronously |
| `xbmc/guilib/GUIAction.cpp` | `ExecuteActions()` sends `GUI_MSG_EXECUTE` — sync or async depending on `m_sendThreadMessages` |
| `xbmc/interfaces/builtins/PlayerBuiltins.cpp` | `PlayMedia()` builtin eventually calls `CServiceBroker::GetPlayerCoreFactory()` |
| `addons/skin.estouchy/xml/Home.xml` | Example skin that triggers `PlayMedia` from `<onload>` on the Home window |

---

## Root Cause

`CApplication::Initialize()` (`Application.cpp`) contains a fatal ordering flaw:

```
Step 1 (line ~831):  GetWindowManager().ActivateWindow(firstWindow)
                       └─ CGUIWindowHome::OnInitWindow()
                            └─ CGUIWindow::RunLoadActions()           ← fires <onload> now
                                 └─ CGUIAction::ExecuteActions()       ← SYNCHRONOUS SendMessage
                                      └─ CApplication::ExecuteXBMCAction()
                                           └─ PlayMedia(...)
                                                └─ CServiceBroker::GetPlayerCoreFactory()
                                                     └─ *m_playerCoreFactory  ← NULL DEREF 💥

Step 2 (line ~849):  m_ServiceManager->InitStageThree(profileManager)
                       └─ m_playerCoreFactory.reset(new CPlayerCoreFactory(...))  ← too late
```

`CServiceBroker::GetPlayerCoreFactory()` calls
`g_application.m_ServiceManager->GetPlayerCoreFactory()`, which dereferences the
`std::unique_ptr<CPlayerCoreFactory>` before it has been constructed.

`CServiceBroker::IsServiceManagerUp()` already exists and correctly checks
`init_level == 3` — but nothing in the `PlayMedia` call path uses this guard.

---

## Fix Options

### Fix 1 — Guard the call site in `CApplication::PlayFile()` *(quick, safe, minimal)*

**File**: `xbmc/Application.cpp`

Before calling `m_appPlayer.OpenFile(...)`, check whether Stage 3 (and therefore
`CPlayerCoreFactory`) is ready:

```cpp
// proposed addition
if (!CServiceBroker::IsServiceManagerUp())
{
    CLog::LogF(LOGWARNING, "PlayFile called before PlayerCoreFactory is ready, ignoring");
    return false;
}
m_appPlayer.OpenFile(item, options, m_ServiceManager->GetPlayerCoreFactory(), player, *this);
```

`CServiceBroker::IsServiceManagerUp()` is already implemented (`ServiceBroker.cpp:152`) and
checks `init_level == 3`.

**Pros**: One-line, zero risk of cascading side effects.  
**Cons**: Silently drops the first `PlayMedia` request; the background video only starts if the
skin re-triggers the action (e.g. returning to the Home window). Acceptable because this case is
cosmetic.

---

### Fix 2 — Move `InitStageThree` before `ActivateWindow(firstWindow)` *(root-cause fix)*

**File**: `xbmc/Application.cpp`

Reorder `Application::Initialize()` so that `InitStageThree` is called **before** the first real
window is activated:

```cpp
// Before:
//   ActivateWindow(firstWindow);   // <── onload fires here, factory not yet created
//   ...
//   InitStageThree(profileManager);

// After:
if (!m_ServiceManager->InitStageThree(profileManager))
    CLog::Log(LOGERROR, "Application - Init3 failed");

int firstWindow = g_SkinInfo->GetFirstWindow();
CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(firstWindow);
```

`CPlayerCoreFactory`, PVR, game services, peripherals, and `CContextMenuManager` are all
initialised before any skin `<onload>` action can fire.

**Pros**: Eliminates the root cause systemically; any skin builtin becomes safe to call from `<onload>`.  
**Cons**: Stage-3 components (`CPVRManager::Init`, `CPeripherals::Initialise`, etc.) now run
before the first frame is rendered — a minor delay that is not user-visible. A review of all
stage-3 constructors confirms none of them require an active home window.

---

### Fix 3 — Null guard in `CServiceManager::GetPlayerCoreFactory()` *(defensive hardening)*

**File**: `xbmc/ServiceManager.cpp`

Add an assertion so that debug builds produce a clear diagnostic instead of a silent CRT abort:

```cpp
CPlayerCoreFactory& CServiceManager::GetPlayerCoreFactory()
{
    assert(m_playerCoreFactory && "GetPlayerCoreFactory() called before InitStageThree");
    return *m_playerCoreFactory;
}
```

**Pros**: Converts undefined behaviour / CRT abort into an obvious assertion failure; pairs well
with Fix 1 or Fix 2.  
**Cons**: Not a standalone fix — only transforms the crash into a more visible diagnostic.

---

### Fix 4 — Make `<onload>` actions asynchronous *(framework-level fix)*

**File**: `xbmc/guilib/GUIWindow.cpp` (line 185)

`<onfocus>` and `<onunfocus>` actions are already flagged as async thread messages
(`GUIControlFactory.cpp:853`). Apply the same treatment to `<onload>`:

```cpp
CGUIControlFactory::GetActions(pRootElement, "onload", m_loadActions);
m_loadActions.m_sendThreadMessages = true;  // add this line
```

With this change, `<onload>` actions are queued as `GUI_MSG_EXECUTE` thread messages and
dispatched inside `CApplication::Process()`, which runs **after** `Initialize()` returns — at
which point `InitStageThree` has already completed.

**Pros**: Fixes the root cause at the GUI framework level; all skins benefit automatically without
any core reordering.  
**Cons**: Behaviour change: skins that relied on `<onload>` completing synchronously before
`OnInitWindow()` returns may see different behaviour. In practice `<onload>` actions are
fire-and-forget (media playback, API calls), so async dispatch is semantically correct.

---

### Fix 5 — Skin-side workaround in `addons/skin.estouchy/xml/Home.xml` *(non-invasive)*

Add a condition to the `PlayMedia` action that prevents it from firing during the startup
race window, for example by checking a skin property that is only set after the player is
confirmed to be available:

```xml
<onload condition="Skin.HasSetting(PlayerReady)">PlayMedia(...)</onload>
```

A short Python script (or a Kodi built-in) sets `Skin.SetBoolSetting(PlayerReady)` once the
application signals `GUI_MSG_UI_READY`.

**Pros**: Zero core change; isolated to the skin.  
**Cons**: Fragile; requires every skin author to know about Kodi's internal lifecycle. Does not
fix the underlying core bug.

---

## Recommended Approach

Apply two fixes together:

1. **Fix 2** — Move `InitStageThree` before `ActivateWindow(firstWindow)` in `Application.cpp`.  
   This resolves the root cause so that `CPlayerCoreFactory` (and all other stage-3 services) are
   available to any builtin action fired from a skin window's `<onload>`.

2. **Fix 3** — Add an `assert` in `CServiceManager::GetPlayerCoreFactory()`.  
   This turns any future regression of the same class into an immediately visible assertion failure
   in debug builds rather than a silent CRT crash.

**Fix 4** (async `<onload>`) is architecturally the most correct long-term solution and is
consistent with how focus/unfocus actions already work. It is recommended as a follow-up after
the immediate reordering fix has been validated.

---

## Related Code Locations (quick reference)

| Symbol | File:Line |
|--------|-----------|
| `CServiceManager::InitStageThree` | `xbmc/ServiceManager.cpp:166` |
| `CServiceManager::GetPlayerCoreFactory` | `xbmc/ServiceManager.cpp:~360` |
| `CServiceBroker::GetPlayerCoreFactory` | `xbmc/ServiceBroker.cpp:193` |
| `CServiceBroker::IsServiceManagerUp` | `xbmc/ServiceBroker.cpp:152` |
| `CApplication::Initialize` — window activation | `xbmc/Application.cpp:~831` |
| `CApplication::Initialize` — InitStageThree call | `xbmc/Application.cpp:~849` |
| `CGUIWindow::RunLoadActions` | `xbmc/guilib/GUIWindow.cpp:1049` |
| `CGUIAction::ExecuteActions` | `xbmc/guilib/GUIAction.cpp:21` |
| `m_sendThreadMessages` flag for async dispatch | `xbmc/guilib/GUIAction.h:55` |
| `PlayMedia` builtin | `xbmc/interfaces/builtins/PlayerBuiltins.cpp:381` |
| `skin.estouchy` Home `<onload>` PlayMedia | `addons/skin.estouchy/xml/Home.xml:5` |
