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
#include "cores/VideoPlayer/DVDSubtitles/SubtitlesStyle.h"
#include "settings/SubtitlesSettings.h"
#include "threads/CriticalSection.h"
#include "utils/Observer.h"

#include <atomic>
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

  class COverlay
  {
  public:
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

    float m_x;
    float m_y;
    float m_width;
    float m_height;
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

    void AddOverlay(CDVDOverlay* o, double pts, int index);
    virtual void Render(int idx);

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
    bool HasOverlay(int idx);
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
      SElement()
      {
        overlay_dvd = NULL;
        pts = 0.0;
      }
      double pts;
      CDVDOverlay* overlay_dvd;
    };

    void Render(COverlay* o);
    COverlay* Convert(CDVDOverlay* o, double pts);
    /*!
    * \brief Convert the overlay to a overlay renderer
    * \param o The overlay to convert
    * \param pts The current PTS time
    * \param subStyle The style to be used, MUST BE SET ONLY at the first call or when user change settings
    * \return True if success, false if error
    */
    COverlay* ConvertLibass(
        CDVDOverlayLibass* o,
        double pts,
        bool updateStyle,
        const std::shared_ptr<struct KODI::SUBTITLES::STYLE::style>& overlayStyle);

    void CreateSubtitlesStyle();

    void Release(std::vector<SElement>& list);
    void ReleaseCache();
    void ReleaseUnused();

    /*!
     * \brief Load and store settings locally
     */
    void LoadSettings();

    CCriticalSection m_section;
    std::vector<SElement> m_buffers[NUM_BUFFERS];
    std::map<unsigned int, COverlay*> m_textureCache;
    static unsigned int m_textureid;
    CRect m_rv, m_rs, m_rd;
    std::string m_stereomode;
    int m_subtitlePosition{0}; // Current subtitle position
    int m_subtitlePosResInfo{-1}; // Current subtitle position from resolution info
    int m_subtitleVerticalMargin{0};
    bool m_saveSubtitlePosition{false}; // To save subtitle position permanently
    KODI::SUBTITLES::HorizontalAlign m_subtitleHorizontalAlign{
        KODI::SUBTITLES::HorizontalAlign::CENTER};

    std::shared_ptr<struct KODI::SUBTITLES::STYLE::style> m_overlayStyle;
    std::atomic<bool> m_isSettingsChanged{false};
  };
}
