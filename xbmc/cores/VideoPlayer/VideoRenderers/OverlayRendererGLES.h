/*
 *      Initial code sponsored by: Voddler Inc (voddler.com)
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "OverlayRenderer.h"

#include "system_gl.h"

class CDVDOverlay;
class CDVDOverlayImage;
class CDVDOverlaySpu;
class CDVDOverlaySSA;

namespace OVERLAY
{

class COverlayTextureGLES : public COverlay
{
public:
  /*! \brief Create the overlay for rendering
     *  \param o The overlay image
     *  \param rSource The video source rect size
     */
  explicit COverlayTextureGLES(const CDVDOverlayImage& o, CRect& rSource);
  explicit COverlayTextureGLES(const CDVDOverlaySpu& o);
  ~COverlayTextureGLES() override;

  void Render(SRenderState& state) override;

  GLuint m_texture = 0;
  float m_u;
  float m_v;
  bool m_pma; /*< is alpha in texture premultiplied in the values */
};

class COverlayGlyphGLES : public COverlay
{
public:
  COverlayGlyphGLES(ASS_Image* images, float width, float height);

  ~COverlayGlyphGLES() override;

  void Render(SRenderState& state) override;

  struct VERTEX
  {
    GLfloat u, v;
    GLubyte r, g, b, a;
    GLfloat x, y, z;
  };

  std::vector<VERTEX> m_vertex;

  GLuint m_texture = 0;
  float m_u;
  float m_v;
};

} // namespace OVERLAY
