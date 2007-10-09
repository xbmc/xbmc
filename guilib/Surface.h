/*!
\file Surface.h
\brief 
*/

#ifndef GUILIB_SURFACE_H
#define GUILIB_SURFACE_H

#pragma once

#include <string>
#ifdef HAS_SDL
#include <SDL/SDL.h>
#endif

#ifdef HAS_SDL_OPENGL
#include <GL/glew.h>
#endif

namespace Surface {
#ifdef HAS_GLX
#include <GL/glx.h>
/*
static Bool WaitForNotify(Display *dpy, XEvent *event, XPointer arg) {
  return (event->type == MapNotify) && (event->xmap.window == (Window) arg);
}
*/
#endif
 
class CSurface
{
public:
  CSurface(CSurface* src) {
    memcpy(this, src, sizeof(CSurface));
    m_pShared = src;
  }
#ifdef HAS_SDL
  CSurface(int width, int height, bool doublebuffer, CSurface* shared,
	   CSurface* associatedWindow, SDL_Surface* parent=0, bool fullscreen=false,
	   bool offscreen=false, bool pbuffer=false, int antialias=0);
#endif
  
  virtual ~CSurface(void);

  int GetWidth() const { return m_iWidth; }
  int GetHeight() const { return m_iHeight; }
  bool IsShared() const { return m_pShared?true:false; }
  bool IsFullscreen() const { return m_bFullscreen; }
  bool IsDoublebuffered() const { return m_bDoublebuffer; }
  bool IsValid() { return m_bOK; }
  void Flip();
  bool MakeCurrent();
  void ReleaseContext();
  void EnableVSync(bool enable=true);
  bool ResizeSurface(int newWidth, int newHeight);
#ifdef HAS_GLX
  GLXContext GetContext() {return m_glContext;}
  GLXWindow GetWindow() {return m_glWindow;}
  GLXPbuffer GetPBuffer() {return m_glPBuffer;}
  GLXPixmap GetPixmap() {return m_glPixmap;}
  bool MakePBuffer();
  bool MakePixmap();
#endif
#ifdef HAS_SDL_OPENGL
  std::string& GetGLVendor() { return s_glVendor; }
  std::string& GetGLRenderer() { return s_glRenderer; }
  void GetGLVersion(int& maj, int&min);
#endif

  // SDL_Surface always there - just sometimes not in use (HAS_GLX)
  SDL_Surface* SDL() {return m_SDLSurface;}

 protected:
  CSurface* m_pShared;
  int m_iWidth;
  int m_iHeight;
  bool m_bFullscreen;
  bool m_bDoublebuffer;
  bool m_bOK;
  bool m_bVSync;
  int m_iVSyncMode;
  short int m_iRedSize;
  short int m_iGreenSize;
  short int m_iBlueSize;
  short int m_iAlphaSize;
#ifdef HAS_GLX
  GLXContext m_glContext;
  GLXWindow  m_glWindow;
  Window  m_parentWindow;
  GLXPixmap  m_glPixmap;
  GLXPbuffer  m_glPBuffer;
  static Display* s_dpy;
#endif
  static bool b_glewInit;
  static std::string s_glVendor;
  static std::string s_glRenderer;

  SDL_Surface* m_SDLSurface;
};

}

#endif // GUILIB_SURFACE_H
