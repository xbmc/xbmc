/*!
\file Surface.h
\brief
*/

#ifndef GUILIB_SURFACE_GL_H
#define GUILIB_SURFACE_GL_H

#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <string>
#include "Surface.h"
#ifdef HAS_SDL
#include <SDL/SDL.h>
#endif

#ifdef HAS_GLX
#include <GL/glx.h>
#endif

#include "GraphicContext.h"

namespace Surface {

#if defined(_LINUX) && !defined(__APPLE__)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

class CSurfaceGL : public CSurface
{
public:
  CSurfaceGL(CSurface* src);
  virtual ~CSurfaceGL();
  CSurfaceGL& operator =(const CSurface &base);

  CSurfaceGL(int width, int height, bool doublebuffer, CSurface* shared,
    CSurface* associatedWindow, CBaseTexture* parent=0, bool fullscreen=false,
           bool offscreen=false, bool pbuffer=false, int antialias=0);

#ifdef HAS_GLX
  GLXContext GetContext() {return m_glContext;}
  GLXWindow GetWindow() {return m_glWindow;}
  GLXPbuffer GetPBuffer() {return m_glPBuffer;}
  bool MakePBuffer();
  bool MakePixmap(int width, int height);
  Display* GetDisplay() {return s_dpy;}
#endif

#ifdef __APPLE__
  void* GetContext() { return m_glContext; }
#endif

  virtual void Flip();
  virtual bool MakeCurrent();
  virtual void ReleaseContext();
  virtual void EnableVSync(bool enable=true);
  virtual bool ResizeSurface(int newWidth, int newHeight);
  virtual void RefreshCurrentContext();
  virtual void* GetRenderWindow();

  bool glxIsSupported(const char* extension);

  // SDL_Surface always there - just sometimes not in use (HAS_GLX)
  CBaseTexture* SDL() {return m_Surface;}

 protected:
  static bool b_glewInit;
  std::string s_glxExt;
#ifdef HAS_GLX
  GLXContext m_glContext;
  GLXWindow  m_glWindow;
  Window  m_parentWindow;
  GLXPbuffer  m_glPBuffer;
  static Display* s_dpy;
#endif
#ifdef __APPLE__
  void* m_glContext;
#endif
#ifdef _WIN32
  HDC m_glDC;
  HGLRC m_glContext;
#endif
};

}

#endif // GUILIB_SURFACE_H
