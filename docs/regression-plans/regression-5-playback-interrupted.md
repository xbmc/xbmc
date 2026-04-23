# Regression 5 — Deferred `PlayMedia` Interrupts User-Initiated Playback at Flush

**Repository:** OrganizerRo/xbmc  
**Date:** 2024  
**Severity:** High — a skin background video (or other auto-play action) that was deferred during boot fires at flush time even if the user has already started watching something, abruptly stopping the user's playback.

---

## 1. Root Cause Analysis

### The Deferral Bug

`CApplication::ExecuteXBMCAction` defers all actions during `m_bInitializing`:

```cpp
bool CApplication::ExecuteXBMCAction(std::string actionStr, const CGUIListItemPtr &item)
{
  if (m_bInitializing) {
    m_deferredActions.push_back(actionStr);  // enqueued with no condition re-evaluation
    return true;
  }
  // ...
}
```

The condition that caused the skin to fire the `PlayMedia` action is **evaluated at enqueue time** (during boot). But the action is executed at **flush time** (after boot), when conditions may have changed.

### The skin.estouchy Case

`skin.estouchy/xml/Home.xml` fires `PlayMedia($INFO[Window.Property(CustomVideoBackground)])` when:
1. `BackgroundMode` == `video`
2. `CustomVideoBackground` is set (a user-configured video path)
3. `!Player.HasMedia` — only if nothing is currently playing

```xml
<!-- Home.xml (simplified) -->
<onload condition="!Player.HasMedia + [Skin.HasSetting(BackgroundMode) | ...]">
  PlayMedia($INFO[Window.Property(CustomVideoBackground)])
</onload>
```

The `!Player.HasMedia` condition is satisfied at enqueue time (boot, nothing is playing). The action is deferred. If the user navigates and starts playing something before boot completes (possible on slow systems or with autoplay from a widget), the flush fires `PlayMedia(background_video.mp4)` anyway — stopping the user's playback.

### The General Pattern

Any playback-initiating action (`PlayMedia(`, `PlayFile(`, `Playlist.Play`, `PlayerControl(Play)`, etc.) that is:
1. Fired during `m_bInitializing` (boot)
2. Conditioned on `!Player.HasMedia` at fire time

...will be silently dequeued and executed even if the player state has changed by flush time.

### Key File:Line References

| Location | Significance |
|----------|-------------|
| `xbmc/Application.h:433` | `bool m_bInitializing = true;` |
| `xbmc/Application.cpp:3964–4013` | `ExecuteXBMCAction` body |
| `addons/skin.estouchy/xml/Home.xml:5–10` | `PlayMedia` on `<onload>` with `!Player.HasMedia` condition |
| `xbmc/Application.cpp` | `m_appPlayer.IsPlaying()` — player state query |
| `xbmc/guilib/GUIInfoManager.cpp` | `Player.HasMedia` info evaluation |

---

## 2. Exact Call Path

### Enqueue Time (Boot)

```
CApplication::Initialize()
  m_bInitializing = true
  → GUIWindowManager.ActivateWindow(WINDOW_HOME)
      → Home.xml <onload> condition evaluated:
          !Player.HasMedia = true  (nothing playing yet)
          BackgroundMode = video
          CustomVideoBackground = "/path/to/bg.mp4"
          → condition is TRUE
      → CGUIAction::ExecuteActions("PlayMedia($INFO[Window.Property(CustomVideoBackground)])", ...)
          → CGUIMessage(GUI_MSG_EXECUTE, ..., "PlayMedia(/path/to/bg.mp4)")
              → CApplication::ExecuteXBMCAction("PlayMedia(/path/to/bg.mp4)")
                  → m_bInitializing == true
                  → m_deferredActions.push_back("PlayMedia(/path/to/bg.mp4)")  ← ENQUEUED
                  → return true
```

### Flush Time (After Boot)

```
[User navigates to a video widget and clicks a movie]
  → Movie starts playing via normal input path
  → Player.HasMedia = true, Player.IsPlaying = true

m_bInitializing = false
→ FlushDeferredActions()
    → ExecuteXBMCAction("PlayMedia(/path/to/bg.mp4)")
        → Player.IsPlaying check: NOT PRESENT
        → PlayMedia("/path/to/bg.mp4")  ← INTERRUPTS USER'S MOVIE
```

---

## 3. Candidate Fix Approaches

### Approach A — Tag Playback Actions with `skipIfPlaying` Flag (Recommended)

Extend the deferred action struct (see R3) with a `bool skipIfPlaying` field. When enqueuing a playback-initiating action, set `skipIfPlaying = true`. At flush time, skip any deferred action tagged `skipIfPlaying` if the player is active.

**Guard condition — `IsPlaying()` covers paused state:**  
`CApplicationPlayer::IsPlaying()` calls `player->IsPlaying()`. In all Kodi player implementations, `IsPlaying()` returns `true` even when playback is paused (speed = 0); `IsPausedPlayback()` is a narrower check (`IsPlaying() && speed == 0`). Using `m_appPlayer.IsPlaying()` in the guard therefore correctly protects both actively playing **and** paused sessions from interruption. No separate `IsPausedPlayback()` check is needed.

```cpp
struct DeferredAction
{
  std::string actionStr;
  CGUIListItemPtr item;
  bool skipIfPlaying = false;   // R5: skip playback builtins if player active at flush
};
```

At enqueue:

```cpp
bool CApplication::ExecuteXBMCAction(std::string actionStr, const CGUIListItemPtr &item)
{
  const std::string in_actionStr(actionStr);

  if (m_bInitializing)
  {
    // R3+R5: deferral check moved before label resolution; use in_actionStr (original template).
    // Prefix check runs on the original string so $INFO[...] templates are still matched.
    static const std::array<std::string_view, 4> playbackPrefixes = {
        "PlayMedia(", "PlayFile(", "Playlist.Play", "PlayerControl(Play"
    };
    bool isPlayback = std::any_of(playbackPrefixes.begin(), playbackPrefixes.end(),
        [&](std::string_view prefix)
        { return StringUtils::StartsWithNoCase(in_actionStr, std::string(prefix)); });

    m_deferredActions.push_back({in_actionStr, item, isPlayback});  // store original string
    return false;  // R4 fix
  }
  // ...
}
```

At flush:

```cpp
for (const auto& deferred : m_deferredActions)
{
  // IsPlaying() returns true for both actively playing and paused sessions,
  // so paused playback is correctly protected from interruption.
  if (deferred.skipIfPlaying && m_appPlayer.IsPlaying())
    continue;  // user is already watching something; skip background autoplay
  ExecuteXBMCAction(deferred.actionStr, deferred.item);
}
m_deferredActions.clear();
```

**Pros:**
- Minimal struct addition; composes with R3 `DeferredAction`.
- Condition re-evaluated at the right time (flush), not baked in at enqueue.
- Background auto-play fires if nothing is playing at flush (correct), skipped if user has started playback (correct).

**Cons:**
- Requires a list of playback-initiating action prefixes (same maintenance concern as R1's boot-critical list).
- If a skin author uses an unusual playback action string, it may not be caught.

---

### Approach B — Re-Evaluate the Full Action Condition at Flush Time

Store not just the action string but also the XML condition expression (from `CGUIInfoBool` or `CGUICondition`). At flush time, re-evaluate the condition; if it is no longer true, skip the action.

**Pros:** Semantically perfect — the action is only replayed if it would still fire today.  
**Cons:** Requires storing and re-evaluating a `CGUIInfoBool` expression; the info manager must be fully up at flush time (it should be, but adds a dependency). Significant implementation effort.

---

### Approach C — Don't Defer Playback Actions at All

If the action is a playback-initiating builtin, skip deferral and execute immediately during boot. This requires playback services to be ready during boot — which they should be for most playback builtins, but there is a risk of crashes if the player manager is not fully initialised.

**Cons:** Risk of crash; not safe for early-boot window; not recommended.

---

### Approach D — Let Background Video Re-trigger After Boot Naturally

Remove deferral for `Home.xml <onload>` actions entirely (i.e., do not defer actions from window-init handlers when transitioning from the startup window). The background video fires naturally when `Home.xml` is re-entered after boot, which is when `m_bInitializing == false`.

**Pros:** Zero changes to the deferral machinery.  
**Cons:** Works for home-screen background videos but does not generalise to all deferred playback action sources.

---

## 4. Recommended Fix — Approach A

### Detailed Code Changes

#### `xbmc/Application.h`

The `DeferredAction` struct (already introduced in R3) gains one field:

```diff
 struct DeferredAction
 {
   std::string actionStr;
   CGUIListItemPtr item;
+  bool skipIfPlaying = false;
 };
```

#### `xbmc/Application.cpp` — `ExecuteXBMCAction` deferral point

```diff
 bool CApplication::ExecuteXBMCAction(std::string actionStr, const CGUIListItemPtr &item)
 {
   if (m_bInitializing)
   {
+    static const std::array<std::string_view, 4> playbackPrefixes = {
+        "PlayMedia(", "PlayFile(", "Playlist.Play", "PlayerControl(Play"
+    };
+    bool isPlayback = std::any_of(playbackPrefixes.begin(), playbackPrefixes.end(),
+        [&](std::string_view prefix)
+        { return StringUtils::StartsWithNoCase(actionStr, std::string(prefix)); });
+
-    m_deferredActions.push_back({actionStr, item});
+    m_deferredActions.push_back({actionStr, item, isPlayback});
     return false;
   }
   // ...
 }
```

#### `xbmc/Application.cpp` — flush loop

```diff
 for (const auto& deferred : m_deferredActions)
+{
+  if (deferred.skipIfPlaying && m_appPlayer.IsPlaying())
+    continue;
   ExecuteXBMCAction(deferred.actionStr, deferred.item);
+}
 m_deferredActions.clear();
```

---

## 5. Pseudocode / Diff Sketch

```diff
--- a/xbmc/Application.h
+++ b/xbmc/Application.h
@@ DeferredAction struct (introduced by R3)
   struct DeferredAction
   {
     std::string actionStr;   // original unresolved string (in_actionStr)
     CGUIListItemPtr item;
+    bool skipIfPlaying = false;
   };

--- a/xbmc/Application.cpp
+++ b/xbmc/Application.cpp
@@ ExecuteXBMCAction deferral (R3+R5: deferral check before label resolution)
+  if (m_bInitializing)
+  {
+    static const std::array<std::string_view, 4> playbackPrefixes = {
+        "PlayMedia(", "PlayFile(", "Playlist.Play", "PlayerControl(Play"
+    };
+    bool isPlayback = std::any_of(playbackPrefixes.begin(), playbackPrefixes.end(),
+        [&](std::string_view p){ return StringUtils::StartsWithNoCase(in_actionStr, std::string(p)); });
-    m_deferredActions.push_back({in_actionStr, item});          // R3 (without R5)
+    m_deferredActions.push_back({in_actionStr, item, isPlayback});  // R3+R5

@@ flush loop
   for (const auto& deferred : m_deferredActions)
+  {
+    // IsPlaying() covers both active and paused sessions; paused playback is protected.
+    if (deferred.skipIfPlaying && m_appPlayer.IsPlaying())
+      continue;
     ExecuteXBMCAction(deferred.actionStr, deferred.item);
+  }
```

---

## 6. Edge Cases

### 6a. Background Video Fires Correctly When Nothing Is Playing at Flush
`skipIfPlaying == true` but `m_appPlayer.IsPlaying() == false` → action executes normally → background video starts. Correct.

### 6b. User Starts Playback After Flush Begins
If the flush loop has already started iterating and the user starts a video partway through the flush (race condition), the check fires per-entry. Entries before the user's playback start execute normally; entries after the player becomes active are skipped. This is acceptable.

### 6c. Playlist Auto-Start
`Playlist.Play` is tagged `skipIfPlaying`. If the user has manually added items to the playlist and started it before boot completes, the deferred `Playlist.Play` is correctly skipped.

### 6d. `PlayerControl(Play)` vs `PlayerControl(Pause)` vs `PlayerControl(Stop)`
Only `PlayerControl(Play` (prefix) is tagged `skipIfPlaying`. `PlayerControl(Pause)` and `PlayerControl(Stop)` are not playback-initiating (they control an existing stream) and should not carry `skipIfPlaying`. The prefix check correctly excludes them.

Note: `m_appPlayer.IsPlaying()` returns `true` even when playback is paused (speed = 0), so a paused session is also correctly protected. `IsPausedPlayback()` (which returns `IsPlaying() && speed == 0`) is a subset of this guard and does not need to be checked separately.

### 6e. Plugin-Based Playback (`plugin://`)
`PlayMedia(plugin://...)` is caught by the `PlayMedia(` prefix. Works correctly.

### 6f. Action That Is Both `PlayMedia` and Critical Window Navigation
This combination does not exist in practice. If a skin somehow combines them, the action is tagged `skipIfPlaying`; if nothing is playing at flush, it fires — no harm.

### 6g. `skin.estouchy` Background Video Re-trigger
After the fix: if nothing is playing at flush, `PlayMedia(bg.mp4)` fires → background video starts. This is the pre-regression correct behaviour. If something is playing, the background video is skipped — also correct (the user is watching something).

### 6h. Multiple Deferred Playback Actions
If the skin fires several `PlayMedia` actions during boot (unusual), all are tagged `skipIfPlaying`. At flush, all are skipped if playing, or all execute in order if not. The last one "wins" — same as pre-deferral behaviour.

---

## 7. Test Scenarios

### T1 — Background Video Fires When Nothing Playing at Flush
1. Set `m_bInitializing = true`.
2. Send `PlayMedia(bg.mp4)` via `ExecuteXBMCAction`.
3. Verify action is deferred with `skipIfPlaying = true`.
4. Set `m_bInitializing = false`, player is NOT playing.
5. Flush deferred actions.
6. Verify `PlayMedia(bg.mp4)` fires.

### T2 — Background Video Skipped When User Is Playing at Flush
1. Set `m_bInitializing = true`.
2. Send `PlayMedia(bg.mp4)` via `ExecuteXBMCAction` → deferred.
3. User starts playing a movie via normal input path.
4. `m_appPlayer.IsPlaying() = true`.
5. Set `m_bInitializing = false`, flush.
6. Verify `PlayMedia(bg.mp4)` is **NOT** executed.
7. Verify user's movie continues uninterrupted.

### T3 — Non-Playback Action Not Affected
1. Defer `ActivateWindow(Home)` and `SetSetting(...)` during boot.
2. User starts playing a movie.
3. Flush deferred actions.
4. Verify non-playback actions execute normally (R1 guard may prevent `ActivateWindow` from being deferred, but `SetSetting` should execute regardless of player state).

### T4 — `Playlist.Play` During Boot, User Playing at Flush
1. Defer `Playlist.Play` during boot.
2. User starts movie.
3. Verify `Playlist.Play` is skipped at flush.

### T5 — `PlayerControl(Pause)` Not Tagged
1. Defer `PlayerControl(Pause)` during boot.
2. User starts movie.
3. Verify `PlayerControl(Pause)` DOES execute at flush (it controls existing playback, not initiating).

---

## 8. Relationship to Other Regressions

This fix composes with R3 and R4 into a single `DeferredAction` struct refactor:

```cpp
struct DeferredAction
{
  std::string     actionStr;
  CGUIListItemPtr item;           // R3: preserves item via shared_ptr
  bool            skipIfPlaying;  // R5: skip playback builtins if player active
};
```

R4's `return false` change is independent and can be applied first.  
R1's boot-critical bypass is also independent.  
R2 is a separate lifecycle fix in `CProfileManager`.

Suggested implementation order: **R1 → R4 → R3+R5 (combined struct refactor) → R2**.

---

## 9. Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Playback prefix list misses a custom action | Low | Medium | Add `xbmc.playmedia`, `xbmc.playfile` aliases if present in CBuiltins |
| Background video never starts (false positive skip) | Very low | Low | Condition is `m_appPlayer.IsPlaying()` — precise; only true if media active |
| Race condition between flush start and user playback start | Low | Low | Per-entry check in flush loop handles this gracefully |
| `skipIfPlaying` flag stored in persistent deferred queue breaks R3 struct | None | None | All three fields (`actionStr`, `item`, `skipIfPlaying`) are trivially copyable or shared_ptr-managed |
| Playlist pre-loaded but not started: IsPlaying() = false | Intentional | None | Pre-loaded but not started is not "playing"; deferred action fires — correct |

### Overall Risk
**Low.** The change adds a small flag to the deferred action struct and a two-line guard in the flush loop. The condition `m_appPlayer.IsPlaying()` is an existing, well-tested API. The fix precisely targets the scenario (deferred playback action vs. active user playback) without affecting any other deferral behaviour.
