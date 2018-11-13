/*
 *      Initial code sponsored by: Voddler Inc (voddler.com)
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "BaseRenderer.h"

#include <vector>
#include <map>

class CDVDOverlay;
class CDVDOverlayImage;
class CDVDOverlaySpu;
class CDVDOverlaySSA;

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
    virtual void PrepareRender() {};

    enum EType
    { TYPE_NONE
    , TYPE_TEXTURE
    , TYPE_GUITEXT
    } m_type;

    enum EAlign
    { ALIGN_SCREEN
    , ALIGN_VIDEO
    , ALIGN_SUBTITLE
    } m_align;

    enum EPosition
    { POSITION_ABSOLUTE
    , POSITION_ABSOLUTE_SCREEN
    , POSITION_RELATIVE
    } m_pos;

    float m_x;
    float m_y;
    float m_width;
    float m_height;
  };

  class CRenderer
  {
  public:
    CRenderer();
    virtual ~CRenderer();

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

    void Render(COverlay* o, float adjust_height);
    COverlay* Convert(CDVDOverlay* o, double pts);
    COverlay* Convert(CDVDOverlaySSA* o, double pts);

    void Release(std::vector<SElement>& list);
    void ReleaseCache();
    void ReleaseUnused();

    CCriticalSection m_section;
    std::vector<SElement> m_buffers[NUM_BUFFERS];
    std::map<unsigned int, COverlay*> m_textureCache;
    static unsigned int m_textureid;
    CRect m_rv, m_rs, m_rd;
    std::string m_font, m_fontBorder;
    std::string m_stereomode;
  };
}
