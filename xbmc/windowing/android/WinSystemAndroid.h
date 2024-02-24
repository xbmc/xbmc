/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AndroidUtils.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "system_egl.h"
#include "threads/CriticalSection.h"
#include "threads/Timer.h"
#include "utils/HDRCapabilities.h"
#include "windowing/WinSystem.h"

#include <memory>

class CDecoderFilterManager;
class IDispResource;
class CNativeWindow;

class CWinSystemAndroid : public CWinSystemBase, public ITimerCallback
{
public:
  CWinSystemAndroid();
  ~CWinSystemAndroid() override;

  bool InitWindowSystem() override;
  bool DestroyWindowSystem() override;

  bool CreateNewWindow(const std::string& name,
                       bool fullScreen,
                       RESOLUTION_INFO& res) override;

  bool DestroyWindow() override;
  void UpdateResolutions() override;

  void InitiateModeChange();
  bool IsHdmiModeTriggered() const { return m_HdmiModeTriggered; }
  void SetHdmiState(bool connected);

  void UpdateDisplayModes();

  bool HasCursor() override { return false; }

  bool Minimize() override;
  bool Hide() override;
  bool Show(bool raise = true) override;
  void Register(IDispResource *resource) override;
  void Unregister(IDispResource *resource) override;

  void MessagePush(XBMC_Event *newEvent);

  // winevents override
  bool MessagePump() override;
  bool IsHDRDisplay() override;

  CHDRCapabilities GetDisplayHDRCapabilities() const override;
  float GetGuiSdrPeakLuminance() const override;

protected:
  std::unique_ptr<KODI::WINDOWING::IOSScreenSaver> GetOSScreenSaverImpl() override;
  void OnTimeout() override;

  CAndroidUtils *m_android;

  EGLDisplay m_nativeDisplay = EGL_NO_DISPLAY;
  std::shared_ptr<CNativeWindow> m_nativeWindow;

  int m_displayWidth;
  int m_displayHeight;

  RENDER_STEREO_MODE m_stereo_mode;

  CTimer *m_dispResetTimer;

  CCriticalSection m_resourceSection;
  std::vector<IDispResource*> m_resources;
  CDecoderFilterManager *m_decoderFilterManager;

private:
  bool m_HdmiModeTriggered = false;
  void UpdateResolutions(bool bUpdateDesktopRes);
};
