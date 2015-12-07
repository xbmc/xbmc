/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#if defined(HAVE_X11)

#include "WinSystemX11.h"
#include "GL/glx.h"
#include "EGL/egl.h"
#include "rendering/gl/RenderSystemGL.h"
#include "utils/GlobalsHandling.h"

class CGLContext;

class CWinSystemX11GLContext : public CWinSystemX11, public CRenderSystemGL
{
public:
  CWinSystemX11GLContext();
  virtual ~CWinSystemX11GLContext();
  virtual bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction);
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop);
  virtual bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays);
  virtual bool DestroyWindowSystem();
  virtual bool DestroyWindow();

  virtual bool IsExtSupported(const char* extension);

  GLXWindow GetWindow() const;
  GLXContext GetGlxContext() const;
  EGLDisplay GetEGLDisplay() const;
  EGLSurface GetEGLSurface() const;
  EGLContext GetEGLContext() const;
  EGLConfig GetEGLConfig() const;

protected:
  virtual bool SetWindow(int width, int height, bool fullscreen, const std::string &output, int *winstate = NULL);
  virtual bool PresentRenderImpl(const CDirtyRegionList& dirty);
  virtual void SetVSyncImpl(bool enable);
  virtual bool RefreshGLContext(bool force);
  virtual bool DestroyGLContext();
  virtual XVisualInfo* GetVisual();

  CGLContext *m_pGLContext;
  bool m_newGlContext;
};

XBMC_GLOBAL_REF(CWinSystemX11GLContext,g_Windowing);
#define g_Windowing XBMC_GLOBAL_USE(CWinSystemX11GLContext)

#endif //HAVE_X11
