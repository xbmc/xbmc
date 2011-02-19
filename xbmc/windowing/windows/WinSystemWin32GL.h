/*!
\file Surface.h
\brief
*/

#ifndef WINDOW_SYSTEM_WIN32_GL_H
#define WINDOW_SYSTEM_WIN32_GL_H

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
#include "WinSystemWin32.h"
#include "rendering/gl/RenderSystemGL.h"
#include "utils/ReferenceCounting.h"

class CWinSystemWin32GL : public CWinSystemWin32, public CRenderSystemGL, public virtual xbmcutil::Referenced
{
public:
  CWinSystemWin32GL();
  virtual ~CWinSystemWin32GL();

  virtual bool InitRenderSystem();
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop);
  virtual bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays);

protected:
  virtual void SetVSyncImpl(bool enable);
  virtual bool PresentRenderImpl();
  HGLRC m_hglrc;
  BOOL (APIENTRY *m_wglSwapIntervalEXT)( int );
};

#ifdef HAS_GL

/**
 * This is a hack. There will be an instance of a ref to this "global" statically in each
 *  file that includes this header. This is so that the reference couting will
 *  work correctly from the data segment.
 */
static xbmcutil::Referenced::ref<CWinSystemWin32GL> g_WindowingRef(xbmcutil::Singleton<CWinSystemWin32GL>::getInstance);
#define g_Windowing (*(g_WindowingRef))
#endif

#endif // WINDOW_SYSTEM_H

