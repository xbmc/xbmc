/*
 *      Initial code sponsored by: Voddler Inc (voddler.com)
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "system_gl.h"
#include "OverlayRenderer.h"

class CDVDOverlay;
class CDVDOverlayImage;
class CDVDOverlaySpu;
class CDVDOverlaySSA;
typedef struct ass_image ASS_Image;

namespace OVERLAY {

  class COverlayTextureGL : public COverlay
  {
  public:
     explicit COverlayTextureGL(CDVDOverlayImage* o);
     explicit COverlayTextureGL(CDVDOverlaySpu* o);
    ~COverlayTextureGL() override;

    void Render(SRenderState& state) override;

    GLuint m_texture;
    float  m_u;
    float  m_v;
    bool   m_pma; /*< is alpha in texture premultiplied in the values */
  };

  class COverlayGlyphGL : public COverlay
  {
  public:
   COverlayGlyphGL(ASS_Image* images, int width, int height);

   ~COverlayGlyphGL() override;

   void Render(SRenderState& state) override;

    struct VERTEX
    {
       GLfloat u, v;
       GLubyte r, g, b, a;
       GLfloat x, y, z;
    };

   VERTEX* m_vertex;
   int     m_count;

   GLuint m_texture;
   float  m_u;
   float  m_v;
  };

}
