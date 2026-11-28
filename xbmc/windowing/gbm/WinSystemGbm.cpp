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
#include "cores/VideoPlayer/VideoReferenceClock.h"
#include "drm/DRMAtomic.h"
#include "drm/DRMLegacy.h"
#include "drm/OffScreenModeSetting.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "utils/DRMHelpers.h"
#include "utils/DisplayInfo.h"
#include "utils/HDRUtils.h"
#include "utils/Map.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#include <algorithm>
#include <array>
#include <mutex>
#include <string.h>
#include <vector>

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

constexpr const char* BROADCAST_RGB = "Broadcast RGB";
constexpr const char* BROADCAST_RGB_AUTOMATIC = "Automatic";
constexpr const char* BROADCAST_RGB_FULL = "Full";
constexpr const char* BROADCAST_RGB_LIMITED = "Limited 16:235";

constexpr const char* COLOR_FORMAT = "color format";

constexpr const char* MAX_BPC = "max bpc";
// indexed by the "Maximum bit depth" setting, 0 is Auto
constexpr std::array<uint64_t, 4> MAX_BPC_VALUES{0, 8, 10, 12};
// The "Adaptive" option's value of the setting. It is stored in the user's settings and numbered in
// linux.xml, so it stays put when a depth is added to MAX_BPC_VALUES, which then has to be checked.
constexpr int MAX_BPC_ADAPTIVE = 4;
static_assert(MAX_BPC_VALUES.size() <= static_cast<size_t>(MAX_BPC_ADAPTIVE),
              "a new depth would take the value of Adaptive, see linux.xml");

// HDMI 2.0 carries at most 600 MHz of TMDS characters: the pixel clock times bits per component
// / 8 for RGB and YCbCr 4:4:4, half of that for 4:2:0, and as is for 4:2:2 (24 bit container).
// 4K 50/60 Hz (594 MHz) fits as 8 bit RGB, but needs 742 MHz as 10 bit.
constexpr uint64_t HDMI_MAX_TMDS_KHZ = 600000;
// YCbCr 4:2:0 over TMDS carries twice the pixel clock, so an HDMI 2.0 source also lists 4:2:0
// modes up to 1.2 GHz (e.g. 4K120 at 1188 MHz). Only a mode above that can be FRL for sure.
constexpr uint64_t HDMI_FRL_PROOF_CLOCK_KHZ = 2 * HDMI_MAX_TMDS_KHZ;
// Max_FRL_Rate code 3 (6 Gbit/s x 4 lanes) is the lowest that carries 4K 60 Hz 10 bit RGB
constexpr int HDMI_FRL_MIN_RATE_RGB_10BIT = 3;
// the kernel's enum names of "color format", indexed by ColorFormat
constexpr std::array<const char*, 5> COLOR_FORMAT_NAMES{"AUTO", "RGB", "YUV 4:4:4", "YUV 4:2:2",
                                                        "YUV 4:2:0"};

//! The smallest depth of the "Maximum bit depth" options that carries the bits per component, the
//! largest one if none does. Only for estimating bandwidth, "max bpc" is only a limit.
uint64_t MaxBpcForContent(int bitsPerComponent)
{
  for (size_t i = 1; i < MAX_BPC_VALUES.size(); ++i)
  {
    if (MAX_BPC_VALUES[i] >= static_cast<uint64_t>(bitsPerComponent))
      return MAX_BPC_VALUES[i];
  }
  return MAX_BPC_VALUES.back();
}

// These map to the definitions in the linux kernel
// drivers/gpu/drm/drm_connector.c

constexpr auto ColorimetryMap = make_map<KODI::UTILS::Colorimetry, std::string_view>({
    {KODI::UTILS::Colorimetry::DEFAULT, "Default"},
    {KODI::UTILS::Colorimetry::SMPTE_170M_YCC, "SMPTE_170M_YCC"},
    {KODI::UTILS::Colorimetry::BT709_YCC, "BT709_YCC"},
    {KODI::UTILS::Colorimetry::XVYCC_601, "XVYCC_601"},
    {KODI::UTILS::Colorimetry::XVYCC_709, "XVYCC_709"},
    {KODI::UTILS::Colorimetry::SYCC_601, "SYCC_601"},
    {KODI::UTILS::Colorimetry::OPYCC_601, "opYCC_601"},
    {KODI::UTILS::Colorimetry::OPRGB, "opRGB"},
    {KODI::UTILS::Colorimetry::BT2020_CYCC, "BT2020_CYCC"},
    {KODI::UTILS::Colorimetry::BT2020_RGB, "BT2020_RGB"},
    {KODI::UTILS::Colorimetry::BT2020_YCC, "BT2020_YCC"},
    {KODI::UTILS::Colorimetry::DCI_P3_RGB_D65, "DCI-P3_RGB_D65"},
    {KODI::UTILS::Colorimetry::DCI_P3_RGB_THEATER, "DCI-P3_RGB_Theater"},
    {KODI::UTILS::Colorimetry::RGB_WIDE_FIXED, "RGB_WIDE_FIXED"},
    {KODI::UTILS::Colorimetry::RGB_WIDE_FLOAT, "RGB_WIDE_FLOAT"},
    {KODI::UTILS::Colorimetry::BT601_YCC, "BT601_YCC"},
    {KODI::UTILS::Colorimetry::ST2113_RGB, "Default"},
    {KODI::UTILS::Colorimetry::ICTCP, "Default"},
});

// Builds the static HDR10/HLG metadata blob contents. Shared by SetHDR() and
// RefreshHDRLightMetadata() so both always describe the stream identically.
hdr_output_metadata BuildHDRMetadata(const VideoPicture& videoPicture, KODI::UTILS::Eotf eotf)
{
  hdr_output_metadata hdr_metadata = {};

  hdr_metadata.metadata_type = KODI::UTILS::HDMI_STATIC_METADATA_TYPE1;
  hdr_metadata.hdmi_metadata_type1.eotf = static_cast<uint8_t>(eotf);
  hdr_metadata.hdmi_metadata_type1.metadata_type = KODI::UTILS::HDMI_STATIC_METADATA_TYPE1;

  if (!hdr_metadata.hdmi_metadata_type1.eotf)
    return hdr_metadata;

  const AVMasteringDisplayMetadata* mdmd = KODI::UTILS::GetMasteringDisplayMetadata(videoPicture);
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

      CLog::Log(LOGDEBUG, LOGVIDEO, "CWinSystemGbm::{} - display_primaries[{}].x: {}", __FUNCTION__,
                i, hdr_metadata.hdmi_metadata_type1.display_primaries[i].x);
      CLog::Log(LOGDEBUG, LOGVIDEO, "CWinSystemGbm::{} - display_primaries[{}].y: {}", __FUNCTION__,
                i, hdr_metadata.hdmi_metadata_type1.display_primaries[i].y);
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

  const AVContentLightMetadata* clmd = KODI::UTILS::GetContentLightMetadata(videoPicture);
  if (clmd)
  {
    hdr_metadata.hdmi_metadata_type1.max_cll = clmd->MaxCLL;
    hdr_metadata.hdmi_metadata_type1.max_fall = clmd->MaxFALL;

    CLog::Log(LOGDEBUG, LOGVIDEO, "CWinSystemGbm::{} - max_cll: {}", __FUNCTION__,
              hdr_metadata.hdmi_metadata_type1.max_cll);
    CLog::Log(LOGDEBUG, LOGVIDEO, "CWinSystemGbm::{} - max_fall: {}", __FUNCTION__,
              hdr_metadata.hdmi_metadata_type1.max_fall);
  }

  return hdr_metadata;
}

} // namespace

CWinSystemGbm::CWinSystemGbm()
  : m_DRM(nullptr),
    m_GBM(std::make_unique<CGBMUtils>()),
    m_libinput(std::make_unique<CLibInputHandler>())
{
  m_dpms = std::make_shared<CGBMDPMSSupport>();
  m_libinput->Start();

  // registered for the lifetime of the object: applying the setting cycles the window system,
  // which would otherwise unregister the callback that is doing the applying
  CServiceBroker::GetSettingsComponent()->GetSettings()->GetSettingsManager()->RegisterCallback(
      this, {SETTING_VIDEOSCREEN_LIMITGUISIZE, SETTING_VIDEOSCREEN_COLORRANGE,
             SETTING_VIDEOSCREEN_COLORFORMAT, SETTING_VIDEOSCREEN_MAXBPC});
}

CWinSystemGbm::~CWinSystemGbm()
{
  RestoreDefaultMaxBpc();

  CServiceBroker::GetSettingsComponent()->GetSettings()->GetSettingsManager()->UnregisterCallback(
      this);
}

const std::string CWinSystemGbm::GetName()
{
  const auto* gui = m_DRM->GetGuiPlane();
  const auto* video = m_DRM->GetVideoPlane();

  std::string name = "gbm (";
  if (gui)
    name += DRMHELPERS::FourCCToString(gui->GetFormat());
  if (gui && video)
    name += " / ";
  if (video)
    name += DRMHELPERS::FourCCToString(video->GetFormat());
  name += ")";
  return name;
}

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

    // Legacy DRM is only for drivers with no atomic modesetting support. An
    // atomic-capable driver that failed to init (e.g. no connector ready yet
    // at cold boot) must never be demoted to legacy; go headless instead.
    if (!CDRMAtomic::SupportsAtomicModesetting())
    {
      m_DRM = std::make_shared<CDRMLegacy>();
      if (!m_DRM->InitDrm())
      {
        CLog::Log(LOGERROR, "CWinSystemGbm::{} - failed to initialize Legacy DRM", __FUNCTION__);
        m_DRM.reset();
      }
    }

    if (!m_DRM)
    {
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
    m_DRM.reset();
    return false;
  }

  SetColorimetry(nullptr);
  SetHDR(nullptr);

  auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return false;

  auto settings = settingsComponent->GetSettings();
  if (!settings)
    return false;

  MigrateLimitedRangeSetting();
  if (auto rangeSetting = settings->GetSetting(SETTING_VIDEOSCREEN_COLORRANGE))
  {
    m_colorRange = settings->GetInt(SETTING_VIDEOSCREEN_COLORRANGE);
    rangeSetting->SetVisible(true);
    if (auto legacySetting = settings->GetSetting(CSettings::SETTING_VIDEOSCREEN_LIMITEDRANGE))
      legacySetting->SetVisible(false);
  }

  if (auto formatSetting = settings->GetSetting(SETTING_VIDEOSCREEN_COLORFORMAT))
  {
    m_colorFormat = settings->GetInt(SETTING_VIDEOSCREEN_COLORFORMAT);

    const auto connector = m_DRM ? m_DRM->GetConnector() : nullptr;
    formatSetting->SetVisible(std::dynamic_pointer_cast<CDRMAtomic>(m_DRM) && connector &&
                              connector->SupportsProperty(COLOR_FORMAT));

    if (connector && connector->GetType().rfind("HDMI", 0) == 0)
    {
      CLog::LogF(LOGINFO, "HDMI sink max FRL rate code {}, a mode above {} kHz is listed: {}",
                 connector->GetMaxFrlRate(), HDMI_FRL_PROOF_CLOCK_KHZ,
                 connector->HasModeAboveClock(HDMI_FRL_PROOF_CLOCK_KHZ));
    }
  }

  if (auto bpcSetting = settings->GetSetting(SETTING_VIDEOSCREEN_MAXBPC))
  {
    m_maxBpc = settings->GetInt(SETTING_VIDEOSCREEN_MAXBPC);

    const auto connector = m_DRM ? m_DRM->GetConnector() : nullptr;
    const bool supported = std::dynamic_pointer_cast<CDRMAtomic>(m_DRM) && connector &&
                           connector->SupportsProperty(MAX_BPC);

    // Only once, after a cycle the property holds what Kodi wrote. A new process reads what the
    // previous one left unless it was put back on exit, see RestoreDefaultMaxBpc().
    if (supported && !m_defaultMaxBpc)
      m_defaultMaxBpc = connector->GetPropertyValue(MAX_BPC);

    bpcSetting->SetVisible(supported);
  }

  auto setting = settings->GetSetting(SETTING_VIDEOSCREEN_LIMITGUISIZE);
  if (setting)
    setting->SetVisible(true);

  SetHDR(nullptr);

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
  // a mode change that failed on a disconnected connector leaves no mode, and no modes to list
  // either: keep the resolutions that are known rather than replace them with an empty one
  if (!m_DRM->HasCurrentMode())
  {
    CLog::LogF(LOGWARNING, "there is no current mode, keeping the resolutions");
    return;
  }

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

    for (auto& res : resolutions)
    {
      CServiceBroker::GetWinSystem()->GetGfxContext().ResetOverscan(res);
      CDisplaySettings::GetInstance().AddResolutionInfo(res);

      if (current.iScreenWidth == res.iScreenWidth && current.iScreenHeight == res.iScreenHeight &&
          current.iWidth == res.iWidth && current.iHeight == res.iHeight &&
          current.fRefreshRate == res.fRefreshRate && current.dwFlags == res.dwFlags)
      {
        CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP) = res;
      }

      CLog::Log(LOGINFO, "Found resolution {}x{} with {}x{}{} @ {:f} Hz", res.iWidth, res.iHeight,
                res.iScreenWidth, res.iScreenHeight,
                res.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "", res.fRefreshRate);
    }
  }

  CDisplaySettings::GetInstance().ApplyCalibrations();

  // Ensure that scrubbed calibrations are always saved, otherwise would depend on calibration GUI
  CDisplaySettings::GetInstance().UpdateCalibrations();
}

void CWinSystemGbm::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (!setting)
    return;

  if (setting->GetId() == SETTING_VIDEOSCREEN_LIMITGUISIZE)
  {
    // post rather than call: this can run off the main thread (e.g. JSON-RPC), and it runs with
    // the settings lock held, which applying the mode needs on the main thread to write back
    // videoscreen.screenmode
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_CALLBACK, -1, -1, &m_limitGuiSizeCallback);
  }
  else if (setting->GetId() == SETTING_VIDEOSCREEN_COLORRANGE)
  {
    m_colorRange = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();

    if (m_migratingColorRange)
      return;

    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_CALLBACK, -1, -1, &m_colorRangeCallback);
  }
  else if (setting->GetId() == SETTING_VIDEOSCREEN_COLORFORMAT)
  {
    m_colorFormat = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();

    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_CALLBACK, -1, -1, &m_colorFormatCallback);
  }
  else if (setting->GetId() == SETTING_VIDEOSCREEN_MAXBPC)
  {
    m_maxBpc = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();

    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_CALLBACK, -1, -1, &m_maxBpcCallback);
  }
}

void CWinSystemGbm::ApplyMaxBpcSetting(void* userptr)
{
  auto winSystem = static_cast<CWinSystemGbm*>(userptr);

  winSystem->m_maxBpcFallback = false;
  winSystem->m_bpcProbe = false;

  // a format may have been refused only at the previous depth
  winSystem->m_colorFormatSkip = 0;
  winSystem->ApplyConnectorProperties();
}

void CWinSystemGbm::RestoreDefaultMaxBpc()
{
  // the kernel keeps the last written value after Kodi exits, so the next process would take it
  // for the driver default (Adaptive leaves 8 behind whenever no video plays)
  if (!m_defaultMaxBpc)
    return;

  const auto connector = m_DRM ? m_DRM->GetConnector() : nullptr;
  if (!connector || !connector->SupportsProperty(MAX_BPC))
    return;

  const auto current = connector->GetPropertyValue(MAX_BPC);
  if (current && *current == *m_defaultMaxBpc)
    return;

  // the original mode that the DRM object restores when it is destroyed picks this up
  if (connector->SetProperty(MAX_BPC, *m_defaultMaxBpc))
    CLog::LogF(LOGDEBUG, "put \"{}\" back to the default {}", MAX_BPC, *m_defaultMaxBpc);
  else
    CLog::LogF(LOGWARNING, "could not put \"{}\" back to the default {}", MAX_BPC,
               *m_defaultMaxBpc);
}

std::optional<uint64_t> CWinSystemGbm::GetChosenMaxBpc() const
{
  if (IsMaxBpcFallback())
    return std::nullopt;

  return GetSettingMaxBpc();
}

bool CWinSystemGbm::IsMaxBpcFallback() const
{
  if (m_bpcProbe)
    return true;

  if (!m_maxBpcFallback)
    return false;

  // The refusal holds for the depth that was refused, which is the one that was queued: within the
  // limits of the driver. Another target (Adaptive going back to 8 bit when the video stops) is a
  // new request that gets its own attempt.
  const auto setting = GetSettingMaxBpc();
  return setting && ClampMaxBpc(*setting) == m_maxBpcRefused;
}

uint64_t CWinSystemGbm::ClampMaxBpc(uint64_t value) const
{
  const auto connector = m_DRM ? m_DRM->GetConnector() : nullptr;
  if (!connector)
    return value;

  // the driver's limits, e.g. Intel stops at 12
  if (const auto limits = connector->GetRangePropertyLimits(MAX_BPC))
    return std::clamp(value, limits->first, limits->second);

  return value;
}

std::optional<uint64_t> CWinSystemGbm::GetSettingMaxBpc() const
{
  const int setting = m_maxBpc;
  if (setting == MAX_BPC_ADAPTIVE)
    return GetAdaptiveMaxBpc();

  if (setting > 0 && setting < static_cast<int>(MAX_BPC_VALUES.size()))
    return MAX_BPC_VALUES[setting];

  return std::nullopt;
}

uint64_t CWinSystemGbm::GetAdaptiveMaxBpc() const
{
  // 8 bit is enough for 8 bit video and the GUI, and keeps the link within reach of any mode
  uint64_t depth = MAX_BPC_VALUES[1];

  // Video with more bits gets deep colour. "max bpc" is a limit, not the depth itself: the driver
  // uses the deepest depth it supports at or below it. Asking for the depth of the video would get
  // 8 bit on hardware that has no 10 bit, so ask for the most there is.
  if (m_contentBits > 8)
    depth = MAX_BPC_VALUES.back();

  return depth;
}

std::optional<uint64_t> CWinSystemGbm::GetTargetMaxBpc() const
{
  const auto connector = m_DRM ? m_DRM->GetConnector() : nullptr;
  if (!connector || !connector->SupportsProperty(MAX_BPC))
    return std::nullopt;

  uint64_t value = 0;
  const auto limits = connector->GetRangePropertyLimits(MAX_BPC);
  if (const auto chosen = GetChosenMaxBpc())
    value = *chosen;
  else if (m_defaultMaxBpc)
    value = *m_defaultMaxBpc; // Auto, or back to it after a refusal
  else if (limits)
    value = limits->second; // fallback when the initial value was unavailable
  else
    return std::nullopt; // never written, so there is nothing to put back

  return ClampMaxBpc(value);
}

uint64_t CWinSystemGbm::GetEffectiveMaxBpc() const
{
  // what is queued, which the connector only shows once it is committed
  if (const auto target = GetTargetMaxBpc())
    return *target;

  const auto connector = m_DRM ? m_DRM->GetConnector() : nullptr;
  if (!connector)
    return 0;

  return connector->GetPropertyValue(MAX_BPC).value_or(0);
}

void CWinSystemGbm::ApplyColorFormatSetting(void* userptr)
{
  auto winSystem = static_cast<CWinSystemGbm*>(userptr);

  // a new choice gets a fresh try, even for a mode where the previous one was refused
  winSystem->m_colorFormatSkip = 0;

  // The format can be why the depth was refused (RGB may not carry 12 bit where 4:2:0 does), so
  // the chosen depth gets a fresh try as well
  winSystem->m_maxBpcFallback = false;
  winSystem->m_bpcProbe = false;

  // a format that the connector does not offer would only act as Auto, so show that
  if (!winSystem->IsColorFormatOffered(winSystem->GetColorFormat()))
  {
    CLog::LogF(LOGINFO, "the connector does not offer the chosen colour format, using Auto");
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetInt(
        SETTING_VIDEOSCREEN_COLORFORMAT, static_cast<int>(ColorFormat::AUTO));
  }

  winSystem->ApplyConnectorProperties();
}

bool CWinSystemGbm::IsColorFormatOffered(ColorFormat format) const
{
  const auto connector = m_DRM ? m_DRM->GetConnector() : nullptr;

  // nothing to judge it by, leave the choice alone
  if (!std::dynamic_pointer_cast<CDRMAtomic>(m_DRM) || !connector ||
      !connector->SupportsProperty(COLOR_FORMAT))
    return true;

  const auto listed = [&](ColorFormat candidate)
  {
    const char* name = COLOR_FORMAT_NAMES[static_cast<size_t>(candidate)];
    return connector->GetPropertyEnumValue(COLOR_FORMAT, name).has_value();
  };

  // Adaptive only ever asks for the subsampled formats
  if (format == ColorFormat::ADAPTIVE)
    return listed(ColorFormat::YUV422) || listed(ColorFormat::YUV420);

  return format == ColorFormat::AUTO || listed(format);
}

ColorFormat CWinSystemGbm::GetColorFormat() const
{
  const int value = m_colorFormat;
  if (value < static_cast<int>(ColorFormat::AUTO) ||
      value > static_cast<int>(ColorFormat::ADAPTIVE))
    return ColorFormat::AUTO;

  return static_cast<ColorFormat>(value);
}

ColorFormat CWinSystemGbm::GetRequestedColorFormat() const
{
  // The probe at the default depth changes the depth and nothing else. With Adaptive the format
  // depends on the depth, and would go back to Auto, so a success would not tell the depth refused.
  if (m_bpcProbe)
    return m_probeFormat;

  if (!std::dynamic_pointer_cast<CDRMAtomic>(m_DRM))
    return ColorFormat::AUTO;

  const auto connector = m_DRM->GetConnector();
  if (!connector || !connector->SupportsProperty(COLOR_FORMAT))
    return ColorFormat::AUTO;

  std::vector<ColorFormat> candidates;
  const ColorFormat format = GetColorFormat();
  if (format == ColorFormat::ADAPTIVE)
  {
    if (ContentNeedsSubsampling())
    {
      // 4:2:2 keeps the full vertical resolution of the chroma, but only fits up to 600 MHz
      if (m_DRM->GetModeClock() <= HDMI_MAX_TMDS_KHZ)
        candidates.push_back(ColorFormat::YUV422);
      candidates.push_back(ColorFormat::YUV420);
    }
  }
  else if (format != ColorFormat::AUTO)
  {
    candidates.push_back(format);
  }

  // Only formats that the driver lists (Intel has no YCbCr 4:2:2 on HDMI), or a colorspace tag
  // would describe the wrong encoding.
  candidates.erase(std::remove_if(candidates.begin(), candidates.end(), [&](ColorFormat candidate)
                                  { return !IsColorFormatOffered(candidate); }),
                   candidates.end());

  // Auto is the last resort, after the formats refused for this mode
  candidates.push_back(ColorFormat::AUTO);

  const size_t skip = static_cast<size_t>(std::max(m_colorFormatSkip, 0));
  return candidates[std::min(skip, candidates.size() - 1)];
}

bool CWinSystemGbm::IsHdmiFrlLink() const
{
  const auto connector = m_DRM ? m_DRM->GetConnector() : nullptr;
  if (!connector)
    return false;

  // no property tells: the sink has to do FRL, and a mode above the proof clock shows that the
  // source does too
  return connector->GetMaxFrlRate() >= HDMI_FRL_MIN_RATE_RGB_10BIT &&
         connector->HasModeAboveClock(HDMI_FRL_PROOF_CLOCK_KHZ);
}

bool CWinSystemGbm::ContentNeedsSubsampling() const
{
  if (m_contentBits <= 8 || !m_DRM)
    return false;

  // DisplayPort has the bandwidth or compresses, leave it to Auto
  const auto connector = m_DRM->GetConnector();
  if (!connector || connector->GetType().rfind("HDMI", 0) != 0)
    return false;

  if (IsHdmiFrlLink())
    return false;

  // The depth to keep is the one of the video, as far as the driver may go
  uint64_t depth = MaxBpcForContent(m_contentBits);
  if (const uint64_t bpc = GetEffectiveMaxBpc(); bpc != 0)
  {
    // RGB fits at 8 bit
    if (bpc < 10)
      return false;

    depth = std::min(depth, bpc);
  }

  // RGB needs depth/8 of the pixel clock, YCbCr 4:2:0 half of that, and 4:2:2 the pixel clock
  // itself, which 4:2:0 never exceeds. Subsampling only helps in between: below RGB fits, above not
  // even 4:2:0 does, or the link is HDMI 2.1 and carries RGB.
  const uint64_t clock = m_DRM->GetModeClock();

  return clock * depth / 8 > HDMI_MAX_TMDS_KHZ && clock * depth / 16 <= HDMI_MAX_TMDS_KHZ;
}

static bool IsYuvFormat(ColorFormat format)
{
  return format == ColorFormat::YUV444 || format == ColorFormat::YUV422 ||
         format == ColorFormat::YUV420;
}

bool CWinSystemGbm::IsYuvRequested() const
{
  return IsYuvFormat(GetRequestedColorFormat());
}

bool CWinSystemGbm::IsYuvFormatCommitted(const CDRMConnector& connector) const
{
  const auto current = connector.GetPropertyValue(COLOR_FORMAT);
  if (!current)
    return false;

  for (const ColorFormat format : {ColorFormat::YUV444, ColorFormat::YUV422, ColorFormat::YUV420})
  {
    const auto value = connector.GetPropertyEnumValue(
        COLOR_FORMAT, COLOR_FORMAT_NAMES[static_cast<size_t>(format)]);
    if (value && *value == *current)
      return true;
  }

  return false;
}

void CWinSystemGbm::ApplyColorRangeSetting(void* userptr)
{
  static_cast<CWinSystemGbm*>(userptr)->ApplyConnectorProperties();
}

ColorRange CWinSystemGbm::GetColorRange() const
{
  const int value = m_colorRange;
  if (value < static_cast<int>(ColorRange::AUTO) || value > static_cast<int>(ColorRange::LIMITED))
    return ColorRange::AUTO;

  return static_cast<ColorRange>(value);
}

void CWinSystemGbm::MigrateLimitedRangeSetting()
{
  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  if (!settings->GetBool(CSettings::SETTING_VIDEOSCREEN_LIMITEDRANGE))
    return;

  const auto rangeSetting = settings->GetSetting(SETTING_VIDEOSCREEN_COLORRANGE);
  if (!rangeSetting)
    return;

  // cleared either way, or choosing Auto later would be undone by the next start
  if (rangeSetting->IsDefault())
  {
    CLog::LogF(LOGINFO, "migrating \"use limited colour range\" to the colour range setting");
    m_migratingColorRange = true;
    settings->SetInt(SETTING_VIDEOSCREEN_COLORRANGE, static_cast<int>(ColorRange::LIMITED));
    m_migratingColorRange = false;
  }

  settings->SetBool(CSettings::SETTING_VIDEOSCREEN_LIMITEDRANGE, false);
}

void CWinSystemGbm::ApplyConnectorProperties(bool retry)
{
  if (!retry)
    m_connectorRetries = 0;

  ApplyBroadcastRange();
  ApplyMaxBpc();
  ApplyColorFormat();
}

void CWinSystemGbm::ApplyBroadcastRange()
{
  const char* targetName = BROADCAST_RGB_AUTOMATIC;
  switch (GetColorRange())
  {
    case ColorRange::FULL:
      targetName = BROADCAST_RGB_FULL;
      break;
    case ColorRange::LIMITED:
      targetName = BROADCAST_RGB_LIMITED;
      break;
    case ColorRange::AUTO:
      break;
  }

  QueueConnectorProperty(m_rangeRequest, BROADCAST_RGB, targetName);
}

bool CWinSystemGbm::QueueConnectorProperty(PropertyRequest& request,
                                           const char* property,
                                           const char* valueName)
{
  const auto connector = m_DRM ? m_DRM->GetConnector() : nullptr;
  if (!connector)
    return false;

  const auto target = connector->GetPropertyEnumValue(property, valueName);
  if (!target)
    return false;

  return QueueConnectorProperty(request, property, *target, valueName);
}

bool CWinSystemGbm::QueueConnectorProperty(PropertyRequest& request,
                                           const char* property,
                                           uint64_t target,
                                           std::string_view label)
{
  auto drm = std::dynamic_pointer_cast<CDRMAtomic>(m_DRM);
  if (!drm)
    return false;

  auto connector = drm->GetConnector();
  if (!connector || !connector->SupportsProperty(property))
    return false;

  if (connector->IsPropertyImmutable(property).value_or(false))
    return false;

  // always queue it, to replace a pending value that is not committed yet
  if (!drm->AddProperty(connector, property, target))
    return false;

  request.property = property;
  request.queued = true;
  request.target = target;

  CLog::LogF(LOGDEBUG, "queued connector \"{}\" = \"{}\"", property, label);

  const auto current = connector->GetPropertyValue(property);
  if (!current || *current != target)
    drm->RequestModeset();

  return true;
}

void CWinSystemGbm::ApplyMaxBpc()
{
  const auto value = GetTargetMaxBpc();
  if (!value)
    return;

  QueueConnectorProperty(m_bpcRequest, MAX_BPC, *value, std::to_string(*value));
}

void CWinSystemGbm::ApplyColorFormat()
{
  const ColorFormat format = GetRequestedColorFormat();

  QueueConnectorProperty(m_formatRequest, COLOR_FORMAT,
                         COLOR_FORMAT_NAMES[static_cast<size_t>(format)]);

  // the format is read once, so that the tag describes the encoding that is queued with it
  const bool yuv = IsYuvFormat(format);
  if (yuv != m_scanoutYuv)
    ApplyColorspace(yuv);
}

bool CWinSystemGbm::IsRequestRejected(PropertyRequest& request)
{
  if (!request.queued)
    return false;

  request.queued = false;

  const CDRMConnector* connector = m_DRM ? m_DRM->GetConnector() : nullptr;
  if (!connector)
    return false;

  // a refused commit is replaced by the previous request, which may hold the same value (e.g. when
  // switching modes), so the value alone does not tell
  return m_DRM->LastCommitFellBack() ||
         connector->GetPropertyValue(request.property) != request.target;
}

void CWinSystemGbm::RequeueConnectorProperties()
{
  ApplyConnectorProperties(true);

  ApplyColorspace();
  if (auto drm = std::dynamic_pointer_cast<CDRMAtomic>(m_DRM);
      drm && drm->GetConnector() && drm->GetConnector()->SupportsProperty("HDR_OUTPUT_METADATA"))
  {
    if (m_hdrBlob && m_hdrBlob->IsValid())
    {
      drm->AddBlobProperty(drm->GetConnector(), "HDR_OUTPUT_METADATA", m_hdrBlob);
    }
    else if (m_hdrClearRejected)
    {
      // the metadata was cleared and the rejected commit did not carry that
      drm->AddProperty(drm->GetConnector(), "HDR_OUTPUT_METADATA", m_hdrRequest.target);
      m_hdrRequest.queued = true;
    }
  }

  // The rejected commit took the modeset request along, and queueing only requests one when the
  // cached value differs from the target, which it may not (same depth, other refresh rate).
  if (m_DRM)
    m_DRM->RequestModeset();
}

bool CWinSystemGbm::RetryDroppedCommit()
{
  if (!m_DRM)
    return false;

  const bool notSubmitted = !m_DRM->LastFlipSubmitted();
  const bool failed = m_DRM->LastCommitFailed();
  if (!notSubmitted && !failed)
    return false;

  const bool modesetLost = m_DRM->LastFlipCarriedModeset();
  const bool queued =
      m_rangeRequest.queued || m_formatRequest.queued || m_bpcRequest.queued || m_hdrRequest.queued;
  const bool propertiesLost = failed && queued;

  if (!modesetLost && !propertiesLost)
    return notSubmitted; // a plain flip that failed has nothing to replay

  if (propertiesLost)
  {
    m_hdrClearRejected = m_hdrRequest.queued;
    m_rangeRequest.queued = false;
    m_formatRequest.queued = false;
    m_bpcRequest.queued = false;
    m_hdrRequest.queued = false;
  }

  if (m_connectorRetries++ < 1)
  {
    CLog::LogF(LOGWARNING, "the commit did not go through, queueing the modeset {}again",
               queued ? "and the connector properties " : "");

    // the modeset alone is enough when no property was queued: the active state and the mode are
    // read when it is committed
    if (queued)
      RequeueConnectorProperties();
    else
      m_DRM->RequestModeset();

    return true;
  }

  CLog::LogF(LOGWARNING, "the commit did not go through again, the modeset is not applied");

  // a probe that cannot be evaluated must not stay on
  if (m_bpcProbe)
  {
    m_bpcProbe = false;
    m_maxBpcFallback = false;
  }

  return true;
}

void CWinSystemGbm::RetryConnectorProperties()
{
  if (RetryDroppedCommit())
    return;

  const bool bpcPending = m_bpcRequest.queued;
  const bool rangeRejected = IsRequestRejected(m_rangeRequest);
  const bool formatRejected = IsRequestRejected(m_formatRequest);
  const bool bpcRejected = IsRequestRejected(m_bpcRequest);
  const bool hdrRejected = IsRequestRejected(m_hdrRequest);
  m_hdrClearRejected = hdrRejected;

  if (!rangeRejected && !formatRejected && !bpcRejected && !hdrRejected)
  {
    // the requests went through, a later rejection gets its own retry
    m_connectorRetries = 0;
    if (m_bpcProbe && bpcPending)
    {
      // the default depth went through, so the chosen one was refused
      m_bpcProbe = false;

      CLog::LogF(LOGWARNING,
                 "connector \"{}\" could not be committed with the chosen value, using the default",
                 MAX_BPC);

      // The probe kept the format that went with the chosen depth. With the default depth that
      // format may be another one (Adaptive back to Auto), which is not committed yet.
      ApplyConnectorProperties();
    }
    return;
  }

  // the default depth did not help, so it was not the cause: take the choice back
  bool probeFailed = false;
  if (m_bpcProbe)
  {
    m_bpcProbe = false;
    m_maxBpcFallback = false;
    probeFailed = true;
  }

  // an explicit format that the display or mode refuses is not worth a retry, likely the cause
  const bool explicitFormat = formatRejected && GetRequestedColorFormat() != ColorFormat::AUTO;
  const bool customBpc = bpcRejected && m_maxBpc > 0 && !IsMaxBpcFallback();

  // format and depth are both suspects, try the default depth first so the format is kept
  if (explicitFormat && customBpc && !probeFailed)
  {
    m_probeFormat = GetRequestedColorFormat();
    m_bpcProbe = true;
    m_maxBpcFallback = true;
    m_maxBpcRefused = m_bpcRequest.target;
    m_maxBpcFailedMode = m_currentModeId;
    m_connectorRetries = 0;
    RequeueConnectorProperties();
    return;
  }

  // a rejected commit drops the queued properties and the modeset request, so queue them again
  if (!explicitFormat && m_connectorRetries++ < 1)
  {
    RequeueConnectorProperties();
    return;
  }

  if (explicitFormat)
  {
    const ColorFormat refused = GetRequestedColorFormat();
    m_colorFormatFailedMode = m_currentModeId;
    ++m_colorFormatSkip;

    CLog::LogF(LOGWARNING, "connector \"{}\" = \"{}\" could not be committed, using \"{}\"",
               COLOR_FORMAT, COLOR_FORMAT_NAMES[static_cast<size_t>(refused)],
               COLOR_FORMAT_NAMES[static_cast<size_t>(GetRequestedColorFormat())]);

    m_connectorRetries = 0;
    RequeueConnectorProperties();
    return;
  }

  // no format left to blame, the chosen depth may be the cause
  if (bpcRejected && m_maxBpc > 0 && !IsMaxBpcFallback())
  {
    CLog::LogF(LOGWARNING, "connector \"{}\" = {} could not be committed, using the default",
               MAX_BPC, m_bpcRequest.target);

    m_maxBpcFallback = true;
    m_maxBpcRefused = m_bpcRequest.target;
    m_maxBpcFailedMode = m_currentModeId;
    m_connectorRetries = 0;
    RequeueConnectorProperties();
    return;
  }

  if (rangeRejected)
    CLog::LogF(LOGWARNING, "connector \"{}\" could not be committed", m_rangeRequest.property);
  if (formatRejected)
    CLog::LogF(LOGWARNING, "connector \"{}\" could not be committed", m_formatRequest.property);
  if (bpcRejected)
    CLog::LogF(LOGWARNING, "connector \"{}\" could not be committed", m_bpcRequest.property);
  if (hdrRejected)
    CLog::LogF(LOGWARNING, "connector \"{}\" could not be committed", m_hdrRequest.property);
}

void CWinSystemGbm::UpdateDriverLimitedRange()
{
  bool limited = false;

  if (const CDRMConnector* connector = m_DRM ? m_DRM->GetConnector() : nullptr)
  {
    const auto current = connector->GetPropertyValue(BROADCAST_RGB);
    const auto limitedValue = connector->GetPropertyEnumValue(BROADCAST_RGB, BROADCAST_RGB_LIMITED);
    limited = current && limitedValue && *current == *limitedValue;

    // the driver's RGB to YCbCr conversion is limited range
    if (!limited)
      limited = IsYuvFormatCommitted(*connector);
  }

  m_driverLimitedRange = limited;
}

void CWinSystemGbm::ApplyLimitGuiSize(void* userptr)
{
  auto* winSystem = static_cast<CWinSystemGbm*>(userptr);

  // the limit is applied as the mode list is built, so the list has to be rebuilt and the
  // current mode re-applied for the GUI to be recreated at the new size
  CDisplaySettings& displaySettings = CDisplaySettings::GetInstance();
  const RESOLUTION_INFO desktop = displaySettings.GetResolutionInfo(RES_DESKTOP);
  winSystem->UpdateResolutions();

  // UpdateResolutions() makes the current mode the desktop mode, which during playback is the
  // mode chosen for the video rather than the one to return to when it stops
  for (size_t i = RES_CUSTOM; i < displaySettings.ResolutionInfoSize(); ++i)
  {
    const RESOLUTION_INFO& res = displaySettings.GetResolutionInfo(i);
    if (res.iScreenWidth == desktop.iScreenWidth && res.iScreenHeight == desktop.iScreenHeight &&
        res.fRefreshRate == desktop.fRefreshRate && res.dwFlags == desktop.dwFlags)
    {
      displaySettings.GetResolutionInfo(RES_DESKTOP) = res;
      break;
    }
  }

  CGraphicContext& gfx = winSystem->GetGfxContext();
  gfx.SetVideoResolution(gfx.GetVideoResolution(), true);
}

bool CWinSystemGbm::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  return true;
}

bool CWinSystemGbm::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  // Notify other subsystems that we will change resolution
  OnLostDevice();

  if (!m_DRM->SetMode(res))
  {
    CLog::Log(LOGERROR, "CWinSystemGbm::{} - failed to set DRM mode", __FUNCTION__);
    return false;
  }

  m_currentModeId = res.strId;
  // a refusal of a depth or format only holds for the mode it happened in
  if ((m_maxBpcFallback || m_bpcProbe) && m_maxBpcFailedMode != m_currentModeId)
  {
    m_maxBpcFallback = false;
    m_bpcProbe = false;
  }
  if (m_colorFormatSkip != 0 && m_colorFormatFailedMode != m_currentModeId)
    m_colorFormatSkip = 0;

  // the properties go out with the modeset, and are retried if it is rejected
  ApplyConnectorProperties();

  struct gbm_bo* bo = nullptr;

  if (!std::dynamic_pointer_cast<CDRMAtomic>(m_DRM))
  {
    bo = m_GBM->GetDevice().GetSurface().LockFrontBuffer().Get();
    if (!bo)
    {
      CLog::Log(LOGERROR, "CWinSystemGbm::{} - failed to lock front buffer", __FUNCTION__);
      return false;
    }
  }

  auto result = m_DRM->SetVideoMode(res, bo);

  // For atomic DRM, SetVideoMode only queues the modeset (m_need_modeset).
  // Commit it now so the kernel CRTC matches the new surface before any
  // subsequent eglSwapBuffers; otherwise mesa logs a spurious
  // EGL_BAD_SURFACE on the next frame while userspace surface and kernel
  // CRTC are out of sync. Legacy DRM already committed in SetVideoMode.
  if (result && std::dynamic_pointer_cast<CDRMAtomic>(m_DRM))
    FlipPage(true, false, false);

  auto delay =
      std::chrono::milliseconds(CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
                                    "videoscreen.delayrefreshchange") *
                                100);
  if (delay > 0ms)
    m_dispResetTimer.Set(delay);

  return result;
}

void CWinSystemGbm::FlipPage(bool rendered, bool videoLayer, bool async)
{
  if (m_videoLayerBridge && !videoLayer)
  {
    // disable video plane when video layer no longer is active
    m_videoLayerBridge->Disable();
  }

  struct gbm_bo* bo = nullptr;

  if (rendered)
  {
    bo = m_GBM->GetDevice().GetSurface().LockFrontBuffer().Get();
  }

  m_DRM->FlipPage(bo, rendered, videoLayer, async);

  UpdateDriverLimitedRange();
  RetryConnectorProperties();

  // !videoLayer alone cannot gate teardown: FlipPage also runs with videoLayer
  // false during a mode switch or renderer swap. use_count reaches 1 only when
  // CRendererDRMPRIME drops its bridge reference, which happens at video stop.
  if (m_videoLayerBridge && !videoLayer && m_videoLayerBridge.use_count() == 1)
  {
    // delete video layer bridge when video layer no longer is active
    m_videoLayerBridge.reset();

    //! @todo unify D2P and single-plane teardown behind one winsystem plane API
    m_DRM->ReleaseVideoPlane();
    auto* gui = m_DRM->GetGuiPlane();
    m_DRM->FindGuiPlane(gui->GetFormat(), gui->GetModifier());
  }
}

bool CWinSystemGbm::UseLimitedColor()
{
  // the driver converts while limited is committed, so don't compress a second time
  if (m_driverLimitedRange)
    return false;

  return GetColorRange() == ColorRange::LIMITED;
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

void CWinSystemGbm::Register(IDispResource* resource)
{
  std::unique_lock lock(m_resourceSection);
  m_resources.push_back(resource);
}

void CWinSystemGbm::Unregister(IDispResource* resource)
{
  std::unique_lock lock(m_resourceSection);
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

  std::unique_lock lock(m_resourceSection);
  std::vector<IDispResource*> resources = m_resources;

  for (auto resource : resources)
  {
    if (find(m_resources.begin(), m_resources.end(), resource) == m_resources.end())
      continue;

    resource->OnLostDisplay();
  }
}

void CWinSystemGbm::OnResetDisplay()
{
  CLog::Log(LOGDEBUG, "{} - notify display reset event", __FUNCTION__);

  std::unique_lock lock(m_resourceSection);
  std::vector<IDispResource*> resources = m_resources;

  for (auto resource : resources)
  {
    if (find(m_resources.begin(), m_resources.end(), resource) == m_resources.end())
      continue;

    resource->OnResetDisplay();
  }
}

std::unique_ptr<CVideoSync> CWinSystemGbm::GetVideoSync(CVideoReferenceClock* clock)
{
  return std::make_unique<CVideoSyncGbm>(clock);
}

std::vector<std::string> CWinSystemGbm::GetConnectedOutputs()
{
  std::vector<std::string> outputs;
  outputs.emplace_back(OUTPUT_NAME_DEFAULT);
  for (const auto& name : m_DRM->GetConnectedConnectorNames())
  {
    outputs.emplace_back(name);
  }
  return outputs;
}

bool CWinSystemGbm::SetVideoOutput(const VideoPicture* videoPicture)
{
  // Base: flip the gui/video plane role for the single shared output
  // plane on GBM. videoPicture set means a video is starting and the
  // output plane should be tracked as the video plane; nullptr means
  // playback ended and the plane reverts to the gui role.
  //
  // Format and modifier come from whichever plane is currently active.
  // EGL backends override this and rebuild the GBM/EGL output surface
  // at the target bit depth before chaining here, so by the time we
  // run the active plane's cached format reflects the post-rebuild
  // surface (e.g. AR30 for 10-bit content).
  CDRMPlane* current = m_DRM->GetGuiPlane() ? m_DRM->GetGuiPlane() : m_DRM->GetVideoPlane();
  if (!current)
    return false;

  return videoPicture ? m_DRM->FindVideoPlane(current->GetFormat(), current->GetModifier())
                      : m_DRM->FindGuiPlane(current->GetFormat(), current->GetModifier());
}

void CWinSystemGbm::SetColorimetry(const VideoPicture* videoPicture)
{
  auto drm = std::dynamic_pointer_cast<CDRMAtomic>(m_DRM);
  if (!drm)
    return;

  // Adaptive follows the video: >8 bit content may need deep colour or YCbCr, so evaluate when it
  // starts or stops. The depth goes first, as the choice of format depends on it.
  m_contentBits = videoPicture ? videoPicture->colorBits : 0;
  if (m_maxBpc == MAX_BPC_ADAPTIVE)
    ApplyMaxBpc();
  if (GetColorFormat() == ColorFormat::ADAPTIVE)
    ApplyColorFormat();

  auto connector = drm->GetConnector();
  if (!connector || !connector->SupportsProperty("Colorspace"))
    return;

  KODI::UTILS::Colorimetry colorimetry = KODI::UTILS::Colorimetry::DEFAULT;

  if (videoPicture)
    colorimetry = KODI::UTILS::GetColorimetry(*videoPicture);

  m_colorimetry = colorimetry;

  ApplyColorspace();
}

void CWinSystemGbm::ApplyColorspace()
{
  ApplyColorspace(IsYuvRequested());
}

void CWinSystemGbm::ApplyColorspace(bool yuv)
{
  auto drm = std::dynamic_pointer_cast<CDRMAtomic>(m_DRM);
  if (!drm)
    return;

  auto connector = drm->GetConnector();
  if (!connector || !connector->SupportsProperty("Colorspace"))
    return;

  const KODI::UTILS::Colorimetry colorimetry = m_colorimetry;
  m_scanoutYuv = yuv;

  // The DRM Colorspace connector property controls what colorimetry tag the
  // driver transmits in the HDMI AVI InfoFrame (or DisplayPort VSC SDP). The
  // transmitted tag must match the pixel encoding actually on the wire.
  // Kodi scans out to an RGB primary plane (XR24/XR30/AB4H), which the driver sends as RGB or
  // converts when a YCbCr "color format" is requested. Signaling YCC while sending RGB, or the
  // opposite, causes most DP-to-HDMI bridges (incl. LSPCON) to misinterpret the signal.
  //
  // The CTA-861 standard (used by both HDMI and DisplayPort for colorimetry
  // signaling) defines explicit RGB variants only for wide-gamut spaces
  // (BT.2020_RGB, DCI-P3_RGB). BT.709 RGB and BT.601 RGB are signaled via
  // "Default", with the sink inferring primaries from the transmitted mode
  // (HD resolution -> BT.709, SD resolution -> BT.601).
  //
  // For YCbCr, wide gamut content is tagged BT2020_YCC and the rest BT709_YCC: the driver's
  // conversion does not depend on the resolution, so "Default" could read an SD mode as BT.601.
  KODI::UTILS::Colorimetry scanoutColorimetry;
  switch (colorimetry)
  {
    case KODI::UTILS::Colorimetry::BT2020_YCC:
    case KODI::UTILS::Colorimetry::BT2020_CYCC:
      scanoutColorimetry =
          yuv ? KODI::UTILS::Colorimetry::BT2020_YCC : KODI::UTILS::Colorimetry::BT2020_RGB;
      break;
    default:
      // BT709_YCC, SMPTE_170M_YCC and all other YCC variants map to Default for RGB, see above
      scanoutColorimetry =
          yuv ? KODI::UTILS::Colorimetry::BT709_YCC : KODI::UTILS::Colorimetry::DEFAULT;
      break;
  }

  std::optional<uint64_t> colorspace =
      connector->GetPropertyEnumValue("Colorspace", ColorimetryMap.at(scanoutColorimetry));
  if (!colorspace && yuv)
  {
    // no YCC variant listed: an untagged signal is better than a wrong one
    scanoutColorimetry = KODI::UTILS::Colorimetry::DEFAULT;
    colorspace =
        connector->GetPropertyEnumValue("Colorspace", ColorimetryMap.at(scanoutColorimetry));
  }

  if (colorspace)
  {
    CLog::LogF(LOGDEBUG, "setting connector colorspace to {} (source {}, {} output)",
               KODI::UTILS::ColorimetryToString(scanoutColorimetry),
               KODI::UTILS::ColorimetryToString(colorimetry), yuv ? "YCbCr" : "RGB");
    drm->AddProperty(connector, "Colorspace", colorspace.value());
    drm->SetActive(true);
  }
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
    if (connector->SupportsProperty("HDR_OUTPUT_METADATA"))
    {
      CLog::LogF(LOGDEBUG, "clearing HDR_OUTPUT_METADATA");
      drm->AddProperty(connector, "HDR_OUTPUT_METADATA", 0);
      drm->SetActive(true);

      // tracked, as a rejected commit falls back to the previous request, which still has the HDR
      // metadata, and nothing else would send the clear again
      m_hdrRequest.property = "HDR_OUTPUT_METADATA";
      m_hdrRequest.queued = true;
      m_hdrRequest.target = 0;

      m_hdrBlob.reset();
    }

    m_eotf = KODI::UTILS::Eotf::TRADITIONAL_SDR;
    return false;
  }

  // Only enable HDR for PQ (HDR10/HDR10+/DV) or HLG transfer functions
  if (videoPicture->color_transfer != AVCOL_TRC_SMPTE2084 &&
      videoPicture->color_transfer != AVCOL_TRC_ARIB_STD_B67)
  {
    m_eotf = KODI::UTILS::Eotf::TRADITIONAL_SDR;
    return false;
  }

  KODI::UTILS::Eotf eotf = KODI::UTILS::GetEOTF(*videoPicture);
  m_eotf = eotf;

  if (connector->SupportsProperty("HDR_OUTPUT_METADATA") && m_info &&
      m_info->SupportsHDRStaticMetadataType1() && m_info->SupportsEOTF(eotf))
  {
    // Requests that were queued earlier keep the previous blob alive until they are released
    m_hdrBlob.reset();

    if (static_cast<uint8_t>(eotf))
    {
      hdr_output_metadata hdr_metadata = BuildHDRMetadata(*videoPicture, eotf);
      auto blob = std::make_shared<CDRMPropertyBlob>(drm->GetFileDescriptor(), &hdr_metadata,
                                                     sizeof(hdr_metadata));
      if (blob->IsValid())
        m_hdrBlob = std::move(blob);
      else
        CLog::LogF(LOGERROR, "failed to create the HDR_OUTPUT_METADATA blob");
    }

    if (m_hdrBlob)
      drm->AddBlobProperty(connector, "HDR_OUTPUT_METADATA", m_hdrBlob);
    else
      drm->AddProperty(connector, "HDR_OUTPUT_METADATA", 0);
    drm->SetActive(true);

    // replaces a clear that was not committed yet
    m_hdrRequest.queued = false;
  }

  return m_hdrBlob && m_hdrBlob->IsValid();
}

bool CWinSystemGbm::SupportsLiveHDRMetadata() const
{
  // HDR_OUTPUT_METADATA is only queued through the atomic API. Without a valid blob from
  // SetHDR() a metadata change has to reconfigure, so that SetHDR() gets a chance to retry.
  auto drm = std::dynamic_pointer_cast<CDRMAtomic>(m_DRM);
  if (!drm || !m_hdrBlob || !m_hdrBlob->IsValid())
    return false;

  auto connector = drm->GetConnector();
  return connector && connector->SupportsProperty("HDR_OUTPUT_METADATA");
}

bool CWinSystemGbm::RefreshHDRLightMetadata(const VideoPicture* videoPicture)
{
  if (!videoPicture || !m_hdrBlob || !m_hdrBlob->IsValid())
    return false;

  auto drm = std::dynamic_pointer_cast<CDRMAtomic>(m_DRM);
  if (!drm)
    return false;

  auto connector = drm->GetConnector();
  if (!connector || !connector->SupportsProperty("HDR_OUTPUT_METADATA"))
    return false;

  // keep the EOTF negotiated by SetHDR()
  hdr_output_metadata hdr_metadata = BuildHDRMetadata(*videoPicture, m_eotf);
  auto blob = std::make_shared<CDRMPropertyBlob>(drm->GetFileDescriptor(), &hdr_metadata,
                                                 sizeof(hdr_metadata));
  if (!blob->IsValid())
    return false;

  // Deliberately no drm->SetActive(): that forces a modeset (black screen). The property is
  // queued and applied with the next ordinary page flip.
  // The request takes shared ownership of the blob. The superseded blob stays alive for as long
  // as a queued request, including the fallback request, still references its id.
  if (!drm->AddBlobProperty(connector, "HDR_OUTPUT_METADATA", blob))
    return false;

  m_hdrBlob = std::move(blob);

  CLog::LogF(LOGDEBUG,
             "refreshed HDR metadata (max_cll: {}, max_fall: {}, max_lum: {} nits, "
             "min_lum: {:.4f} nits, wp: {:.4f}/{:.4f}, "
             "R: {:.4f}/{:.4f}, G: {:.4f}/{:.4f}, B: {:.4f}/{:.4f})",
             hdr_metadata.hdmi_metadata_type1.max_cll, hdr_metadata.hdmi_metadata_type1.max_fall,
             hdr_metadata.hdmi_metadata_type1.max_display_mastering_luminance,
             hdr_metadata.hdmi_metadata_type1.min_display_mastering_luminance / 10000.0,
             hdr_metadata.hdmi_metadata_type1.white_point.x / 50000.0,
             hdr_metadata.hdmi_metadata_type1.white_point.y / 50000.0,
             hdr_metadata.hdmi_metadata_type1.display_primaries[0].x / 50000.0,
             hdr_metadata.hdmi_metadata_type1.display_primaries[0].y / 50000.0,
             hdr_metadata.hdmi_metadata_type1.display_primaries[1].x / 50000.0,
             hdr_metadata.hdmi_metadata_type1.display_primaries[1].y / 50000.0,
             hdr_metadata.hdmi_metadata_type1.display_primaries[2].x / 50000.0,
             hdr_metadata.hdmi_metadata_type1.display_primaries[2].y / 50000.0);

  return true;
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

  caps.SetMaxLuminance(m_info->GetMaxLuminance());

  return caps;
}
