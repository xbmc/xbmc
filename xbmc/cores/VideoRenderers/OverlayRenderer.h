/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *      Initial code sponsored by: Voddler Inc (voddler.com)
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "threads/CriticalSection.h"

#include <vector>

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

    virtual COverlay* Acquire();
    virtual long      Release();
    virtual void      Render(SRenderState& state) = 0;

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
    , POSITION_RELATIVE
    } m_pos;

    float m_x;
    float m_y;
    float m_width;
    float m_height;

  protected:
    long m_references;
  };

  class COverlayMainThread
      : public COverlay
  {
  public:
    virtual ~COverlayMainThread() {}
    virtual long Release();
  };


  class CRenderer
  {
  public:
     CRenderer();
    ~CRenderer();

    void AddOverlay(CDVDOverlay* o, double pts);
    void AddOverlay(COverlay*    o, double pts);
    void AddCleanup(COverlay*    o);
    void Flip();
    void Render();
    void Flush();

  protected:

    struct SElement
    {
      SElement()
      {
        overlay_dvd = NULL;
        overlay     = NULL;
      }
      double pts;
      CDVDOverlay* overlay_dvd;
      COverlay*    overlay;
    };

    typedef std::vector<COverlay*>  COverlayV;
    typedef std::vector<SElement>   SElementV;

    void      Render(COverlay* o);
    COverlay* Convert(CDVDOverlay* o, double pts);
    COverlay* Convert(CDVDOverlaySSA* o, double pts);

    void      Release(COverlayV& list);
    void      Release(SElementV& list);

    CCriticalSection m_section;
    SElementV        m_buffers[2];
    int              m_decode;
    int              m_render;

    COverlayV        m_cleanup;
  };
}
