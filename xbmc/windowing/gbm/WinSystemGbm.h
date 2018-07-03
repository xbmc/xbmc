/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#include <gbm.h>
#include <EGL/egl.h>

#include "platform/linux/input/LibInputHandler.h"
#include "platform/linux/OptionalsReg.h"
#include "threads/CriticalSection.h"
#include "windowing/WinSystem.h"
#include "DRMUtils.h"

class IDispResource;

class CWinSystemGbm : public CWinSystemBase
{
public:
  CWinSystemGbm();
  virtual ~CWinSystemGbm() = default;

  bool InitWindowSystem() override;
  bool DestroyWindowSystem() override;

  bool CreateNewWindow(const std::string& name,
                       bool fullScreen,
                       RESOLUTION_INFO& res) override;

  bool DestroyWindow() override;

  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;

  void FlipPage(bool rendered, bool videoLayer);
  void WaitVBlank();

  bool CanDoWindowed() override { return false; }
  void UpdateResolutions() override;

  bool UseLimitedColor() override;

  bool Hide() override;
  bool Show(bool raise = true) override;
  virtual void Register(IDispResource *resource);
  virtual void Unregister(IDispResource *resource);

  std::string GetModule() const { return m_DRM->GetModule(); }
  std::string GetDevicePath() const { return m_DRM->GetDevicePath(); }
  struct gbm_device *GetGBMDevice() const { return m_GBM->GetDevice(); }
  std::shared_ptr<CDRMUtils> GetDrm() const { return m_DRM; }

protected:
  void OnLostDevice();

  std::shared_ptr<CDRMUtils> m_DRM;
  std::unique_ptr<CGBMUtils> m_GBM;

  CCriticalSection m_resourceSection;
  std::vector<IDispResource*>  m_resources;

  bool m_delayDispReset = false;
  XbmcThreads::EndTime m_dispResetTimer;
  std::unique_ptr<OPTIONALS::CLircContainer, OPTIONALS::delete_CLircContainer> m_lirc;
  std::unique_ptr<CLibInputHandler> m_libinput;
};
