/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "AndroidUtils.h"

#include "ServiceBroker.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/SettingsManager.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#include "platform/android/activity/XBMCApp.h"

#include <androidjni/MediaCodecInfo.h>
#include <androidjni/MediaCodecList.h>
#include <androidjni/View.h>
#include <androidjni/Window.h>

static std::vector<RESOLUTION_INFO> s_res_displayModes;
static RESOLUTION_INFO s_res_cur_displayMode;

static void fetchDisplayModes()
{
  s_res_displayModes.clear();

  CJNIDisplay display = CXBMCApp::getWindow().getDecorView().getDisplay();

  if (display)
  {
    CJNIDisplayMode m = display.getMode();
    if (m)
    {
      const bool landscape = m.getPhysicalWidth() > m.getPhysicalHeight();
      CLog::Log(LOGDEBUG, "CAndroidUtils: current mode: {}: {}x{}@{:f}", m.getModeId(),
                m.getPhysicalWidth(), m.getPhysicalHeight(), m.getRefreshRate());
      s_res_cur_displayMode.strId = std::to_string(m.getModeId());
      s_res_cur_displayMode.iWidth = s_res_cur_displayMode.iScreenWidth =
          landscape ? m.getPhysicalWidth() : m.getPhysicalHeight();
      s_res_cur_displayMode.iHeight = s_res_cur_displayMode.iScreenHeight =
          landscape ? m.getPhysicalHeight() : m.getPhysicalWidth();
      s_res_cur_displayMode.fRefreshRate = m.getRefreshRate();
      s_res_cur_displayMode.dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
      s_res_cur_displayMode.bFullScreen = true;
      s_res_cur_displayMode.iSubtitles = s_res_cur_displayMode.iHeight;
      s_res_cur_displayMode.fPixelRatio = 1.0f;
      s_res_cur_displayMode.strMode = StringUtils::Format(
          "{}x{} @ {:.6f}{} - Full Screen", s_res_cur_displayMode.iScreenWidth,
          s_res_cur_displayMode.iScreenHeight, s_res_cur_displayMode.fRefreshRate,
          s_res_cur_displayMode.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");

      std::vector<CJNIDisplayMode> modes = display.getSupportedModes();
      for (CJNIDisplayMode& m : modes)
      {
        CLog::Log(LOGDEBUG, "CAndroidUtils: available mode: {}: {}x{}@{:f}", m.getModeId(),
                  m.getPhysicalWidth(), m.getPhysicalHeight(), m.getRefreshRate());

        RESOLUTION_INFO res;
        res.strId = std::to_string(m.getModeId());
        res.iWidth = res.iScreenWidth = landscape ? m.getPhysicalWidth() : m.getPhysicalHeight();
        res.iHeight = res.iScreenHeight = landscape ? m.getPhysicalHeight() : m.getPhysicalWidth();
        res.fRefreshRate = m.getRefreshRate();
        res.dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
        res.bFullScreen = true;
        res.iSubtitles = res.iHeight;
        res.fPixelRatio = 1.0f;
        res.strMode = StringUtils::Format("{}x{} @ {:.6f}{} - Full Screen", res.iScreenWidth,
                                          res.iScreenHeight, res.fRefreshRate,
                                          res.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");

        s_res_displayModes.push_back(res);
      }
    }
  }
}

namespace
{
std::string HdrTypeString(int type)
{
  const std::map<int, std::string> hdrTypeMap = {
      {CJNIDisplayHdrCapabilities::HDR_TYPE_HDR10, "HDR10"},
      {CJNIDisplayHdrCapabilities::HDR_TYPE_HLG, "HLG"},
      {CJNIDisplayHdrCapabilities::HDR_TYPE_HDR10_PLUS, "HDR10+"},
      {CJNIDisplayHdrCapabilities::HDR_TYPE_DOLBY_VISION, "Dolby Vision"}};

  auto hdr = hdrTypeMap.find(type);
  if (hdr != hdrTypeMap.end())
    return hdr->second;

  return "Unknown";
}
} // unnamed namespace

const std::string CAndroidUtils::SETTING_LIMITGUI = "videoscreen.limitgui";

CAndroidUtils::CAndroidUtils()
{
  m_width = m_height = 0;

  fetchDisplayModes();
  for (const RESOLUTION_INFO& res : s_res_displayModes)
  {
    if (res.iWidth > m_width || res.iHeight > m_height)
    {
      m_width = res.iWidth;
      m_height = res.iHeight;
    }
  }

  CLog::Log(LOGDEBUG, "CAndroidUtils: maximum/current resolution: {}x{}", m_width, m_height);
  int limit = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CAndroidUtils::SETTING_LIMITGUI);
  switch (limit)
  {
    case 0: // auto
      m_width = 0;
      m_height = 0;
      break;

    case 9999: // unlimited
      break;

    case 720:
      if (m_height > 720)
      {
        m_width = 1280;
        m_height = 720;
      }
      break;

    case 1080:
      if (m_height > 1080)
      {
        m_width = 1920;
        m_height = 1080;
      }
      break;
  }
  CLog::Log(LOGDEBUG, "CAndroidUtils: selected resolution: {}x{}", m_width, m_height);

  CServiceBroker::GetSettingsComponent()->GetSettings()->GetSettingsManager()->RegisterCallback(
      this, {CAndroidUtils::SETTING_LIMITGUI});

  LogDisplaySupportedHdrTypes();
}

bool CAndroidUtils::GetNativeResolution(RESOLUTION_INFO* res) const
{
  const std::shared_ptr<CNativeWindow> nativeWindow = CXBMCApp::Get().GetNativeWindow(30000);
  if (!nativeWindow)
    return false;

  if (!m_width || !m_height)
  {
    m_width = nativeWindow->GetWidth();
    m_height = nativeWindow->GetHeight();
    CLog::Log(LOGINFO, "CAndroidUtils: window resolution: {}x{}", m_width, m_height);
  }

  *res = s_res_cur_displayMode;
  res->iWidth = m_width;
  res->iHeight = m_height;
  res->iSubtitles = res->iHeight;
  res->strMode =
      StringUtils::Format("{}x{} @ {:.6f}{} - Full Screen", res->iScreenWidth, res->iScreenHeight,
                          res->fRefreshRate, res->dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");
  CLog::Log(LOGINFO, "CAndroidUtils: Current resolution: {}x{} {}", res->iWidth, res->iHeight,
            res->strMode);
  return true;
}

bool CAndroidUtils::SetNativeResolution(const RESOLUTION_INFO& res)
{
  CLog::Log(LOGINFO, "CAndroidUtils: SetNativeResolution: {}: {}x{} {}x{}@{:f}", res.strId,
            res.iWidth, res.iHeight, res.iScreenWidth, res.iScreenHeight, res.fRefreshRate);

  CXBMCApp::Get().SetDisplayMode(std::atoi(res.strId.c_str()), res.fRefreshRate);
  s_res_cur_displayMode = res;
  CXBMCApp::Get().SetBuffersGeometry(res.iWidth, res.iHeight, 0);

  return true;
}

bool CAndroidUtils::ProbeResolutions(std::vector<RESOLUTION_INFO>& resolutions)
{
  RESOLUTION_INFO cur_res;
  GetNativeResolution(&cur_res);

  CLog::Log(LOGDEBUG, "CAndroidUtils: ProbeResolutions: {}x{}", m_width, m_height);

  for (RESOLUTION_INFO res : s_res_displayModes)
  {
    if (m_width && m_height)
    {
      res.iWidth = std::min(res.iWidth, m_width);
      res.iHeight = std::min(res.iHeight, m_height);
      res.iSubtitles = res.iHeight;
    }
    resolutions.push_back(res);
  }
  return true;
}

bool CAndroidUtils::UpdateDisplayModes()
{
  fetchDisplayModes();
  return true;
}

bool CAndroidUtils::IsHDRDisplay()
{
  CJNIWindow window = CXBMCApp::getWindow();
  bool ret = false;

  if (window)
  {
    CJNIView view = window.getDecorView();
    if (view)
    {
      CJNIDisplay display = view.getDisplay();
      if (display)
        ret = display.isHdr();
    }
  }
  CLog::Log(LOGDEBUG, "CAndroidUtils: IsHDRDisplay: {}", ret ? "true" : "false");
  return ret;
}

void CAndroidUtils::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  const std::string& settingId = setting->GetId();
  /* Calibration (overscan / subtitles) are based on GUI size -> reset required */
  if (settingId == CAndroidUtils::SETTING_LIMITGUI)
    CDisplaySettings::GetInstance().ClearCalibrations();
}

std::vector<int> CAndroidUtils::GetDisplaySupportedHdrTypes()
{
  CJNIWindow window = CXBMCApp::getWindow();

  if (window)
  {
    CJNIView view = window.getDecorView();
    if (view)
    {
      CJNIDisplay display = view.getDisplay();
      if (display)
      {
        CJNIDisplayHdrCapabilities caps = display.getHdrCapabilities();
        if (caps)
          return caps.getSupportedHdrTypes();
      }
    }
  }

  return {};
}

void CAndroidUtils::LogDisplaySupportedHdrTypes()
{
  const std::vector<int> hdrTypes = GetDisplaySupportedHdrTypes();
  std::string text;

  for (const int& type : hdrTypes)
  {
    text += " " + HdrTypeString(type);
  }

  CLog::Log(LOGDEBUG, "CAndroidUtils: Display supported HDR types:{}",
            text.empty() ? " None" : text);
}

CHDRCapabilities CAndroidUtils::GetDisplayHDRCapabilities()
{
  CHDRCapabilities caps;
  const std::vector<int> types = GetDisplaySupportedHdrTypes();

  if (std::find(types.begin(), types.end(), CJNIDisplayHdrCapabilities::HDR_TYPE_HDR10) !=
      types.end())
    caps.SetHDR10();

  if (std::find(types.begin(), types.end(), CJNIDisplayHdrCapabilities::HDR_TYPE_HLG) !=
      types.end())
    caps.SetHLG();

  if (std::find(types.begin(), types.end(), CJNIDisplayHdrCapabilities::HDR_TYPE_HDR10_PLUS) !=
      types.end())
    caps.SetHDR10Plus();

  if (std::find(types.begin(), types.end(), CJNIDisplayHdrCapabilities::HDR_TYPE_DOLBY_VISION) !=
      types.end())
    caps.SetDolbyVision();

  return caps;
}

bool CAndroidUtils::SupportsMediaCodecMimeType(const std::string& mimeType)
{
  const std::vector<CJNIMediaCodecInfo> codecInfos =
      CJNIMediaCodecList(CJNIMediaCodecList::REGULAR_CODECS).getCodecInfos();

  for (const CJNIMediaCodecInfo& codec_info : codecInfos)
  {
    if (codec_info.isEncoder())
      continue;

    std::vector<std::string> types = codec_info.getSupportedTypes();
    if (std::find(types.begin(), types.end(), mimeType) != types.end())
      return true;
  }

  return false;
}

std::pair<bool, bool> CAndroidUtils::GetDolbyVisionCapabilities()
{
  const bool displaySupportsDovi = GetDisplayHDRCapabilities().SupportsDolbyVision();
  const bool mediaCodecSupportsDovi = SupportsMediaCodecMimeType("video/dolby-vision");

  CLog::Log(LOGDEBUG, "CAndroidUtils::GetDolbyVisionCapabilities Display: {}, MediaCodec: {}",
            displaySupportsDovi, mediaCodecSupportsDovi);

  return std::make_pair(displaySupportsDovi, mediaCodecSupportsDovi);
}
