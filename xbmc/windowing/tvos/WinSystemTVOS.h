/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>

#include "windowing/WinSystem.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "threads/CriticalSection.h"
#include "threads/Timer.h"

class IDispResource;
class CVideoSyncTVos;
struct CADisplayLinkWrapper;

class CWinSystemTVOS : public CWinSystemBase, public CRenderSystemGLES, public ITimerCallback
{
public:
  CWinSystemTVOS();
  virtual ~CWinSystemTVOS();

  // ITimerCallback interface
  virtual void OnTimeout() override {}

  void MessagePush(XBMC_Event *newEvent);
  size_t GetQueueSize();
    void AnnounceOnLostDevice();
    void AnnounceOnResetDevice();
    void StartLostDeviceTimer();
    void StopLostDeviceTimer();
  int GetDisplayIndexFromSettings();
  // Implementation of CWinSystemBase
  CRenderSystemBase *GetRenderSystem() override { return this; }
  bool InitWindowSystem() override;
  bool DestroyWindowSystem() override;
  bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) override;
  bool DestroyWindow() override;
  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;
  void UpdateResolutions() override;
  bool CanDoWindowed() override { return false; }

  void ShowOSMouse(bool show) override {}
  bool HasCursor() override;

  void NotifyAppActiveChange(bool bActivated) override;

  bool Minimize() override;
  bool Restore() override;
  bool Hide() override;
  bool Show(bool raise = true) override;

  bool IsExtSupported(const char* extension) const override;

  bool BeginRender() override;
  bool EndRender() override;

  void Register(IDispResource* resource) override;
  void Unregister(IDispResource* resource) override;

  //virtual std::unique_ptr<CVideoSync> GetVideoSync(void* clock) override;

  bool InitDisplayLink(CVideoSyncTVos* syncImpl);
  void DeinitDisplayLink(void);
  void OnAppFocusChange(bool focus);
  bool IsBackgrounded() const { return m_bIsBackgrounded; }
  void* GetEAGLContextObj();
  void GetConnectedOutputs(std::vector<std::string>* outputs);
  void MoveToTouchscreen();

  // winevents override
  bool MessagePump() override;

protected:
  void PresentRenderImpl(bool rendered) override;
  void SetVSyncImpl(bool enable) override {}

  void* m_glView; // EAGLView opaque
  void* m_WorkingContext; // shared EAGLContext opaque
  bool m_bWasFullScreenBeforeMinimize;
  std::string m_eglext;
  CCriticalSection m_resourceSection;
  std::vector<IDispResource*> m_resources;
  bool m_bIsBackgrounded;
  CTimer m_lostDeviceTimer;

private:
  bool GetScreenResolution(int* w, int* h, double* fps, int screenIdx);
  void FillInVideoModes(int screenIdx);
  bool SwitchToVideoMode(int width, int height, double refreshrate);
  CADisplayLinkWrapper* m_pDisplayLink;
  int m_internalTouchscreenResolutionWidth = -1;
  int m_internalTouchscreenResolutionHeight = -1;
};
