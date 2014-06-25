#ifndef WINDOW_SYSTEM_X11_GL_H
#define WINDOW_SYSTEM_X11_GL_H

#pragma once

#if defined(HAVE_LIBGL)
/*
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
#include "WinSystemX11.h"
#include "rendering/gl/RenderSystemGL.h"
#include "utils/GlobalsHandling.h"

class CWinSystemX11GL : public CWinSystemX11, public CRenderSystemGL
{
public:
  CWinSystemX11GL();
  virtual ~CWinSystemX11GL();
  virtual bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction);
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop);
  virtual bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays);

  virtual bool IsExtSupported(const char* extension);

protected:
  virtual bool PresentRenderImpl(const CDirtyRegionList& dirty);
  virtual void SetVSyncImpl(bool enable);

  std::string m_glxext;

  int (*m_glXGetVideoSyncSGI)(unsigned int*);
  int (*m_glXWaitVideoSyncSGI)(int, int, unsigned int*);
  int (*m_glXSwapIntervalMESA)(int);
  PFNGLXSWAPINTERVALEXTPROC m_glXSwapIntervalEXT;

  int m_iVSyncErrors;
};

XBMC_GLOBAL_REF(CWinSystemX11GL,g_Windowing);
#define g_Windowing XBMC_GLOBAL_USE(CWinSystemX11GL)

#endif // HAVE_LIBGL

#endif // WINDOW_SYSTEM_H
