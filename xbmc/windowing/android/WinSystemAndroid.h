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
#include "threads/CriticalSection.h"
#include "windowing/WinSystem.h"
#include "threads/Timer.h"
#include "EGL/egl.h"

class CDecoderFilterManager;
class IDispResource;

class CWinSystemAndroid : public CWinSystemBase, public ITimerCallback
{
public:
  CWinSystemAndroid();
  virtual ~CWinSystemAndroid();

  bool InitWindowSystem() override;
  bool DestroyWindowSystem() override;

  bool CreateNewWindow(const std::string& name,
                       bool fullScreen,
                       RESOLUTION_INFO& res) override;

  bool DestroyWindow() override;
  void UpdateResolutions() override;

  void SetHDMIState(uint8_t state);

  void UpdateDisplayModes();

  bool HasCursor() override { return false; };

  bool Hide() override;
  bool Show(bool raise = true) override;
  void Register(IDispResource *resource) override;
  void Unregister(IDispResource *resource) override;

  void MessagePush(XBMC_Event *newEvent);

  // winevents override
  bool MessagePump() override;

  enum HDMISTATE : uint8_t
  {
    HDMI_STATE_UNCONNECTED = 0,
    HDMI_STATE_CONNECTED = 1,
    HDMI_STATE_UNCONNECTED_TIMER = 2,
  };

protected:
  std::unique_ptr<KODI::WINDOWING::IOSScreenSaver> GetOSScreenSaverImpl() override;
  void OnTimeout() override;

  CAndroidUtils *m_android;

  EGLDisplay m_nativeDisplay;
  EGLNativeWindowType m_nativeWindow;

  int m_displayWidth;
  int m_displayHeight;

  RENDER_STEREO_MODE m_stereo_mode;

  enum RESETSTATE : uint8_t
  {
    RESET_WAIT_TIMER = 1U << 0,
    RESET_WAIT_HDMIPLUG = 1U << 1,
  };

  uint8_t m_dispResetState;
  CTimer *m_dispResetTimer;

  CCriticalSection m_resourceSection;
  std::vector<IDispResource*> m_resources;
  CDecoderFilterManager *m_decoderFilterManager;

private:
  void UpdateResolutions(bool bUpdateDesktopRes);
};
