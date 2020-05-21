/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RPIUtils.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "threads/CriticalSection.h"
#include "threads/SystemClock.h"
#include "windowing/WinSystem.h"

#include "platform/freebsd/OptionalsReg.h"
#include "platform/linux/OptionalsReg.h"
#include "platform/linux/input/LibInputHandler.h"

#include <EGL/egl.h>

class IDispResource;

class CWinSystemRpi : public CWinSystemBase
{
public:
  CWinSystemRpi();
  virtual ~CWinSystemRpi();

  bool InitWindowSystem() override;
  bool DestroyWindowSystem() override;

  bool CreateNewWindow(const std::string& name,
                       bool fullScreen,
                       RESOLUTION_INFO& res) override;

  bool DestroyWindow() override;
  void UpdateResolutions() override;

  bool Hide() override;
  bool Show(bool raise = true) override;
  void SetVisible(bool visible);
  virtual void Register(IDispResource *resource);
  virtual void Unregister(IDispResource *resource);
protected:
  CRPIUtils *m_rpi;
  EGLDisplay m_nativeDisplay;
  EGLSurface m_nativeWindow;

  int m_displayWidth;
  int m_displayHeight;

  RENDER_STEREO_MODE m_stereo_mode;

  bool m_delayDispReset;
  XbmcThreads::EndTime m_dispResetTimer;

  CCriticalSection m_resourceSection;
  std::vector<IDispResource*> m_resources;
  std::unique_ptr<OPTIONALS::CLircContainer, OPTIONALS::delete_CLircContainer> m_lirc;
  std::unique_ptr<CLibInputHandler> m_libinput;
};
