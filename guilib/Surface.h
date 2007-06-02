/*!
\file Surface.h
\brief 
*/

#ifndef GUILIB_SURFACE_H
#define GUILIB_SURFACE_H

#pragma once

namespace Surface {

#ifdef HAS_GLX
#include <GL/gl.h>
#include <GL/glx.h>
static Bool WaitForNotify(Display *dpy, XEvent *event, XPointer arg) {
  return (event->type == MapNotify) && (event->xmap.window == (Window) arg);
}
#endif

class CSurface
{
public:
#ifdef HAS_SDL
  CSurface(int width, int height, bool doublebuffer, CSurface* shared,
	   CSurface* associatedWindow, SDL_Surface* parent=0, bool fullscreen=false);
#endif
  
  virtual ~CSurface(void);

  int GetWidth() const { return m_iWidth; }
  int GetHeight() const { return m_iHeight; }
  bool IsShared() const { return m_bShared; }
  bool IsFullscreen() const { return m_bFullscreen; }
  bool IsDoublebuffered() const { return m_bDoublebuffer; }
  bool IsValid() { return m_bOK; }
  void Flip();
#ifdef HAS_GLX
  GLXContext GetContext() {return m_glContext;}
  GLXWindow GetWindow() {return m_glWindow;}
#else
  SDL_Surface* SDL() {return m_SDLSurface;}
#endif

 protected:
  bool m_bShared;
  int m_iWidth;
  int m_iHeight;
  bool m_bFullscreen;
  bool m_bDoublebuffer;
  bool m_bOK;
  short int m_iRedSize;
  short int m_iGreenSize;
  short int m_iBlueSize;
  short int m_iAlphaSize;
#ifdef HAS_GLX
  GLXContext m_glContext;
  GLXWindow  m_glWindow;
  GLXPixmap  m_glPixmap;
  GLXPbuffer  m_glPbuffer;
  static Display* s_dpy;
#else
  SDL_Surface* m_SDLSurface;
#endif
};

}

#endif // GUILIB_SURFACE_H
