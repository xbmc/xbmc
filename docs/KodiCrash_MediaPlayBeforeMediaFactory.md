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

A Kodi built-in sets `Skin.SetBool(PlayerReady)` once the application signals
`GUI_MSG_UI_READY`. `Skin.SetBool` is the correct builtin for toggling a named skin setting
to `true`; `Skin.SetBoolSetting` does not exist in this codebase
(`xbmc/interfaces/builtins/SkinBuiltins.cpp:464`).

**Pros**: Zero core change; isolated to the skin.  
**Cons**: Fragile; requires every skin author to know about Kodi's internal lifecycle. Does not
fix the underlying core bug.

---

### Fix 6 — Defer/replay queue for early playback actions *(application-layer safety net)*

Instead of dropping a `PlayMedia` request that arrives before `InitStageThree` completes,
buffer it and replay it once the application has finished initialising.

**Files changed**:

| File | Change |
|------|--------|
| `xbmc/Application.h` | Add `std::deque<std::string> m_deferredActions` private member |
| `xbmc/Application.cpp` | Intercept early actions in `ExecuteXBMCAction()`; flush in `OnMessage()` on `GUI_MSG_UI_READY` |

#### How it works

**1 — `Application.h`**: add a queue member

```cpp
// new private member
std::deque<std::string> m_deferredActions;
```

**2 — `Application.cpp` — `ExecuteXBMCAction()`** (line ~3974, after label resolution):

```cpp
bool CApplication::ExecuteXBMCAction(std::string actionStr, const CGUIListItemPtr &item)
{
  const std::string in_actionStr(actionStr);
  if (item)
    actionStr = GUILIB::GUIINFO::CGUIInfoLabel::GetItemLabel(actionStr, item.get());
  else
    actionStr = GUILIB::GUIINFO::CGUIInfoLabel::GetLabel(actionStr);

  // NEW: defer any builtin action that arrives before stage-3 services are ready
  if (m_bInitializing)
  {
    CLog::LogF(LOGDEBUG, "Deferring action '%s' until init completes", actionStr.c_str());
    m_deferredActions.push_back(actionStr);
    return true;
  }

  // ... existing dispatch code unchanged ...
}
```

Label resolution (`CGUIInfoLabel::GetLabel`) has already run, so skin string values (e.g.
`$INFO[Skin.String(CustomVideoBackground)]`) are baked into the stored string. The skin-side
condition (`!Player.HasMedia`) is evaluated by `CGUIAction::ExecuteActions()` **before**
`GUI_MSG_EXECUTE` is sent, so only actions whose conditions were satisfied at `<onload>` time
reach this function — and therefore only those are queued.

**3 — `Application.cpp` — `OnMessage()`, `GUI_MSG_UI_READY` branch** (line ~3739):

```cpp
else if (message.GetParam1() == GUI_MSG_UI_READY)
{
  // existing cleanup ...
  m_bInitializing = false;

  // NEW: replay any actions that arrived before InitStageThree completed
  for (const auto& action : m_deferredActions)
  {
    CLog::LogF(LOGDEBUG, "Replaying deferred startup action '%s'", action.c_str());
    ExecuteXBMCAction(action, nullptr);
  }
  m_deferredActions.clear();

  // ... rest of existing code ...
}
```

`GUI_MSG_UI_READY` is sent by `CApplication::Initialize()` (line ~884) via
`SendThreadMessage`, which enqueues it for dispatch in `CApplication::Process()` →
`CGUIWindowManager::DispatchThreadMessages()`. By the time this message is dispatched,
`InitStageThree()` (line ~849) has long since completed and `CPlayerCoreFactory` is fully
constructed. The replay call to `ExecuteXBMCAction` re-enters normally (without deferring,
because `m_bInitializing` is now `false`).

#### Timing guarantee

```
Initialize():
  line ~831: ActivateWindow(firstWindow)
               └─ <onload> fires → ExecuteXBMCAction() → m_bInitializing==true
                  └─ action pushed onto m_deferredActions ✓

  line ~849: InitStageThree()
               └─ m_playerCoreFactory.reset(new ...) ← factory now live

  line ~884: SendThreadMessage(GUI_MSG_UI_READY) ← queued, not dispatched yet
Initialize() returns.

Process() loop:
  DispatchThreadMessages() → GUI_MSG_UI_READY → OnMessage()
    m_bInitializing = false
    for action in m_deferredActions:
      ExecuteXBMCAction(action) → PlayMedia() → factory ready ✓
```

**Pros**: Preserves the original intent of the skin's `<onload>` action; the background video
will start even on first boot without requiring a window navigation. Works as a safety net
regardless of whether Fix 2 or Fix 4 is also applied. Handles any skin builtin, not just
`PlayMedia`.  
**Cons**: If Init takes long enough that the user has manually started playback by the time the
queue flushes, replaying a `PlayMedia` command will interrupt it. Mitigated in practice because
`GUI_MSG_UI_READY` fires very quickly after `InitStageThree`. Storing resolved action strings
means skin-string conditions cannot be re-evaluated at replay time (they were evaluated once, at
`<onload>` time).

---

## Recommended Approach

Apply two fixes together:

1. **Fix 2** — Move `InitStageThree` before `ActivateWindow(firstWindow)` in `Application.cpp`.  
   This resolves the root cause so that `CPlayerCoreFactory` (and all other stage-3 services) are
   available to any builtin action fired from a skin window's `<onload>`.

2. **Fix 3** — Add a null-pointer check with logging (and an `assert` in debug builds) in
   `CServiceManager::GetPlayerCoreFactory()`.  
   In release builds, log an error and either return a dummy reference or abort gracefully.
   In debug builds, assert immediately so regressions are caught early.

**Fix 6** (defer/replay queue) is the best alternative to Fix 2 when reordering the init sequence
is not acceptable (e.g. when a stage-3 component is later found to have a dependency on an active
window). It preserves correct skin behaviour without silently dropping the first playback request.

**Fix 4** (async `<onload>`) is architecturally the most correct long-term solution and is
consistent with how focus/unfocus actions already work. It is recommended as a follow-up after
the immediate reordering fix has been validated.

---

## Related Code Locations (quick reference)

| Symbol | File:Line |
|--------|-----------|
| `CServiceManager::InitStageThree` | `xbmc/ServiceManager.cpp:166` |
| `CServiceManager::GetPlayerCoreFactory` | `xbmc/ServiceManager.cpp:~361` |
| `CServiceBroker::GetPlayerCoreFactory` | `xbmc/ServiceBroker.cpp:193` |
| `CServiceBroker::IsServiceManagerUp` | `xbmc/ServiceBroker.cpp:152` |
| `CApplication::Initialize` — window activation | `xbmc/Application.cpp:~831` |
| `CApplication::Initialize` — InitStageThree call | `xbmc/Application.cpp:~849` |
| `CApplication::Initialize` — GUI_MSG_UI_READY sent | `xbmc/Application.cpp:~884` |
| `CApplication::OnMessage` — GUI_MSG_UI_READY handler | `xbmc/Application.cpp:~3739` |
| `CApplication::ExecuteXBMCAction` | `xbmc/Application.cpp:~3964` |
| `CApplication::m_bInitializing` | `xbmc/Application.h:433` |
| `CGUIWindowManager::SendThreadMessage` | `xbmc/guilib/GUIWindowManager.cpp:1414` |
| `CGUIWindowManager::DispatchThreadMessages` | `xbmc/guilib/GUIWindowManager.cpp:1422` |
| `CApplication::Process` — dispatch loop | `xbmc/Application.cpp:~4035` |
| `CGUIWindow::RunLoadActions` | `xbmc/guilib/GUIWindow.cpp:1049` |
| `CGUIAction::ExecuteActions` | `xbmc/guilib/GUIAction.cpp:21` |
| `m_sendThreadMessages` flag for async dispatch | `xbmc/guilib/GUIAction.h:55` |
| `PlayMedia` builtin | `xbmc/interfaces/builtins/PlayerBuiltins.cpp:381` |
| `Skin.SetBool` builtin | `xbmc/interfaces/builtins/SkinBuiltins.cpp:464` |
| `skin.estouchy` Home `<onload>` PlayMedia | `addons/skin.estouchy/xml/Home.xml:6` |
