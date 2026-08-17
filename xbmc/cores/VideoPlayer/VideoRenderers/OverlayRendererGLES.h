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
#include "OverlayRendererUtil.h"

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
  //! How this overlay's PGS (Blu-ray bitmap) palette was handled when its
  //! texture was built - see OVERLAY::GetPgsHdrHandling(). NONE for every
  //! non-PGS overlay.
  //!
  //! Resolved once, here, from current HdrPgsMode/display-tag state.
  //! Unlike CDVDOverlayImage::isPgs (DVDOverlayImage.h), which is a pure
  //! decode-time content fact, this depends on live state. It is
  //! intentionally not re-read per frame; mode/tag changes are expected
  //! to be accompanied by a new overlay/rebuild. If that expectation
  //! ever changes, this must be invalidated accordingly.
  OVERLAY::PgsHdrHandling m_pgsHandling = OVERLAY::PgsHdrHandling::NONE;
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
  GLuint m_VBO = 0;
  GLsizei m_vertexCount = 0;
  float m_u;
  float m_v;
};

} // namespace OVERLAY
