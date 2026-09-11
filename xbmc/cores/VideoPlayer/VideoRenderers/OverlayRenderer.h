/*
 *      Initial code sponsored by: Voddler Inc (voddler.com)
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "BaseRenderer.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlay.h"
#include "cores/VideoPlayer/DVDSubtitles/DVDSubtitlesLibass.h"
#include "cores/VideoPlayer/DVDSubtitles/SubtitlesStyle.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "OverlayRendererAsync.h"
#include "OverlayRendererUtil.h"
#include "settings/SubtitlesSettings.h"
#include "threads/CriticalSection.h"
#include "utils/Observer.h"

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>

class CDVDOverlay;
class CDVDOverlayLibass;
class CDVDOverlayImage;
class CDVDOverlaySpu;
class CDVDOverlaySSA;
class CDVDOverlayText;

namespace OVERLAY {

  struct SRenderState
  {
    float x;
    float y;
    float width;
    float height;
  };

  /*!
   * \brief Mark the entire GUI dirty so the next render pass runs
   *  (not skipped). Overlays (subtitles, debug OSD) are not CGUIControls
   *  and do not set m_controlDirtyState automatically; callers invoke
   *  this at overlay state transitions and on per-frame updates where
   *  needed (debug OSD).
   */
  void MarkDirty();

  /*!
   * \brief Immutable, self-contained output of one asynchronous libass
   *  render pass: ass_render_frame() plus the CPU-side glyph-atlas packing
   *  (convert_quad) that used to run inline on the GUI/render thread.
   *  convert_quad() copies every pixel it needs out of libass's internal
   *  ASS_Image list, so this struct owns everything it refers to and
   *  remains valid indefinitely - in particular, past the point where the
   *  next ass_render_frame() call on the same renderer would invalidate
   *  the original ASS_Image list.
   */
  struct SAssRenderResult
  {
    double pts{0.0};
    bool hasImage{false};
    float frameWidth{0.0f};
    float frameHeight{0.0f};
    SQuads quads;
  };

  //! One request for the async libass worker. Equality (used for request
  //! coalescing) intentionally ignores nothing: an unchanged pts, style
  //! generation, and geometry means the previous render is still valid.
  struct SAssRenderJob
  {
    std::shared_ptr<CDVDSubtitlesLibass> handler;
    double pts{0.0};
    KODI::SUBTITLES::STYLE::renderOpts opts{};
    std::shared_ptr<struct KODI::SUBTITLES::STYLE::style> subStyle;
    // Bumped by CRenderer whenever the user's subtitle style settings
    // change. The worker (not the submitter) decides whether a given
    // render needs to re-apply style, by comparing generations, so a
    // style change can never be lost to request coalescing.
    uint64_t styleGeneration{0};

    bool operator==(const SAssRenderJob& other) const
    {
      return handler == other.handler && pts == other.pts && subStyle == other.subStyle &&
             styleGeneration == other.styleGeneration &&
             opts.frameWidth == other.opts.frameWidth &&
             opts.frameHeight == other.opts.frameHeight &&
             opts.videoWidth == other.opts.videoWidth &&
             opts.videoHeight == other.opts.videoHeight &&
             opts.sourceWidth == other.opts.sourceWidth &&
             opts.sourceHeight == other.opts.sourceHeight && opts.m_par == other.opts.m_par &&
             opts.marginsMode == other.opts.marginsMode && opts.position == other.opts.position &&
             opts.horizontalAlignment == other.opts.horizontalAlignment;
    }
  };

  /*!
   * \brief Decoded, CPU-owned output of one bitmap subtitle (PGS/DVB image
   *  or DVD SPU) decode pass. Unlike libass, the source CDVDOverlay for
   *  these types is stable/immutable once constructed - there is no
   *  ephemeral-pointer hazard - so this struct exists purely to move the
   *  (potentially large) convert_rgba() CPU cost off the GUI thread; the
   *  render thread still reads placement fields directly off the source
   *  CDVDOverlayImage/CDVDOverlaySpu object.
   */
  struct SBitmapRenderResult
  {
    // Not used for pts-matching (bitmap overlays are event-scoped, not
    // re-evaluated per frame); present for interface uniformity only.
    double pts{0.0};
    const CDVDOverlay* sourceOverlay{nullptr}; ///< Identity of the decoded from CDVDOverlay
    bool hasImage{false};
    std::vector<uint32_t> rgba; // tightly packed, width*height, row-major
    int width{0};
    int height{0};
    // SPU only: convert_rgba()'s visible-content crop. Image overlays set
    // these to the full [0,width) x [0,height) rect.
    int minX{0};
    int maxX{0};
    int minY{0};
    int maxY{0};
  };

  struct SBitmapRenderJob
  {
    std::shared_ptr<CDVDOverlay> overlay; // CDVDOverlayImage or CDVDOverlaySpu

    bool operator==(const SBitmapRenderJob& other) const
    {
      return overlay.get() == other.overlay.get();
    }
  };

  class COverlay
  {
  public:
    static std::shared_ptr<COverlay> Create(const CDVDOverlayImage& o,
                                            const SBitmapRenderResult& decoded,
                                            CRect& rSource);
    static std::shared_ptr<COverlay> Create(const CDVDOverlaySpu& o,
                                            const SBitmapRenderResult& decoded);
    static std::shared_ptr<COverlay> Create(const SQuads& quads, float width, float height);

    COverlay();
    virtual ~COverlay();

    virtual void Render(SRenderState& state) = 0;

    enum EType
    {
      TYPE_NONE,
      TYPE_TEXTURE
    } m_type;

    enum EAlign
    {
      ALIGN_SCREEN,
      ALIGN_SCREEN_AR,
      ALIGN_VIDEO,
      ALIGN_SUBTITLE
    } m_align;

    enum EPosition
    {
      POSITION_ABSOLUTE,
      POSITION_ABSOLUTE_SCREEN,
      POSITION_RELATIVE
    } m_pos;

    float m_x{0};
    float m_y{0};
    float m_width{1.0f};
    float m_height{1.0f};
    float m_source_width{0}; // Video source width resolution used to calculate aspect ratio
    float m_source_height{0}; // Video source height resolution used to calculate aspect ratio

  protected:
    /*!
     * \brief Given the resolution ratio determines if it is a 4/3 resolution
     * \param resRatio The resolution ratio (the results of width / height)
     * \return True if the ratio refer to a 4/3 resolution, otherwise false
     */
    bool IsSquareResolution(float resRatio) { return resRatio > 1.22f && resRatio < 1.34f; }
  };

  class CRenderer : public Observer
  {
  public:
    CRenderer();
    virtual ~CRenderer();

    // Implementation of Observer
    void Notify(const Observable& obs, const ObservableMessage msg) override;

    void AddOverlay(std::shared_ptr<CDVDOverlay> o, double pts, int index);
    virtual void Render(int idx, float depth = 0.0f);

    /*!
     * \brief Create the background decode worker(s). Called once per
     *  playback session from CRenderManager::PreInit(), which already
     *  guarantees a consistent thread regardless of caller; torn down in
     *  UnInit(). Cheap (thread creation only), deliberately not deferred
     *  to first use so the one-time cost lands during session startup
     *  rather than when the first subtitle needs to be shown.
     */
    void PreInit();

    /*!
     * \brief Pre-walk hook: submit this frame's (and, when available, the
     *  next presented frame's) subtitle decode requests to the async
     *  workers, and fetch whatever results are ready. Called once per
     *  frame on the GUI/main thread; never blocks on libass or bitmap
     *  decode. Calls MarkDirty internally when the visible/cached result
     *  changes.
     * \param idx the buffer slot about to be presented this frame.
     * \param lookaheadPts pts of the next frame due to be presented after
     *  this one (already subtitle-delay adjusted, matching e.pts), or
     *  DVD_NOPTS_VALUE if none is known yet (paused, trick-play, or
     *  nothing queued) - in which case idx's own pts is used instead.
     */
    void PrepareOverlays(int idx, double lookaheadPts);

    /*!
     * \brief Release resources
     */
    void UnInit();

    void Flush();

    /*!
     * \brief Reset to default values
     */
    void Reset();

    void Release(int idx);

    /*
     * \brief pts already recorded (by AddOverlay, delay-adjusted) for the
     *  TEXT/SSA overlay in buffer slot 'idx', or DVD_NOPTS_VALUE if that
     *  slot has no such overlay. Used by CRenderManager::FrameMove to
     *  supply PrepareOverlays' lookahead pts from the next queued slot,
     *  before that slot becomes the presented one.
     */
    double GetOverlayPts(int idx)const;

    /*!
     * \brief True if any overlay in m_buffers[idx] is visible this frame.
     *  For libass entries this reflects whichever async result
     *  PrepareOverlays most recently selected for this SElement, which may
     *  be a frame or two stale under load - see class-level notes.
     *
     *  Must be called after PrepareOverlays has run this frame; before that
     *  the selected result reflects the previous frame's state.
     */
    bool HasVisibleOverlay(int idx) const;
    void SetVideoRect(CRect &source, CRect &dest, CRect &view);
    void SetStereoMode(const std::string &stereomode);

    /*!
     * \brief Set the subtitle vertical position,
     * it depends on current screen resolution
     * \param value The subtitle position in pixels
     * \param save If true, the value will be saved to resolution info
     */
    void SetSubtitleVerticalPosition(const int value, bool save);

  protected:
    /*!
     * \brief Reset the subtitle position to default value
     */
    void ResetSubtitlePosition();

    /*!
     * \brief Called when the screen resolution is changed
     */
    void OnViewChange();

    struct SElement
    {
      SElement() : overlay_dvd(NULL) { pts = 0.0; }
      double pts;
      std::shared_ptr<CDVDOverlay> overlay_dvd;
      // Most recent async result PrepareOverlays selected for this
      // SElement this frame (libass: closest-pts match within tolerance;
      // bitmap: latest result matching this overlay's identity). May be
      // null if no result is available/acceptable yet.
      std::shared_ptr<const SAssRenderResult> assResult;
      std::shared_ptr<const SBitmapRenderResult> bitmapResult;
    };

    void Render(COverlay* o);
    std::shared_ptr<COverlay> Convert(SElement& e);
    // Build a COverlay (cached or freshly created) from the libass output
    // already produced by PrepareOverlays. Does not call ass_render_frame.
    std::shared_ptr<COverlay> ConvertLibass(SElement& e);
    // Build a COverlay (cached or freshly created) from the bitmap decode
    // already produced by PrepareOverlays. Does not call convert_rgba.
    std::shared_ptr<COverlay> ConvertBitmap(SElement& e);

    void CreateSubtitlesStyle();

    void Release(std::vector<SElement>& list);
    void ReleaseCache();
    void ReleaseUnused();

    /*!
     * \brief Load and store settings locally
     */
    void LoadSettings();

    enum PositonResInfoState
    {
      POSRESINFO_UNSET = -1,
      POSRESINFO_SAVE_CHANGES = -2,
    };

    mutable CCriticalSection m_section;
    std::vector<SElement> m_buffers[NUM_BUFFERS];
    std::map<unsigned int, std::shared_ptr<COverlay>> m_textureCache;
    static unsigned int m_textureid;
    CRect m_rv; // Frame size
    CRect m_rs; // Source size
    CRect m_rd; // Video size, may be influenced by video settings (e.g. zoom)
    std::string m_stereomode;

    // Async decode workers. Created in PreInit(), destroyed in UnInit();
    // null in between playback sessions and briefly at startup.
    std::unique_ptr<CAsyncSubtitleRenderer<SAssRenderJob, SAssRenderResult>> m_assRenderer;
    std::unique_ptr<CAsyncSubtitleRenderer<SBitmapRenderJob, SBitmapRenderResult>> m_bitmapRenderer;

    // Worker-thread-only: read/written exclusively from inside the
    // m_assRenderer render function, never touched from the GUI thread.
    uint64_t m_assLastAppliedStyleGeneration{~0ULL};

    // Bumped whenever the subtitle style is rebuilt; see SAssRenderJob.
    uint64_t m_styleGeneration{0};

    // GUI-thread-only pointer-identity caches: avoid re-uploading to the
    // GPU when the async result driving a slot's overlay hasn't changed
    // since the last frame.
    std::shared_ptr<const SAssRenderResult> m_lastConvertedAssResult;
    std::shared_ptr<COverlay> m_cachedAssOverlay;
    std::shared_ptr<const SBitmapRenderResult> m_lastConvertedBitmapResult;
    const CDVDOverlay* m_cachedBitmapOverlaySource{nullptr};
    std::shared_ptr<COverlay> m_cachedBitmapOverlay;

    // GUI-thread-only: last e.pts seen for the libass container, used to
    // detect seeks/discontinuities directly at the point pts is consumed
    // (see PrepareOverlays) rather than relying on a flush call's timing,
    // which can land before the presented pts actually jumps.
    double m_lastPreparedAssPts{DVD_NOPTS_VALUE};

    // Current subtitle position
    int m_subtitlePosition{0};
    // Current subtitle position from resolution info,
    // or PositonResInfoState enum values for deferred processing
    int m_subtitlePosResInfo{POSRESINFO_UNSET};
    int m_subtitleVerticalMargin{0};
    bool m_saveSubtitlePosition{false}; // To save subtitle position permanently
    KODI::SUBTITLES::HorizontalAlign m_subtitleHorizontalAlign{
        KODI::SUBTITLES::HorizontalAlign::CENTER};
    KODI::SUBTITLES::Align m_subtitleAlign{KODI::SUBTITLES::Align::BOTTOM_OUTSIDE};

    std::shared_ptr<struct KODI::SUBTITLES::STYLE::style> m_overlayStyle;
    std::atomic<bool> m_isSettingsChanged{false};
    // Whether last frame had any image/SPU overlay. Used by PrepareOverlays
    // to detect arrival/disappearance transitions (image/SPU have no
    // per-frame change signal of their own, unlike libass detect_change).
    bool m_prevHadImageSpu{false};
  };
}
