/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemGbm.h"

#include "GBMDPMSSupport.h"
#include "OptionalsReg.h"
#include "ServiceBroker.h"
#include "VideoSyncGbm.h"
#include "cores/VideoPlayer/Buffers/VideoBufferDRMPRIME.h"
#include "cores/VideoPlayer/VideoReferenceClock.h"
#include "drm/DRMAtomic.h"
#include "drm/DRMLegacy.h"
#include "drm/OffScreenModeSetting.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/DisplayInfo.h"
#include "utils/Map.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#include <mutex>
#include <string.h>

#ifndef HAVE_HDR_OUTPUT_METADATA
// HDR structs is copied from linux include/linux/hdmi.h
struct hdr_metadata_infoframe
{
  uint8_t eotf;
  uint8_t metadata_type;
  struct
  {
    uint16_t x, y;
  } display_primaries[3];
  struct
  {
    uint16_t x, y;
  } white_point;
  uint16_t max_display_mastering_luminance;
  uint16_t min_display_mastering_luminance;
  uint16_t max_cll;
  uint16_t max_fall;
};
struct hdr_output_metadata
{
  uint32_t metadata_type;
  union
  {
    struct hdr_metadata_infoframe hdmi_metadata_type1;
  };
};
#endif

using namespace KODI::WINDOWING::GBM;

using namespace std::chrono_literals;

namespace
{

// These map to the definitions in the linux kernel
// drivers/gpu/drm/drm_connector.c

constexpr auto ColorimetryMap = make_map<KODI::UTILS::Colorimetry, std::string_view>({
    {KODI::UTILS::Colorimetry::DEFAULT, "Default"},
    {KODI::UTILS::Colorimetry::XVYCC_601, "XVYCC_601"},
    {KODI::UTILS::Colorimetry::XVYCC_709, "XVYCC_709"},
    {KODI::UTILS::Colorimetry::SYCC_601, "SYCC_601"},
    {KODI::UTILS::Colorimetry::OPYCC_601, "opYCC_601"},
    {KODI::UTILS::Colorimetry::OPRGB, "opRGB"},
    {KODI::UTILS::Colorimetry::BT2020_CYCC, "BT2020_CYCC"},
    {KODI::UTILS::Colorimetry::BT2020_YCC, "BT2020_YCC"},
    {KODI::UTILS::Colorimetry::BT2020_RGB, "BT2020_RGB"},
    {KODI::UTILS::Colorimetry::ST2113_RGB, "Default"},
    {KODI::UTILS::Colorimetry::ICTCP, "Default"},
});
} // namespace

CWinSystemGbm::CWinSystemGbm() :
  m_DRM(nullptr),
  m_GBM(new CGBMUtils),
  m_libinput(new CLibInputHandler)
{
  m_dpms = std::make_shared<CGBMDPMSSupport>();
  m_libinput->Start();
}

CWinSystemGbm::~CWinSystemGbm() = default;

bool CWinSystemGbm::InitWindowSystem()
{
  const char* x11 = getenv("DISPLAY");
  const char* wayland = getenv("WAYLAND_DISPLAY");
  if (x11 || wayland)
  {
    CLog::Log(LOGDEBUG, "CWinSystemGbm::{} - not allowed to run GBM under a window manager",
              __FUNCTION__);
    return false;
  }

  m_DRM = std::make_shared<CDRMAtomic>();

  if (!m_DRM->InitDrm())
  {
    CLog::Log(LOGERROR, "CWinSystemGbm::{} - failed to initialize Atomic DRM", __FUNCTION__);
    m_DRM.reset();

    m_DRM = std::make_shared<CDRMLegacy>();

    if (!m_DRM->InitDrm())
    {
      CLog::Log(LOGERROR, "CWinSystemGbm::{} - failed to initialize Legacy DRM", __FUNCTION__);
      m_DRM.reset();

      m_DRM = std::make_shared<COffScreenModeSetting>();
      if (!m_DRM->InitDrm())
      {
        CLog::Log(LOGERROR, "CWinSystemGbm::{} - failed to initialize off screen DRM",
                  __FUNCTION__);
        m_DRM.reset();
        return false;
      }
    }
  }

  CDRMConnector* connector = m_DRM->GetConnector();
  if (connector)
  {
    std::vector<uint8_t> edid = connector->GetEDID();
    if (!edid.empty())
    {
      m_info = UTILS::CDisplayInfo::Create(edid);
    }
  }

  if (!m_GBM->CreateDevice(m_DRM->GetFileDescriptor()))
  {
    m_GBM.reset();
    return false;
  }

  auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return false;

  auto settings = settingsComponent->GetSettings();
  if (!settings)
    return false;

  auto setting = settings->GetSetting(CSettings::SETTING_VIDEOSCREEN_LIMITEDRANGE);
  if (setting)
    setting->SetVisible(true);

  setting = settings->GetSetting("videoscreen.limitguisize");
  if (setting)
    setting->SetVisible(true);

  CLog::Log(LOGDEBUG, "CWinSystemGbm::{} - initialized DRM", __FUNCTION__);
  return CWinSystemBase::InitWindowSystem();
}

bool CWinSystemGbm::DestroyWindowSystem()
{
  CLog::Log(LOGDEBUG, "CWinSystemGbm::{} - deinitialized DRM", __FUNCTION__);

  m_libinput.reset();

  return true;
}

void CWinSystemGbm::UpdateResolutions()
{
  RESOLUTION_INFO current = m_DRM->GetCurrentMode();

  auto resolutions = m_DRM->GetModes();
  if (resolutions.empty())
  {
    CDisplaySettings::GetInstance().ClearCustomResolutions();

    CServiceBroker::GetWinSystem()->GetGfxContext().ResetOverscan(current);
    CDisplaySettings::GetInstance().AddResolutionInfo(current);
    CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP) = current;

    CLog::Log(LOGINFO, "Found resolution {}x{} with {}x{}{} @ {:f} Hz", current.iWidth,
              current.iHeight, current.iScreenWidth, current.iScreenHeight,
              current.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "", current.fRefreshRate);
  }
  else
  {
    CDisplaySettings::GetInstance().ClearCustomResolutions();

    for (auto &res : resolutions)
    {
      CServiceBroker::GetWinSystem()->GetGfxContext().ResetOverscan(res);
      CDisplaySettings::GetInstance().AddResolutionInfo(res);

      if (current.iScreenWidth == res.iScreenWidth &&
          current.iScreenHeight == res.iScreenHeight &&
          current.iWidth == res.iWidth &&
          current.iHeight == res.iHeight &&
          current.fRefreshRate == res.fRefreshRate &&
          current.dwFlags == res.dwFlags)
      {
        CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP) = res;
      }

      CLog::Log(LOGINFO, "Found resolution {}x{} with {}x{}{} @ {:f} Hz", res.iWidth, res.iHeight,
                res.iScreenWidth, res.iScreenHeight,
                res.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "", res.fRefreshRate);
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
    CLog::Log(LOGERROR, "CWinSystemGbm::{} - failed to set DRM mode", __FUNCTION__);
    return false;
  }

  struct gbm_bo *bo = nullptr;

  if (!std::dynamic_pointer_cast<CDRMAtomic>(m_DRM))
  {
    bo = m_GBM->GetDevice().GetSurface().LockFrontBuffer().Get();
  }

  auto result = m_DRM->SetVideoMode(res, bo);

  auto delay =
      std::chrono::milliseconds(CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
                                    "videoscreen.delayrefreshchange") *
                                100);
  if (delay > 0ms)
    m_dispResetTimer.Set(delay);

  return result;
}

bool CWinSystemGbm::DisplayHardwareScalingEnabled()
{
  auto drmAtomic = std::dynamic_pointer_cast<CDRMAtomic>(m_DRM);
  if (drmAtomic && drmAtomic->DisplayHardwareScalingEnabled())
    return true;

  return false;
}

void CWinSystemGbm::UpdateDisplayHardwareScaling(const RESOLUTION_INFO& resInfo)
{
  if (!DisplayHardwareScalingEnabled())
    return;

  //! @todo The PR that made the res struct constant was abandoned due to drama.
  // It should be const-corrected and changed here.
  RESOLUTION_INFO& resMutable = const_cast<RESOLUTION_INFO&>(resInfo);

  SetFullScreen(true, resMutable, false);
}

void CWinSystemGbm::FlipPage(bool rendered, bool videoLayer, bool async)
{
  if (m_videoLayerBridge && !videoLayer)
  {
    // disable video plane when video layer no longer is active
    m_videoLayerBridge->Disable();
  }

  struct gbm_bo *bo = nullptr;

  if (rendered)
  {
    bo = m_GBM->GetDevice().GetSurface().LockFrontBuffer().Get();
  }

  m_DRM->FlipPage(bo, rendered, videoLayer, async);

  if (m_videoLayerBridge && !videoLayer)
  {
    // delete video layer bridge when video layer no longer is active
    m_videoLayerBridge.reset();
  }
}

bool CWinSystemGbm::UseLimitedColor()
{
  return CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_VIDEOSCREEN_LIMITEDRANGE);
}

bool CWinSystemGbm::Hide()
{
  bool ret = m_DRM->SetActive(false);
  FlipPage(false, false, false);
  return ret;
}

bool CWinSystemGbm::Show(bool raise)
{
  bool ret = m_DRM->SetActive(true);
  FlipPage(false, false, false);
  return ret;
}

void CWinSystemGbm::Register(IDispResource *resource)
{
  std::unique_lock<CCriticalSection> lock(m_resourceSection);
  m_resources.push_back(resource);
}

void CWinSystemGbm::Unregister(IDispResource *resource)
{
  std::unique_lock<CCriticalSection> lock(m_resourceSection);
  std::vector<IDispResource*>::iterator i = find(m_resources.begin(), m_resources.end(), resource);
  if (i != m_resources.end())
  {
    m_resources.erase(i);
  }
}

void CWinSystemGbm::OnLostDevice()
{
  CLog::Log(LOGDEBUG, "{} - notify display change event", __FUNCTION__);
  m_dispReset = true;

  std::unique_lock<CCriticalSection> lock(m_resourceSection);
  for (auto resource : m_resources)
    resource->OnLostDisplay();
}

std::unique_ptr<CVideoSync> CWinSystemGbm::GetVideoSync(CVideoReferenceClock* clock)
{
  return std::make_unique<CVideoSyncGbm>(clock);
}

std::vector<std::string> CWinSystemGbm::GetConnectedOutputs()
{
  return m_DRM->GetConnectedConnectorNames();
}

bool CWinSystemGbm::SetHDR(const VideoPicture* videoPicture)
{
  auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return false;

  auto settings = settingsComponent->GetSettings();
  if (!settings)
    return false;

  if (!settings->GetBool(SETTING_WINSYSTEM_IS_HDR_DISPLAY))
    return false;

  auto drm = std::dynamic_pointer_cast<CDRMAtomic>(m_DRM);
  if (!drm)
    return false;

  auto connector = drm->GetConnector();
  if (!connector)
    return false;

  if (!videoPicture)
  {
    if (connector->SupportsProperty("Colorspace"))
    {
      std::optional<uint64_t> colorspace = connector->GetPropertyValue("Colorspace", "Default");
      if (colorspace)
      {
        CLog::LogF(LOGDEBUG, "setting connector colorspace to Default");
        drm->AddProperty(connector, "Colorspace", colorspace.value());
        drm->SetActive(true);
      }
    }

    if (connector->SupportsProperty("HDR_OUTPUT_METADATA"))
    {
      drm->AddProperty(connector, "HDR_OUTPUT_METADATA", 0);
      drm->SetActive(true);

      if (m_hdr_blob_id)
        drmModeDestroyPropertyBlob(drm->GetFileDescriptor(), m_hdr_blob_id);
      m_hdr_blob_id = 0;
    }

    return false;
  }

  KODI::UTILS::Colorimetry colorimetry = DRMPRIME::GetColorimetry(*videoPicture);

  if (connector->SupportsProperty("Colorspace") && m_info &&
      m_info->SupportsColorimetry(colorimetry))
  {
    std::optional<uint64_t> colorspace =
        connector->GetPropertyValue("Colorspace", ColorimetryMap.at(colorimetry));
    if (colorspace)
    {
      CLog::LogF(LOGDEBUG, "setting connector colorspace to {}", ColorimetryMap.at(colorimetry));
      drm->AddProperty(connector, "Colorspace", colorspace.value());
      drm->SetActive(true);
    }
  }

  KODI::UTILS::Eotf eotf = DRMPRIME::GetEOTF(*videoPicture);

  if (connector->SupportsProperty("HDR_OUTPUT_METADATA") && m_info &&
      m_info->SupportsHDRStaticMetadataType1() && m_info->SupportsEOTF(eotf))
  {
    hdr_output_metadata hdr_metadata = {};

    hdr_metadata.metadata_type = DRMPRIME::HDMI_STATIC_METADATA_TYPE1;
    hdr_metadata.hdmi_metadata_type1.eotf = static_cast<uint8_t>(eotf);
    hdr_metadata.hdmi_metadata_type1.metadata_type = DRMPRIME::HDMI_STATIC_METADATA_TYPE1;

    if (m_hdr_blob_id)
      drmModeDestroyPropertyBlob(drm->GetFileDescriptor(), m_hdr_blob_id);
    m_hdr_blob_id = 0;

    if (hdr_metadata.hdmi_metadata_type1.eotf)
    {
      const AVMasteringDisplayMetadata* mdmd = DRMPRIME::GetMasteringDisplayMetadata(*videoPicture);
      if (mdmd && mdmd->has_primaries)
      {
        // Convert to unsigned 16-bit values in units of 0.00002,
        // where 0x0000 represents zero and 0xC350 represents 1.0000
        for (int i = 0; i < 3; i++)
        {
          hdr_metadata.hdmi_metadata_type1.display_primaries[i].x =
              std::round(av_q2d(mdmd->display_primaries[i][0]) * 50000.0);
          hdr_metadata.hdmi_metadata_type1.display_primaries[i].y =
              std::round(av_q2d(mdmd->display_primaries[i][1]) * 50000.0);

          CLog::Log(LOGDEBUG, LOGVIDEO, "CWinSystemGbm::{} - display_primaries[{}].x: {}",
                    __FUNCTION__, i, hdr_metadata.hdmi_metadata_type1.display_primaries[i].x);
          CLog::Log(LOGDEBUG, LOGVIDEO, "CWinSystemGbm::{} - display_primaries[{}].y: {}",
                    __FUNCTION__, i, hdr_metadata.hdmi_metadata_type1.display_primaries[i].y);
        }
        hdr_metadata.hdmi_metadata_type1.white_point.x =
            std::round(av_q2d(mdmd->white_point[0]) * 50000.0);
        hdr_metadata.hdmi_metadata_type1.white_point.y =
            std::round(av_q2d(mdmd->white_point[1]) * 50000.0);

        CLog::Log(LOGDEBUG, LOGVIDEO, "CWinSystemGbm::{} - white_point.x: {}", __FUNCTION__,
                  hdr_metadata.hdmi_metadata_type1.white_point.x);
        CLog::Log(LOGDEBUG, LOGVIDEO, "CWinSystemGbm::{} - white_point.y: {}", __FUNCTION__,
                  hdr_metadata.hdmi_metadata_type1.white_point.y);
      }
      if (mdmd && mdmd->has_luminance)
      {
        // Convert to unsigned 16-bit value in units of 1 cd/m2,
        // where 0x0001 represents 1 cd/m2 and 0xFFFF represents 65535 cd/m2
        hdr_metadata.hdmi_metadata_type1.max_display_mastering_luminance =
            std::round(av_q2d(mdmd->max_luminance));

        // Convert to unsigned 16-bit value in units of 0.0001 cd/m2,
        // where 0x0001 represents 0.0001 cd/m2 and 0xFFFF represents 6.5535 cd/m2
        hdr_metadata.hdmi_metadata_type1.min_display_mastering_luminance =
            std::round(av_q2d(mdmd->min_luminance) * 10000.0);

        CLog::Log(LOGDEBUG, LOGVIDEO, "CWinSystemGbm::{} - max_display_mastering_luminance: {}",
                  __FUNCTION__, hdr_metadata.hdmi_metadata_type1.max_display_mastering_luminance);
        CLog::Log(LOGDEBUG, LOGVIDEO, "CWinSystemGbm::{} - min_display_mastering_luminance: {}",
                  __FUNCTION__, hdr_metadata.hdmi_metadata_type1.min_display_mastering_luminance);
      }

      const AVContentLightMetadata* clmd = DRMPRIME::GetContentLightMetadata(*videoPicture);
      if (clmd)
      {
        hdr_metadata.hdmi_metadata_type1.max_cll = clmd->MaxCLL;
        hdr_metadata.hdmi_metadata_type1.max_fall = clmd->MaxFALL;

        CLog::Log(LOGDEBUG, LOGVIDEO, "CWinSystemGbm::{} - max_cll: {}", __FUNCTION__,
                  hdr_metadata.hdmi_metadata_type1.max_cll);
        CLog::Log(LOGDEBUG, LOGVIDEO, "CWinSystemGbm::{} - max_fall: {}", __FUNCTION__,
                  hdr_metadata.hdmi_metadata_type1.max_fall);
      }

      drmModeCreatePropertyBlob(drm->GetFileDescriptor(), &hdr_metadata, sizeof(hdr_metadata),
                                &m_hdr_blob_id);
    }

    drm->AddProperty(connector, "HDR_OUTPUT_METADATA", m_hdr_blob_id);
    drm->SetActive(true);
  }

  return m_hdr_blob_id != 0;
}

bool CWinSystemGbm::IsHDRDisplay()
{
  auto drm = std::dynamic_pointer_cast<CDRMAtomic>(m_DRM);
  if (!drm)
    return false;

  auto connector = drm->GetConnector();
  if (!connector)
    return false;

  return connector->SupportsProperty("HDR_OUTPUT_METADATA") && m_info &&
         m_info->SupportsHDRStaticMetadataType1();
}

CHDRCapabilities CWinSystemGbm::GetDisplayHDRCapabilities() const
{
  if (!m_info)
    return {};

  CHDRCapabilities caps;

  if (m_info->SupportsEOTF(UTILS::Eotf::PQ))
    caps.SetHDR10();

  if (m_info->SupportsEOTF(UTILS::Eotf::HLG))
    caps.SetHLG();

  return caps;
}
