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
#include "threads/CriticalSection.h"
#include "utils/Observer.h"

#include <map>
#include <memory>
#include <vector>

class CDVDOverlay;
class CDVDOverlayLibass;
class CDVDOverlayImage;
class CDVDOverlaySpu;
class CDVDOverlaySSA;
class CDVDOverlayText;

enum SubtitleAlign
{
  SUBTITLE_ALIGN_MANUAL = 0,
  SUBTITLE_ALIGN_BOTTOM_INSIDE,
  SUBTITLE_ALIGN_BOTTOM_OUTSIDE,
  SUBTITLE_ALIGN_TOP_INSIDE,
  SUBTITLE_ALIGN_TOP_OUTSIDE
};

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
  };

  class CRenderer : public Observer
  {
  public:
    CRenderer();
    virtual ~CRenderer();

    // implementation of Observer
    void Notify(const Observable& obs, const ObservableMessage msg) override;

    void AddOverlay(CDVDOverlay* o, double pts, int index);
    virtual void Render(int idx);
    void Flush();
    void Release(int idx);
    bool HasOverlay(int idx);
    void SetVideoRect(CRect &source, CRect &dest, CRect &view);
    void SetStereoMode(const std::string &stereomode);

  protected:

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
    COverlay* ConvertLibass(CDVDOverlayLibass* o,
                            double pts,
                            bool updateStyle,
                            std::shared_ptr<struct KODI::SUBTITLES::style> overlayStyle);

    void CreateSubtitlesStyle();

    void Release(std::vector<SElement>& list);
    void ReleaseCache();
    void ReleaseUnused();

    CCriticalSection m_section;
    std::vector<SElement> m_buffers[NUM_BUFFERS];
    std::map<unsigned int, COverlay*> m_textureCache;
    static unsigned int m_textureid;
    CRect m_rv, m_rs, m_rd;
    std::string m_stereomode;

    std::shared_ptr<struct KODI::SUBTITLES::style> m_overlayStyle;
    bool m_forceUpdateOverlayStyle;
  };
}
