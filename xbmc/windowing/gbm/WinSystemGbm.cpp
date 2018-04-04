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

#include "WinSystemGbm.h"
#include "ServiceBroker.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/lib/Setting.h"
#include <string.h>

#include "OptionalsReg.h"
#include "guilib/GraphicContext.h"
#include "platform/linux/powermanagement/LinuxPowerSyscall.h"
#include "settings/DisplaySettings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "../WinEventsLinux.h"
#include "DRMAtomic.h"
#include "DRMLegacy.h"
#include "messaging/ApplicationMessenger.h"


CWinSystemGbm::CWinSystemGbm() :
  m_DRM(nullptr),
  m_GBM(new CGBMUtils),
  m_delayDispReset(false)
{
  std::string envSink;
  if (getenv("AE_SINK"))
    envSink = getenv("AE_SINK");
  if (StringUtils::EqualsNoCase(envSink, "ALSA"))
  {
    GBM::ALSARegister();
  }
  else if (StringUtils::EqualsNoCase(envSink, "PULSE"))
  {
    GBM::PulseAudioRegister();
  }
  else if (StringUtils::EqualsNoCase(envSink, "SNDIO"))
  {
    GBM::SndioRegister();
  }
  else
  {
    if (!GBM::PulseAudioRegister())
    {
      if (!GBM::ALSARegister())
      {
        GBM::SndioRegister();
      }
    }
  }

  m_winEvents.reset(new CWinEventsLinux());
  CLinuxPowerSyscall::Register();
}

bool CWinSystemGbm::InitWindowSystem()
{
  m_DRM = std::make_shared<CDRMAtomic>();

  if (!m_DRM->InitDrm())
  {
    CLog::Log(LOGERROR, "CWinSystemGbm::%s - failed to initialize Atomic DRM", __FUNCTION__);
    m_DRM.reset();

    m_DRM = std::make_shared<CDRMLegacy>();

    if (!m_DRM->InitDrm())
    {
      CLog::Log(LOGERROR, "CWinSystemGbm::%s - failed to initialize Legacy DRM", __FUNCTION__);
      m_DRM.reset();
      return false;
    }
  }

  if (!m_GBM->CreateDevice(m_DRM->m_fd))
  {
    m_GBM.reset();
    return false;
  }

  CLog::Log(LOGDEBUG, "CWinSystemGbm::%s - initialized DRM", __FUNCTION__);
  return CWinSystemBase::InitWindowSystem();
}

bool CWinSystemGbm::DestroyWindowSystem()
{
  m_GBM->DestroySurface();
  m_GBM->DestroyDevice();

  CLog::Log(LOGDEBUG, "CWinSystemGbm::%s - deinitialized DRM", __FUNCTION__);
  return true;
}

bool CWinSystemGbm::CreateNewWindow(const std::string& name,
                                    bool fullScreen,
                                    RESOLUTION_INFO& res)
{
  //Notify other subsystems that we change resolution
  OnLostDevice();

  if(!m_DRM->SetMode(res))
  {
    CLog::Log(LOGERROR, "CWinSystemGbm::%s - failed to set DRM mode", __FUNCTION__);
    return false;
  }

  if(!m_GBM->CreateSurface(m_DRM->m_mode->hdisplay, m_DRM->m_mode->vdisplay))
  {
    CLog::Log(LOGERROR, "CWinSystemGbm::%s - failed to initialize GBM", __FUNCTION__);
    return false;
  }

  CLog::Log(LOGDEBUG, "CWinSystemGbm::%s - initialized GBM", __FUNCTION__);
  return true;
}

bool CWinSystemGbm::DestroyWindow()
{
  m_GBM->DestroySurface();

  CLog::Log(LOGDEBUG, "CWinSystemGbm::%s - deinitialized GBM", __FUNCTION__);
  return true;
}

void CWinSystemGbm::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();

  UpdateDesktopResolution(CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP),
                          0,
                          m_DRM->m_mode->hdisplay,
                          m_DRM->m_mode->vdisplay,
                          m_DRM->m_mode->vrefresh);

  std::vector<RESOLUTION_INFO> resolutions;

  if (!m_DRM->GetModes(resolutions) || resolutions.empty())
  {
    CLog::Log(LOGWARNING, "CWinSystemGbm::%s - Failed to get resolutions", __FUNCTION__);
  }
  else
  {
    CDisplaySettings::GetInstance().ClearCustomResolutions();

    for (unsigned int i = 0; i < resolutions.size(); i++)
    {
      g_graphicsContext.ResetOverscan(resolutions[i]);
      CDisplaySettings::GetInstance().AddResolutionInfo(resolutions[i]);

      CLog::Log(LOGNOTICE, "Found resolution %dx%d for display %d with %dx%d%s @ %f Hz",
                resolutions[i].iWidth,
                resolutions[i].iHeight,
                resolutions[i].iScreen,
                resolutions[i].iScreenWidth,
                resolutions[i].iScreenHeight,
                resolutions[i].dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "",
                resolutions[i].fRefreshRate);
    }
  }

  CDisplaySettings::GetInstance().ApplyCalibrations();
}

bool CWinSystemGbm::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  return true;
}

bool CWinSystemGbm::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  // Notify other subsystems that we will change resolution
  OnLostDevice();

  if(!m_DRM->SetMode(res))
  {
    CLog::Log(LOGERROR, "CWinSystemGbm::%s - failed to set DRM mode", __FUNCTION__);
    return false;
  }

  struct gbm_bo *bo = nullptr;

  if (!m_DRM->m_req)
  {
    bo = m_GBM->LockFrontBuffer();
  }

  auto result = m_DRM->SetVideoMode(res, bo);
  m_GBM->ReleaseBuffer();

  int delay = CServiceBroker::GetSettings().GetInt("videoscreen.delayrefreshchange");
  if (delay > 0)
  {
    m_delayDispReset = true;
    m_dispResetTimer.Set(delay * 100);
  }

  return result;
}

void CWinSystemGbm::FlipPage(bool rendered, bool videoLayer)
{
  struct gbm_bo *bo = m_GBM->LockFrontBuffer();

  m_DRM->FlipPage(bo, rendered, videoLayer);

  m_GBM->ReleaseBuffer();
}

void CWinSystemGbm::WaitVBlank()
{
  m_DRM->WaitVBlank();
}

bool CWinSystemGbm::Hide()
{
  return false;
}

bool CWinSystemGbm::Show(bool raise)
{
  return true;
}

void CWinSystemGbm::Register(IDispResource *resource)
{
  CSingleLock lock(m_resourceSection);
  m_resources.push_back(resource);
}

void CWinSystemGbm::Unregister(IDispResource *resource)
{
  CSingleLock lock(m_resourceSection);
  std::vector<IDispResource*>::iterator i = find(m_resources.begin(), m_resources.end(), resource);
  if (i != m_resources.end())
  {
    m_resources.erase(i);
  }
}

void CWinSystemGbm::OnLostDevice()
{
  CLog::Log(LOGDEBUG, "%s - notify display change event", __FUNCTION__);

  { CSingleLock lock(m_resourceSection);
    for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
      (*i)->OnLostDisplay();
  }
}

