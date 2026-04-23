# Regression 2 — Login-Screen Deferred Queue Flushed in Wrong Profile Context

**Repository:** OrganizerRo/xbmc  
**Date:** 2024  
**Severity:** High — deferred actions execute inside a newly-loaded user profile's language/skin context instead of the master-profile context; causes skin/language mismatch, repeated ReloadSkin on every logout, and potential crashes on multi-profile setups.

---

## 1. Root Cause Analysis

### The Two Bugs

This regression has two related but independent defects introduced by the deferral PR:

#### Bug 2a — Login-screen branch never clears `m_bInitializing`

`CApplication::Initialize()` has two exit paths:

1. **Normal path:** No login screen → the UI is brought up, `GUI_MSG_UI_READY` fires → `m_bInitializing = false` at line 3759.
2. **Login-screen path:** A login screen is presented. In this path the code exits `Initialize()` before `GUI_MSG_UI_READY` is processed by the normal codepath, and `m_bInitializing` is **never set to `false`**.

As a result:
- Any action fired during the login screen (e.g., skin `<onload>` actions, background refresh actions) is deferred indefinitely.
- When the user enters their PIN and `FinalizeLoadProfile()` is called, `m_bInitializing` is finally set to `false` and the deferred queue is flushed — but the current language, skin, and user context are now those of the newly-loaded profile, not the master profile context in which the actions were originally fired.

```
CApplication::Initialize()
  → LoginScreen shown
  → ... user enters PIN ...
  → FinalizeLoadProfile(profile N)
      → LoadSkin(profile_N_skin)        ← different skin/language loaded
      → m_bInitializing = false         ← BUG: set AFTER profile switch
      → FlushDeferredActions()          ← replays in wrong context
```

#### Bug 2b — `ReloadSkin` fired on every `LogOff`

`FinalizeLoadProfile()` unconditionally calls `ReloadSkin()` to apply the newly-loaded profile's skin settings. This fires even when logging out back to the master profile, causing:
- Flickering skin reload on every logout.
- If a deferred `ReloadSkin` is also in the queue (fired during boot in master context), two successive `ReloadSkin` calls fire back-to-back — visible as a double-flash and wasted initialisation.

### Key File:Line References

| Location | Significance |
|----------|-------------|
| `xbmc/Application.h:433` | `bool m_bInitializing = true;` |
| `xbmc/Application.cpp:3759` | `m_bInitializing = false;` — normal-path assignment |
| `xbmc/Application.cpp` | `FinalizeLoadProfile()` — sets `m_bInitializing = false` in login-screen path |
| `xbmc/profiles/ProfileManager.h` | `m_firstRealProfileLoad` — proposed new one-shot flag |
| `xbmc/profiles/ProfileManager.cpp` | `LogOff()` / `LoadProfile()` — lifecycle management |
| `xbmc/Application.cpp` | `ReloadSkin()` call inside `FinalizeLoadProfile()` |

---

## 2. Exact Call Path

### 2a — Deferred Queue Flushed in Wrong Context

```
[Boot, master profile]
  m_bInitializing = true
  → Show LoginScreen window
      → LoginScreen <onload> fires actions  → deferred into m_deferredActions

[User enters PIN]
  → FinalizeLoadProfile(profile 1)
      → CProfileManager::LoadProfile(1)
          → language/skin of profile 1 loaded  ← context switch
      → m_bInitializing = false
      → FlushDeferredActions()
          → ExecuteXBMCAction("...", item)
              ← resolved with profile 1's skin/language/info manager  ← WRONG
```

### 2b — Double ReloadSkin on LogOff

```
[User logs off]
  → CProfileManager::LogOff()
      → LoadProfile(MASTER_PROFILE_INDEX)
          → FinalizeLoadProfile()
              → ReloadSkin()               ← always fires
              → FlushDeferredActions()     ← may fire another ReloadSkin from queue
```

---

## 3. Candidate Fix Approaches

### Fix for Bug 2a — Set `m_bInitializing = false` BEFORE profile context switch

Move the `m_bInitializing = false` assignment to happen **immediately after the login screen is dismissed** but **before** `FinalizeLoadProfile` switches the skin/language context. This ensures deferred actions execute in the master-profile context that was active when they were fired.

Alternatively, clear the deferred queue entirely when a login-screen transition is detected (actions fired at the login screen are not relevant to the new profile's context anyway).

### Fix for Bug 2b — Gate `ReloadSkin` with a one-shot flag in `CProfileManager`

Add a `bool m_firstRealProfileLoad = true` flag to `CProfileManager`. `FinalizeLoadProfile` only calls `ReloadSkin()` if `m_firstRealProfileLoad` is `true`. After the first real profile load, clear the flag. On `LogOff`, do not reset the flag — subsequent profile switches only reload the skin if the skin settings actually differ.

---

## 4. Recommended Fixes

### Fix 2a — Flush or Discard Deferred Queue Before Profile Switch

#### `xbmc/Application.cpp` — `FinalizeLoadProfile`

```diff
 void CApplication::FinalizeLoadProfile(int profileIndex)
 {
+  // Flush (or discard) any deferred boot actions BEFORE switching profile context.
+  // Actions fired during the login screen belong to the master profile context.
+  if (!m_deferredActions.empty())
+  {
+    for (const auto& deferred : m_deferredActions)
+      ExecuteXBMCAction(deferred.actionStr, deferred.item);
+    m_deferredActions.clear();
+  }
+  m_bInitializing = false;
+
   CProfileManager::GetInstance().LoadProfile(profileIndex);
   // ... skin/language load ...
-  m_bInitializing = false;
-  FlushDeferredActions();
 }
```

**Note:** If the login-screen actions are entirely irrelevant to the new user context (e.g., skin background start-up actions for the master skin), it is also acceptable to **discard** rather than flush:

```diff
+  m_deferredActions.clear();   // discard; login-screen context no longer valid
+  m_bInitializing = false;
   CProfileManager::GetInstance().LoadProfile(profileIndex);
```

The safer approach is to flush in the correct context; the discard approach is acceptable if login-screen actions are provably skin-only.

---

### Fix 2b — One-Shot `ReloadSkin` Guard

#### `xbmc/profiles/ProfileManager.h`

```diff
 class CProfileManager
 {
 private:
+  bool m_firstRealProfileLoad = true;
   // ...
 };
```

#### `xbmc/profiles/ProfileManager.cpp` — `FinalizeLoadProfile` / `LogOff`

```diff
 void CApplication::FinalizeLoadProfile(int profileIndex)
 {
   CProfileManager::GetInstance().LoadProfile(profileIndex);
-  ReloadSkin();
+  if (CProfileManager::GetInstance().IsFirstRealProfileLoad())
+  {
+    CProfileManager::GetInstance().ClearFirstRealProfileLoad();
+    ReloadSkin();
+  }
   // ...
 }
```

Or, more cleanly, make `ReloadSkin` conditional on the skin actually changing:

```diff
-  ReloadSkin();
+  const std::string newSkin = CProfileManager::GetInstance().GetCurrentProfile().GetSkin();
+  if (newSkin != g_SkinInfo->ID())
+    ReloadSkin();
```

This last approach (compare skin IDs) is preferred as it requires no new flag and handles all cases — including a user switching back and forth between identical-skin profiles.

---

## 5. Pseudocode / Diff Sketch

```diff
--- a/xbmc/Application.cpp
+++ b/xbmc/Application.cpp
@@ -FinalizeLoadProfile
+  // Flush deferred queue in current (master) context before profile switch
+  m_bInitializing = false;
+  for (const auto& deferred : m_deferredActions)
+    ExecuteXBMCAction(deferred.actionStr, deferred.item);
+  m_deferredActions.clear();
+
   CProfileManager::GetInstance().LoadProfile(profileIndex);
-  m_bInitializing = false;
-  FlushDeferredActions();

@@ -ReloadSkin call
-  ReloadSkin();
+  const std::string newSkin = CProfileManager::GetInstance().GetCurrentProfile().GetSkin();
+  if (newSkin != g_SkinInfo->ID())
+    ReloadSkin();
```

---

## 6. Edge Cases

### 6a. User Cancels Login
If the user presses Back on the login screen, `FinalizeLoadProfile` is not called. `m_bInitializing` stays `true`. Deferred actions accumulate. On next profile-load attempt the flush fires. This is acceptable — same as pre-regression behaviour.

### 6b. Multiple Profiles With the Same Skin
Switching between two profiles that both use Estouchy should not fire `ReloadSkin`. The skin-ID comparison fix handles this correctly.

### 6c. First Boot (No Login Screen)
Normal path: `m_bInitializing = false` fires at line 3759 via `GUI_MSG_UI_READY`. The `FinalizeLoadProfile` changes are not reached. No impact.

### 6d. Guest / Kiosk Profile
A kiosk profile may have no custom skin. The skin-ID comparison defaults to master skin == profile skin → no `ReloadSkin`. Correct.

### 6e. Deferred Queue Contains `ReloadSkin` Action
If boot fired a `ReloadSkin()` action that was deferred, it is now executed in the correct master-profile context (before the switch). The profile skin is then loaded if and only if it differs. The two `ReloadSkin` calls still fire sequentially, but now with the correct context for the first one and a valid reason (skin change) for the second.

---

## 7. Test Scenarios

### T1 — Login Screen → Profile Switch → Deferred Actions Execute in Master Context
1. Configure Kodi with a login screen.
2. During login-screen display, force a deferred action that references a master-profile `$INFO` token.
3. Log in as profile 1 (different skin/language).
4. Verify the deferred action was resolved in master-profile context, not profile-1 context.

### T2 — No Double `ReloadSkin` on LogOff
1. Log in as any profile.
2. Log off back to master.
3. Verify `ReloadSkin()` fires at most once.
4. Verify no double-flash in the UI.

### T3 — Single-Profile Setup (No Login Screen)
1. Boot with no login screen.
2. Verify `m_bInitializing` transitions to `false` via the normal `GUI_MSG_UI_READY` path.
3. Verify deferred actions flush in the correct context.
4. Verify no behaviour change from pre-regression.

### T4 — Switch Between Same-Skin Profiles
1. Create two profiles both using `skin.estouchy`.
2. Switch from profile 1 to profile 2.
3. Verify `ReloadSkin()` is **not** called (skin IDs match).

### T5 — Switch Between Different-Skin Profiles
1. Create profile 1 (skin.estouchy) and profile 2 (skin.confluence).
2. Switch.
3. Verify `ReloadSkin()` IS called exactly once.

---

## 8. Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Flush before profile switch executes action in wrong skin context anyway | Low | Medium | Skin/language are still at master-profile settings at flush time; this is correct |
| Discard approach silently loses valid boot actions | Medium | Medium | Prefer flush approach; only discard if actions are provably master-skin-only |
| Skin-ID comparison misses skin parameter changes | Low | Low | Extend comparison to include skin parameters if needed |
| `m_firstRealProfileLoad` flag never reset if profile load fails | Low | Low | Reset flag in `LoadProfile` error path |

### Overall Risk
**Medium.** The profile lifecycle is more complex than the other regressions. The ordering of `m_bInitializing = false` relative to `LoadProfile()` is the core fix and carries low risk. The `ReloadSkin` guard is a quality-of-life improvement; the skin-ID comparison approach is clean and self-contained.
