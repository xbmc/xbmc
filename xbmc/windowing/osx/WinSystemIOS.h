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
#include <string>
#include <vector>

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

  bool InitWindowSystem() override;
  bool DestroyWindowSystem() override;
  bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) override;
  bool DestroyWindow() override;
  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;
  void UpdateResolutions() override;
  bool CanDoWindowed() override { return false; }

  void ShowOSMouse(bool show) override;
  bool HasCursor() override;

  void NotifyAppActiveChange(bool bActivated) override;

  bool Minimize() override;
  bool Restore() override;
  bool Hide() override;
  bool Show(bool raise = true) override;

  bool IsExtSupported(const char* extension) override;

  bool BeginRender() override;
  bool EndRender() override;
  
  virtual void Register(IDispResource *resource);
  virtual void Unregister(IDispResource *resource);
  
  int GetNumScreens() override;
  int GetCurrentScreen() override;
  
  virtual std::unique_ptr<CVideoSync> GetVideoSync(void *clock) override;

  bool InitDisplayLink(CVideoSyncIos *syncImpl);
  void DeinitDisplayLink(void);
  void OnAppFocusChange(bool focus);
  bool IsBackgrounded() const { return m_bIsBackgrounded; }
  void* GetEAGLContextObj();

protected:
  void PresentRenderImpl(bool rendered) override;
  void SetVSyncImpl(bool enable) override;

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
