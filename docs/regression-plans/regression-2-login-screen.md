# Regression 2 — Login-Screen Deferred Queue Flushed in Wrong Profile Context

**Repository:** OrganizerRo/xbmc  
**Date:** 2024  
**Severity:** High — deferred actions execute inside a newly-loaded user profile's language/skin context instead of the master-profile context; causes skin/language mismatch, repeated ReloadSkin on every logout, and potential crashes on multi-profile setups.

---

## 1. Root Cause Analysis

### The Two Bugs

This regression has two related but independent defects introduced by the deferral PR:

#### Bug 2a — Deferred queue flushed after profile context switch, not before it

`CApplication::Initialize()` has two exit paths:

1. **Normal path (no login screen):** Boot completes normally → `CProfileManager::FinalizeLoadProfile()` sends `GUI_MSG_UI_READY` with sender `WINDOW_SETTINGS_PROFILES` → `CApplication::OnMessage()` receives it → sets `m_bInitializing = false` at line 3759 → flushes deferred queue at lines 3763–3772. At this point **no profile switch has occurred**, so the flush happens in the correct master-profile context.

2. **Login-screen path:** Boot shows the login screen instead. The user's skin `<onload>` actions and other early actions are deferred into `m_deferredActions`. When the user enters their PIN, `TMSG_LOADPROFILE` is posted → `CProfileManager::LoadProfile(N)` is called → it calls `settings->Load()` (switching the active settings to profile N's settings file), `LoadLanguage()`, and other profile-specific initialization → **only then** `FinalizeLoadProfile()` sends `GUI_MSG_UI_READY` → `CApplication::OnMessage()` sets `m_bInitializing = false` and flushes the deferred queue. By this point the active language, skin, and settings context are **already those of profile N**, not the master profile. All deferred actions execute with the wrong context.

```
[Boot, master profile — m_bInitializing = true]
  → LoginScreen shown
      → skin <onload> fires "PlayMedia(...)", "$INFO[...]" actions
      → all deferred into m_deferredActions (Application.cpp:3993)

[User enters PIN → TMSG_LOADPROFILE posted]
  → CApplication::ProcessMessages()                    (Application.cpp:2272)
      → CProfileManager::LoadProfile(N)               (ProfileManager.cpp:283)
          → settings->Load()                          (ProfileManager.cpp:323)  ← settings switch
          → FinalizeLoadProfile()                     (ProfileManager.cpp:373)
              → LoadLanguage()                        (ProfileManager.cpp:407)  ← language switch
              → ...other profile init...
              → SendThreadMessage(GUI_MSG_UI_READY)   (ProfileManager.cpp:441)

[GUI_MSG_UI_READY received — CApplication::OnMessage()]  (Application.cpp:3759)
  → m_bInitializing = false
  → FlushDeferredActions()
      → ExecuteXBMCAction(action, nullptr)
          ← $INFO tokens resolved against profile N's language/skin  ← WRONG
```

#### Bug 2b — `ReloadSkin` fired on every profile load regardless of whether the skin changed

In `CApplication::OnMessage()` (Application.cpp:3774–3775), when `GUI_MSG_UI_READY` arrives with sender `WINDOW_SETTINGS_PROFILES`, `ReloadSkin()` is called unconditionally. This fires even when the master profile's skin and the new profile's skin are the same (e.g., both use `skin.estouchy`), causing:
- A visible skin flash and wasted re-initialisation on every profile switch or logout.
- If the deferred queue also contains a `ReloadSkin` action (e.g., from the master skin's `<onload>`), two successive `ReloadSkin` calls fire back-to-back.

`ReloadSkin()` (Application.cpp:1144–1155) already reads `CSettings::SETTING_LOOKANDFEEL_SKIN` and compares it internally to load the new skin — but because it is called unconditionally, it always performs the full reload cycle even for a no-op switch.

### Key File:Line References

| Location | What it does |
|----------|-------------|
| `xbmc/Application.h:433` | `bool m_bInitializing = true;` — flag declaration |
| `xbmc/Application.h:482` | `std::deque<std::string> m_deferredActions;` — the deferred queue (currently stores strings only; see R3 for the item-preserving struct upgrade) |
| `xbmc/Application.cpp:3759` | `m_bInitializing = false;` — cleared on `GUI_MSG_UI_READY` |
| `xbmc/Application.cpp:3763–3772` | Deferred queue flush loop — replay with `nullptr` item |
| `xbmc/Application.cpp:3774–3775` | `if (sender == WINDOW_SETTINGS_PROFILES) ReloadSkin(false);` — unconditional skin reload |
| `xbmc/Application.cpp:2272–2277` | `TMSG_LOADPROFILE` handler — calls `CProfileManager::LoadProfile()` |
| `xbmc/Application.cpp:1144–1155` | `CApplication::ReloadSkin()` — reads new skin from settings, loads it |
| `xbmc/profiles/ProfileManager.cpp:283` | `CProfileManager::LoadProfile()` — calls `settings->Load()` then `FinalizeLoadProfile()` |
| `xbmc/profiles/ProfileManager.cpp:323` | `settings->Load()` — first point where the profile context switches |
| `xbmc/profiles/ProfileManager.cpp:373` | `FinalizeLoadProfile()` call — profile settings already active here |
| `xbmc/profiles/ProfileManager.cpp:380` | `CProfileManager::FinalizeLoadProfile()` — loads language, starts services, sends `GUI_MSG_UI_READY` |
| `xbmc/profiles/ProfileManager.cpp:407` | `g_application.LoadLanguage(true)` inside `FinalizeLoadProfile()` |
| `xbmc/profiles/ProfileManager.cpp:441` | `SendThreadMessage(GUI_MSG_UI_READY)` — triggers flush in Application |
| `xbmc/profiles/ProfileManager.cpp:445` | `CProfileManager::LogOff()` — calls `LoadMasterProfileForLogin()` then activates login window |

---

## 2. Exact Call Path (Annotated with Line Numbers)

### 2a — Login-Screen Path: Wrong Flush Context

```
Application.cpp:2272  TMSG_LOADPROFILE
  → ProfileManager.cpp:283   CProfileManager::LoadProfile(N)
      → ProfileManager.cpp:285   PrepareLoadProfile(N)   [stops services]
      → ProfileManager.cpp:323   settings->Load()         ← CONTEXT SWITCH 1: settings
      → ProfileManager.cpp:373   FinalizeLoadProfile()
          → ProfileManager.cpp:407   LoadLanguage(true)   ← CONTEXT SWITCH 2: language
          → ...
          → ProfileManager.cpp:441   SendThreadMessage(GUI_MSG_UI_READY, WINDOW_SETTINGS_PROFILES)

Application.cpp:3759  OnMessage(GUI_MSG_UI_READY)
  → m_bInitializing = false
  → Application.cpp:3763  flush loop
      → Application.cpp:3770  ExecuteXBMCAction(action, nullptr)
          ← ALL $INFO resolution uses profile N context  ← BUG
```

### 2b — Unconditional ReloadSkin

```
Application.cpp:3774  OnMessage(GUI_MSG_UI_READY) — same handler, continues:
  if (message.GetSenderId() == WINDOW_SETTINGS_PROFILES)
    g_application.ReloadSkin(false);   ← fires even if skin unchanged
```

---

## 3. Decision: Flush, Not Discard

**The deferred queue must be flushed (executed), not discarded.**

Actions queued during the login screen (e.g., `PlayMedia` for the master skin's background video, `$INFO[System.BuildVersion]` overlays) belong to the master-profile context and are valid user-visible operations. Discarding them silently would break the master skin's expected startup behaviour every time a login screen is shown.

The fix therefore flushes the queue **before** `settings->Load()` switches the active context.

---

## 4. Implementation Plan

### Step 1 — Add `FlushDeferredActionsNow()` to `CApplication`

`CProfileManager` needs to trigger the flush from `LoadProfile()`, but `m_deferredActions` and `m_bInitializing` are private members of `CApplication`. Add a single public method to expose this safely.

#### `xbmc/Application.h` — public section

```diff
+  // Flush any deferred startup actions immediately in the current context.
+  // Called by CProfileManager::LoadProfile() before switching profile context.
+  void FlushDeferredActionsNow();
```

#### `xbmc/Application.cpp`

Add the new method (can go near the existing flush logic around line 3763):

```diff
+void CApplication::FlushDeferredActionsNow()
+{
+  if (m_bInitializing && !m_deferredActions.empty())
+  {
+    m_bInitializing = false;
+    std::deque<std::string> pending;
+    pending.swap(m_deferredActions);
+    for (const auto& action : pending)
+    {
+      CLog::LogF(LOGDEBUG, "Replaying deferred startup action '%s' (pre-profile-switch)", action.c_str());
+      ExecuteXBMCAction(action, nullptr);
+    }
+  }
+  // Always clear the initializing flag so subsequent actions are not deferred.
+  m_bInitializing = false;
+}
```

> **Note on item context:** If R3 (item-context struct upgrade) is applied first, change the queue type to `std::deque<DeferredAction>` and call `ExecuteXBMCAction(deferred.actionStr, deferred.item)`. See R3 plan for details.

---

### Step 2 — Call `FlushDeferredActionsNow()` Before the Settings Context Switch

The context switch starts at `settings->Load()` (ProfileManager.cpp:323). Call the flush just before that line, inside `CProfileManager::LoadProfile()`.

#### `xbmc/profiles/ProfileManager.cpp` — `CProfileManager::LoadProfile()`

```diff
   // save any settings of the currently used skin but only if the (master)
   // profile hasn't just been loaded as a temporary profile for login
   if (g_SkinInfo != nullptr && !m_previousProfileLoadedForLogin)
     g_SkinInfo->SaveSettings();

   // @todo: why is m_settings not used here?
   const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();

+  // Flush any deferred boot actions NOW, while still in master-profile context.
+  // settings->Load() below switches the active profile; any $INFO tokens in
+  // deferred actions must be resolved before that context switch occurs.
+  g_application.FlushDeferredActionsNow();
+
   // unload any old settings
   settings->Unload();

   SetCurrentProfileId(index);
   m_previousProfileLoadedForLogin = false;

   // load the new settings
   if (!settings->Load())
```

The insertion point is after `g_SkinInfo->SaveSettings()` and before `settings->Unload()` / `settings->Load()`. At this point:
- `g_SkinInfo` still points to the master skin ✓
- `g_localizeStrings` still contains the master language ✓
- `CServiceBroker::GetGUI()->GetInfoManager()` still resolves against master profile ✓

---

### Step 3 — Guard the `OnMessage` Flush Against Double-Execution

`CApplication::OnMessage()` at line 3759 already sets `m_bInitializing = false` and flushes the queue when `GUI_MSG_UI_READY` arrives. After Step 1 the queue will already be empty and `m_bInitializing` already `false` for the login-screen path, so this code becomes a no-op. No change is required here — the existing guard `if (!m_deferredActions.empty())` naturally handles this. The normal-boot path (no login screen) is also unaffected because `FlushDeferredActionsNow()` is never called on that path.

---

### Step 4 — Gate `ReloadSkin` on Skin ID Change

#### `xbmc/Application.cpp` — `OnMessage()` around line 3774

```diff
-      if (message.GetSenderId() == WINDOW_SETTINGS_PROFILES)
-        g_application.ReloadSkin(false);
+      if (message.GetSenderId() == WINDOW_SETTINGS_PROFILES)
+      {
+        // Only reload the skin if the newly-active profile's skin differs from
+        // the currently-loaded skin.  Avoids redundant flash on same-skin switches.
+        const std::shared_ptr<CSettings> settings =
+            CServiceBroker::GetSettingsComponent()->GetSettings();
+        const std::string newSkinId = settings->GetString(CSettings::SETTING_LOOKANDFEEL_SKIN);
+        if (!g_SkinInfo || newSkinId != g_SkinInfo->ID())
+          g_application.ReloadSkin(false);
+      }
```

`settings->GetString(CSettings::SETTING_LOOKANDFEEL_SKIN)` returns the value from the currently-loaded settings file, which is the new profile's settings by the time `GUI_MSG_UI_READY` is processed. `g_SkinInfo->ID()` is the skin that is currently rendering. If they differ, `ReloadSkin()` must be called; if they are the same, the reload is skipped.

---

## 5. Complete Change Summary

| File | Location | Change |
|------|----------|--------|
| `xbmc/Application.h` | public section | Add `void FlushDeferredActionsNow();` declaration |
| `xbmc/Application.cpp` | near line 3763 | Add `FlushDeferredActionsNow()` implementation |
| `xbmc/profiles/ProfileManager.cpp` | line ~315, before `settings->Unload()` | Call `g_application.FlushDeferredActionsNow()` |
| `xbmc/Application.cpp` | line 3774–3775 | Gate `ReloadSkin(false)` behind skin-ID comparison |

No CMake changes required. No new headers. No platform guards. Total new code: ~20 lines across 4 touch points.

---

## 6. Edge Cases

### 6a. User Cancels Login
Pressing Back on the login screen does not trigger `TMSG_LOADPROFILE` or `LoadProfile()`, so `FlushDeferredActionsNow()` is not called. `m_bInitializing` stays `true`. Deferred actions accumulate and will flush on the next `GUI_MSG_UI_READY` — either on the next login attempt or on normal shutdown recovery. This matches pre-regression behaviour.

### 6b. Multiple Profiles With the Same Skin
`newSkinId == g_SkinInfo->ID()` → `ReloadSkin()` skipped → no flash. Correct.

### 6c. First Boot (No Login Screen)
`FlushDeferredActionsNow()` is never called. `m_bInitializing = false` fires at line 3759 via `GUI_MSG_UI_READY` on the normal path. Deferred queue is flushed at lines 3763–3772 as before. Zero behaviour change.

### 6d. Guest / Kiosk Profile (No Custom Skin)
Kiosk profile uses the same skin as master → `newSkinId == g_SkinInfo->ID()` → no reload. Correct.

### 6e. Deferred Queue Contains a `ReloadSkin` Action
A queued `ReloadSkin` action (e.g., fired by the master skin at startup) executes in master-profile context (before the switch). Then when `GUI_MSG_UI_READY` arrives, the skin-ID check determines whether a second `ReloadSkin` is needed. If the new profile's skin differs, exactly one more reload fires (justified). If same skin, no second reload. Net result: at most two `ReloadSkin` calls total and both are justified.

### 6f. `LoadProfile` Error Path
If `settings->Load()` returns `false` (line 325–328 in ProfileManager.cpp), `LoadProfile()` returns `false` before `FinalizeLoadProfile()` is called. By this point `FlushDeferredActionsNow()` has already run and `m_deferredActions` is empty, so no further action is needed. `m_bInitializing` is now `false`, which is correct — the failed load leaves Kodi partially initialised but further input should no longer be deferred.

### 6g. Interaction with R3 (Item-Context Struct Upgrade)
R3 changes `m_deferredActions` from `std::deque<std::string>` to `std::deque<DeferredAction>`. If R3 is applied first, update `FlushDeferredActionsNow()` to iterate over `DeferredAction` structs and pass `deferred.item` to `ExecuteXBMCAction`. The calling convention is otherwise identical.

---

## 7. Test Scenarios

### T1 — Login Screen → Profile Switch → Deferred Actions Execute in Master Context
1. Configure Kodi with a login screen (Settings → Profiles → Show login screen at startup).
2. Add a visible `<onload>PlayMedia($INFO[Profile.Name]-bg.mp4)</onload>` to the master skin's `LoginScreen.xml`.
3. Boot Kodi and wait for the login screen.
4. Log in as profile 1 which has a different profile name.
5. **Expected:** The `PlayMedia` action resolves the `$INFO[Profile.Name]` token to the master profile's name (i.e., the name that was active at the time of deferral), **not** profile 1's name.
6. Check the Kodi debug log for `Replaying deferred startup action` — it must appear before the `LoadProfile` context-switch log entries.

### T2 — No Double `ReloadSkin` on LogOff (Same Skin)
1. Configure two profiles, both using `skin.estouchy`.
2. Log in as profile 1.
3. Log off back to master.
4. **Expected:** `ReloadSkin()` fires zero times on logout (both profiles share the same skin ID).
5. Verify in debug log: no `Loading skin` log entry during the logout sequence.

### T3 — Exactly One `ReloadSkin` on Profile Switch (Different Skins)
1. Configure profile 1 with `skin.estouchy`, profile 2 with `skin.confluence`.
2. Switch from profile 1 to profile 2.
3. **Expected:** Exactly one `ReloadSkin()` call — confirmed by a single `Loading skin 'skin.confluence'` log entry.

### T4 — Single-Profile Setup (No Login Screen) Unaffected
1. Boot with no login screen (default Kodi single-user setup).
2. **Expected:** Deferred actions flush via the existing `GUI_MSG_UI_READY` path at Application.cpp:3763.
3. Verify in debug log: `Replaying deferred startup action` entries appear, behaviour identical to pre-regression.

### T5 — Cancelled Login Does Not Lose Deferred Actions
1. Show the login screen.
2. Press Back to cancel without logging in.
3. Log in on the second attempt.
4. **Expected:** All deferred actions from step 1 (pre-cancellation) are replayed correctly at step 3.

### T6 — `LoadProfile` Failure Does Not Deadlock
1. Simulate a settings-load failure for profile N (e.g., corrupt `guisettings.xml`).
2. **Expected:** `FlushDeferredActionsNow()` already ran; Kodi is no longer in the `m_bInitializing` state; UI remains responsive.

---

## 8. Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| `FlushDeferredActionsNow()` called on non-login path by accident | Very Low | Low | The call site is inside the `if (index >= m_profiles.size())` guard block in `LoadProfile` — only reachable on actual profile switches |
| Deferred actions execute before PVR/service restart in `FinalizeLoadProfile` | Low | Medium | Deferred actions fired at login screen are skin-level UI actions (PlayMedia, ActivateWindow), not service-dependent library calls; PVR is a separate service |
| Skin-ID comparison uses stale cached value | Very Low | Low | `settings->GetString(SETTING_LOOKANDFEEL_SKIN)` reads from the live settings object which is already loaded for the new profile at flush time |
| `g_SkinInfo` null when ReloadSkin guard executes | Very Low | None | Added `!g_SkinInfo` null check in the guard ensures safe fallback to calling `ReloadSkin()` |
| R3 not yet applied — `FlushDeferredActionsNow()` uses string-only queue | None | None | Functionally correct without R3; item context is a separate improvement |

### Overall Risk
**Low.** The fix is precisely targeted: one new public method in `CApplication`, one call site before `settings->Load()` in `CProfileManager::LoadProfile()`, and one conditional guard around the existing `ReloadSkin()` call. No architecture changes. The normal-boot path is completely untouched.
