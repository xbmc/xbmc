# Regression 3 — CGUIListItemPtr Item Context Lost on Replay

**Repository:** OrganizerRo/xbmc  
**Date:** 2024  
**Severity:** Medium-High — silent behavioral regression; no crash, but wrong playback/navigation for item-bearing actions during early boot.

---

## 1. Root Cause Analysis

### The Deferral Bug

A PR added early-boot deferral logic to `CApplication::ExecuteXBMCAction` guarded by `m_bInitializing == true`. In the actual code, label resolution via `GetItemLabel`/`GetLabel` occurs **before** the deferral check — so the deferred queue stores the **already-resolved** action string, not the original template:

```cpp
// Application.cpp — actual code (BUGGY)
bool CApplication::ExecuteXBMCAction(std::string actionStr, const CGUIListItemPtr &item)
{
  const std::string in_actionStr(actionStr);   // ← original unresolved string saved here

  // BUG: resolution happens at boot time when GUI info context is not fully ready
  if (item)
    actionStr = GetItemLabel(actionStr, item.get());  // may return empty/wrong at boot
  else
    actionStr = GetLabel(actionStr);                  // resolves against boot-time context

  if (m_bInitializing) {
    m_deferredActions.push_back(actionStr);  // ← BUG: stores baked (possibly wrong) string;
    return true;                             //         in_actionStr and item both lost
  }
  // … rest of function
}

// Later, when m_bInitializing becomes false:
for (const auto& action : m_deferredActions)
  ExecuteXBMCAction(action, nullptr);  // re-runs GetLabel on already-baked string; item = nullptr
```

The two compounding bugs are:

1. **Resolution at boot time** — `GetItemLabel`/`GetLabel` are called while GUI info context (skin, window state, infoManager) may not be fully initialised, potentially returning empty or incorrect strings for `$INFO[ListItem.*]` tokens.
2. **Original string discarded** — `in_actionStr` (the raw unresolved template) is never stored; the partially-resolved string is baked into the queue, so correct re-resolution at replay time is impossible even when GUI context is fully ready.

### Why the Item and the Original String Both Matter

`ExecuteXBMCAction` resolves `$INFO[ListItem.*]` tokens at **Application.cpp:3984–3987** (before the deferral guard at 3990):

```cpp
const std::string in_actionStr(actionStr);   // original template
if (item)
  actionStr = GUILIB::GUIINFO::CGUIInfoLabel::GetItemLabel(actionStr, item.get());
else
  actionStr = GUILIB::GUIINFO::CGUIInfoLabel::GetLabel(actionStr);
// ← deferral check follows here at 3990
```

When `item` is present, `GetItemLabel` resolves `$INFO[ListItem.*]` tokens against the specific list item's properties (e.g., `ListItem.FileNameAndPath`, `ListItem.Property(node.target)`, PVR tags, artwork URLs). At boot time the GUI info manager may not yet be fully initialised, so `GetItemLabel` can return empty strings or wrong values for those tokens. The resulting partially-resolved string is then baked into `m_deferredActions` and replayed verbatim — there is no second chance to use the correct item context.

At replay (line 3770), `ExecuteXBMCAction(action, nullptr)` is called; `GetLabel(action)` is invoked again on the already-baked string. If no `$INFO[]` tokens remain, `GetLabel` returns the string unchanged — correct if the first resolution succeeded, silently wrong if it did not.

### Key File:Line References

| Location | Significance |
|----------|-------------|
| `xbmc/Application.h:433` | `bool m_bInitializing = true;` — flag that guards deferral |
| `xbmc/Application.h:290` | `bool ExecuteXBMCAction(std::string action, const CGUIListItemPtr &item = NULL);` |
| `xbmc/Application.cpp:3969–3971` | `GUI_MSG_EXECUTE` handler: `ExecuteXBMCAction(message.GetStringParam(), message.GetItem())` |
| `xbmc/Application.cpp:3977–4013` | Full `ExecuteXBMCAction` body |
| `xbmc/Application.cpp:3983` | `const std::string in_actionStr(actionStr);` — original unresolved string |
| `xbmc/Application.cpp:3984–3987` | Item-conditional label resolution — occurs **before** the deferral guard |
| `xbmc/Application.cpp:3990–3994` | Deferral check — stores already-resolved `actionStr`, losing `in_actionStr` and `item` |
| `xbmc/Application.cpp:3759` | `m_bInitializing = false;` — end of boot |
| `xbmc/guilib/GUIAction.cpp:41` | `CGUIMessage msg(GUI_MSG_EXECUTE, controlID, parentID, 0, 0, item);` — item attached to message |
| `xbmc/listproviders/DirectoryProvider.cpp:360–376` | `OnClick` resolves node.target, creates `GUI_MSG_EXECUTE` |
| `xbmc/guilib/GUIMessage.h:354` | Constructor: `CGUIMessage(int, int, int, int, int, const CGUIListItemPtr&)` |
| `xbmc/guilib/GUIMessage.h:362` | `CGUIListItemPtr GetItem() const;` |

---

## 2. Exact Call Path from DirectoryProvider::OnClick to ExecuteXBMCAction with Item

### Path A — DirectoryProvider (item resolved before send, no item in message)

```
DirectoryProvider::OnClick(const CGUIListItemPtr &item)
  → fileItem = CFileItem copy of item              [DirectoryProvider.cpp:353]
  → target = fileItem.GetProperty("node.target")   [DirectoryProvider.cpp:360]
  → execute = GetFavouritesService().GetExecutePath(fileItem, target) [line 371]
  → CGUIMessage message(GUI_MSG_EXECUTE, 0, 0)     [line 374]
  → message.SetStringParam(execute)                [line 375]
  → WindowManager.SendMessage(message)             [line 376]
      → CApplication::OnMessage(message)
          → case GUI_MSG_EXECUTE:                  [Application.cpp:3956]
              ExecuteXBMCAction(
                message.GetStringParam(),          // execute string
                message.GetItem()                  // ← nullptr! item was NOT put in message
              )
```

**Note:** In this path `node.target` and `node.target_url` are already resolved _before_ the message is sent. The execute string is fully baked. However, if `execute` itself contains `$INFO[ListItem.*]` tokens (which can happen if the skin/favourites service uses them), item context is still needed.

### Path B — CGUIAction::ExecuteActions (item explicitly in message)

```
CGUIAction::ExecuteActions(controlID, parentID, item)
  → for each action string i:
      CGUIMessage msg(GUI_MSG_EXECUTE, controlID, parentID, 0, 0, item)  [GUIAction.cpp:41]
      msg.SetStringParam(i)                        [line 42]
      WindowManager.SendMessage(msg)               [line 45-46]
          → CApplication::OnMessage(message)
              → case GUI_MSG_EXECUTE:
                  ExecuteXBMCAction(
                    message.GetStringParam(),      // may contain $INFO[ListItem.*]
                    message.GetItem()              // ← item IS provided
                  )
```

This is the **primary regression path** — action strings like `PlayMedia($INFO[ListItem.FileNameAndPath])` or `ActivateWindow(MyVideoNav,$INFO[ListItem.Property(node.target_url)])` depend on item context for resolution at line 3972.

---

## 3. What Item Properties Are Used After the Deferral Point

All property resolution happens at `Application.cpp:3971–3974`, **before** any command dispatch. The following item properties are thus affected by the deferral:

| Token/Property | Use Case |
|----------------|----------|
| `$INFO[ListItem.FileNameAndPath]` | `PlayMedia(...)` actions on media items |
| `$INFO[ListItem.Property(node.target)]` | Widget/skin navigation to custom window targets |
| `$INFO[ListItem.Property(node.target_url)]` | Directory/path navigation |
| `$INFO[ListItem.Property(Addon.ID)]` | Addon-specific actions |
| `$INFO[ListItem.PVRChannelNumber]` | PVR channel switch |
| `ListItem.Art(thumb)`, `ListItem.Art(fanart)` | Artwork-based commands |
| Custom playlist properties | Skin-injected playlist references |
| `$INFO[ListItem.Label]` | Label-based routing in skin XML |

Any action string that contains `$INFO[ListItem.*]` will silently produce wrong output when replayed without the item.

---

## 4. Candidate Fix Approaches

### Approach A — Store `{original string, item}` Pairs (Recommended)

Change `m_deferredActions` from `std::deque<std::string>` to `std::deque<DeferredAction>` where each entry holds the **original unresolved** action string (`in_actionStr`) and the item pointer. On replay, call `ExecuteXBMCAction(deferred.actionStr, deferred.item)` — resolution then happens when GUI services are fully ready.

**Pros:**
- Minimal change; preserves all lazy-resolution semantics.
- Item lifetime is extended by `shared_ptr` ref-counting; no dangling pointer risk.
- Correct resolution context at replay time.

**Cons:**
- If the item's properties change between deferral and replay (e.g., a background update modifies the CFileItem), the resolved action may differ. This is acceptable — it matches the semantics of "the action the user intended".

---

### Approach B — Resolve Labels Eagerly at Deferral Time

At the deferral point, immediately call `GetItemLabel` / `GetLabel` to resolve `$INFO[...]` tokens, then store the fully-resolved string. On replay, no item is needed.

**Pros:**
- No item lifetime management.
- Simple: `m_deferredActions` stays `std::deque<std::string>`.

**Cons:**
- `CGUIInfoManager` may not be fully initialised when `m_bInitializing == true`, causing GetItemLabel to return empty strings anyway.
- Loses true lazy-resolution; if the resolved string depends on state that changes between boot and replay, the value is stale.
- Does not fix the PVR tag / artwork cases that use item data not expressible as a label token.

---

### Approach C — Skip Deferral for Item-Bearing Actions

If `item != nullptr`, bypass the deferral and execute immediately even during `m_bInitializing`. The assumption is that any caller providing an item has already ensured sufficient context is available.

**Pros:**
- Zero change to deferral storage.
- Semantically clear: "I have enough context, run now."

**Cons:**
- Executing during initialisation may crash or produce undefined results if required services (e.g., PlaybackManager, PVRManager) are not ready.
- Partial deferral (some actions deferred, some not) introduces ordering hazards if actions are interdependent.
- Not safe for all item-bearing actions.

---

### Approach D — Store the Full CGUIMessage

Instead of `std::deque<std::string>`, keep a `std::deque<CGUIMessage>`. When deferring, push the full message. On replay, re-dispatch via `CApplication::OnMessage`.

**Pros:** Perfect fidelity — all message fields (senderID, controlID, params, item) are preserved.
Future-proof: any new fields added to GUI_MSG_EXECUTE are automatically preserved.

**Cons:**
- CGUIMessage is not cheaply copyable (vector of strings, shared_ptr).

**Pros:**
- Perfect fidelity — all message fields (senderID, controlID, params, item) are preserved.
- Future-proof: any new fields added to GUI_MSG_EXECUTE are automatically preserved.

**Cons:**
- CGUIMessage is not cheaply copyable (vector of strings, shared_ptr).
- Re-dispatching through OnMessage may trigger unintended side-effects if other message handling code has changed state between deferral and replay.
- Slightly over-engineered for this specific bug.

---

## 5. Recommended Fix — Approach A: Store `{string, item}` Pairs

Approach A is the correct surgical fix. It preserves all existing semantics, is safe with `shared_ptr` lifetime management, and requires changes in only two places.

### Detailed Code Changes

#### `xbmc/Application.h`

**1. Add the deferred action type and member:**

```cpp
// Before (actual code):
std::deque<std::string> m_deferredActions;

// After (FIXED):
// In the private section, near m_bInitializing:
struct DeferredAction
{
  std::string actionStr;   // original unresolved string (in_actionStr, not post-GetItemLabel)
  CGUIListItemPtr item;
};
std::deque<DeferredAction> m_deferredActions;
```

#### `xbmc/Application.cpp`

**2. Change the deferral push in `ExecuteXBMCAction`:**

```cpp
// Before (actual code — BUGGY):
bool CApplication::ExecuteXBMCAction(std::string actionStr, const CGUIListItemPtr &item)
{
  const std::string in_actionStr(actionStr);
  if (item)
    actionStr = GetItemLabel(actionStr, item.get());  // ← resolves at boot (context not ready)
  else
    actionStr = GetLabel(actionStr);

  if (m_bInitializing) {
    m_deferredActions.push_back(actionStr);   // ← stores baked string; in_actionStr and item lost
    return true;
  }
  // ...
}

// After (FIXED):
bool CApplication::ExecuteXBMCAction(std::string actionStr, const CGUIListItemPtr &item)
{
  const std::string in_actionStr(actionStr);

  if (m_bInitializing) {
    // Store original string + item; resolution deferred to flush time when GUI context is ready.
    m_deferredActions.push_back({in_actionStr, item});
    return true;
  }

  // Resolution now only happens for immediate (non-deferred) execution:
  if (item)
    actionStr = GetItemLabel(actionStr, item.get());
  else
    actionStr = GetLabel(actionStr);
  // ...
}
```

**3. Change the replay loop (wherever deferred actions are flushed):**

```cpp
// Before (BUGGY):
for (const auto& action : m_deferredActions)
  ExecuteXBMCAction(action);
m_deferredActions.clear();

// After (FIXED):
for (const auto& deferred : m_deferredActions)
  ExecuteXBMCAction(deferred.actionStr, deferred.item);
m_deferredActions.clear();
```

---

## 6. Pseudocode / Diff Sketch

```diff
--- a/xbmc/Application.h
+++ b/xbmc/Application.h
@@ -430,7 +430,14 @@ private:
   bool m_bInitializing = true;
   bool m_bPlatformDirectories = true;

-  std::deque<std::string> m_deferredActions;
+  struct DeferredAction
+  {
+    std::string actionStr;   // original unresolved string (in_actionStr)
+    CGUIListItemPtr item;
+  };
+  std::deque<DeferredAction> m_deferredActions;

--- a/xbmc/Application.cpp
+++ b/xbmc/Application.cpp
@@ -ExecuteXBMCAction — move deferral check before label resolution
+  if (m_bInitializing)
+  {
+    m_deferredActions.push_back({in_actionStr, item});  // ← defer with original string + item
+    return true;
+  }
+
   // label resolution now only for immediate (non-deferred) execution:
   if (item)
     actionStr = GetItemLabel(actionStr, item.get());
   else
     actionStr = GetLabel(actionStr);

-  if (m_bInitializing)
-  {
-    m_deferredActions.push_back(actionStr);   // ← was: baked string, item lost
-    return true;
-  }

@@ -replay flush
-  for (const auto& action : m_deferredActions)
-    ExecuteXBMCAction(action);
+  for (const auto& deferred : m_deferredActions)
+    ExecuteXBMCAction(deferred.actionStr, deferred.item);  // resolves at flush time
   m_deferredActions.clear();
```

---

## 7. Edge Cases

### 7a. Item Is a Plugin Item (`plugin://`)
- Plugin items carry their resolved plugin URL in `GetPath()`.  
- If `actionStr` contains `$INFO[ListItem.FileNameAndPath]`, it resolves to `plugin://...` — **must** be resolved with item context.  
- Without the item, `GetLabel()` would return the _focused_ item's path or empty, silently playing/navigating to the wrong plugin.

### 7b. Item Has PVR Tags
- PVR items store channel, recording, EPG, and timer tags.  
- Actions like `PlayPVRChannelOnLastActivePlayer(...)` or `PVR.SearchMissingChannelIcons` with `$INFO[ListItem.PVRChannelName]` need the PVR item context.  
- At replay time (`m_bInitializing == false`), the PVR manager is up, but the PVR info tag is on the _original_ item — if item is dropped, resolution falls back to the currently focused item in the PVR guide, which may be completely different.

### 7c. Item Has Custom `node.target` / `node.target_url`
- `DirectoryProvider::OnClick` reads these at click time and bakes them into the `execute` string via `GetExecutePath`.  
- For this specific path, the item properties are already consumed before the `GUI_MSG_EXECUTE` is sent — the message item is `nullptr`.  
- **Not directly affected** by the deferral regression, but tests should confirm `GetExecutePath` always returns a fully-resolved, item-free string.

### 7d. Item Is `nullptr` from the Caller
- `ExecuteXBMCAction` already handles `item == nullptr` correctly at lines 3971–3974.  
- Deferred storage of a null `item` is safe; replay is identical to the original call.

### 7e. Item Is Deleted Before Replay
- `CGUIListItemPtr` is `std::shared_ptr<CGUIListItem>`. Storing a copy of the `shared_ptr` in `DeferredAction` extends the item's lifetime until the deferred list is flushed.  
- No dangling pointer risk.

### 7f. Long Deferral Window (Very Slow Boot)
- If boot takes longer than expected (e.g., slow network mount, large library scan), the item's properties reflect the state at click time, not at replay time. This is the **correct** behavior — the user clicked on _that_ item.

### 7g. Multiple Deferred Actions for the Same Item
- Each `DeferredAction` holds an independent `shared_ptr` copy. Multiple deferred actions against the same item are safe; each holds its own ref.

---

## 8. Test Scenarios

### T1 — Basic Deferral with Item (CGUIAction path)
1. Force `m_bInitializing = true`.  
2. Construct a `CGUIListItemPtr` with `FileNameAndPath = "smb://server/movie.mkv"`.  
3. Send `GUI_MSG_EXECUTE` with action `"PlayMedia($INFO[ListItem.FileNameAndPath])"` and the item.  
4. Verify action is deferred (not executed immediately).  
5. Set `m_bInitializing = false` and trigger the flush.  
6. Verify `PlayMedia("smb://server/movie.mkv")` is called.  
**Expected (fixed):** Correct path played.  
**Bug behavior:** `GetLabel("PlayMedia($INFO[ListItem.FileNameAndPath])")` resolves against focused item or returns empty → wrong/no playback.

### T2 — Deferral with nullptr Item
1. Send `GUI_MSG_EXECUTE` with a plain action string like `"ActivateWindow(Home)"` and no item.  
2. Defer and replay.  
3. Verify window activation fires correctly.  
**Expected:** No regression; null item path unchanged.

### T3 — DirectoryProvider::OnClick During Boot
1. Simulate OnClick with a CFileItem that has `node.target = "MyVideoNav"` and a custom URL.  
2. Verify the `execute` string from `GetExecutePath` is fully resolved.  
3. Defer and replay.  
4. Verify the navigation target is correct (no item needed at replay since string is pre-baked).

### T4 — PVR Channel Action During Boot
1. Create a PVR CFileItem with a specific channel.  
2. Send `GUI_MSG_EXECUTE` with action `"PlayPVRChannelOnLastActivePlayer($INFO[ListItem.Label])"` and the item.  
3. Defer.  
4. Change focused GUI item to a different channel.  
5. Flush deferred actions.  
6. Verify the _original_ channel is played, not the currently focused one.

### T5 — Plugin Item Action
1. Plugin CFileItem with path `"plugin://video.plugin/?action=play&id=123"`.  
2. Action: `"PlayMedia($INFO[ListItem.FileNameAndPath])"`.  
3. Defer and replay.  
4. Verify plugin URL `plugin://video.plugin/?action=play&id=123` is dispatched.

### T6 — Item Freed Between Deferral and Replay
1. Create a `CFileItemPtr`, send a deferred action with it.  
2. Release the original `CFileItemPtr` in the caller.  
3. Trigger replay.  
4. Verify the action executes correctly (shared_ptr in `DeferredAction` still holds the item).

---

## 9. Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Stored item mutated between defer and replay | Low | Medium | `shared_ptr` copies share state; if caller modifies the underlying CFileItem, replay uses modified state. Acceptable — reflects reality. |
| Increased memory usage during boot | Low | Low | Only items clicked during `m_bInitializing` window are retained. Boot is fast; few items deferred in practice. |
| `CGUIListItemPtr` not thread-safe to access from replay thread | Medium | High | Ensure replay always happens on the GUI thread (same thread as original defer). If the flush loop runs on a non-GUI thread, add a guard. |
| Ordering of deferred actions | Low | Medium | `std::deque` preserves insertion order; FIFO replay is correct. |
| Approach C (skip deferral for item-bearing actions) causes crash if services not ready | High (if chosen) | High | Approach A avoids this entirely by always deferring and replaying in order. |
| `DeferredAction` struct ABI break if other TUs cache the type | None | None | `DeferredAction` is private to `CApplication`. |
| Pre-baked strings in DirectoryProvider already correct | Verified | None | Node.target and node.target_url are resolved before `GUI_MSG_EXECUTE`; no item needed at replay for that path. |

### Overall Risk of the Fix (Approach A)
**Low.** The change is surgical: two lines changed in `.cpp`, one struct + one field change in `.h`. No new threading, no new lifetime management beyond what `shared_ptr` already provides. The fix restores the original contract: "replay an action exactly as if the user had clicked during normal operation."
