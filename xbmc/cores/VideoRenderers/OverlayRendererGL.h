/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once
#include "OverlayRenderer.h"

#include <GL/glew.h>

class CDVDOverlay;
class CDVDOverlayImage;
class CDVDOverlaySpu;
class CDVDOverlaySSA;

#ifdef HAS_GL

namespace OVERLAY {

  class COverlayGL
      : public COverlay
  {
    virtual long Release();
  };


  class COverlayTextureGL
      : public COverlayGL
  {
  public:
     COverlayTextureGL(CDVDOverlayImage* o);
     COverlayTextureGL(CDVDOverlaySpu* o);
    ~COverlayTextureGL();

    void Render(SRenderState& state);

    GLuint m_texture;
    float  m_u;
    float  m_v;
  };

  class COverlayGlyphGL
     : public COverlayGL
  {
  public:
   COverlayGlyphGL(CDVDOverlaySSA* o, double pts);
   ~COverlayGlyphGL();

   void Render(SRenderState& state);

   struct GlyphPosition
   {
     float u0, v0;
     float u1, v1;

     float x0, y0;
     float x1, y1;
   };

   GlyphPosition* m_quads;
   int            m_count;

   GLuint m_texture;
   float  m_u;
   float  m_v;
  }; 

}

#endif

