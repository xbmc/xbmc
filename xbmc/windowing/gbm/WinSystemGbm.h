/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
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

  void UpdateResolutions() override;

  bool UseLimitedColor() override;

  bool Hide() override;
  bool Show(bool raise = true) override;
  virtual void Register(IDispResource *resource);
  virtual void Unregister(IDispResource *resource);

  std::string GetModule() const { return m_DRM->GetModule(); }
  std::string GetDevicePath() const { return m_DRM->GetDevicePath(); }

  std::shared_ptr<CDRMUtils> m_DRM;

protected:
  void OnLostDevice();

  std::unique_ptr<CGBMUtils> m_GBM;

  CCriticalSection m_resourceSection;
  std::vector<IDispResource*>  m_resources;

  bool m_delayDispReset;
  XbmcThreads::EndTime m_dispResetTimer;
  std::unique_ptr<OPTIONALS::CLircContainer, OPTIONALS::delete_CLircContainer> m_lirc;
  std::unique_ptr<CLibInputHandler> m_libinput;
};
