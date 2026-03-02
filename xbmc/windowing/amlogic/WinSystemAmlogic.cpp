/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemAmlogic.h"

#include <algorithm>
#include <cmath>
#include <string.h>
#include <float.h>

#include "ServiceBroker.h"
#include "cores/RetroPlayer/process/amlogic/RPProcessInfoAmlogic.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererOpenGLES.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodecAmlogic.h"
#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGLES.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererAML.h"
#include "windowing/GraphicContext.h"
#include "windowing/Resolution.h"
#include "platform/linux/powermanagement/LinuxPowerSyscall.h"
#include "platform/linux/ScreenshotSurfaceAML.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "guilib/DispResource.h"
#include "utils/AMLUtils.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "DolbyVisionAML.h"

#include "platform/linux/SysfsPath.h"

#include <linux/fb.h>
#include <linux/version.h>

#include "system_egl.h"

using namespace KODI;

CWinSystemAmlogic::CWinSystemAmlogic()
:  m_nativeWindow(nullptr)
,  m_libinput(new CLibInputHandler)
,  m_force_mode_switch(false)
{
  const char *env_framebuffer = getenv("FRAMEBUFFER");

  // default to framebuffer 0
  m_framebuffer_name = "fb0";
  if (env_framebuffer)
  {
    std::string framebuffer(env_framebuffer);
    std::string::size_type start = framebuffer.find("fb");
    m_framebuffer_name = framebuffer.substr(start);
  }

  m_nativeDisplay = EGL_NO_DISPLAY;

  m_stereo_mode = RENDER_STEREO_MODE_OFF;
  m_delayDispReset = false;

  m_libinput->Start();
}

bool CWinSystemAmlogic::InitWindowSystem()
{
  // Setup DV UI Elements etc.
  m_dolbyVisionAML = std::make_unique<CDolbyVisionAML>();
  if (!m_dolbyVisionAML->Setup()) m_dolbyVisionAML.reset();

  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  if (settings->GetBool(CSettings::SETTING_COREELEC_AMLOGIC_NOISEREDUCTION))
  {
     CLog::Log(LOGDEBUG, "CWinSystemAmlogic::InitWindowSystem -- disabling noise reduction");
     CSysfsPath("/sys/module/di/parameters/nr2_en", 0);
  }

  int sdr2hdr = settings->GetBool(CSettings::SETTING_COREELEC_AMLOGIC_SDR2HDR);
  if (sdr2hdr)
  {
    CLog::Log(LOGDEBUG, "CWinSystemAmlogic::InitWindowSystem -- setting sdr2hdr mode to {:d}", sdr2hdr);
    CSysfsPath("/sys/module/am_vecm/parameters/sdr_mode", 1);
    CSysfsPath("/sys/module/amdolby_vision/parameters/dolby_vision_policy", 0);
    CSysfsPath("/sys/module/am_vecm/parameters/hdr_policy", 0);
  }

  int hdr2sdr = settings->GetBool(CSettings::SETTING_COREELEC_AMLOGIC_HDR2SDR);
  if (hdr2sdr)
  {
    CLog::Log(LOGDEBUG, "CWinSystemAmlogic::InitWindowSystem -- setting hdr2sdr mode to {:d}", hdr2sdr);
    CSysfsPath("/sys/module/am_vecm/parameters/hdr_mode", 1);
  }

  if (((LINUX_VERSION_CODE >> 16) & 0xFF) < 5)
  {
    auto setting = settings->GetSetting(CSettings::SETTING_COREELEC_AMLOGIC_DISABLEGUISCALING);
    if (setting)
    {
      setting->SetVisible(false);
      settings->SetBool(CSettings::SETTING_COREELEC_AMLOGIC_DISABLEGUISCALING, false);
    }
  }

  m_nativeDisplay = EGL_DEFAULT_DISPLAY;

  CDVDVideoCodecAmlogic::Register();
  CLinuxRendererGLES::Register();
  RETRO::CRPProcessInfoAmlogic::Register();
  RETRO::CRPProcessInfoAmlogic::RegisterRendererFactory(new RETRO::CRendererFactoryOpenGLES);
  CRendererAML::Register();
  CScreenshotSurfaceAML::Register();

  if (aml_get_cpufamily_id() <= AML_GXL)
    aml_set_framebuffer_resolution(1920, 1080, m_framebuffer_name);

  auto setting = settings->GetSetting(CSettings::SETTING_VIDEOPLAYER_USEDISPLAYASCLOCK);
  if (setting)
  {
    setting->SetVisible(false);
    settings->SetBool(CSettings::SETTING_VIDEOPLAYER_USEDISPLAYASCLOCK, false);
  }

  // kill a running animation
  CLog::Log(LOGDEBUG,"CWinSystemAmlogic: Sending SIGUSR1 to 'splash-image'");
  std::system("killall -s SIGUSR1 splash-image &> /dev/null");

  // Close the OpenVFD splash and switch the display into time mode.
  CSysfsPath("/tmp/openvfd_service", 0);

  return CWinSystemBase::InitWindowSystem();
}

bool CWinSystemAmlogic::DestroyWindowSystem()
{
  return true;
}

bool CWinSystemAmlogic::CreateNewWindow(const std::string& name,
                                    bool fullScreen,
                                    RESOLUTION_INFO& res)
{
  m_nWidth        = res.iWidth;
  m_nHeight       = res.iHeight;
  m_fRefreshRate  = res.fRefreshRate;

  if (m_nativeWindow == nullptr)
    m_nativeWindow = new fbdev_window;

  m_nativeWindow->width = res.iWidth;
  m_nativeWindow->height = res.iHeight;

  int delay = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt("videoscreen.delayrefreshchange");
  if (delay > 0)
  {
    m_delayDispReset = true;
    m_dispResetTimer.Set(std::chrono::milliseconds(static_cast<unsigned int>(delay * 100)));
  }

  {
    std::lock_guard lock(m_resourceSection);

    for (auto i = m_resources.begin(); i != m_resources.end(); ++i)
    {
      (*i)->OnLostDisplay();
    }
  }

  aml_set_native_resolution(res, m_framebuffer_name, m_stereo_mode, m_force_mode_switch);
  // reset force mode switch
  m_force_mode_switch = false;

  if (!m_delayDispReset)
  {
    std::lock_guard lock(m_resourceSection);

    // tell any shared resources
    for (auto i = m_resources.begin(); i != m_resources.end(); ++i)
    {
      (*i)->OnResetDisplay();
    }
  }

  // Make sure DV Display activates if enabled - TODO: Why needed?
  aml_dv_display_trigger();

  m_bWindowCreated = true;
  return true;
}

bool CWinSystemAmlogic::DestroyWindow()
{
  if (m_nativeWindow != nullptr)
  {
    delete(m_nativeWindow);
    m_nativeWindow = nullptr;
  }

  m_bWindowCreated = false;
  return true;
}

void CWinSystemAmlogic::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();

  RESOLUTION_INFO resDesktop, curDisplay;
  std::vector<RESOLUTION_INFO> resolutions;

  if (!aml_probe_resolutions(resolutions) || resolutions.empty())
  {
    CLog::Log(LOGWARNING, "{}: ProbeResolutions failed.",__FUNCTION__);
  }

  /* ProbeResolutions includes already all resolutions.
   * Only get desktop resolution so we can replace xbmc's desktop res
   */
  if (aml_get_native_resolution(curDisplay))
  {
    resDesktop = curDisplay;
  }

  RESOLUTION ResDesktop = RES_INVALID;
  RESOLUTION res_index  = RES_DESKTOP;

  for (size_t i = 0; i < resolutions.size(); i++)
  {
    // if this is a new setting,
    // create a new empty setting to fill in.
    if ((int)CDisplaySettings::GetInstance().ResolutionInfoSize() <= res_index)
    {
      RESOLUTION_INFO res;
      CDisplaySettings::GetInstance().AddResolutionInfo(res);
    }

    CServiceBroker::GetWinSystem()->GetGfxContext().ResetOverscan(resolutions[i]);
    CDisplaySettings::GetInstance().GetResolutionInfo(res_index) = resolutions[i];

    CLog::Log(LOGINFO, "Found resolution {:d} x {:d} with {:d} x {:d}{} @ {:f} Hz",
      resolutions[i].iWidth,
      resolutions[i].iHeight,
      resolutions[i].iScreenWidth,
      resolutions[i].iScreenHeight,
      resolutions[i].dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "",
      resolutions[i].fRefreshRate);

    if(resDesktop.iWidth == resolutions[i].iWidth &&
       resDesktop.iHeight == resolutions[i].iHeight &&
       resDesktop.iScreenWidth == resolutions[i].iScreenWidth &&
       resDesktop.iScreenHeight == resolutions[i].iScreenHeight &&
       (resDesktop.dwFlags & D3DPRESENTFLAG_MODEMASK) == (resolutions[i].dwFlags & D3DPRESENTFLAG_MODEMASK) &&
       fabs(resDesktop.fRefreshRate - resolutions[i].fRefreshRate) < FLT_EPSILON)
    {
      ResDesktop = res_index;
    }

    res_index = (RESOLUTION)((int)res_index + 1);
  }

  // set RES_DESKTOP
  if (ResDesktop != RES_INVALID)
  {
    CLog::Log(LOGINFO, "Found ({:d}x{:d}{}@{:f}) at {:d}, setting to RES_DESKTOP at {:d}",
      resDesktop.iWidth, resDesktop.iHeight,
      resDesktop.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "",
      resDesktop.fRefreshRate,
      (int)ResDesktop, (int)RES_DESKTOP);

    CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP) = CDisplaySettings::GetInstance().GetResolutionInfo(ResDesktop);
  }
}

bool CWinSystemAmlogic::IsHDRDisplay()
{
  CSysfsPath hdr_cap{"/sys/class/amhdmitx/amhdmitx0/hdr_cap"};
  CSysfsPath dv_cap{"/sys/class/amhdmitx/amhdmitx0/dv_cap"};
  std::string valstr;

  if (hdr_cap.Exists())
  {
    valstr = hdr_cap.Get<std::string>().value();
    if (valstr.find("Traditional HDR: 1") != std::string::npos)
      m_hdr_caps.SetHDR10();

    if (valstr.find("HDR10Plus Supported: 1") != std::string::npos)
      m_hdr_caps.SetHDR10Plus();

    if (valstr.find("Hybrid Log-Gamma: 1") != std::string::npos)
      m_hdr_caps.SetHLG();
  }

  if (dv_cap.Exists())
  {
    valstr = dv_cap.Get<std::string>().value();
    if (valstr.find("DolbyVision RX support list") != std::string::npos)
      m_hdr_caps.SetDolbyVision();
  }

  return (m_hdr_caps.SupportsHDR10() | m_hdr_caps.SupportsHDR10Plus() | m_hdr_caps.SupportsHLG());
}

CHDRCapabilities CWinSystemAmlogic::GetDisplayHDRCapabilities() const
{
  return m_hdr_caps;
}

float CWinSystemAmlogic::GetDisplayLatency()
{
  return 0.0f;
}

float CWinSystemAmlogic::GetGuiSdrPeakLuminance() const
{
  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const int guiSdrPeak = settings->GetInt(CSettings::SETTING_VIDEOSCREEN_GUISDRPEAKLUMINANCE);

  // Map the 0-100 setting to a usable SDR white level in HDR mode.
  // The shader expects this value as "nits / 100".
  // Use an exponential curve but anchor the endpoints to a sensible range:
  //   0   -> 30 nits
  //   100 -> 500 nits
  constexpr float kMinNits = 30.0f;
  constexpr float kMaxNits = 1000.0f;
  const float exponent = std::log(kMaxNits / kMinNits) * (static_cast<float>(guiSdrPeak) / 100.0f);
  float sdrWhiteNits = std::clamp(kMinNits * std::exp(exponent), kMinNits, kMaxNits);

  // Shader expects "nits / 100".
  // Keep within the PQ domain (10,000 nits max) after any boost.
  sdrWhiteNits = std::clamp(sdrWhiteNits, 0.0f, 10000.0f);
  return sdrWhiteNits / 100.0f;
}

float CWinSystemAmlogic::GetGuiSdrSaturation() const
{
  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  // UI is 0..100, where 50 is neutral. Map to shader saturation factor 0..2.
  const int satClamped = std::clamp(settings->GetInt(CSettings::SETTING_VIDEOSCREEN_GUISDRSATURATION), 0, 100);

  float saturation = static_cast<float>(satClamped) / 50.0f;

  return std::clamp(saturation, 0.0f, 2.0f);
}

bool CWinSystemAmlogic::Hide()
{
  return false;
}

bool CWinSystemAmlogic::Show(bool show)
{
  CSysfsPath("/sys/class/graphics/" + m_framebuffer_name + "/blank", (show ? 0 : 1));
  return true;
}

void CWinSystemAmlogic::Register(IDispResource *resource)
{
  std::lock_guard lock(m_resourceSection);

  m_resources.push_back(resource);
}

void CWinSystemAmlogic::Unregister(IDispResource *resource)
{
  std::lock_guard lock(m_resourceSection);

  auto i = find(m_resources.begin(), m_resources.end(), resource);
  if (i != m_resources.end())
    m_resources.erase(i);
}
