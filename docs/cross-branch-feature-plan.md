# Cross-Branch Feature Plan: VST2/VST3 Support, Video Homepage Backgrounds, and Large GIF Textures

**Last Updated:** 2026-04-19  
**Based On:** Leia and Krypton branch PRs from OrganizerRo/xbmc  
**Purpose:** Detailed plan to apply these features to any other Kodi branch (Nexus, Omega, Matrix, master, etc.)

---

## Table of Contents

1. [Overview](#overview)
2. [Feature 1: VST2 and VST3 Plugin Support](#feature-1-vst2-and-vst3-plugin-support)
3. [Feature 2: Video Homepage Background / Texture Support](#feature-2-video-homepage-background--texture-support)
4. [Feature 3: Large Size (256M) GIF Texture Support](#feature-3-large-size-256m-gif-texture-support)
5. [Branch Compatibility Matrix](#branch-compatibility-matrix)
6. [Verification Steps](#verification-steps)
7. [Reference: Source PRs and Commits](#reference-source-prs-and-commits)

---

## Overview

This document describes how to port three features from the **Leia** (v18) and **Krypton** (v17) branches to any other Kodi branch. Each feature section includes:

- What the feature does
- Architecture and design decisions
- Files to create or modify (with detailed change descriptions)
- Branch-specific adaptation notes
- Verification checklist

### Key Differences Across Branches

| Aspect | Krypton (v17) | Leia (v18) | Matrix (v19) | Nexus (v20) | Omega (v21+) / master |
|--------|--------------|------------|--------------|-------------|----------------------|
| Default skin | skin.estouchy + skin.estuary | skin.estouchy + skin.estuary | skin.estuary only | skin.estouchy + skin.estuary | skin.estuary only |
| GUITexture class | `CGUITextureBase` (direct) | `CGUITextureBase` (direct) | `CGUITexture` (factory pattern) | `CGUITexture` (factory pattern + `CreateGUITextureFunc`) | `CGUITexture` (factory) |
| ADSP addon API | ✅ `xbmc.audiodsp` present | ✅ `xbmc.audiodsp` present | ❌ Removed | ❌ Removed (only doc references in `GUIInfoManager.cpp`) | ❌ Removed |
| Settings API | `CSettings::GetInstance()` (Krypton) / `CServiceBroker::GetSettingsComponent()` (Leia) | `CServiceBroker` | `CServiceBroker` | `CServiceBroker` | `CServiceBroker` |
| FFmpeg API | Legacy `avcodec_decode_video2`, `avpicture_fill` | Legacy `avcodec_decode_video2`, `avpicture_fill` | Modern `avcodec_send_packet`/`avcodec_receive_frame` | Modern API | Modern API |
| `String.IsEqual()` skin condition | ✅ (added in v17) | ✅ | ✅ | ✅ | ✅ |
| `Skin.SetFile` builtin | ✅ | ✅ | ✅ | ✅ | ✅ |

---

## Feature 1: VST2 and VST3 Plugin Support

### What It Does

Adds a Kodi Audio DSP binary addon (`audiodsp.vsthost`) that lets users load and chain VST2 (`.dll`) and VST3 (`.vst3`) audio effect plugins into Kodi's master audio processing stage. Audio passes through the configured plugin chain in real time during playback.

### Architecture (from Leia/Krypton Implementation)

```
┌─────────────────────────────────────────────────────────────────────┐
│                    Kodi ActiveAE Audio Engine                       │
│                                                                     │
│  Media Source ──► CActiveAEStream ──► DSP Pipeline ──► Audio Sink   │
│                                           │                        │
│                              ┌────────────┼────────────┐           │
│                              │    ADSP Plugin Chain     │           │
│                              │  (xbmc.audiodsp binary)  │           │
│                              │                          │           │
│                              │  StreamCreate()          │           │
│                              │  MasterProcess()         │           │
│                              │    float** in / out      │           │
│                              │    │                     │           │
│                              │    ▼                     │           │
│                              │  [DSPChain]              │           │
│                              │    ├─ VSTPlugin #1       │           │
│                              │    ├─ VSTPlugin #2       │           │
│                              │    └─ VSTPlugin #N       │           │
│                              └──────────────────────────┘           │
└─────────────────────────────────────────────────────────────────────┘
```

### Addon Project Structure

```
kodi-audiodsp-vsthost/
├── CMakeLists.txt              ← Build entry point (CMake 3.16+, C++17)
├── README.md                   ← User-facing documentation
├── resources/
│   └── addon.xml               ← Addon manifest (requires xbmc.audiodsp v0.1.8)
├── scanner/
│   └── vstscanner.cpp          ← Out-of-process plugin scanner executable
└── src/
    ├── addon_main.cpp/.h       ← Addon entry point (get_addon() export, capabilities)
    ├── dsp/
    │   ├── DSPChain.cpp/.h     ← Ordered list of VSTPlugin instances, serialization
    │   └── DSPProcessor.cpp/.h ← Per-stream state: StreamCreate/Destroy/MasterProcess
    ├── plugin/
    │   └── IVSTPlugin.h        ← Abstract interface for VST2 and VST3 plugins
    ├── settings/
    │   ├── PluginSettings.cpp/.h    ← Load/save settings.json, scan paths
    │   └── VSTPluginManager.cpp/.h  ← Scan, cache, load/unload plugins
    ├── util/
    │   └── ParamQueue.h        ← Lock-free parameter change queue
    ├── vst2/
    │   ├── VSTPlugin2.cpp/.h   ← VST2 plugin wrapper (uses vestige headers)
    │   ├── vestige/
    │   │   └── aeffectx.h      ← Open-source VST2 ABI header (vestige project)
    │   └── vst2_types.h        ← VST2 type definitions
    └── vst3/
        ├── VSTHostContext.cpp/.h  ← IHostApplication implementation for VST3
        ├── VSTPlugin3.cpp/.h      ← VST3 plugin wrapper (uses official SDK)
        └── (VST3 SDK fetched via CMake FetchContent)
```

### Key Design Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| VST2 support | Via open-source vestige headers | Original Steinberg VST2 SDK discontinued in 2018; vestige provides ABI-compatible headers legally |
| VST3 support | Via official Steinberg VST3 SDK | Re-licensed to MIT in 2024/2025 — GPL-compatible |
| Core modification | **None** — addon only | Uses existing `xbmc.audiodsp` binary addon API; no Kodi core changes required |
| Audio format | `float**` planar buffers | Kodi's ADSP passes `float**` planar (one pointer per channel), which is natively compatible with VST3 `AudioBusBuffers::channelBuffers32` |
| Crash isolation | Out-of-process scanner (`vstscanner.exe`) | Bad plugins crash the scanner, not Kodi |
| Configuration | JSON files in addon data directory | `settings.json` (scan paths), `chain.json` (plugin chain + state) |
| Platform | Windows initially | Plugin loading is platform-specific; Linux/macOS requires platform adaptations |

### Steps to Apply to Another Branch

#### Step 1: Check ADSP API Availability

The VST addon depends on the `xbmc.audiodsp` binary addon API. Check if it exists:

```bash
# From the target branch root:
find . -path "*/kodi-addon-dev-kit/include/kodi/kodi_adsp*" -o -name "xbmc.audiodsp*"
grep -rn "audiodsp\|ADSP" xbmc/cores/AudioEngine/ | head -20
```

**If ADSP API exists (Krypton, Leia):** Proceed with the addon as-is. The addon.xml `<import addon="xbmc.audiodsp" version="0.1.8"/>` will work.

**If ADSP API was removed (Matrix v19+, Nexus v20+, Omega v21+):** The ADSP addon system was removed from Kodi after Leia. You must either:

- **Option A (Recommended):** Implement as an **AudioEncoder** or **AudioDecoder** binary addon that intercepts audio before the sink. This requires creating a new addon interface or using `xbmc.audioencoder` / `xbmc.audiodecoder` with a passthrough wrapper.
- **Option B:** Re-introduce the ADSP framework from Leia into the target branch (significant effort — requires porting `CActiveAEDSP*` classes from Leia's `xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAEDSP*`).
- **Option C:** Implement as a standalone audio pipeline hook using PipeWire/PulseAudio on Linux or WASAPI on Windows (system-level, outside Kodi).

#### Step 2: Copy the Addon Directory

```bash
cp -r kodi-audiodsp-vsthost/ /path/to/target-branch/kodi-audiodsp-vsthost/
```

#### Step 3: Adapt CMakeLists.txt

Update the `KODI_ADSP_SDK_DIR` default path to match the target branch's addon SDK location:

- **Krypton/Leia:** `xbmc/addons/kodi-addon-dev-kit/include/kodi`
- **Matrix+:** If re-introducing ADSP, match your ported SDK path. If using a different addon type, replace with the appropriate SDK include path.

#### Step 4: Adapt FFmpeg Usage in `vstscanner.cpp` (if applicable)

The scanner does not use FFmpeg directly, but if you add audio format probing, use the branch-appropriate FFmpeg API:
- **Krypton/Leia:** `avcodec_decode_audio4()`
- **Matrix+:** `avcodec_send_packet()` / `avcodec_receive_frame()`

#### Step 5: Build and Install

```bash
cd kodi-audiodsp-vsthost
cmake -S . -B build -DKODI_ADSP_SDK_DIR="/path/to/sdk/include/kodi"
cmake --build build --config Release
# Copy output DLL + addon.xml to Kodi addon directory
```

### Configuration Files

**`settings.json`** (in Kodi addon data directory):
```json
{
  "scannerPath": "C:\\Kodi\\addons\\audiodsp.vsthost\\vstscanner.exe",
  "searchPaths": [
    "C:\\Program Files\\VSTPlugins",
    "C:\\Program Files\\Common Files\\VST3"
  ],
  "activeConfig": "default"
}
```

**`chain.json`** (plugin processing chain):
```json
{
  "plugins": [
    {
      "path": "C:\\VSTPlugins\\MyEQ.dll",
      "format": "vst2",
      "bypassed": false,
      "state": "<base64-encoded plugin state>"
    },
    {
      "path": "C:\\Program Files\\Common Files\\VST3\\Compressor.vst3",
      "format": "vst3",
      "bypassed": false,
      "state": "<base64-encoded plugin state>"
    }
  ]
}
```

### Runtime Flow

1. Kodi starts an audio stream → calls `StreamCreate()`
2. `DSPProcessor` reads `chain.json` from addon data directory
3. Each VST plugin in the chain is loaded and initialized (sample rate, channels, block size)
4. On every audio block, `MasterProcess()` passes audio through the chain sequentially (ping-pong buffers)
5. When playback stops → `StreamDestroy()` unloads all plugins

---

## Feature 2: Video Homepage Background / Texture Support

### What It Does

Adds two capabilities:

1. **Video file backgrounds in the GUI texture system** (`CGUITextureBase` / `CGUITexture`): Allows video files (MP4, MKV, AVI, MOV, M4V, WebM, WMV, FLV, TS) to be used anywhere the skin XML uses `<texture>`, decoded on-the-fly using FFmpeg with only one frame in memory at a time.

2. **Video background loop on the Home screen** (skin XML): Adds a three-mode background selector (Default / Custom Image / Custom Video) to skin settings, with a looping video background that plays in windowed mode on the Home screen.

### Part A: C++ — Video Texture Decoder (`CVideoBackgroundDecoder`)

#### New Files to Create

**`xbmc/guilib/VideoBackgroundDecoder.h`:**
```cpp
class CVideoBackgroundDecoder
{
public:
  CVideoBackgroundDecoder();
  ~CVideoBackgroundDecoder();

  bool Open(const std::string& filename);
  void Close();
  bool IsOpen() const;
  bool Update(unsigned int currentTimeMs);
  const uint8_t* GetCurrentFrame(int& width, int& height) const;

private:
  bool DecodeNextFrame();
  void SeekToStart();

  AVFormatContext* m_fmtCtx;
  AVCodecContext* m_codecCtx;
  SwsContext* m_swsCtx;
  AVFrame* m_avFrame;
  AVFrame* m_rgbFrame;
  uint8_t* m_rgbBuffer;
  int m_videoStream;
  int m_width, m_height;
  double m_timeBase;
  unsigned int m_nextFrameMs;
  bool m_isOpen;
};
```

**Key design:** Only one BGRA frame buffer (~8 MB for 1080p) is kept in memory, versus ~91 MB for GIF frames. Video decoding is streaming — frames are decoded on-the-fly each render tick.

#### Branch-Specific FFmpeg API Adaptation

**Krypton / Leia (legacy FFmpeg API):**
```cpp
// Codec context from stream:
m_codecCtx = m_fmtCtx->streams[m_videoStream]->codec;
AVCodec* codec = avcodec_find_decoder(m_codecCtx->codec_id);
avcodec_open2(m_codecCtx, codec, nullptr);

// Decode:
int gotFrame = 0;
avcodec_decode_video2(m_codecCtx, m_avFrame, &gotFrame, &pkt);

// Buffer setup:
int bufSize = avpicture_get_size(AV_PIX_FMT_BGRA, m_width, m_height);
avpicture_fill((AVPicture*)m_rgbFrame, m_rgbBuffer, AV_PIX_FMT_BGRA, m_width, m_height);
```

**Matrix / Nexus / Omega / master (modern FFmpeg API):**
```cpp
// Codec context allocation (required since stream->codec is deprecated):
const AVCodec* codec = avcodec_find_decoder(m_fmtCtx->streams[m_videoStream]->codecpar->codec_id);
m_codecCtx = avcodec_alloc_context3(codec);
avcodec_parameters_to_context(m_codecCtx, m_fmtCtx->streams[m_videoStream]->codecpar);
avcodec_open2(m_codecCtx, codec, nullptr);

// Decode (send/receive pattern):
avcodec_send_packet(m_codecCtx, &pkt);
int ret = avcodec_receive_frame(m_codecCtx, m_avFrame);
if (ret == 0) { /* gotFrame */ }

// Buffer setup (avpicture_fill is deprecated):
int bufSize = av_image_get_buffer_size(AV_PIX_FMT_BGRA, m_width, m_height, 1);
av_image_fill_arrays(m_rgbFrame->data, m_rgbFrame->linesize,
                     m_rgbBuffer, AV_PIX_FMT_BGRA, m_width, m_height, 1);
```

#### Files to Modify

**`xbmc/guilib/CMakeLists.txt`** — Add `VideoBackgroundDecoder.cpp` and `VideoBackgroundDecoder.h` to the source lists.

**`xbmc/guilib/GUITexture.h`** — Add members to the texture class:

For **Krypton/Leia** (`CGUITextureBase`):
```cpp
#include <memory>

class CVideoBackgroundDecoder;  // forward declare

// In CGUITextureBase protected section:
std::unique_ptr<CVideoBackgroundDecoder> m_videoDecoder;
CTexture* m_videoTexture;
bool m_isVideoBackground;
unsigned int m_videoStartTime;

bool UpdateVideoFrame(unsigned int currentTime);
static bool IsVideo(const std::string& filename);
```

For **Matrix/Nexus/Omega** (`CGUITexture` with factory pattern):
```cpp
// Same members but in CGUITexture's protected section
// The factory pattern means you add to the base class declaration
```

**`xbmc/guilib/GUITexture.cpp`** — Changes in three areas:

1. **Constructor initialization:** Set `m_videoTexture = nullptr`, `m_isVideoBackground = false`, `m_videoStartTime = 0`
2. **`AllocResources()`:** Before the normal image/GIF path, check `IsVideo(m_info.filename)`. If true, open `CVideoBackgroundDecoder`, decode the first frame, create a `CTexture` from the BGRA pixels, and inject it into `m_texture`.
3. **`Process()`:** Before `UpdateAnimFrame()`, check `m_isVideoBackground` and call `UpdateVideoFrame(currentTime)` which decodes the next frame on schedule and re-uploads to GPU.
4. **`FreeResources()`:** Clean up `m_videoDecoder`, `m_videoTexture`, and clear the texture array.
5. **`UpdateVideoFrame()`:** New method that checks elapsed time, calls `m_videoDecoder->Update()`, gets the frame, and updates the GPU texture via `LoadFromMemory` + `LoadToGPU`.
6. **`IsVideo()`:** New static method that checks file extension against video types.

### Part B: Skin XML — Video Homepage Background Loop

This part is **skin-only** (no C++ changes beyond Part A) and applies to skins that have a Home screen with background settings.

#### Applicable Skins by Branch

| Branch | `skin.estouchy` | `skin.estuary` |
|--------|----------------|----------------|
| Krypton | ✅ Apply | ✅ Can apply (similar structure) |
| Leia | ✅ Apply | ✅ Can apply |
| Matrix | ❌ Not present | ✅ Apply (adapt) |
| Nexus | ✅ Apply | ✅ Apply (adapt) |
| Omega+ / master | ❌ Not present | ✅ Apply (adapt) |

#### Step 1: Background Mode Skin String

Replace the boolean `UseCustomBackground` toggle with a three-mode string:

| `Skin.String(BackgroundMode)` Value | Meaning |
|------|---------|
| `""` (empty / not set) | Default — skin's built-in gradient + fanart |
| `"image"` | User-chosen still image or image resource-pack slideshow |
| `"video"` | User-chosen video file played as a looping background |

**In `Includes.xml` (`CommonBackground`):**
- Replace `Skin.HasSetting(UseCustomBackground)` with `String.IsEqual(Skin.String(BackgroundMode),image)` for the custom image control.
- Add a `<control type="multiimage">` for resource-pack slideshows (visible when BackgroundMode=image AND `CustomBackground.path` is set).
- Keep the existing single-file custom image as fallback.
- The existing `<control type="videowindow">` needs **no changes** — it already shows the main player's video.

#### Step 2: Settings UI

Replace the Background grouplist with:

1. **Mode selector** — three radiobuttons:
   - "Default background" → `Skin.SetString(BackgroundMode,)` (clears)
   - "Custom image background" → `Skin.SetString(BackgroundMode,image)`
   - "Custom video background" → `Skin.SetString(BackgroundMode,video)`

2. **Image sub-section** (enabled when `BackgroundMode=image`):
   - "Pick image file" → `Skin.SetImage(CustomBackground)` (existing)
   - "Select image resource pack" → `RunScript(script.image.resource.select,property=CustomBackground&type=resource.images.skinbackgrounds)` (new)

3. **Video sub-section** (enabled when `BackgroundMode=video`):
   - "Pick background video" → `Skin.SetFile(CustomVideoBackground,.mp4|.mkv|.avi|.mov|.m4v|.wmv|.ts|.mpg|.flv)`

#### Step 3: Home.xml — Video Loop Lifecycle

**Critical design: Uses the MAIN player, not a secondary player.**

```xml
<!-- Start background video windowed + loop if nothing playing -->
<onload condition="String.IsEqual(Skin.String(BackgroundMode),video) + !String.IsEmpty(Skin.String(CustomVideoBackground)) + !Player.HasMedia">
  PlayMedia($INFO[Skin.String(CustomVideoBackground)],1,noresume)
</onload>
<!-- Re-enable RepeatOne — whether we just started or are resuming -->
<onload condition="String.IsEqual(Skin.String(BackgroundMode),video) + Player.HasVideo">
  PlayerControl(RepeatOne)
</onload>

<!-- On leaving Home: disable repeat, NEVER stop -->
<onunload condition="String.IsEqual(Skin.String(BackgroundMode),video)">
  PlayerControl(RepeatOff)
</onunload>
```

**Why these exact parameters:**

| Parameter | Reason |
|-----------|--------|
| `PlayMedia(...,1,noresume)` | The `1` parameter calls `SetVideoStartWindowed(true)`, preventing fullscreen switch so the video stays on the Home screen. `noresume` prevents a resume dialog. |
| `PlayerControl(RepeatOne)` | Separate from `PlayMedia` because `PlayMedia` does not accept `repeat` as a parameter. Sets `REPEAT_ONE` on `PLAYLIST_VIDEO`. |
| `PlayerControl(RepeatOff)` on unload | **NOT `Stop`**. When a user selects a movie from Home, the async fullscreen switch (`TMSG_SWITCHTOFULLSCREEN`) means `<onunload>` fires while the *user's movie* is playing. `Stop` would kill it. `RepeatOff` only clears the repeat flag — safe for all scenarios. |
| `!Player.HasMedia` guard | Prevents interrupting audio playback or any existing media. |
| Second `<onload>` with `Player.HasVideo` | Re-enables `RepeatOne` when returning to Home while the background video is still playing (it was set to `RepeatOff` when navigating away). |

#### Verified Scenarios

**Scenario A — User navigates away without playing media:**
1. `<onunload>` → `RepeatOff` → loop disabled
2. Background video continues playing until EOF, then stops naturally
3. User returns to Home → `!Player.HasMedia` = true → `PlayMedia` restarts background

**Scenario B — User selects a movie from Home:**
1. `PlayMedia(movie)` replaces background in main player
2. Async `TMSG_SWITCHTOFULLSCREEN` → `ActivateWindow(FULLSCREEN_VIDEO)` → `CloseWindowSync(Home)` → `<onunload>` fires
3. At unload time, player IS playing user's movie → `RepeatOff` clears repeat (safe)
4. User's movie plays normally, does not loop unexpectedly

**Scenario C — User returns to Home while movie still playing:**
1. `<onload>` → `!Player.HasMedia` = false → `PlayMedia` skipped (correct)
2. `<onload>` → `Player.HasVideo` = true → `RepeatOne` fires (minor side effect: user's movie gets repeat, user can toggle off)

#### Step 4: Localization Strings

Add to `strings.po` (first available IDs in the 31566+ block):

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

Add the same IDs (with empty `msgstr ""`) to all other language `strings.po` files.

---

## Feature 3: Large Size (256M) GIF Texture Support

### What It Does

Increases the GIF animated texture memory limit from **~91 MB** (~12 full-HD frames, ~9 seconds of animation) to **256 MB** (~33 full-HD frames, ~27 seconds), allowing longer animated GIF backgrounds.

### Current Limitation (All Branches)

In `xbmc/guilib/TextureManager.cpp`, the `CGUITextureManager::Load()` method has a hard-coded memory cap:

```cpp
uint64_t maxMemoryUsage = 91238400; // 1920*1080*4*11 bytes ≈ 91 MB
```

When loading animated GIFs, frames are pre-extracted into GPU textures. Once total memory usage exceeds this limit, loading stops and remaining frames are discarded.

### The Change

Replace the hard-coded limit with **268435456** (256 × 1024 × 1024 = 256 MB):

```cpp
uint64_t maxMemoryUsage = 268435456; // 256 MB — allows ~33 full-HD frames
```

### Location by Branch

| Branch | File | Line (approximate) |
|--------|------|-----|
| Krypton | `xbmc/guilib/TextureManager.cpp` | ~425 |
| Leia | `xbmc/guilib/TextureManager.cpp` | ~389 |
| Matrix | `xbmc/guilib/TextureManager.cpp` | ~391 (verify) |
| Nexus | `xbmc/guilib/TextureManager.cpp` | ~391 |
| Omega / master | `xbmc/guilib/TextureManager.cpp` | ~403 |

All branches use the identical pattern:
```cpp
uint64_t maxMemoryUsage = 91238400;// 1920*1080*4*11 bytes, i.e, a total of approx. 12 full hd frames
```

### Steps to Apply

1. Open `xbmc/guilib/TextureManager.cpp`
2. Find the line containing `maxMemoryUsage = 91238400`
3. Replace with:
   ```cpp
   uint64_t maxMemoryUsage = 268435456; // 256 MB — allows ~33 full-HD frames for animated GIF backgrounds
   ```
4. Update the log message below it (search for `(maxMemoryUsage/11)*12` or `(maxMemoryUsage / 11) * 12`) to use `(maxMemoryUsage/33)*34` or simply `maxMemoryUsage` to reflect the new limit.

### Memory Impact Considerations

- **91 MB (current):** Safe on devices with ≥512 MB GPU memory (all modern devices)
- **256 MB (proposed):** Requires devices with ≥1 GB GPU memory. This is standard for any device from 2015+, including Raspberry Pi 3/4 and all x86 systems. However, for embedded/low-memory devices (RPi 1/2, very old Android boxes), consider making this configurable via `advancedsettings.xml`.

### Optional: Make Configurable

For a more robust implementation, add an `advancedsettings.xml` option:

```xml
<advancedsettings>
  <gui>
    <giftexturememorylimit>268435456</giftexturememorylimit>
  </gui>
</advancedsettings>
```

This requires:
1. Adding `m_guiGifTextureMemoryLimit` to `CAdvancedSettings` (`xbmc/settings/AdvancedSettings.h/.cpp`)
2. Parsing the value in `CAdvancedSettings::ParseSettingsFile()`
3. Using `CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiGifTextureMemoryLimit` in `TextureManager.cpp` instead of the hard-coded value

---

## Branch Compatibility Matrix

### Feature Applicability

| Feature | Krypton | Leia | Matrix | Nexus | Omega+ / master |
|---------|---------|------|--------|-------|-----------------|
| VST2/VST3 addon | ✅ Direct (ADSP API exists) | ✅ Direct (ADSP API exists) | ⚠️ Needs ADSP API port or alternative addon type | ⚠️ Needs ADSP API port or alternative addon type | ⚠️ Needs ADSP API port or alternative addon type |
| Video texture decoder (C++) | ✅ Legacy FFmpeg | ✅ Legacy FFmpeg | ⚠️ Modern FFmpeg API | ⚠️ Modern FFmpeg API | ⚠️ Modern FFmpeg API |
| Video homepage background (skin) | ✅ skin.estouchy | ✅ skin.estouchy | ✅ skin.estuary (adapt) | ✅ skin.estouchy + skin.estuary | ✅ skin.estuary (adapt) |
| 256M GIF texture limit | ✅ Direct | ✅ Direct | ✅ Direct | ✅ Direct | ✅ Direct |

### GUITexture Class Adaptation

**Krypton / Leia:**
- Class: `CGUITextureBase` in `GUITexture.h`/`GUITexture.cpp`
- Direct member access — add members to protected section
- No factory pattern

**Matrix / Nexus / Omega / master:**
- Class: `CGUITexture` with `CreateGUITextureFunc` factory
- Virtual destructor: `virtual ~CGUITexture() = default;` — override in implementation
- Must add video members to the base `CGUITexture` class (not to platform-specific subclasses like `CGUITextureGL`, `CGUITextureGLES`, `CGUITextureDX`)
- The `Process()`, `AllocResources()`, `FreeResources()` methods remain in the base class

---

## Verification Steps

### V1: VST2/VST3 Addon Verification

- [ ] **V1.1 — Build check:** The addon compiles without errors with the target branch's addon SDK.
  ```bash
  cd kodi-audiodsp-vsthost
  cmake -S . -B build -DKODI_ADSP_SDK_DIR="/path/to/sdk"
  cmake --build build --config Release
  ```
  Expected: Build succeeds, produces `audiodsp.vsthost.dll` (or `.so` on Linux).

- [ ] **V1.2 — Addon XML validation:** `addon.xml` passes schema validation and the `<import addon="xbmc.audiodsp"...>` dependency exists in the target branch.
  ```bash
  grep -r "xbmc.audiodsp" /path/to/target-branch/
  ```
  Expected: At least the addon definition file and the core addon-types registration reference `xbmc.audiodsp`. If not found, the ADSP API needs to be ported (see Step 1 in Feature 1).

- [ ] **V1.3 — Runtime test:** Install the addon in Kodi. Create `settings.json` and `chain.json` with at least one known-good VST3 plugin. Play an audio file. Verify audio passes through the VST chain (audible effect or log output).
  ```
  Expected log entries:
  - "audiodsp.vsthost: StreamCreate ..."
  - "audiodsp.vsthost: Loaded VST3 plugin: ..."
  - "audiodsp.vsthost: MasterProcess called ..."
  ```

- [ ] **V1.4 — Scanner test:** Run `vstscanner.exe` with a directory containing both VST2 and VST3 plugins. Verify it produces `plugin_cache.json` without crashes.

- [ ] **V1.5 — No core modifications:** Confirm zero changes to Kodi core source files (the addon should be self-contained).
  ```bash
  git diff --stat HEAD  # Should show only files within kodi-audiodsp-vsthost/
  ```

### V2: Video Texture Decoder Verification

- [ ] **V2.1 — Build check:** Target branch compiles with `VideoBackgroundDecoder.cpp/.h` added to `xbmc/guilib/CMakeLists.txt`.
  ```bash
  cmake --build . --target kodi
  ```
  Expected: No compilation errors in `VideoBackgroundDecoder.cpp`, `GUITexture.cpp`, or `GUITexture.h`.

- [ ] **V2.2 — FFmpeg API compatibility:** Verify no deprecation warnings/errors for the FFmpeg API used.
  - Krypton/Leia: `avcodec_decode_video2`, `avpicture_fill` — should compile cleanly
  - Matrix+: Must use `avcodec_send_packet`/`avcodec_receive_frame`, `av_image_fill_arrays` — verify these are available in the branch's FFmpeg dependency version

- [ ] **V2.3 — Video extension detection test:** In `GUITexture.cpp`, the `IsVideo()` static method should return `true` for `.mp4`, `.mkv`, `.avi`, `.mov`, `.m4v`, `.webm`, `.wmv`, `.flv`, `.ts` and `false` for `.gif`, `.png`, `.jpg`.

- [ ] **V2.4 — Functional test:** Create a skin XML with `<texture>test.mp4</texture>` pointing to a short video file. Verify:
  - The video decodes and displays as a texture
  - Frames advance smoothly (not frozen on first frame)
  - Memory usage stays stable (~8 MB for 1080p, not growing)
  - Video loops back to the beginning when it reaches EOF

- [ ] **V2.5 — Fallback test:** Set `<texture>nonexistent.mp4</texture>`. Verify:
  - Log shows `"CGUITextureBase: Failed to open video background: ..."`
  - No crash
  - `m_isAllocated` is `NORMAL_FAILED`

### V3: Video Homepage Background Verification

- [ ] **V3.1 — Skin XML validation:** Verify all skin XML files parse correctly (Kodi starts without XML errors in log).

- [ ] **V3.2 — Three-mode selector:** Open Skin Settings → Background section. Verify three modes are available and switching between them updates `Skin.String(BackgroundMode)`.

- [ ] **V3.3 — Default mode:** With BackgroundMode empty, verify the default gradient (`primary.jpg`) is displayed.

- [ ] **V3.4 — Image mode:** Set BackgroundMode to "image", pick an image file. Verify:
  - Custom image is displayed as background
  - Image resource-pack selector works (if `script.image.resource.select` is installed)

- [ ] **V3.5 — Video mode — Basic loop:** Set BackgroundMode to "video", pick a video file. Navigate to Home. Verify:
  - Video plays in windowed mode on the Home screen (not fullscreen)
  - Video loops continuously
  - The `videowindow` control shows the video

- [ ] **V3.6 — Video mode — Navigate away:** From Home with video playing, navigate to Settings. Verify:
  - `RepeatOff` fires (check log for `PlayerControl(RepeatOff)`)
  - Video continues playing until EOF, then stops
  - Returning to Home restarts the background video

- [ ] **V3.7 — Video mode — User selects a movie:** From Home with video playing, start a movie. Verify:
  - User's movie plays normally in fullscreen
  - Background video is replaced (not stopped abruptly)
  - `RepeatOff` fires in `<onunload>`, user's movie does NOT loop
  - After movie ends and user returns to Home, background video restarts

- [ ] **V3.8 — Localization:** Verify string IDs 31566–31572 appear in all language `strings.po` files (en_gb with translations, others with empty `msgstr ""`).

### V4: Large GIF Texture Verification

- [ ] **V4.1 — Code change:** Verify `maxMemoryUsage` in `TextureManager.cpp` is changed from `91238400` to `268435456`.

- [ ] **V4.2 — Functional test with large GIF:** Create or obtain a GIF file with >12 full-HD frames (e.g., 30 frames at 1920×1080). Load it as a skin background texture. Verify:
  - More than 12 frames are loaded (check log: `"Memory limit ... exceeded, N frames extracted"` should show N > 12)
  - Animation plays smoothly through all loaded frames

- [ ] **V4.3 — Memory monitoring:** While the large GIF is loaded, monitor Kodi's memory usage. Verify:
  - Peak memory does not exceed ~300 MB for the texture (256 MB limit + overhead)
  - No out-of-memory crashes on systems with ≥2 GB RAM

- [ ] **V4.4 — Backward compatibility:** Load a small GIF (< 91 MB total frames). Verify it still loads and animates correctly (all frames loaded, no truncation).

---

## Reference: Source PRs and Commits

### Leia Branch

| PR / Commit | Description |
|-------------|-------------|
| PR #26 — `skin.estouchy: Add video homepage backgrounds and image resource-pack selector` | Skin XML + localization for video backgrounds (merged) |
| Commit `97daf1eb48` — `feat(guilib): add video file support for texture backgrounds` | C++ VideoBackgroundDecoder + GUITexture integration |
| Commit `e717925247` — `feat: vst plugins support` | VST2/VST3 addon + build files (note: this commit also overwrote some Leia core files — see `54cea3ff2a` for the fix) |
| Commit `54cea3ff2a` — `fix(build): restore Leia source files regressed by VST commit` | Restores core files accidentally replaced by the VST commit |

### Krypton Branch

| Commit | Description |
|--------|-------------|
| `7b9704e423` — `Add VST support planning, and video background support` | VST research docs + VideoBackgroundDecoder + TextureManager video extension detection |
| `1a5372c948` — `Add VST build. Fix win32 steps` | Complete VST addon build (kodi-audiodsp-vsthost/) |
| `ca38ac5457` / `c842443ef4` — `chore: 9 - VST Usage Examples` | VST configuration examples |
| `53a280875e` — `chore: 10 Addons Build & Usage Examples` | Build documentation |

### Supporting Documents

| Document | Location | Description |
|----------|----------|-------------|
| `docs/estouchy-video-backgrounds-plan.md` | Leia branch | Detailed skin XML plan with C++ source verification |
| `VST_RESEARCH/VST_IMPLEMENTATION_PLAN.md` | Krypton branch | Full VST3 architecture and implementation plan |
| `VST_RESEARCH/audio_pipeline.md` | Krypton branch | Kodi audio pipeline research |
| `VST_RESEARCH/vst2_technical.md` | Krypton branch | VST2 ABI technical details |
| `VST_RESEARCH/vst_hosting.md` | Krypton branch | VST hosting strategies and SDK licensing |
| `VIDEO_BACKGROUND_PLAN.md` | Krypton branch | Video background architecture and DLL feasibility analysis |
| `kodi-audiodsp-vsthost/README.md` | Both branches | VST addon user documentation |
