# Plan: Video Backgrounds & Image Resource Selectors for `skin.estouchy` (Leia & Krypton)

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

3. **`multiimage` handles both a single file *and* a resource-pack directory path.**  
   `CGUIMultiImage::CMultiImageJob::DoWork()` uses `CGUITextureManager::GetTexturePath()` to
   resolve `resource://` URIs, then calls `CDirectory::GetDirectory()` to enumerate images. A
   plain file path also works (fast-path in `LoadDirectory()`).

4. **`script.image.resource.select` sets three skin strings** when `property=Foo` is passed:  
   `Skin.String(Foo.path)` (directory), `Skin.String(Foo.ext)` (image extension),
   `Skin.String(Foo.name)` (display name). This is the same convention used by Estuary.

5. **`Skin.SetFile` mask for video files.**  
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

**In `Home.xml`**, add conditional `onload` / `onunload` actions:

```xml
<!-- Start background video loop when BackgroundMode=video, video path is set, and nothing else is playing -->
<onload condition="String.IsEqual(Skin.String(BackgroundMode),video) + !String.IsEmpty(Skin.String(CustomVideoBackground)) + !Player.HasMedia">
  PlayMedia($INFO[Skin.String(CustomVideoBackground)],noresume)
</onload>
<onload condition="String.IsEqual(Skin.String(BackgroundMode),video) + !String.IsEmpty(Skin.String(CustomVideoBackground)) + !Player.HasMedia">
  PlayerControl(RepeatOne)
</onload>
<!-- Stop background video when leaving Home (only if we started it) -->
<onunload condition="String.IsEqual(Skin.String(BackgroundMode),video) + Player.HasVideo">
  PlayerControl(Stop)
</onunload>
```

The existing `<control type="videowindow">` in `CommonBackground` needs **no changes** — its
current `<visible>` condition (`Player.HasVideo + !Skin.HasSetting(HideVideoBackground) + ...`) will
naturally display the looping background video when it is playing, and the user's real media when
that takes over.

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

## Looping Behaviour — Sequence Diagram

```
User navigates to Home
  └─ <onload> fires
       ├─ condition: BackgroundMode=video AND CustomVideoBackground set AND !Player.HasMedia
       ├─ PlayMedia($INFO[Skin.String(CustomVideoBackground)],noresume)   → starts main player
       └─ PlayerControl(RepeatOne)                                         → enables single-file repeat

videowindow (already in CommonBackground) becomes visible
  └─ condition: Player.HasVideo (now true) + !HideVideoBackground + !PVR windows

User starts playing actual video/audio
  └─ PlayMedia for the real item takes over the main player
     Background loop is interrupted — expected behaviour

User leaves Home screen
  └─ <onunload> fires
       ├─ condition: BackgroundMode=video AND Player.HasVideo (background was still running)
       └─ PlayerControl(Stop)                                              → stops background loop

User returns to Home
  └─ <onload> fires again — cycle restarts (only if !Player.HasMedia)
```
