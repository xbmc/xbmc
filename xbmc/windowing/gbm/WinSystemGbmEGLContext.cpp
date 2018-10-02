/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"

#include "OptionalsReg.h"
#include "ServiceBroker.h"
#include "settings/DisplaySettings.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"
#include "WinSystemGbmEGLContext.h"

using namespace KODI::WINDOWING::GBM;

CWinSystemGbmEGLContext::CWinSystemGbmEGLContext(EGLenum platform, std::string const& platformExtension) :
  m_eglContext(platform, platformExtension)
{
  std::set<std::string> settingSet;
  settingSet.insert(CEGLContextUtils::SETTING_VIDEOSCREEN_MSAA);
  CServiceBroker::GetSettingsComponent()->GetSettings()->GetSettingsManager()->RegisterCallback(this, settingSet);
}

CWinSystemGbmEGLContext::~CWinSystemGbmEGLContext()
{
  CSettingsComponent *settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return;

  const std::shared_ptr<CSettings> settings = settingsComponent->GetSettings();
  if (!settings)
    return;

  settings->GetSettingsManager()->UnregisterCallback(this);
}

bool CWinSystemGbmEGLContext::InitWindowSystemEGL(EGLint renderableType, EGLint apiType)
{
  if (!CWinSystemGbm::InitWindowSystem())
  {
    return false;
  }

  // we need to provide an alpha format to egl to workaround a mesa bug
  int visualId = CDRMUtils::FourCCWithAlpha(CWinSystemGbm::GetDrm()->GetOverlayPlane()->format);

  if (!m_eglContext.CreatePlatformDisplay(m_GBM->GetDevice(), m_GBM->GetDevice(), renderableType, apiType, visualId))
  {
    return false;
  }

  if (!CreateContext())
  {
    return false;
  }

  if (CEGLUtils::HasExtension(m_eglContext.GetEGLDisplay(), "EGL_KHR_no_config_context"))
    CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(CEGLContextUtils::SETTING_VIDEOSCREEN_MSAA)->SetVisible(true);

  return true;
}

bool CWinSystemGbmEGLContext::CreateNewWindow(const std::string& name,
                                              bool fullScreen,
                                              RESOLUTION_INFO& res)
{
  m_eglContext.DestroySurface();

  if (!CWinSystemGbm::DestroyWindow())
  {
    return false;
  }

  if (!CWinSystemGbm::CreateNewWindow(name, fullScreen, res))
  {
    return false;
  }

  // we need to provide an alpha format to egl to workaround a mesa bug
  int visualId = CDRMUtils::FourCCWithAlpha(CWinSystemGbm::GetDrm()->GetOverlayPlane()->format);

  if (!m_eglContext.ChooseConfig(EGL_OPENGL_ES2_BIT, visualId))
  {
    return false;
  }

  // This check + the reinterpret cast is for security reason, if the user has outdated platform header files which often is the case
  static_assert(sizeof(EGLNativeWindowType) == sizeof(gbm_surface*), "Declaration specifier differs in size");

  if (!m_eglContext.CreatePlatformSurface(m_GBM->GetSurface(), reinterpret_cast<EGLNativeWindowType>(m_GBM->GetSurface())))
  {
    return false;
  }

  if (!m_eglContext.BindContext())
  {
    return false;
  }

  return true;
}

bool CWinSystemGbmEGLContext::DestroyWindowSystem()
{
  CDVDFactoryCodec::ClearHWAccels();
  VIDEOPLAYER::CRendererFactory::ClearRenderer();
  m_eglContext.Destroy();

  return CWinSystemGbm::DestroyWindowSystem();
}

void CWinSystemGbmEGLContext::delete_CVaapiProxy::operator()(CVaapiProxy *p) const
{
  VaapiProxyDelete(p);
}

EGLDisplay CWinSystemGbmEGLContext::GetEGLDisplay() const
{
  return m_eglContext.GetEGLDisplay();
}

EGLSurface CWinSystemGbmEGLContext::GetEGLSurface() const
{
  return m_eglContext.GetEGLSurface();
}

EGLContext CWinSystemGbmEGLContext::GetEGLContext() const
{
  return m_eglContext.GetEGLContext();
}

EGLConfig  CWinSystemGbmEGLContext::GetEGLConfig() const
{
  return m_eglContext.GetEGLConfig();
}

void CWinSystemGbmEGLContext::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  if (setting == nullptr)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CEGLContextUtils::SETTING_VIDEOSCREEN_MSAA)
  {
    CLog::Log(LOGDEBUG, "MSAA setting changed - creating a new window");

    auto res = CDisplaySettings::GetInstance().GetCurrentResolution();
    auto info = CDisplaySettings::GetInstance().GetResolutionInfo(res);

    if (!SetFullScreen(true, info, false))
      CLog::Log(LOGDEBUG, "failed to create new window");
  }
}
