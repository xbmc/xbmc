/*!
\file Surface.h
\brief
*/

#ifndef WINDOW_SYSTEM_WIN32_GL_H
#define WINDOW_SYSTEM_WIN32_GL_H

#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "WinSystemWin32.h"
#include "rendering/gl/RenderSystemGL.h"
#include "utils/GlobalsHandling.h"

class CWinSystemWin32GL : public CWinSystemWin32, public CRenderSystemGL
{
public:
  CWinSystemWin32GL();
  virtual ~CWinSystemWin32GL();

  virtual bool InitRenderSystem();
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop);
  virtual bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays);

protected:
  virtual void SetVSyncImpl(bool enable);
  virtual bool PresentRenderImpl(const CDirtyRegionList& dirty);
  HGLRC m_hglrc;
  BOOL (APIENTRY *m_wglSwapIntervalEXT)( int );
};

#ifdef HAS_GL

XBMC_GLOBAL_REF(CWinSystemWin32GL,g_Windowing);
#define g_Windowing XBMC_GLOBAL_USE(CWinSystemWin32GL)

#endif

#endif // WINDOW_SYSTEM_H

