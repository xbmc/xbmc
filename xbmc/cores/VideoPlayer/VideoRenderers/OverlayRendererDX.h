/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "OverlayRenderer.h"
#include "guilib/D3DResource.h"

class CDVDOverlay;
class CDVDOverlayImage;
class CDVDOverlaySpu;
class CDVDOverlaySSA;
typedef struct ass_image ASS_Image;

namespace OVERLAY {

  class COverlayQuadsDX
    : public COverlay
  {
  public:
    COverlayQuadsDX(ASS_Image* images, int width, int height);
    virtual ~COverlayQuadsDX();

    void Render(SRenderState& state);

    int                    m_count;
    DWORD                  m_fvf;
    CD3DTexture            m_texture;
    CD3DBuffer             m_vertex;
  };

  class COverlayImageDX
    : public COverlay
  {
  public:
    explicit COverlayImageDX(CDVDOverlayImage* o);
    explicit COverlayImageDX(CDVDOverlaySpu*   o);
    virtual ~COverlayImageDX();

    void Load(uint32_t* rgba, int width, int height, int stride);
    void Render(SRenderState& state);

    DWORD                  m_fvf;
    CD3DTexture            m_texture;
    CD3DBuffer             m_vertex;
    bool                   m_pma;
  };

}
