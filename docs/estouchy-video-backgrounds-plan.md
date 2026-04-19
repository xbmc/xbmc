# Plan: Video Backgrounds & Image Resource Selectors for `skin.estouchy` (Leia & Krypton)
<!-- Last reviewed: 2026-04-19 — loop path verified against Leia & Krypton C++ source -->

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
| `PlayMedia(path,noresume)` builtin | `PlayerBuiltins.cpp:381` | `PlayerBuiltins.cpp:384` | ✅ both |
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
   <onload condition="...">PlayMedia($INFO[Skin.String(CustomVideoBackground)],noresume)</onload>
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

5. **⚠️ Safety: use a flag bool to guard the `<onunload>` stop action.**  
   The original plan used `Player.HasVideo` as the `<onunload>` condition. This is **incorrect**:
   if the user is watching a movie and navigates away from Home, that condition would also be true
   and would stop their playback.  
   The correct guard is a dedicated skin bool — `Skin.SetBool(BgVideoActive)` — set when we
   start the background loop and cleared when we stop it. This unambiguously distinguishes
   "background we started" from "user media playing":
   ```xml
   <!-- onload: start loop and set flag -->
   <onload condition="String.IsEqual(Skin.String(BackgroundMode),video) + !String.IsEmpty(Skin.String(CustomVideoBackground)) + !Player.HasMedia">
     PlayMedia($INFO[Skin.String(CustomVideoBackground)],noresume)
   </onload>
   <onload condition="String.IsEqual(Skin.String(BackgroundMode),video) + !String.IsEmpty(Skin.String(CustomVideoBackground)) + !Player.HasMedia">
     PlayerControl(RepeatOne)
   </onload>
   <onload condition="String.IsEqual(Skin.String(BackgroundMode),video) + !String.IsEmpty(Skin.String(CustomVideoBackground)) + !Player.HasMedia">
     Skin.SetBool(BgVideoActive)
   </onload>

   <!-- onunload: only stop if WE started the background video -->
   <onunload condition="Skin.HasSetting(BgVideoActive)">PlayerControl(Stop)</onunload>
   <onunload condition="Skin.HasSetting(BgVideoActive)">Skin.Reset(BgVideoActive)</onunload>
   ```

6. **`multiimage` handles both a single file *and* a resource-pack directory path.**  
   `CGUIMultiImage::CMultiImageJob::DoWork()` uses `CGUITextureManager::GetTexturePath()` to
   resolve `resource://` URIs, then calls `CDirectory::GetDirectory()` to enumerate images. A
   plain file path also works (fast-path in `LoadDirectory()`).

7. **`script.image.resource.select` sets three skin strings** when `property=Foo` is passed:  
   `Skin.String(Foo.path)` (directory), `Skin.String(Foo.ext)` (image extension),
   `Skin.String(Foo.name)` (display name). This is the same convention used by Estuary.

8. **`Skin.SetFile` mask for video files.**  
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

**Why three separate `<onload>` actions?**  
`GUIAction::ExecuteActions` evaluates all conditions upfront, then fires matching actions
sequentially via synchronous `SendMessage`. After `PlayMedia(...)` fires, `SetCurrentPlaylist`
has already been called (inside `PlayListPlayer::Play(CFileItemPtr)`), so `PlayerControl(RepeatOne)`
correctly targets `PLAYLIST_VIDEO`. After `RepeatOne` fires, we set a flag bool so the `<onunload>`
knows it was *us* that started the player.

**Why `Skin.SetBool(BgVideoActive)` instead of `Player.HasVideo` in `<onunload>`?**  
Using `Player.HasVideo` as the stop condition would also stop a *user's* own movie if they
navigate away from Home while watching something. The dedicated flag guarantees we only stop
the player when we were the ones who started it.

```xml
<!-- ─── Start background video loop ─── -->
<onload condition="String.IsEqual(Skin.String(BackgroundMode),video) + !String.IsEmpty(Skin.String(CustomVideoBackground)) + !Player.HasMedia">
  PlayMedia($INFO[Skin.String(CustomVideoBackground)],noresume)
</onload>
<onload condition="String.IsEqual(Skin.String(BackgroundMode),video) + !String.IsEmpty(Skin.String(CustomVideoBackground)) + !Player.HasMedia">
  PlayerControl(RepeatOne)
</onload>
<onload condition="String.IsEqual(Skin.String(BackgroundMode),video) + !String.IsEmpty(Skin.String(CustomVideoBackground)) + !Player.HasMedia">
  Skin.SetBool(BgVideoActive)
</onload>

<!-- ─── Stop background video on navigation (only if WE started it) ─── -->
<onunload condition="Skin.HasSetting(BgVideoActive)">PlayerControl(Stop)</onunload>
<onunload condition="Skin.HasSetting(BgVideoActive)">Skin.Reset(BgVideoActive)</onunload>
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
| `addons/skin.estouchy/xml/Home.xml` | Add conditional `<onload>` to start background video loop; add conditional `<onunload>` to stop it on navigation |
| `addons/skin.estouchy/language/resource.language.en_gb/strings.po` | Add string IDs 31566–31572 |
| All other `language/*/strings.po` | Add same IDs with empty `msgstr ""` |

No C++ or build-script changes are required — this is skin XML and `.po` only.

---

## Looping Behaviour — Verified Sequence

The infinite-loop path was traced through the C++ source in both branches:

```
User navigates to Home
  └─ <onload> fires (GUIAction::ExecuteActions — conditions evaluated first, then actions fired)
       ├─ condition: BackgroundMode=video AND CustomVideoBackground set AND !Player.HasMedia  ← all three checked BEFORE any action runs
       ├─ [1] PlayMedia($INFO[Skin.String(CustomVideoBackground)],noresume)
       │         └─ PlayListPlayer::Play(CFileItemPtr)
       │               ├─ ClearPlaylist(PLAYLIST_VIDEO)
       │               ├─ Reset()                ← resets m_iCurrentSong only; m_repeatState NOT cleared
       │               ├─ SetCurrentPlaylist(PLAYLIST_VIDEO)
       │               ├─ Add(PLAYLIST_VIDEO, item)
       │               └─ Play(0) → g_application.PlayFile() → player starts
       ├─ [2] PlayerControl(RepeatOne)
       │         └─ GetCurrentPlaylist() → PLAYLIST_VIDEO  ✓ (already set in step above)
       │               └─ SetRepeat(PLAYLIST_VIDEO, REPEAT_ONE)
       └─ [3] Skin.SetBool(BgVideoActive)          ← marks that WE started the player

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

User starts playing actual video/audio
  └─ New PlayMedia call clears PLAYLIST_VIDEO and starts a new item
     REPEAT_ONE on that playlist stays set — if the user lets their own video loop they may notice.
     (Acceptable: Kodi's existing PlayerControl(RepeatOff) in the Now Playing bar resets it.)

User leaves Home screen
  └─ <onunload> fires
       ├─ condition: Skin.HasSetting(BgVideoActive)   ← only true if WE started the player
       ├─ PlayerControl(Stop)                         ← stops the background loop
       └─ Skin.Reset(BgVideoActive)                   ← clears the flag

User returns to Home
  └─ <onload> fires again — cycle restarts (only if !Player.HasMedia)
```

### Why `Reset()` does not break `REPEAT_ONE`

`CPlayListPlayer::Reset()` in both branches only resets `m_iCurrentSong = -1` and playlist-change
bookkeeping. The `m_repeatState` array is **never touched** by `Reset()`. (Leia `PlayListPlayer.cpp:457`;
Krypton `PlayListPlayer.cpp:435`.)  
This means `SetRepeat(PLAYLIST_VIDEO, REPEAT_ONE)` from `PlayerControl(RepeatOne)` survives every
automatic re-play triggered by `PLAYBACK_ENDED`, producing a true infinite loop.

### Why the flag approach is required for `<onunload>`

If `<onunload>` used `Player.HasVideo` instead of `Skin.HasSetting(BgVideoActive)`:

- User opens Home, background loop starts → flag is set.
- User clicks on a movie → background loop stops (good, user took over).
- User navigates to a sub-screen **while the movie is still playing**.
- `Player.HasVideo` is `true` → **`PlayerControl(Stop)` would kill the user's movie**. ✗

The flag `BgVideoActive` is only set by our `<onload>` and cleared by our `<onunload>`,
so it is true only while the background loop is the active player item.
