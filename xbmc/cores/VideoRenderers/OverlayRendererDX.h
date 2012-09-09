/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
#include "OverlayRenderer.h"
#include "OverlayRendererUtil.h"
#include "rendering/dx/RenderSystemDX.h"
#include "guilib/D3DResource.h"

#ifdef HAS_DX

class CDVDOverlay;
class CDVDOverlayImage;
class CDVDOverlaySpu;
class CDVDOverlaySSA;
typedef struct ass_image ASS_Image;

namespace OVERLAY {

  class COverlayQuadsDX
    : public COverlayMainThread
  {
  public:
    COverlayQuadsDX(ASS_Image* images, int width, int height);
    virtual ~COverlayQuadsDX();

    void Render(SRenderState& state);

    struct VERTEX {
        FLOAT x, y, z;
        DWORD c;
        FLOAT u, v;
    };

    int                          m_count;
    DWORD                        m_fvf;
    CD3DTexture                  m_texture;
    CD3DVertexBuffer             m_vertex;
  };

  class COverlayImageDX
    : public COverlayMainThread
  {
  public:
    COverlayImageDX(CDVDOverlayImage* o);
    COverlayImageDX(CDVDOverlaySpu*   o);
    virtual ~COverlayImageDX();

    void Load(uint32_t* rgba, int width, int height, int stride);
    void Render(SRenderState& state);

    struct VERTEX {
        FLOAT x, y, z;
        FLOAT u, v;
    };

    DWORD                        m_fvf;
    CD3DTexture                  m_texture;
    CD3DVertexBuffer             m_vertex;
  };

}


#endif