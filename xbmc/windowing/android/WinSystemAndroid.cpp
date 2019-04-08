/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemAndroid.h"

#include <string.h>
#include <float.h>

#include <EGL/egl.h>
#include <EGL/eglplatform.h>

#include "addons/interfaces/platform/android/System.h"
#include "cores/RetroPlayer/process/android/RPProcessInfoAndroid.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererOpenGLES.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodecAndroidMediaCodec.h"
#include "cores/VideoPlayer/DVDCodecs/Audio/DVDAudioCodecAndroidMediaCodec.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererMediaCodec.h"
#include "cores/VideoPlayer/Process/android/ProcessInfoAndroid.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererMediaCodecSurface.h"
#include "guilib/DispResource.h"
#include "OSScreenSaverAndroid.h"
#include "platform/android/powermanagement/AndroidPowerSyscall.h"
#include "platform/android/media/drm/MediaDrmCryptoSession.h"
#include "platform/android/media/decoderfilter/MediaCodecDecoderFilterManager.h"
#include "platform/android/activity/XBMCApp.h"
#include "ServiceBroker.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/Resolution.h"
#include "WinEventsAndroid.h"

using namespace KODI;

CWinSystemAndroid::CWinSystemAndroid()
{
  m_nativeDisplay = EGL_NO_DISPLAY;
  m_nativeWindow = nullptr;

  m_displayWidth = 0;
  m_displayHeight = 0;

  m_stereo_mode = RENDER_STEREO_MODE_OFF;

  m_dispResetState = RESET_NOTWAITING;
  m_dispResetTimer = new CTimer(this);

  m_android = nullptr;

  m_winEvents.reset(new CWinEventsAndroid());
  CAndroidPowerSyscall::Register();
}

CWinSystemAndroid::~CWinSystemAndroid()
{
  m_nativeWindow = nullptr;
  delete m_dispResetTimer, m_dispResetTimer = nullptr;
}

bool CWinSystemAndroid::InitWindowSystem()
{
  m_nativeDisplay = EGL_DEFAULT_DISPLAY;

  m_android = new CAndroidUtils();

  m_decoderFilterManager = new(CMediaCodecDecoderFilterManager);
  CServiceBroker::RegisterDecoderFilterManager(m_decoderFilterManager);

  CDVDVideoCodecAndroidMediaCodec::Register();
  CDVDAudioCodecAndroidMediaCodec::Register();

  CLinuxRendererGLES::Register();
  RETRO::CRPProcessInfoAndroid::Register();
  RETRO::CRPProcessInfoAndroid::RegisterRendererFactory(new RETRO::CRendererFactoryOpenGLES);
  CRendererMediaCodec::Register();
  CRendererMediaCodecSurface::Register();
  ADDON::Interface_Android::Register();
  DRM::CMediaDrmCryptoSession::Register();
  VIDEOPLAYER::CProcessInfoAndroid::Register();
  return CWinSystemBase::InitWindowSystem();
}

bool CWinSystemAndroid::DestroyWindowSystem()
{
  CLog::Log(LOGNOTICE, "CWinSystemAndroid::%s", __FUNCTION__);

  delete m_android;
  m_android = nullptr;

  CServiceBroker::RegisterDecoderFilterManager(nullptr);
  delete m_decoderFilterManager;
  m_decoderFilterManager = nullptr;

  return true;
}

bool CWinSystemAndroid::CreateNewWindow(const std::string& name,
                                    bool fullScreen,
                                    RESOLUTION_INFO& res)
{
  RESOLUTION_INFO current_resolution;
  current_resolution.iWidth = current_resolution.iHeight = 0;
  RENDER_STEREO_MODE stereo_mode = CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode();

  m_nWidth        = res.iWidth;
  m_nHeight       = res.iHeight;
  m_displayWidth  = res.iScreenWidth;
  m_displayHeight = res.iScreenHeight;
  m_fRefreshRate  = res.fRefreshRate;

  if ((m_bWindowCreated && m_android->GetNativeResolution(&current_resolution)) &&
    current_resolution.iWidth == res.iWidth && current_resolution.iHeight == res.iHeight &&
    current_resolution.iScreenWidth == res.iScreenWidth && current_resolution.iScreenHeight == res.iScreenHeight &&
    m_bFullScreen == fullScreen && current_resolution.fRefreshRate == res.fRefreshRate &&
    (current_resolution.dwFlags & D3DPRESENTFLAG_MODEMASK) == (res.dwFlags & D3DPRESENTFLAG_MODEMASK) &&
    m_stereo_mode == stereo_mode)
  {
    CLog::Log(LOGDEBUG, "CWinSystemAndroid::CreateNewWindow: No need to create a new window");
    return true;
  }

  if (m_dispResetState != RESET_NOTWAITING)
  {
    CLog::Log(LOGINFO, "CWinSystemAndroid::CreateNewWindow: cannot create window while resetting");
    return false;
  }

  m_stereo_mode = stereo_mode;
  m_bFullScreen = fullScreen;

  m_nativeWindow = CXBMCApp::GetNativeWindow(2000);
  if (!m_nativeWindow)
  {
    CLog::Log(LOGERROR, "CWinSystemAndroid::CreateNewWindow: failed");
    return false;
  }
  m_android->SetNativeResolution(res);

  return true;
}

bool CWinSystemAndroid::DestroyWindow()
{
  CLog::Log(LOGNOTICE, "CWinSystemAndroid::%s", __FUNCTION__);
  m_nativeWindow = nullptr;
  m_bWindowCreated = false;
  return true;
}

void CWinSystemAndroid::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();

  RESOLUTION_INFO resDesktop, curDisplay;
  std::vector<RESOLUTION_INFO> resolutions;

  if (!m_android->ProbeResolutions(resolutions) || resolutions.empty())
  {
    CLog::Log(LOGWARNING, "CWinSystemAndroid::%s failed.", __FUNCTION__);
  }

  /* ProbeResolutions includes already all resolutions.
   * Only get desktop resolution so we can replace xbmc's desktop res
   */
  if (m_android->GetNativeResolution(&curDisplay))
  {
    resDesktop = curDisplay;
  }

  RESOLUTION res_index  = RES_CUSTOM;

  for (size_t i = 0; i < resolutions.size(); i++)
  {
    // if this is a new setting,
    // create a new empty setting to fill in.
    while ((int)CDisplaySettings::GetInstance().ResolutionInfoSize() <= res_index)
    {
      RESOLUTION_INFO res;
      CDisplaySettings::GetInstance().AddResolutionInfo(res);
    }

    CServiceBroker::GetWinSystem()->GetGfxContext().ResetOverscan(resolutions[i]);
    CDisplaySettings::GetInstance().GetResolutionInfo(res_index) = resolutions[i];

    if (resDesktop.iScreenWidth == resolutions[i].iScreenWidth &&
       resDesktop.iScreenHeight == resolutions[i].iScreenHeight &&
       (resDesktop.dwFlags & D3DPRESENTFLAG_MODEMASK) == (resolutions[i].dwFlags & D3DPRESENTFLAG_MODEMASK) &&
       fabs(resDesktop.fRefreshRate - resolutions[i].fRefreshRate) < FLT_EPSILON)
    {
      CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP) = resolutions[i];
    }
    res_index = (RESOLUTION)((int)res_index + 1);
  }
}

void CWinSystemAndroid::OnTimeout()
{
  m_dispResetState = RESET_WAITEVENT;
  SetHDMIState(true);
}

void CWinSystemAndroid::SetHDMIState(bool connected)
{
  CSingleLock lock(m_resourceSection);
  CLog::Log(LOGDEBUG, "CWinSystemAndroid::SetHDMIState: connected: %d, dispResetState: %d", static_cast<int>(connected), m_dispResetState);
  if (connected && m_dispResetState != RESET_NOTWAITING)
  {
    for (auto resource : m_resources)
      resource->OnResetDisplay();
    m_dispResetState = RESET_NOTWAITING;
    m_dispResetTimer->Stop();
  }
  else if (!connected)
  {
    if (m_dispResetState == RESET_WAITTIMER)
    {
      //HDMI_AUDIOPLUG arrived, use this
      m_dispResetTimer->Stop();
      m_dispResetState = RESET_WAITEVENT;
      return;
    }
    else if (m_dispResetState != RESET_NOTWAITING)
      return;

    int delay = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt("videoscreen.delayrefreshchange") * 100;

    if (delay < 2000)
      delay = 2000;

    m_dispResetState = RESET_WAITTIMER;
    m_dispResetTimer->Stop();
    m_dispResetTimer->Start(delay);

    for (auto resource : m_resources)
      resource->OnLostDisplay();
  }
}

bool CWinSystemAndroid::Hide()
{
  return false;
}

bool CWinSystemAndroid::Show(bool raise)
{
  return false;
}

void CWinSystemAndroid::Register(IDispResource *resource)
{
  CSingleLock lock(m_resourceSection);
  m_resources.push_back(resource);
}

void CWinSystemAndroid::Unregister(IDispResource *resource)
{
  CSingleLock lock(m_resourceSection);
  std::vector<IDispResource*>::iterator i = find(m_resources.begin(), m_resources.end(), resource);
  if (i != m_resources.end())
    m_resources.erase(i);
}

void CWinSystemAndroid::MessagePush(XBMC_Event *newEvent)
{
  dynamic_cast<CWinEventsAndroid&>(*m_winEvents).MessagePush(newEvent);
}

bool CWinSystemAndroid::MessagePump()
{
  return m_winEvents->MessagePump();
}

std::unique_ptr<WINDOWING::IOSScreenSaver> CWinSystemAndroid::GetOSScreenSaverImpl()
{
  std::unique_ptr<KODI::WINDOWING::IOSScreenSaver> ret(new COSScreenSaverAndroid());
  return ret;
}
