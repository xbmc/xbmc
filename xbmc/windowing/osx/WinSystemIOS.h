/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#ifndef WINDOW_SYSTEM_IOSEGL_H
#define WINDOW_SYSTEM_IOSEGL_H

#if defined(TARGET_DARWIN_IOS)
#include "windowing/WinSystem.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "utils/GlobalsHandling.h"
#include "threads/CriticalSection.h"

class IDispResource;
class CVideoSyncIos;
struct CADisplayLinkWrapper;

class CWinSystemIOS : public CWinSystemBase, public CRenderSystemGLES
{
public:
  CWinSystemIOS();
  virtual ~CWinSystemIOS();

  virtual bool InitWindowSystem();
  virtual bool DestroyWindowSystem();
  virtual bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction);
  virtual bool DestroyWindow();
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop);
  virtual bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays);
  virtual void UpdateResolutions();
  virtual bool CanDoWindowed() { return false; }

  virtual void ShowOSMouse(bool show);
  virtual bool HasCursor();

  virtual void NotifyAppActiveChange(bool bActivated);

  virtual bool Minimize();
  virtual bool Restore() ;
  virtual bool Hide();
  virtual bool Show(bool raise = true);

  virtual bool IsExtSupported(const char* extension);

  virtual bool BeginRender();
  virtual bool EndRender();
  
  virtual void Register(IDispResource *resource);
  virtual void Unregister(IDispResource *resource);
  
  virtual int GetNumScreens();    
  virtual int GetCurrentScreen();
  
          bool InitDisplayLink(CVideoSyncIos *syncImpl);
          void DeinitDisplayLink(void);
          void OnAppFocusChange(bool focus);
          bool IsBackgrounded() const { return m_bIsBackgrounded; }

protected:
  virtual bool PresentRenderImpl(const CDirtyRegionList &dirty);
  virtual void SetVSyncImpl(bool enable);

  void        *m_glView; // EAGLView opaque
  void        *m_WorkingContext; // shared EAGLContext opaque
  bool         m_bWasFullScreenBeforeMinimize;
  std::string   m_eglext;
  int          m_iVSyncErrors;
  CCriticalSection             m_resourceSection;
  std::vector<IDispResource*>  m_resources;
  bool         m_bIsBackgrounded;
  
private:
  bool GetScreenResolution(int* w, int* h, double* fps, int screenIdx);
  void FillInVideoModes();
  bool SwitchToVideoMode(int width, int height, double refreshrate, int screenIdx);
  CADisplayLinkWrapper *m_pDisplayLink;
};

XBMC_GLOBAL_REF(CWinSystemIOS,g_Windowing);
#define g_Windowing XBMC_GLOBAL_USE(CWinSystemIOS)

#endif

#endif // WINDOW_SYSTEM_IOSEGL_H
