/*
 *      Initial code sponsored by: Voddler Inc (voddler.com)
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
#include "system_gl.h"
#include "OverlayRenderer.h"

class CDVDOverlay;
class CDVDOverlayImage;
class CDVDOverlaySpu;
class CDVDOverlaySSA;
typedef struct ass_image ASS_Image;

#if defined(HAS_GL) || HAS_GLES == 2

namespace OVERLAY {

  class COverlayTextureGL : public COverlay
  {
  public:
     COverlayTextureGL(CDVDOverlayImage* o);
     COverlayTextureGL(CDVDOverlaySpu* o);
    virtual ~COverlayTextureGL();

    void Render(SRenderState& state);

    GLuint m_texture;
    float  m_u;
    float  m_v;
    bool   m_pma; /*< is alpha in texture premultiplied in the values */
  };

  class COverlayGlyphGL : public COverlay
  {
  public:
   COverlayGlyphGL(ASS_Image* images, int width, int height);

   virtual ~COverlayGlyphGL();

   void Render(SRenderState& state);

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

#endif

