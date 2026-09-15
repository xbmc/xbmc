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
#include "utils/GLBufferObject.h"

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

private:
  KODI::UTILS::GL::CGLBufferObject m_posVBO{GL_ARRAY_BUFFER};
  KODI::UTILS::GL::CGLBufferObject m_texVBO{GL_ARRAY_BUFFER};
  KODI::UTILS::GL::CGLBufferObject m_IBO{GL_ELEMENT_ARRAY_BUFFER};
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

  GLuint m_texture = 0;
  float m_u;
  float m_v;

private:
  KODI::UTILS::GL::CGLBufferObject m_VBO{GL_ARRAY_BUFFER};
  GLsizei m_vertexCount = 0;
};

} // namespace OVERLAY
