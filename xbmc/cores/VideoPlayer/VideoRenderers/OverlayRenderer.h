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
#include "cores/VideoPlayer/DVDSubtitles/SubtitlesStyle.h"
#include "settings/SubtitlesSettings.h"
#include "threads/CriticalSection.h"
#include "utils/Observer.h"

#include <atomic>
#include <map>
#include <memory>
#include <vector>

typedef struct ass_image ASS_Image;

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

  class COverlay
  {
  public:
    static std::shared_ptr<COverlay> Create(const CDVDOverlayImage& o, CRect& rSource);
    static std::shared_ptr<COverlay> Create(const CDVDOverlaySpu& o);
    static std::shared_ptr<COverlay> Create(ASS_Image* images, float width, float height);

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
     * \brief Pre-walk hook: render libass output for the present slot.
     *  Called once per frame on the GUI/main thread before the GUI walk-skip
     *  decision. Caches the ASS_Image* and detect_change flag on each
     *  SElement so ConvertLibass can consume them during the walk without
     *  re-entering libass. Calls MarkDirty internally when libass reports
     *  a visible or changed subtitle.
     */
    void PrepareOverlays(int idx);

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

    /*!
     * \brief True if any overlay in m_buffers[idx] is visible this frame.
     *
     *  PGS/DVB and DVD SPU: ProcessOverlays only inserts at the visible PTS,
     *  so any entry in m_buffers means visible.
     *
     *  libass (TEXT/SSA): the container is added once with no stop PTS and
     *  stays in m_buffers for the whole video. Visibility means
     *  ass_render_frame returned images for the current PTS, cached on
     *  e.renderedImages by PrepareOverlays.
     *
     *  Must be called after PrepareOverlays has run this frame; before that
     *  e.renderedImages reflects the previous frame's state.
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
      // libass output cached by PrepareOverlays; read by ConvertLibass during
      // render. libass owns the pointer; valid only until the next
      // ass_render_frame call.
      ASS_Image* renderedImages{nullptr};
      float renderedFrameWidth{0.0f};
      float renderedFrameHeight{0.0f};
    };

    void Render(COverlay* o);
    std::shared_ptr<COverlay> Convert(SElement& e);
    // Build a COverlay (cached or freshly created) from the libass output
    // already produced by PrepareOverlays. Does not call ass_render_frame.
    std::shared_ptr<COverlay> ConvertLibass(SElement& e);

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
