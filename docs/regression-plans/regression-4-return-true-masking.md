# Regression 4 — `return true` Silently Swallows Nyxboard Key Events

**Repository:** OrganizerRo/xbmc  
**Date:** 2024  
**Severity:** Medium — physical key events from Nyxboard (and any other peripheral that checks `ExecuteXBMCAction`'s return value) are silently discarded when fired during boot; the action is deferred but the peripheral layer treats the `true` return as "event consumed and handled", zeroing out the key event before the action ever runs.

---

## 1. Root Cause Analysis

### The Deferral Bug

`CApplication::ExecuteXBMCAction` returns `true` from the deferral branch. In the actual code, the `return true` is at **Application.cpp:3994** (confirmed as of current Leia HEAD — this fix has **not yet been applied**):

```cpp
// Application.cpp — actual code at 3977–3995 (UNFIXED)
bool CApplication::ExecuteXBMCAction(std::string actionStr, const CGUIListItemPtr &item)
{
  const std::string in_actionStr(actionStr);
  if (item)
    actionStr = GetItemLabel(actionStr, item.get());
  else
    actionStr = GetLabel(actionStr);

  if (m_bInitializing) {
    m_deferredActions.push_back(actionStr);
    return true;   // ← "success" — but the action has NOT run yet; fix pending
  }
  // ...
}
```

> **Implementation status:** The `return false` change has **not** been applied to the codebase yet. The above shows the current state of line 3994.

### The Nyxboard Caller

`CPeripheralNyxboard::LookupSymAndUnicode` is the only peripheral caller that checks this return value. When `ExecuteXBMCAction` returns `true`, the Nyxboard driver zeros out the physical key event (`sym = 0`, `unicode = 0`) before it propagates further:

```cpp
// peripherals/devices/PeripheralNyxboard.cpp (simplified)
bool CPeripheralNyxboard::LookupSymAndUnicode(SDL_keysym &keysym, uint16_t *sym, std::string *strUnicode)
{
  // ... map the physical key to an XBMC action string ...
  if (g_application.ExecuteXBMCAction(actionStr))
  {
    // ExecuteXBMCAction returned true → treat as "handled"
    *sym = 0;          // ← zero out: key event will be ignored by everything above
    *strUnicode = "";
    return true;
  }
  return false;
}
```

When `m_bInitializing == true`:
1. `ExecuteXBMCAction` pushes the action string to the deferred queue and returns `true`.
2. `LookupSymAndUnicode` zeroes `sym`/`unicode` and returns `true`.
3. The physical key event (e.g., F7 Nyxboard key) is lost from the input pipeline.
4. At flush time, the deferred action runs — but with `item = nullptr` and without any key event context, so any action that relies on the physical key state is incorrect.

### The Subtle Part

For most GUI callers, the return value of `ExecuteXBMCAction` is discarded — they do not care whether the action ran immediately or was deferred. But `LookupSymAndUnicode` uses `true` as a gate to suppress the hardware key event. This contract is broken by the deferral: returning `true` before the action has run causes the key event suppression to fire prematurely.

### Key File:Line References

| Location | Significance |
|----------|-------------|
| `xbmc/Application.cpp:3977–4013` | `ExecuteXBMCAction` — deferral at line 3990–3994, main body below |
| `xbmc/Application.h:290` | `bool ExecuteXBMCAction(std::string, const CGUIListItemPtr&)` |
| `xbmc/peripherals/devices/PeripheralNyxboard.cpp:41` | `LookupSymAndUnicode` — only caller checking return value |
| `xbmc/Application.h:433` | `bool m_bInitializing = true;` |

---

## 2. Exact Call Path

```
Physical key press on Nyxboard F7
  → SDL event loop
      → CPeripheralNyxboard::LookupSymAndUnicode(keysym, &sym, &strUnicode)
          → actionStr = MapNyxboardKey(keysym)   // e.g., "VolumeUp"
          → if (g_application.ExecuteXBMCAction(actionStr))
                ↓ m_bInitializing == true
                m_deferredActions.push_back("VolumeUp")
                return true                       ← premature "success"
          → *sym = 0; *strUnicode = "";           ← key event zeroed
          → return true
  → SDL key event with sym=0 propagates upward → ignored by all handlers
  → [at flush time] ExecuteXBMCAction("VolumeUp") runs with item=nullptr
     ← correct action, but key-state context (e.g., keypress held?) is gone
```

---

## 3. Candidate Fix Approaches

### Approach A — Return `false` from the Deferral Branch (Recommended)

The contract of the return value is: `true` means "action was executed (or was a no-op that should suppress the key)", `false` means "action was not handled (let the key event through)". Deferring is neither executing nor suppressing — the correct return is `false`.

```diff
 bool CApplication::ExecuteXBMCAction(std::string actionStr, const CGUIListItemPtr &item)
 {
   if (m_bInitializing) {
     m_deferredActions.push_back({actionStr, item});
-    return true;
+    return false;   // action deferred, not yet executed; don't suppress key event
   }
   // ...
 }
```

**Effect on Nyxboard:**
- `LookupSymAndUnicode` receives `false` → does not zero `sym`/`unicode` → key event propagates normally → handled at the correct point in the input pipeline.
- The deferred action still fires at flush time (correct behaviour).
- Double-handling risk: the key event propagates AND the deferred action runs. For most actions (VolumeUp, mute, etc.) this is harmless — the key event is handled by normal input dispatch, and the deferred action also fires at flush. To avoid double-handling, the deferred entry should be **removed** if the key event is handled by a downstream handler; however, this is complex. A simpler guard: only defer if the key event will not be otherwise dispatched (see Approach B).

**Effect on all other GUI callers:**
- All other callers discard the return value → no behavioural change.

---

### Approach B — Don't Defer Peripheral/Key Actions at All

Detect that the action originates from a peripheral key event (pass a `bool fromKeyEvent` parameter through `ExecuteXBMCAction`) and bypass deferral for key-event-sourced actions.

**Pros:** Clean separation of key-event actions from skin/GUI actions.  
**Cons:** API change; wider blast radius; all callers must be updated.

---

### Approach C — Defer but Notify the Caller

Return a new enum value or out-param `ActionState { kExecuted, kDeferred, kFailed }` so callers can distinguish "deferred" from "executed". `LookupSymAndUnicode` can then choose not to zero the key event when the result is `kDeferred`.

**Pros:** Most semantically precise.  
**Cons:** Requires changing the function signature and all callers.

---

## 4. Recommended Fix — Approach A (One-Line Change)

### Code Change

#### `xbmc/Application.cpp`

> **Dependency note:** The diff below shows `m_deferredActions.push_back({actionStr, item})` which assumes R3's `DeferredAction` struct is already applied. If R3 has **not** been applied yet, the push remains `m_deferredActions.push_back(actionStr)` (or `push_back(in_actionStr)` after R3's resolution-order correction). The `return true` → `return false` change is **independent** of R3 and should be applied regardless.

```diff
 bool CApplication::ExecuteXBMCAction(std::string actionStr, const CGUIListItemPtr &item)
 {
   if (m_bInitializing)
   {
     m_deferredActions.push_back({actionStr, item});  // requires R3 struct; else push_back(actionStr)
-    return true;
+    return false;
   }
```

This is a **one-line change**. All GUI-path callers (`CGUIAction::ExecuteActions`, `CApplication::OnMessage`, etc.) discard the return value, so they are unaffected. Only `LookupSymAndUnicode` checks the return value, and the new `false` correctly tells it "the action was not handled — pass the key event through".

### Double-Handling Analysis

When `false` is returned during boot:
- The Nyxboard key event propagates normally → processed by the normal input stack.
- At flush time the deferred action also fires.

For **volume/playback actions** (most common Nyxboard keys): the normal input stack will handle the key event and execute the action immediately. At flush time, a second execution fires. For idempotent actions like `VolumeUp`, this means the volume increases by two steps instead of one — acceptable for a brief boot window.

If double-handling is unacceptable, the deferred entry can be skipped at flush if an identical action was already dispatched:

```cpp
// At flush time, only execute if not already handled:
for (const auto& deferred : m_deferredActions)
{
  // Skip if this action was already dispatched by the normal input pipeline:
  if (!m_handledDuringBoot.count(deferred.actionStr))
    ExecuteXBMCAction(deferred.actionStr, deferred.item);
}
```

This requires a `std::unordered_set<std::string> m_handledDuringBoot` populated by the normal input dispatcher. For the initial fix, this level of deduplication is not necessary.

---

## 5. Pseudocode / Diff Sketch

```diff
--- a/xbmc/Application.cpp
+++ b/xbmc/Application.cpp
@@ -ExecuteXBMCAction deferral branch (line 3990–3994 in current Leia HEAD)
   if (m_bInitializing)
   {
-    m_deferredActions.push_back(actionStr);     // without R3: string only
+    m_deferredActions.push_back({actionStr, item});  // with R3 struct; else keep push_back(actionStr)
-    return true;
+    return false;   // deferred ≠ executed; do not suppress peripheral key events
   }
```

> **Implementation status (current HEAD):** The deferral branch at line 3994 still returns `true`. This fix has **not yet been applied** and is pending implementation.

---

## 6. Edge Cases

### 6a. Callers That Rely on `return true` During Boot as a Guard
If any caller (besides `LookupSymAndUnicode`) uses `return true` to suppress a key/input event during boot, changing to `false` will let those events through. Audit of all callers:

| Caller | Uses return value? | Safe to change? |
|--------|-------------------|-----------------|
| `CApplication::OnMessage (GUI_MSG_EXECUTE)` | No — return discarded | Safe |
| `CGUIAction::ExecuteActions` | No — return discarded | Safe |
| `CPeripheralNyxboard::LookupSymAndUnicode` | Yes | **This is the fix target** |
| Python/script callers | No | Safe |

Only one caller checks the return value; the change is safe.

### 6b. Nyxboard Key Held Down During Boot
Held-key repeat events fire `LookupSymAndUnicode` repeatedly. Each call now returns `false` → key event propagates → each repeat is handled normally. The deferred queue accumulates one entry per repeat. At flush, all deferred entries fire. For volume, this may be excessive. The deduplication guard (Section 4) mitigates this.

### 6c. Action That Must Only Fire Once
If a deferred action is one-shot (e.g., `ActivateWindow(LoginScreen)`) and the normal key event also activates the same window, the window opens twice. This is unlikely for Nyxboard keys but worth noting. The R1 fix (bypass deferral for window-navigation actions) prevents this specific scenario from entering the deferred queue at all.

### 6d. Compatibility With R3 DeferredAction Struct and Resolution Order
The `{actionStr, item}` push in the diff assumes the R3 struct is in place. If R3 is not yet applied, revert the push to `push_back(actionStr)` but still change `return true` to `return false`. The two fixes are independent.

Additionally, when R3's resolution-order correction is applied (deferral check moved to before `GetItemLabel`/`GetLabel`), the push should use `in_actionStr` (the original unresolved string), not the post-resolution `actionStr`. This is a deliberate part of R3 and is not required for this R4 fix to be correct in isolation.

---

## 7. Test Scenarios

### T1 — Nyxboard Key During Boot: Key Event Not Suppressed
1. Set `m_bInitializing = true`.
2. Simulate Nyxboard F7 key press → `LookupSymAndUnicode` called.
3. Verify `ExecuteXBMCAction` returns `false`.
4. Verify `*sym` and `*strUnicode` are **not** zeroed.
5. Verify key event propagates to normal input handlers.

### T2 — Deferred Nyxboard Action Fires at Flush
1. Set `m_bInitializing = true`.
2. Simulate Nyxboard VolumeUp key.
3. Verify action is pushed to `m_deferredActions`.
4. Set `m_bInitializing = false`, flush.
5. Verify `VolumeUp` action fires.

### T3 — Normal Boot (No Nyxboard): Behaviour Unchanged
1. Boot without a Nyxboard peripheral.
2. Verify all GUI actions deferred and flushed as before.
3. Verify no visible change in skin behaviour.

### T4 — Non-Peripheral `ExecuteXBMCAction` Return Value Ignored
1. Set `m_bInitializing = true`.
2. Send `GUI_MSG_EXECUTE` from a GUI action.
3. Verify the return value is not checked by the caller (regression test for API contract).

---

## 8. Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Double-handling of idempotent actions | Medium | Low | Acceptable; can add dedup guard if needed |
| Other callers broken by `return false` | Very low | High | Audit shows only Nyxboard checks return value |
| Volume changes by 2 steps during boot | Low | Very low | Boot window is brief; Nyxboard not typically active at boot |
| Deferred action ordering affected | None | None | Push/flush order unchanged |

### Overall Risk
**Very Low.** This is a one-line change. The only caller that checks the return value is `LookupSymAndUnicode`, and `false` is the semantically correct return for "action deferred, not yet executed." All GUI-path callers discard the return value.
