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

namespace OVERLAY {

  class COverlayQuadsDX
    : public COverlay
  {
  public:
    COverlayQuadsDX(ASS_Image* images, float width, float height);
    virtual ~COverlayQuadsDX();

    void Render(SRenderState& state);

    unsigned int m_count;
    CD3DTexture            m_texture;
    CD3DBuffer             m_vertex;
  };

  class COverlayImageDX
    : public COverlay
  {
  public:
    /*! \brief Create the overlay for rendering
     *  \param o The overlay image
     *  \param rSource The video source rect size
     */
    explicit COverlayImageDX(const CDVDOverlayImage& o, CRect& rSource);
    explicit COverlayImageDX(const CDVDOverlaySpu& o);
    virtual ~COverlayImageDX();

    void Load(const uint32_t* rgba, int width, int height, int stride);
    void Render(SRenderState& state);

    CD3DTexture            m_texture;
    CD3DBuffer             m_vertex;
    bool m_pma{false};
  };

}
