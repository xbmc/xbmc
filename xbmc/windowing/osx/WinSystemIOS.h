/*
 *      Copyright (C) 2010 Team XBMC
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

#pragma once

#ifndef WINDOW_SYSTEM_IOSEGL_H
#define WINDOW_SYSTEM_IOSEGL_H

#if defined(__APPLE__) && defined(__arm__)
#include "WinSystem.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "utils/GlobalsHandling.h"

class CWinSystemIOS : public CWinSystemBase, public CRenderSystemGLES
{
public:
  CWinSystemIOS();
  virtual ~CWinSystemIOS();

  virtual bool InitWindowSystem();
  virtual bool DestroyWindowSystem();
  virtual bool CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction);
  virtual bool DestroyWindow();
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop);
  virtual bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays);
  virtual void UpdateResolutions();

  virtual void ShowOSMouse(bool show);

  virtual void NotifyAppActiveChange(bool bActivated);

  virtual bool Minimize();
  virtual bool Restore() ;
  virtual bool Hide();
  virtual bool Show(bool raise = true);

  virtual bool IsExtSupported(const char* extension);

  virtual bool BeginRender();
  virtual bool EndRender();
  
          void InitDisplayLink(void);
          void DeinitDisplayLink(void);
          double GetDisplayLinkFPS(void);

protected:
  virtual bool PresentRenderImpl();
  virtual void SetVSyncImpl(bool enable);

  void        *m_glView; // EAGLView opaque
  void        *m_WorkingContext; // shared EAGLContext opaque
  bool         m_bWasFullScreenBeforeMinimize;
  CStdString   m_eglext;
  int          m_iVSyncErrors;
};

XBMC_GLOBAL_REF(CWinSystemIOS,g_Windowing);
#define g_Windowing XBMC_GLOBAL_USE(CWinSystemIOS)

#endif

#endif // WINDOW_SYSTEM_IOSEGL_H
