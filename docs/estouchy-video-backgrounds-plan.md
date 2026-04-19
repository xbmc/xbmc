# Plan: Video Backgrounds & Image Resource Selectors for `skin.estouchy` (Leia & Krypton)
<!-- Last reviewed: 2026-04-19 — loop path, fullscreen-switch, and onunload timing verified against Leia & Krypton C++ source -->

**Applies to:** `addons/skin.estouchy/xml/Includes.xml`, `addons/skin.estouchy/xml/Home.xml`,
and `addons/skin.estouchy/language/resource.language.en_gb/strings.po` in both the **Leia** and
**Krypton** branches.

---

## Background: What Exists Today

In both branches the `SkinSettingsContent` include (Background tab, `<control type="grouplist" id="300">`)
offers:

| Feature | Current state |
|---|---|
| "Use Custom Background" toggle | ✅ boolean `UseCustomBackground` |
| Single-file image picker | ✅ `Skin.SetImage(CustomBackground)` |
| Hide video background toggle | ✅ `HideVideoBackground` |
| Hide viz background toggle | ✅ `HideVizBackground` |
| **Image resource-pack selector** | ❌ missing |
| **Video file background loop** | ❌ missing |
| **Background mode switcher** | ❌ missing |

`CommonBackground` renders (in z-order): built-in `primary.jpg` → custom image
(`Skin.String(CustomBackground)`) → fanart overlays → `videowindow` → `visualisation`.

---

## Source-Code Feasibility — Verified Against Both Branches

The table below summarises the research results from the Kodi C++ source in the **Leia** and
**Krypton** branches of this repository.

| Feature | Leia source | Krypton source | Verdict |
|---|---|---|---|
| `<control type="videowindow">` | `GUIControlFactory.cpp:84` | `GUIControlFactory.cpp:93` | ✅ both |
| `<control type="multiimage">` | `GUIControlFactory.cpp:91` | `GUIControlFactory.cpp:99` | ✅ both |
| `Skin.SetFile(name,mask)` builtin | `SkinBuiltins.cpp:215` | `SkinBuiltins.cpp` (SetFile) | ✅ both |
| `PlayMedia(path,1,noresume)` builtin | `PlayerBuiltins.cpp:381` | `PlayerBuiltins.cpp:384` | ✅ both |
| `PlayerControl(RepeatOne)` | `PlayerBuiltins.cpp:306` | `PlayerBuiltins.cpp:306` | ✅ both |
| Conditional `<onload condition="...">` | `GUIControlFactory.cpp:479` | same | ✅ both |
| `<onunload>` action | `GUIWindow.cpp:1056` | `GUIWindow.cpp:1074` | ✅ both |
| `String.IsEqual(info,string)` condition | `GUIInfoManager.cpp:161` (added v17) | v17 = Krypton | ✅ both |
| `Skin.String` / `Skin.HasSetting` conditions | `GUIInfoManager.cpp:8895` | same | ✅ both |

### Critical design notes

1. **`videowindow` renders the MAIN player only.**  
   Both `GUIVideoControl.cpp` implementations call `g_application.m_pPlayer->IsRenderingVideo()`
   (Krypton) / `g_application.GetAppPlayer().IsRenderingVideo()` (Leia).  
   There is no secondary/background player in either codebase. Background video therefore runs
   in the **main player**, which means:
   - When the user starts playing real media, the real media takes over the player — the background
     loop is interrupted, which is the expected behaviour.
   - The existing `videowindow` `<visible>` condition (`Player.HasVideo + ...`) already handles
     both cases correctly (background loop *and* user media).

2. **`PlayMedia` does NOT support a `repeat` inline parameter.**  
   Supported inline params (both branches): `isdir`, `resume`, `noresume`, `playoffset=N`.  
   To loop a file, two **separate** `<onload>` actions are needed:
   ```xml
   <onload condition="...">PlayMedia($INFO[Skin.String(CustomVideoBackground)],1,noresume)</onload>
   <onload condition="...">PlayerControl(RepeatOne)</onload>
   ```

3. **Infinite loop is fully supported by the engine in both branches.**  
   The complete end-of-file path, verified in both `PlayListPlayer.cpp` and `Application.cpp`:

   | Step | Leia source | Krypton source |
   |---|---|---|
   | Video ends → `OnPlayBackEnded()` | `Application.cpp:3038` | `Application.cpp:3565` |
   | Sends `GUI_MSG_PLAYBACK_ENDED` | `Application.cpp:3048` | `Application.cpp:3588` |
   | Handler calls `PlayNext(1, true)` | `Application.cpp:3905` | `Application.cpp:4318` |
   | `PlayNext` calls `GetNextSong()` | `PlayListPlayer.cpp:179` | `PlayListPlayer.cpp:180` |
   | `GetNextSong()` checks `RepeatedOne()` — if true, returns **same** song index | `PlayListPlayer.cpp:154` | `PlayListPlayer.cpp:156` |
   | Same item replays → **infinite loop** | ✅ | ✅ |

   `CPlayListPlayer::Reset()` — called by `PlayListPlayer::Play(CFileItemPtr)` before starting
   the new item — does **not** clear `m_repeatState`. It only resets `m_iCurrentSong = -1`.
   So `SetRepeat(PLAYLIST_VIDEO, REPEAT_ONE)` set by `PlayerControl(RepeatOne)` persists
   across every re-play. (Leia `PlayListPlayer.cpp:457`; Krypton `PlayListPlayer.cpp:435`.)

   `IsSingleItemNonRepeatPlaylist()` is **not** consulted in the `PLAYBACK_ENDED` handler in
   either branch — it is only used when the user presses the Next/Prev buttons. It does not
   block the automatic loop.

4. **`PlayerControl(RepeatOne)` will see `PLAYLIST_VIDEO` as the current playlist.**  
   `GUIAction::ExecuteActions` (Leia `GUIAction.cpp:21`) evaluates **all** conditions first,
   collects the matching action strings, **then** fires them sequentially via synchronous
   `SendMessage`. By the time `PlayerControl(RepeatOne)` fires, `PlayListPlayer::Play(CFileItemPtr)`
   has already called `SetCurrentPlaylist(PLAYLIST_VIDEO)`, so `GetCurrentPlaylist()` correctly
   returns `PLAYLIST_VIDEO`.

5. **⚠️ `PlayMedia` auto-switches to fullscreen — must use `1` parameter.**  
   `CApplication::PlayFile()` sets `options.fullscreen = true` by default (governed by
   `m_fullScreenOnMovieStart`, which defaults to `true` — Leia `AdvancedSettings.cpp:205` /
   Krypton same). The VideoPlayer then posts `TMSG_SWITCHTOFULLSCREEN` (Leia
   `VideoPlayer.cpp:2087`) or sets `CONF_FLAGS_FULLSCREEN` (Krypton `VideoPlayerVideo.cpp:764`),
   which activates `WINDOW_FULLSCREEN_VIDEO` and navigates away from Home.

   The `PlayMedia` builtin accepts `"1"` as a parameter that calls
   `CMediaSettings::SetVideoStartWindowed(true)` (Leia `PlayerBuiltins.cpp:406`; Krypton same
   line). This makes `options.fullscreen = false`, preventing the fullscreen switch.
   `VideoStartWindowed` is a one-shot flag — it's reset to `false` after use at
   `Application.cpp:2946`, so subsequent user-initiated PlayMedia calls default to fullscreen
   normally.

   **Correct background video invocation:**
   ```xml
   PlayMedia($INFO[Skin.String(CustomVideoBackground)],1,noresume)
   ```
   - `1` → start windowed (stay on Home)
   - `noresume` → don't show resume dialog

6. **⚠️ `<onunload>` must use `RepeatOff`, not `Stop`.**  
   A `Skin.SetBool(BgVideoActive)` flag cannot reliably distinguish "our background" from "user
   media" at `<onunload>` time. The problem: when the user selects a movie from Home, the
   sequence is:

   1. onclick → `PlayMedia(movie)` → movie starts playing (replaces background in main player)
   2. `options.fullscreen = true` → VideoPlayer posts `TMSG_SWITCHTOFULLSCREEN` *(async)*
   3. `TMSG_SWITCHTOFULLSCREEN` processed → `SwitchToFullScreen()` → `ActivateWindow(FULLSCREEN_VIDEO)`
   4. `ActivateWindow_Internal` → `CloseWindowSync(Home)` → Home's `OnDeinitWindow` → **`<onunload>` fires**
   5. At this point the player IS playing the **user's movie**, but `BgVideoActive` is still set
   6. `PlayerControl(Stop)` → **kills the user's movie** ✗

   Using `PlayerControl(RepeatOff)` instead of `Stop` avoids this entirely:
   - It only turns off the repeat flag — it never interrupts playback.
   - For **Scenario A** (user navigates to Settings): the background video continues playing
     to its natural end, then stops. When the user returns to Home, `<onload>` restarts it
     (because `!Player.HasMedia` becomes true after EOF).
   - For **Scenario B** (user selects a movie): `RepeatOff` clears `REPEAT_ONE` on
     `PLAYLIST_VIDEO`. The user's movie plays normally without looping unexpectedly.

   The `BgVideoActive` flag and `PlayerControl(Stop)` are **removed** from the design.

   **Correct `<onunload>` actions:**
   ```xml
   <onunload condition="String.IsEqual(Skin.String(BackgroundMode),video)">
     PlayerControl(RepeatOff)
   </onunload>
   ```

7. **Re-enable `RepeatOne` on returning to Home.**  
   When the user returns to Home and the background video is still playing (it hasn't reached
   EOF yet), the `<onload>` with `!Player.HasMedia` won't fire (because media IS playing).
   A second `<onload>` with `Player.HasVideo` re-enables the loop:
   ```xml
   <!-- Start bg video if nothing playing -->
   <onload condition="String.IsEqual(Skin.String(BackgroundMode),video) + !String.IsEmpty(Skin.String(CustomVideoBackground)) + !Player.HasMedia">
     PlayMedia($INFO[Skin.String(CustomVideoBackground)],1,noresume)
   </onload>
   <!-- Re-enable RepeatOne — whether we just started or are resuming -->
   <onload condition="String.IsEqual(Skin.String(BackgroundMode),video) + Player.HasVideo">
     PlayerControl(RepeatOne)
   </onload>
   ```
   Note: the second `<onload>` fires even if the user returns to Home while their own movie
   is playing. This sets `REPEAT_ONE` on the user's movie, which is a minor cosmetic issue.
   The user can toggle it off in the Now Playing bar, and `<onunload>` will clear it when
   they leave Home again.

8. **`multiimage` handles both a single file *and* a resource-pack directory path.**  
   `CGUIMultiImage::CMultiImageJob::DoWork()` uses `CGUITextureManager::GetTexturePath()` to
   resolve `resource://` URIs, then calls `CDirectory::GetDirectory()` to enumerate images. A
   plain file path also works (fast-path in `LoadDirectory()`).

9. **`script.image.resource.select` sets three skin strings** when `property=Foo` is passed:  
   `Skin.String(Foo.path)` (directory), `Skin.String(Foo.ext)` (image extension),
   `Skin.String(Foo.name)` (display name). This is the same convention used by Estuary.

10. **`Skin.SetFile` mask for video files.**  
    Pass a pipe-separated extension list (e.g. `.mp4|.mkv|.avi|.mov|.m4v|.wmv`) rather than the
    word `"video"`, which is not recognised as an addon-type string and is therefore used verbatim
    as a file-extension filter — which does not match any real file extension.

---

## Plan

### Step 1 — Introduce a Background Mode skin string

Replace the existing boolean toggle approach with a **three-mode string setting** stored in
`Skin.String(BackgroundMode)`:

| Value | Meaning |
|---|---|
| `""` (empty / not set) | Default — skin's built-in gradient + fanart |
| `"image"` | User-chosen still image **or** image resource-pack slideshow |
| `"video"` | User-chosen video file played as a looping background |

The existing `UseCustomBackground` bool is removed and its use replaced by
`String.IsEqual(Skin.String(BackgroundMode),image)` in `CommonBackground`.

---

### Step 2 — Image resource-pack selector

Add a new button in the Background grouplist beside the existing file picker:

```xml
<!-- Select from an installed image resource pack -->
<control type="button" id="119">
  <label>- $LOCALIZE[31569]</label>
  <label2>$INFO[Skin.String(CustomBackground.name)]</label2>
  <onclick condition="System.HasAddon(script.image.resource.select)">
    RunScript(script.image.resource.select,property=CustomBackground&amp;type=resource.images.skinbackgrounds)
  </onclick>
  <onclick condition="!System.HasAddon(script.image.resource.select)">
    InstallAddon(script.image.resource.select)
  </onclick>
  <enable>String.IsEqual(Skin.String(BackgroundMode),image)</enable>
  <textoffsetx>40</textoffsetx>
  ...
</control>
```

Change the `<control type="image">` in `CommonBackground` that renders the custom image to a
**`<control type="multiimage">`** using `<imagepath>`:

```xml
<control type="multiimage">
  <posx>0</posx><posy>0</posy>
  <include>ScreenWidth</include>
  <height>960</height>
  <aspectratio>scale</aspectratio>
  <imagepath background="true">$INFO[Skin.String(CustomBackground.path)]$INFO[Skin.String(CustomBackground.ext)]</imagepath>
  <visible>String.IsEqual(Skin.String(BackgroundMode),image) + !String.IsEmpty(Skin.String(CustomBackground.path))</visible>
</control>
<!-- Fallback: single-file path when set directly via Skin.SetImage -->
<control type="image">
  ...
  <texture>$INFO[Skin.String(CustomBackground)]</texture>
  <visible>String.IsEqual(Skin.String(BackgroundMode),image) + String.IsEmpty(Skin.String(CustomBackground.path))</visible>
</control>
```

---

### Step 3 — Video file background picker and renderer

**In `SkinSettingsContent` (Background grouplist id 300)**, add:

```xml
<control type="button" id="120">
  <label>- $LOCALIZE[31571]</label>
  <label2>$INFO[Skin.String(CustomVideoBackground)]</label2>
  <onclick>Skin.SetFile(CustomVideoBackground,.mp4|.mkv|.avi|.mov|.m4v|.wmv|.ts|.mpg|.flv)</onclick>
  <enable>String.IsEqual(Skin.String(BackgroundMode),video)</enable>
  <textoffsetx>40</textoffsetx>
  ...
</control>
```

**In `Home.xml`**, add conditional `onload` / `onunload` actions.

**Why two `<onload>` conditions?**  
`GUIAction::ExecuteActions` evaluates all conditions upfront, then fires matching actions
sequentially via synchronous `SendMessage`. After `PlayMedia(...)` fires, `SetCurrentPlaylist`
has already been called (inside `PlayListPlayer::Play(CFileItemPtr)`), so `PlayerControl(RepeatOne)`
correctly targets `PLAYLIST_VIDEO`.

**Why `1` parameter in `PlayMedia`?**  
Without it, `CApplication::PlayFile` sets `options.fullscreen = true` (default from
`m_fullScreenOnMovieStart`), causing the VideoPlayer to post `TMSG_SWITCHTOFULLSCREEN` which
would activate `WINDOW_FULLSCREEN_VIDEO` and navigate away from Home — defeating the purpose.
Passing `1` calls `SetVideoStartWindowed(true)` so the video starts in windowed mode, staying
on the Home screen where the `videowindow` control renders it.

**Why `PlayerControl(RepeatOff)` instead of `PlayerControl(Stop)` in `<onunload>`?**  
When a user selects a movie from Home, the sequence is:  
PlayMedia(movie) → movie starts (replaces bg video) → async `TMSG_SWITCHTOFULLSCREEN` →
`ActivateWindow(FULLSCREEN_VIDEO)` → `CloseWindowSync(Home)` → Home's `<onunload>` fires.  
At this point the player IS playing the **user's movie**. Using `PlayerControl(Stop)` would
kill it. `PlayerControl(RepeatOff)` only clears the repeat flag — the user's movie continues
uninterrupted and won't loop unexpectedly.

**Second `<onload>` for re-entering Home**: If the user navigates away (RepeatOff fires) and
returns before the background video reaches EOF, the video is still playing but without repeat.
The second `<onload>` with `Player.HasVideo` re-enables `RepeatOne` to restore the loop.

```xml
<!-- ─── Start background video loop (windowed, with repeat) ─── -->
<onload condition="String.IsEqual(Skin.String(BackgroundMode),video) + !String.IsEmpty(Skin.String(CustomVideoBackground)) + !Player.HasMedia">
  PlayMedia($INFO[Skin.String(CustomVideoBackground)],1,noresume)
</onload>
<!-- Re-enable RepeatOne — whether we just started or are resuming after nav away -->
<onload condition="String.IsEqual(Skin.String(BackgroundMode),video) + Player.HasVideo">
  PlayerControl(RepeatOne)
</onload>

<!-- ─── On leaving Home: disable repeat (but never stop the player) ─── -->
<onunload condition="String.IsEqual(Skin.String(BackgroundMode),video)">
  PlayerControl(RepeatOff)
</onunload>
```

The existing `<control type="videowindow">` in `CommonBackground` needs **no changes** — its
current `<visible>` condition (`Player.HasVideo + !Skin.HasSetting(HideVideoBackground) + ...`)
will naturally display the looping background video when it is playing, and the user's real
media when that takes over.

---

### Step 4 — Updated Background Settings UI in `SkinSettingsContent`

Replace the current Background grouplist (id 300) with:

1. **Mode selector** — three radiobuttons (or select buttons) that call
   `Skin.SetString(BackgroundMode,<value>)`:
   - "Default background" → `Skin.SetString(BackgroundMode,)` (clears)
   - "Custom image background" → `Skin.SetString(BackgroundMode,image)`
   - "Custom video background" → `Skin.SetString(BackgroundMode,video)`

2. **Image sub-section** (visible only when `String.IsEqual(Skin.String(BackgroundMode),image)`):
   - "Pick image file" button → `Skin.SetImage(CustomBackground)` (existing)
   - "Select image resource pack" button → `RunScript(script.image.resource.select,...)` (new)
   - Both display label2 with the current path/name

3. **Video sub-section** (visible only when `String.IsEqual(Skin.String(BackgroundMode),video)`):
   - "Pick background video" button → `Skin.SetFile(CustomVideoBackground,.mp4|.mkv|...)`
   - Label2 shows `$INFO[Skin.String(CustomVideoBackground)]`

4. **Existing toggles** (always visible):
   - Hide video background during real media playback
   - Hide visualisation background

---

### Step 5 — New strings in `strings.po`

Add to `addons/skin.estouchy/language/resource.language.en_gb/strings.po` (first available IDs
in the 31566+ empty block):

```po
msgctxt "#31566"
msgid "Custom image background"
msgstr ""

msgctxt "#31567"
msgid "Custom video background"
msgstr ""

msgctxt "#31568"
msgid "Default background"
msgstr ""

msgctxt "#31569"
msgid "Select image resource pack"
msgstr ""

msgctxt "#31570"
msgid "Background image pack:"
msgstr ""

msgctxt "#31571"
msgid "Pick background video"
msgstr ""

msgctxt "#31572"
msgid "Background video:"
msgstr ""
```

Add the same IDs (with empty `msgstr ""`) to all other language `strings.po` files under
`addons/skin.estouchy/language/*/strings.po`.

---

### Step 6 — Apply identically to both branches

The XML and `.po` diffs are identical for Leia and Krypton because:
- Both branches carry the same `skin.estouchy` skin XML files (verified SHA match in SkinSettings.xml).
- Both branches' C++ source supports all required builtins and controls (see table above).

The changes should therefore be applied to **both branches** as the same patch.

---

## Files Changed

| File | Change |
|---|---|
| `addons/skin.estouchy/xml/Includes.xml` | Update `CommonBackground` (replace image control with `multiimage` + add conditional video `videowindow` guard); update `SkinSettingsContent` group 300 (mode selector, image-pack button, video-picker button) |
| `addons/skin.estouchy/xml/Home.xml` | Add conditional `<onload>` to start background video loop (windowed); add `<onunload>` with `RepeatOff` to cleanly disable looping on navigation |
| `addons/skin.estouchy/language/resource.language.en_gb/strings.po` | Add string IDs 31566–31572 |
| All other `language/*/strings.po` | Add same IDs with empty `msgstr ""` |

No C++ or build-script changes are required — this is skin XML and `.po` only.

---

## Looping Behaviour — Verified Sequence

The infinite-loop path was traced through the C++ source in both branches:

```
User navigates to Home (first visit, nothing playing)
  └─ <onload> fires (GUIAction::ExecuteActions — conditions evaluated first, then actions fired)
       ├─ [1] condition: BackgroundMode=video AND CustomVideoBackground set AND !Player.HasMedia
       │       ──► PlayMedia($INFO[Skin.String(CustomVideoBackground)],1,noresume)
       │              └─ PlayListPlayer::Play(CFileItemPtr)
       │                    ├─ ClearPlaylist(PLAYLIST_VIDEO)
       │                    ├─ Reset()                ← resets m_iCurrentSong only; m_repeatState NOT cleared
       │                    ├─ SetCurrentPlaylist(PLAYLIST_VIDEO)
       │                    ├─ Add(PLAYLIST_VIDEO, item)
       │                    └─ Play(0) → g_application.PlayFile(*item)
       │                         └─ options.fullscreen = false  ← because of "1" param (VideoStartWindowed)
       │                         └─ VideoPlayer starts but does NOT post TMSG_SWITCHTOFULLSCREEN
       │                         └─ Video plays in windowed mode on Home screen ✓
       └─ [2] condition: BackgroundMode=video AND Player.HasVideo
              ──► PlayerControl(RepeatOne)
                     └─ GetCurrentPlaylist() → PLAYLIST_VIDEO  ✓ (already set above)
                     └─ SetRepeat(PLAYLIST_VIDEO, REPEAT_ONE)

videowindow (CommonBackground) becomes visible
  └─ condition: Player.HasVideo (now true) + !HideVideoBackground + !PVR windows

─── Video file reaches end of file ───

  Application::OnPlayBackEnded()
    └─ sends GUI_MSG_PLAYBACK_ENDED

  Application::OnMessage(GUI_MSG_PLAYBACK_ENDED)
    └─ g_playlistPlayer.PlayNext(1, true)          ← Leia App.cpp:3905 / Krypton App.cpp:4318
         └─ CPlayListPlayer::PlayNext()
              └─ GetNextSong()
                   └─ RepeatedOne(PLAYLIST_VIDEO) == true  → returns SAME song index
              └─ Play(sameSongIndex)
                   └─ g_application.PlayFile(*item)       ← video replays immediately
  ──► INFINITE LOOP ✓
```

### Scenario A: User navigates away without playing media

```
User on Home (background video looping) → navigates to Settings
  └─ ActivateWindow(Settings) → CloseWindowSync(Home) → OnDeinitWindow → RunUnloadActions
       └─ <onunload> fires
            └─ condition: BackgroundMode=video
            └─ PlayerControl(RepeatOff)       ← disables loop, does NOT stop playback
  └─ Background video continues playing (invisible) until EOF
  └─ Video reaches EOF → PLAYBACK_ENDED → PlayNext → GetNextSong
       └─ RepeatedOne == false → iSong increments past end → playlist exhausted
       └─ Playback stops naturally

User returns to Home
  └─ <onload> fires
       ├─ [1] !Player.HasMedia → true → PlayMedia(bg,1,noresume) → starts bg video fresh ✓
       └─ [2] Player.HasVideo → true → PlayerControl(RepeatOne) → re-enables loop ✓
```

### Scenario B: User selects a movie from Home

```
User on Home (background video looping) → clicks on a movie in widget
  └─ onclick → PlayMedia(movie_path) or PlayListPlayer::Play(movieItem)
       └─ PlayFile(movie) → replaces background video in main player
       └─ options.fullscreen = true (default for user media)
       └─ VideoPlayer posts TMSG_SWITCHTOFULLSCREEN (async)

  └─ TMSG_SWITCHTOFULLSCREEN processed
       └─ SwitchToFullScreen(true) → ActivateWindow(WINDOW_FULLSCREEN_VIDEO)
       └─ ActivateWindow_Internal → CloseWindowSync(Home) → OnDeinitWindow
            └─ <onunload> fires
                 └─ condition: BackgroundMode=video
                 └─ PlayerControl(RepeatOff)   ← turns off repeat on PLAYLIST_VIDEO
                                                  user's movie plays normally, won't loop ✓
                                                  user's movie is NOT stopped ✓

User finishes watching movie → playback ends → Player has no media
User returns to Home
  └─ <onload> fires
       ├─ [1] !Player.HasMedia → true → PlayMedia(bg,1,noresume) → restarts bg video ✓
       └─ [2] Player.HasVideo → true → RepeatOne → loop re-enabled ✓
```

### Scenario C: User returns to Home while movie is still playing

```
User is watching a movie in fullscreen → presses Home button
  └─ ActivateWindow(Home) → FULLSCREEN_VIDEO closes → Home opens
       └─ <onload> fires
            ├─ [1] !Player.HasMedia → false (movie still playing) → PlayMedia skipped ✓
            └─ [2] Player.HasVideo → true → PlayerControl(RepeatOne) fires
                 └─ Sets REPEAT_ONE on PLAYLIST_VIDEO (the user's movie)
                 └─ Minor side effect: if user's movie reaches EOF it will loop
                 └─ Acceptable: user can toggle repeat off in Now Playing bar

videowindow shows the user's movie on Home screen (expected — same as image bg being visible)

User navigates away from Home
  └─ <onunload> → RepeatOff → clears the repeat ✓
```

### Why `Reset()` does not break `REPEAT_ONE`

`CPlayListPlayer::Reset()` in both branches only resets `m_iCurrentSong = -1` and playlist-change
bookkeeping. The `m_repeatState` array is **never touched** by `Reset()`. (Leia `PlayListPlayer.cpp:457`;
Krypton `PlayListPlayer.cpp:435`.)  
This means `SetRepeat(PLAYLIST_VIDEO, REPEAT_ONE)` from `PlayerControl(RepeatOne)` survives every
automatic re-play triggered by `PLAYBACK_ENDED`, producing a true infinite loop.

### Why `RepeatOff` is safer than `Stop` for `<onunload>`

| Approach | Navigate to Settings | Select movie from Home | Return to Home while movie plays |
|---|---|---|---|
| `PlayerControl(Stop)` | ✓ stops bg | ✗ kills user's movie | ✗ kills user's movie |
| `PlayerControl(Stop)` + `BgVideoActive` flag | ✓ stops bg | ✗ flag still set → kills user's movie | ✗ flag still set → kills user's movie |
| **`PlayerControl(RepeatOff)`** | ✓ bg plays to EOF then stops | ✓ movie plays normally, no loop | ✓ movie continues, minor RepeatOne side effect |

The `BgVideoActive` flag approach fails because the flag cannot be cleared between the moment the
user selects a movie and the moment `<onunload>` fires — both happen in the same async message
sequence with no opportunity for skin-level intervention.
