/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DisplaySettings.h"

#include <cstdlib>
#include <float.h>
#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "ServiceBroker.h"
#include "cores/VideoPlayer/VideoRenderers/ColorManager.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "windowing/GraphicContext.h"
#include "guilib/GUIComponent.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/StereoscopicsManager.h"
#include "messaging/helpers/DialogHelper.h"
#include "settings/AdvancedSettings.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "storage/MediaManager.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"
#include "rendering/RenderSystem.h"
#include "windowing/WinSystem.h"

#if defined(HAVE_X11)
#include "windowing/X11/WinSystemX11.h"
#elif defined(TARGET_DARWIN_OSX)
#include "windowing/osx/WinSystemOSX.h"
#elif defined(TARGET_DARWIN_IOS)
#include "windowing/ios/WinSystemIOS.h"
#elif defined(TARGET_DARWIN_TVOS)
#include "windowing/tvos/WinSystemTVOS.h"
#elif defined(HAVE_WAYLAND)
#include "windowing/wayland/WinSystemWayland.h"
#elif defined(TARGET_WINDOWS_DESKTOP)
#include "windowing/windows/WinSystemWin32DX.h"
#endif

using namespace KODI::MESSAGING;

using KODI::MESSAGING::HELPERS::DialogResponse;

// 0.1 second increments
#define MAX_REFRESH_CHANGE_DELAY 200

static RESOLUTION_INFO EmptyResolution;
static RESOLUTION_INFO EmptyModifiableResolution;

float square_error(float x, float y)
{
  float yonx = (x > 0) ? y / x : 0;
  float xony = (y > 0) ? x / y : 0;
  return std::max(yonx, xony);
}

static std::string ModeFlagsToString(unsigned int flags, bool identifier)
{
  std::string res;
  if(flags & D3DPRESENTFLAG_INTERLACED)
    res += "i";
  else
    res += "p";

  if(!identifier)
    res += " ";

  if(flags & D3DPRESENTFLAG_MODE3DSBS)
    res += "sbs";
  else if(flags & D3DPRESENTFLAG_MODE3DTB)
    res += "tab";
  else if(identifier)
    res += "std";
  return res;
}

CDisplaySettings::CDisplaySettings()
{
  m_resolutions.insert(m_resolutions.begin(), RES_CUSTOM, RESOLUTION_INFO());

  m_zoomAmount = 1.0f;
  m_pixelRatio = 1.0f;
  m_verticalShift = 0.0f;
  m_nonLinearStretched = false;
  m_resolutionChangeAborted = false;
}

CDisplaySettings::~CDisplaySettings() = default;

CDisplaySettings& CDisplaySettings::GetInstance()
{
  static CDisplaySettings sDisplaySettings;
  return sDisplaySettings;
}

bool CDisplaySettings::Load(const TiXmlNode *settings)
{
  CSingleLock lock(m_critical);
  m_calibrations.clear();

  if (settings == NULL)
    return false;

  const TiXmlElement *pElement = settings->FirstChildElement("resolutions");
  if (!pElement)
  {
    CLog::Log(LOGERROR, "CDisplaySettings: settings file doesn't contain <resolutions>");
    return false;
  }

  const TiXmlElement *pResolution = pElement->FirstChildElement("resolution");
  while (pResolution)
  {
    // get the data for this calibration
    RESOLUTION_INFO cal;

    XMLUtils::GetString(pResolution, "description", cal.strMode);
    XMLUtils::GetInt(pResolution, "subtitles", cal.iSubtitles);
    XMLUtils::GetFloat(pResolution, "pixelratio", cal.fPixelRatio);
#ifdef HAVE_X11
    XMLUtils::GetFloat(pResolution, "refreshrate", cal.fRefreshRate);
    XMLUtils::GetString(pResolution, "output", cal.strOutput);
    XMLUtils::GetString(pResolution, "xrandrid", cal.strId);
#endif

    const TiXmlElement *pOverscan = pResolution->FirstChildElement("overscan");
    if (pOverscan)
    {
      XMLUtils::GetInt(pOverscan, "left", cal.Overscan.left);
      XMLUtils::GetInt(pOverscan, "top", cal.Overscan.top);
      XMLUtils::GetInt(pOverscan, "right", cal.Overscan.right);
      XMLUtils::GetInt(pOverscan, "bottom", cal.Overscan.bottom);
    }

    // mark calibration as not updated
    // we must not delete those, resolution just might not be available
    cal.iWidth = cal.iHeight = 0;

    // store calibration, avoid adding duplicates
    bool found = false;
    for (ResolutionInfos::const_iterator  it = m_calibrations.begin(); it != m_calibrations.end(); ++it)
    {
      if (StringUtils::EqualsNoCase(it->strMode, cal.strMode))
      {
        found = true;
        break;
      }
    }
    if (!found)
      m_calibrations.push_back(cal);

    // iterate around
    pResolution = pResolution->NextSiblingElement("resolution");
  }

  ApplyCalibrations();
  return true;
}

bool CDisplaySettings::Save(TiXmlNode *settings) const
{
  if (settings == NULL)
    return false;

  CSingleLock lock(m_critical);
  TiXmlElement xmlRootElement("resolutions");
  TiXmlNode *pRoot = settings->InsertEndChild(xmlRootElement);
  if (pRoot == NULL)
    return false;

  // save calibrations
  for (ResolutionInfos::const_iterator it = m_calibrations.begin(); it != m_calibrations.end(); ++it)
  {
    // Write the resolution tag
    TiXmlElement resElement("resolution");
    TiXmlNode *pNode = pRoot->InsertEndChild(resElement);
    if (pNode == NULL)
      return false;

    // Now write each of the pieces of information we need...
    XMLUtils::SetString(pNode, "description", it->strMode);
    XMLUtils::SetInt(pNode, "subtitles", it->iSubtitles);
    XMLUtils::SetFloat(pNode, "pixelratio", it->fPixelRatio);
#ifdef HAVE_X11
    XMLUtils::SetFloat(pNode, "refreshrate", it->fRefreshRate);
    XMLUtils::SetString(pNode, "output", it->strOutput);
    XMLUtils::SetString(pNode, "xrandrid", it->strId);
#endif

    // create the overscan child
    TiXmlElement overscanElement("overscan");
    TiXmlNode *pOverscanNode = pNode->InsertEndChild(overscanElement);
    if (pOverscanNode == NULL)
      return false;

    XMLUtils::SetInt(pOverscanNode, "left", it->Overscan.left);
    XMLUtils::SetInt(pOverscanNode, "top", it->Overscan.top);
    XMLUtils::SetInt(pOverscanNode, "right", it->Overscan.right);
    XMLUtils::SetInt(pOverscanNode, "bottom", it->Overscan.bottom);
  }

  return true;
}

void CDisplaySettings::Clear()
{
  CSingleLock lock(m_critical);
  m_calibrations.clear();
  m_resolutions.clear();

  m_zoomAmount = 1.0f;
  m_pixelRatio = 1.0f;
  m_verticalShift = 0.0f;
  m_nonLinearStretched = false;
}

void CDisplaySettings::OnSettingAction(std::shared_ptr<const CSetting> setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == "videoscreen.cms3dlut")
  {
    std::string path = std::static_pointer_cast<const CSettingString>(setting)->GetValue();
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, ".3dlut", g_localizeStrings.Get(36580), path))
    {
      std::static_pointer_cast<CSettingString>(std::const_pointer_cast<CSetting>(setting))->SetValue(path);
    }
  }
  else if (settingId == "videoscreen.displayprofile")
  {
    std::string path = std::static_pointer_cast<const CSettingString>(setting)->GetValue();
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, ".icc|.icm", g_localizeStrings.Get(36581), path))
    {
      std::static_pointer_cast<CSettingString>(std::const_pointer_cast<CSetting>(setting))->SetValue(path);
    }
  }
}

bool CDisplaySettings::OnSettingChanging(std::shared_ptr<const CSetting> setting)
{
  if (setting == NULL)
    return false;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_VIDEOSCREEN_RESOLUTION ||
      settingId == CSettings::SETTING_VIDEOSCREEN_SCREEN)
  {
    RESOLUTION newRes = RES_DESKTOP;
    if (settingId == CSettings::SETTING_VIDEOSCREEN_RESOLUTION)
      newRes = (RESOLUTION)std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    else if (settingId == CSettings::SETTING_VIDEOSCREEN_SCREEN)
    {
      int screen = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();

      // if triggered by a change of screenmode, screen may not have changed
      if (screen == GetCurrentDisplayMode())
        return true;

      // get desktop resolution for screen
      newRes = GetResolutionForScreen();
    }

    std::string screenmode = GetStringFromResolution(newRes);
    if (!CServiceBroker::GetSettingsComponent()->GetSettings()->SetString(CSettings::SETTING_VIDEOSCREEN_SCREENMODE, screenmode))
      return false;
  }

  if (settingId == CSettings::SETTING_VIDEOSCREEN_SCREENMODE)
  {
    RESOLUTION oldRes = GetCurrentResolution();
    RESOLUTION newRes = GetResolutionFromString(std::static_pointer_cast<const CSettingString>(setting)->GetValue());

    SetCurrentResolution(newRes, false);
    CServiceBroker::GetWinSystem()->GetGfxContext().SetVideoResolution(newRes, false);

    // check if the old or the new resolution was/is windowed
    // in which case we don't show any prompt to the user
    if (oldRes != RES_WINDOW && newRes != RES_WINDOW && oldRes != newRes)
    {
      if (!m_resolutionChangeAborted)
      {
        if (HELPERS::ShowYesNoDialogText(CVariant{13110}, CVariant{13111}, CVariant{""}, CVariant{""}, 15000) !=
          DialogResponse::YES)
        {
          m_resolutionChangeAborted = true;
          return false;
        }
      }
      else
        m_resolutionChangeAborted = false;
    }
  }
  else if (settingId == CSettings::SETTING_VIDEOSCREEN_MONITOR)
  {
    CServiceBroker::GetWinSystem()->UpdateResolutions();
    RESOLUTION newRes = GetResolutionForScreen();

    SetCurrentResolution(newRes, false);
    CServiceBroker::GetWinSystem()->GetGfxContext().SetVideoResolution(newRes, true);

    if (!m_resolutionChangeAborted)
    {
      if (HELPERS::ShowYesNoDialogText(CVariant{13110}, CVariant{13111}, CVariant{""}, CVariant{""}, 10000) !=
        DialogResponse::YES)
      {
        m_resolutionChangeAborted = true;
        return false;
      }
    }
    else
      m_resolutionChangeAborted = false;

    return true;
  }
#if defined(HAVE_X11) || defined(TARGET_WINDOWS_DESKTOP)
  else if (settingId == CSettings::SETTING_VIDEOSCREEN_BLANKDISPLAYS)
  {
    auto winSystem = CServiceBroker::GetWinSystem();
#if defined(HAVE_X11)
    winSystem->UpdateResolutions();
#elif defined(TARGET_WINDOWS_DESKTOP)
    CGraphicContext& gfxContext = winSystem->GetGfxContext();
    gfxContext.SetVideoResolution(gfxContext.GetVideoResolution(), true);
#endif
  }
#endif

  return true;
}

bool CDisplaySettings::OnSettingUpdate(std::shared_ptr<CSetting> setting, const char *oldSettingId, const TiXmlNode *oldSettingNode)
{
  if (setting == NULL)
    return false;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_VIDEOSCREEN_SCREENMODE)
  {
    std::shared_ptr<CSettingString> screenmodeSetting = std::static_pointer_cast<CSettingString>(setting);
    std::string screenmode = screenmodeSetting->GetValue();
    // in Eden there was no character ("i" or "p") indicating interlaced/progressive
    // at the end so we just add a "p" and assume progressive
    // no 3d mode existed before, so just assume std modes
    if (screenmode.size() == 20)
      return screenmodeSetting->SetValue(screenmode + "pstd");
    if (screenmode.size() == 21)
      return screenmodeSetting->SetValue(screenmode + "std");
  }
  else if (settingId == CSettings::SETTING_VIDEOSCREEN_PREFEREDSTEREOSCOPICMODE)
  {
    std::shared_ptr<CSettingInt> stereomodeSetting = std::static_pointer_cast<CSettingInt>(setting);
    const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    STEREOSCOPIC_PLAYBACK_MODE playbackMode = (STEREOSCOPIC_PLAYBACK_MODE) settings->GetInt(CSettings::SETTING_VIDEOPLAYER_STEREOSCOPICPLAYBACKMODE);
    if (stereomodeSetting->GetValue() == RENDER_STEREO_MODE_OFF)
    {
      // if preferred playback mode was OFF, update playback mode to ignore
      if (playbackMode == STEREOSCOPIC_PLAYBACK_MODE_PREFERRED)
        settings->SetInt(CSettings::SETTING_VIDEOPLAYER_STEREOSCOPICPLAYBACKMODE, STEREOSCOPIC_PLAYBACK_MODE_IGNORE);
      return stereomodeSetting->SetValue(RENDER_STEREO_MODE_AUTO);
    }
    else if (stereomodeSetting->GetValue() == RENDER_STEREO_MODE_MONO)
    {
      // if preferred playback mode was MONO, update playback mode
      if (playbackMode == STEREOSCOPIC_PLAYBACK_MODE_PREFERRED)
        settings->SetInt(CSettings::SETTING_VIDEOPLAYER_STEREOSCOPICPLAYBACKMODE, STEREOSCOPIC_PLAYBACK_MODE_MONO);
      return stereomodeSetting->SetValue(RENDER_STEREO_MODE_AUTO);
    }
  }

  return false;
}

void CDisplaySettings::SetMonitor(std::string monitor)
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const std::string curMonitor = settings->GetString(CSettings::SETTING_VIDEOSCREEN_MONITOR);
  if (curMonitor != monitor)
  {
    m_resolutionChangeAborted = true;
    settings->SetString(CSettings::SETTING_VIDEOSCREEN_MONITOR, monitor);
  }
}

void CDisplaySettings::SetCurrentResolution(RESOLUTION resolution, bool save /* = false */)
{
  if (resolution == RES_WINDOW && !CServiceBroker::GetWinSystem()->CanDoWindowed())
    resolution = RES_DESKTOP;

  if (save)
  {
    // Save videoscreen.screenmode setting
    std::string mode = GetStringFromResolution(resolution);
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetString(CSettings::SETTING_VIDEOSCREEN_SCREENMODE, mode.c_str());

    // Check if videoscreen.screen setting also needs to be saved
    // e.g. if ToggleFullscreen is called
    int currentDisplayMode = GetCurrentDisplayMode();
    int currentDisplayModeSetting = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOSCREEN_SCREEN);
    if (currentDisplayMode != currentDisplayModeSetting)
    {
      CServiceBroker::GetSettingsComponent()->GetSettings()->SetInt(CSettings::SETTING_VIDEOSCREEN_SCREEN, currentDisplayMode);
    }
  }
  else if (resolution != m_currentResolution)
  {
    m_currentResolution = resolution;
    SetChanged();
  }
}

RESOLUTION CDisplaySettings::GetDisplayResolution() const
{
  return GetResolutionFromString(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_VIDEOSCREEN_SCREENMODE));
}

const RESOLUTION_INFO& CDisplaySettings::GetResolutionInfo(size_t index) const
{
  CSingleLock lock(m_critical);
  if (index >= m_resolutions.size())
    return EmptyResolution;

  return m_resolutions[index];
}

const RESOLUTION_INFO& CDisplaySettings::GetResolutionInfo(RESOLUTION resolution) const
{
  if (resolution <= RES_INVALID)
    return EmptyResolution;

  return GetResolutionInfo((size_t)resolution);
}

RESOLUTION_INFO& CDisplaySettings::GetResolutionInfo(size_t index)
{
  CSingleLock lock(m_critical);
  if (index >= m_resolutions.size())
  {
    EmptyModifiableResolution = RESOLUTION_INFO();
    return EmptyModifiableResolution;
  }

  return m_resolutions[index];
}

RESOLUTION_INFO& CDisplaySettings::GetResolutionInfo(RESOLUTION resolution)
{
  if (resolution <= RES_INVALID)
  {
    EmptyModifiableResolution = RESOLUTION_INFO();
    return EmptyModifiableResolution;
  }

  return GetResolutionInfo((size_t)resolution);
}

void CDisplaySettings::AddResolutionInfo(const RESOLUTION_INFO &resolution)
{
  CSingleLock lock(m_critical);
  RESOLUTION_INFO res(resolution);

  if((res.dwFlags & D3DPRESENTFLAG_MODE3DTB) == 0)
  {
    /* add corrections for some special case modes frame packing modes */

    if(res.iScreenWidth  == 1920
    && res.iScreenHeight == 2205)
    {
      res.iBlanking = 45;
      res.dwFlags  |= D3DPRESENTFLAG_MODE3DTB;
    }

    if(res.iScreenWidth  == 1280
    && res.iScreenHeight == 1470)
    {
      res.iBlanking = 30;
      res.dwFlags  |= D3DPRESENTFLAG_MODE3DTB;
    }
  }
  m_resolutions.push_back(res);
}

void CDisplaySettings::ApplyCalibrations()
{
  CSingleLock lock(m_critical);
  // apply all calibrations to the resolutions
  for (ResolutionInfos::const_iterator itCal = m_calibrations.begin(); itCal != m_calibrations.end(); ++itCal)
  {
    // find resolutions
    for (size_t res = RES_DESKTOP; res < m_resolutions.size(); ++res)
    {
      if (StringUtils::EqualsNoCase(itCal->strMode, m_resolutions[res].strMode))
      {
        // overscan
        m_resolutions[res].Overscan.left = itCal->Overscan.left;
        if (m_resolutions[res].Overscan.left < -m_resolutions[res].iWidth/4)
          m_resolutions[res].Overscan.left = -m_resolutions[res].iWidth/4;
        if (m_resolutions[res].Overscan.left > m_resolutions[res].iWidth/4)
          m_resolutions[res].Overscan.left = m_resolutions[res].iWidth/4;

        m_resolutions[res].Overscan.top = itCal->Overscan.top;
        if (m_resolutions[res].Overscan.top < -m_resolutions[res].iHeight/4)
          m_resolutions[res].Overscan.top = -m_resolutions[res].iHeight/4;
        if (m_resolutions[res].Overscan.top > m_resolutions[res].iHeight/4)
          m_resolutions[res].Overscan.top = m_resolutions[res].iHeight/4;

        m_resolutions[res].Overscan.right = itCal->Overscan.right;
        if (m_resolutions[res].Overscan.right < m_resolutions[res].iWidth / 2)
          m_resolutions[res].Overscan.right = m_resolutions[res].iWidth / 2;
        if (m_resolutions[res].Overscan.right > m_resolutions[res].iWidth * 3/2)
          m_resolutions[res].Overscan.right = m_resolutions[res].iWidth *3/2;

        m_resolutions[res].Overscan.bottom = itCal->Overscan.bottom;
        if (m_resolutions[res].Overscan.bottom < m_resolutions[res].iHeight / 2)
          m_resolutions[res].Overscan.bottom = m_resolutions[res].iHeight / 2;
        if (m_resolutions[res].Overscan.bottom > m_resolutions[res].iHeight * 3/2)
          m_resolutions[res].Overscan.bottom = m_resolutions[res].iHeight * 3/2;

        m_resolutions[res].iSubtitles = itCal->iSubtitles;
        if (m_resolutions[res].iSubtitles < m_resolutions[res].iHeight / 2)
          m_resolutions[res].iSubtitles = m_resolutions[res].iHeight / 2;
        if (m_resolutions[res].iSubtitles > m_resolutions[res].iHeight* 5/4)
          m_resolutions[res].iSubtitles = m_resolutions[res].iHeight* 5/4;

        m_resolutions[res].fPixelRatio = itCal->fPixelRatio;
        if (m_resolutions[res].fPixelRatio < 0.5f)
          m_resolutions[res].fPixelRatio = 0.5f;
        if (m_resolutions[res].fPixelRatio > 2.0f)
          m_resolutions[res].fPixelRatio = 2.0f;
        break;
      }
    }
  }
}

void CDisplaySettings::UpdateCalibrations()
{
  CSingleLock lock(m_critical);

  if (m_resolutions.size() <= RES_DESKTOP)
    return;

  // Add new (unique) resolutions
  for (ResolutionInfos::const_iterator res(m_resolutions.cbegin() + RES_CUSTOM); res != m_resolutions.cend(); ++res)
    if (std::find_if(m_calibrations.cbegin(), m_calibrations.cend(),
      [&](const RESOLUTION_INFO& info) { return StringUtils::EqualsNoCase(res->strMode, info.strMode); }) == m_calibrations.cend())
        m_calibrations.push_back(*res);

  for (auto &cal : m_calibrations)
  {
    ResolutionInfos::const_iterator res(std::find_if(m_resolutions.cbegin() + RES_DESKTOP, m_resolutions.cend(),
    [&](const RESOLUTION_INFO& info) { return StringUtils::EqualsNoCase(cal.strMode, info.strMode); }));

    if (res != m_resolutions.cend())
    {
      //! @todo erase calibrations with default values
      cal = *res;
    }
  }
}

void CDisplaySettings::ClearCalibrations()
{
  CSingleLock lock(m_critical);
  m_calibrations.clear();
}

DisplayMode CDisplaySettings::GetCurrentDisplayMode() const
{
  if (GetCurrentResolution() == RES_WINDOW)
    return DM_WINDOWED;

  return DM_FULLSCREEN;
}

RESOLUTION CDisplaySettings::FindBestMatchingResolution(const std::map<RESOLUTION, RESOLUTION_INFO> &resolutionInfos, int width, int height, float refreshrate, unsigned flags)
{
  // find the closest match to these in our res vector.  If we have the screen, we score the res
  RESOLUTION bestRes = RES_DESKTOP;
  float bestScore = FLT_MAX;
  flags &= D3DPRESENTFLAG_MODEMASK;

  for (std::map<RESOLUTION, RESOLUTION_INFO>::const_iterator it = resolutionInfos.begin(); it != resolutionInfos.end(); ++it)
  {
    const RESOLUTION_INFO &info = it->second;

    if ((info.dwFlags & D3DPRESENTFLAG_MODEMASK) != flags)
      continue;

    float score = 10 * (square_error((float)info.iScreenWidth, (float)width) +
                  square_error((float)info.iScreenHeight, (float)height) +
                  square_error(info.fRefreshRate, refreshrate));
    if (score < bestScore)
    {
      bestScore = score;
      bestRes = it->first;
    }
  }

  return bestRes;
}

RESOLUTION CDisplaySettings::GetResolutionFromString(const std::string &strResolution)
{

  if (strResolution == "DESKTOP")
    return RES_DESKTOP;
  else if (strResolution == "WINDOW")
    return RES_WINDOW;
  else if (strResolution.size() >= 20)
  {
    // format: WWWWWHHHHHRRR.RRRRRP333, where W = width, H = height, R = refresh, P = interlace, 3 = stereo mode
    int width = std::strtol(StringUtils::Mid(strResolution, 0,5).c_str(), NULL, 10);
    int height = std::strtol(StringUtils::Mid(strResolution, 5,5).c_str(), NULL, 10);
    float refresh = (float)std::strtod(StringUtils::Mid(strResolution, 10,9).c_str(), NULL);
    unsigned flags = 0;

    // look for 'i' and treat everything else as progressive,
    if(StringUtils::Mid(strResolution, 19,1) == "i")
      flags |= D3DPRESENTFLAG_INTERLACED;

    if(StringUtils::Mid(strResolution, 20,3) == "sbs")
      flags |= D3DPRESENTFLAG_MODE3DSBS;
    else if(StringUtils::Mid(strResolution, 20,3) == "tab")
      flags |= D3DPRESENTFLAG_MODE3DTB;

    std::map<RESOLUTION, RESOLUTION_INFO> resolutionInfos;
    for (size_t resolution = RES_DESKTOP; resolution < CDisplaySettings::GetInstance().ResolutionInfoSize(); resolution++)
      resolutionInfos.insert(std::make_pair((RESOLUTION)resolution, CDisplaySettings::GetInstance().GetResolutionInfo(resolution)));

    return FindBestMatchingResolution(resolutionInfos, width, height, refresh, flags);
  }

  return RES_DESKTOP;
}

std::string CDisplaySettings::GetStringFromResolution(RESOLUTION resolution, float refreshrate /* = 0.0f */)
{
  if (resolution == RES_WINDOW)
    return "WINDOW";

  if (resolution >= RES_DESKTOP && resolution < (RESOLUTION)CDisplaySettings::GetInstance().ResolutionInfoSize())
  {
    const RESOLUTION_INFO &info = CDisplaySettings::GetInstance().GetResolutionInfo(resolution);
    // also handle RES_DESKTOP resolutions with non-default refresh rates
    if (resolution != RES_DESKTOP || (refreshrate > 0.0f && refreshrate != info.fRefreshRate))
    {
      return StringUtils::Format("%05i%05i%09.5f%s",
                                 info.iScreenWidth, info.iScreenHeight,
                                 refreshrate > 0.0f ? refreshrate : info.fRefreshRate, ModeFlagsToString(info.dwFlags, true).c_str());
    }
  }

  return "DESKTOP";
}

RESOLUTION CDisplaySettings::GetResolutionForScreen()
{
  DisplayMode mode = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOSCREEN_SCREEN);
  if (mode == DM_WINDOWED)
    return RES_WINDOW;

  return RES_DESKTOP;
}

static inline bool ModeSort(StringSettingOption i,StringSettingOption j)
{
  return (i.value > j.value);
}

void CDisplaySettings::SettingOptionsModesFiller(std::shared_ptr<const CSetting> setting, std::vector<StringSettingOption> &list, std::string &current, void *data)
{
  RESOLUTION res = CDisplaySettings::GetInstance().GetDisplayResolution();
  RESOLUTION_INFO info = CDisplaySettings::GetInstance().GetResolutionInfo(res);

  for (auto index = (unsigned int)RES_CUSTOM; index < CDisplaySettings::GetInstance().ResolutionInfoSize(); ++index)
  {
    const auto mode = CDisplaySettings::GetInstance().GetResolutionInfo(index);

    if (mode.dwFlags ^ D3DPRESENTFLAG_INTERLACED)
    {
      auto setting = GetStringFromResolution((RESOLUTION)index, mode.fRefreshRate);

      list.push_back(StringSettingOption(
        StringUtils::Format("%dx%d%s %0.2fHz", mode.iScreenWidth, mode.iScreenHeight,
                            ModeFlagsToString(mode.dwFlags, false).c_str(),
                            mode.fRefreshRate),
                            setting));
    }
  }

  std::sort(list.begin(), list.end(), ModeSort);
}

void CDisplaySettings::SettingOptionsRefreshChangeDelaysFiller(SettingConstPtr setting, std::vector<IntegerSettingOption> &list, int &current, void *data)
{
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(13551), 0));

  for (int i = 1; i <= MAX_REFRESH_CHANGE_DELAY; i++)
    list.push_back(IntegerSettingOption(StringUtils::Format(g_localizeStrings.Get(13553).c_str(), (double)i / 10.0), i));
}

void CDisplaySettings::SettingOptionsRefreshRatesFiller(SettingConstPtr setting, std::vector<StringSettingOption> &list, std::string &current, void *data)
{
  // get the proper resolution
  RESOLUTION res = CDisplaySettings::GetInstance().GetDisplayResolution();
  if (res < RES_WINDOW)
    return;

  // only add "Windowed" if in windowed mode
  if (res == RES_WINDOW)
  {
    current = "WINDOW";
    list.push_back(StringSettingOption(g_localizeStrings.Get(242), current));
    return;
  }

  RESOLUTION_INFO resInfo = CDisplaySettings::GetInstance().GetResolutionInfo(res);
  // The only meaningful parts of res here are iScreenWidth, iScreenHeight
  std::vector<REFRESHRATE> refreshrates = CServiceBroker::GetWinSystem()->RefreshRates(resInfo.iScreenWidth, resInfo.iScreenHeight, resInfo.dwFlags);

  bool match = false;
  for (std::vector<REFRESHRATE>::const_iterator refreshrate = refreshrates.begin(); refreshrate != refreshrates.end(); ++refreshrate)
  {
    std::string screenmode = GetStringFromResolution((RESOLUTION)refreshrate->ResInfo_Index, refreshrate->RefreshRate);
    if (!match && StringUtils::EqualsNoCase(std::static_pointer_cast<const CSettingString>(setting)->GetValue(), screenmode))
      match = true;
    list.push_back(StringSettingOption(StringUtils::Format("%.2f", refreshrate->RefreshRate), screenmode));
  }

  if (!match)
    current = GetStringFromResolution(res, CServiceBroker::GetWinSystem()->DefaultRefreshRate(refreshrates).RefreshRate);
}

void CDisplaySettings::SettingOptionsResolutionsFiller(SettingConstPtr setting, std::vector<IntegerSettingOption> &list, int &current, void *data)
{
  RESOLUTION res = CDisplaySettings::GetInstance().GetDisplayResolution();
  RESOLUTION_INFO info = CDisplaySettings::GetInstance().GetResolutionInfo(res);
  if (res == RES_WINDOW)
  {
    current = res;
    list.push_back(IntegerSettingOption(g_localizeStrings.Get(242), res));
  }
  else
  {
    std::map<RESOLUTION, RESOLUTION_INFO> resolutionInfos;
    std::vector<RESOLUTION_WHR> resolutions = CServiceBroker::GetWinSystem()->ScreenResolutions(info.fRefreshRate);
    for (std::vector<RESOLUTION_WHR>::const_iterator resolution = resolutions.begin(); resolution != resolutions.end(); ++resolution)
    {
      list.push_back(IntegerSettingOption(
        StringUtils::Format("%dx%d%s", resolution->width, resolution->height,
                            ModeFlagsToString(resolution->flags, false).c_str()),
                            resolution->ResInfo_Index));

      resolutionInfos.insert(std::make_pair((RESOLUTION)resolution->ResInfo_Index, CDisplaySettings::GetInstance().GetResolutionInfo(resolution->ResInfo_Index)));
    }

    current = FindBestMatchingResolution(resolutionInfos,
                                         info.iScreenWidth, info.iScreenHeight,
                                         info.fRefreshRate, info.dwFlags);
  }
}

void CDisplaySettings::SettingOptionsDispModeFiller(SettingConstPtr setting, std::vector<IntegerSettingOption> &list, int &current, void *data)
{
  // The user should only be able to disable windowed modes with the canwindowed
  // setting. When the user sets canwindowed to true but the windowing system
  // does not support windowed modes, we would just shoot ourselves in the foot
  // by offering the option.
  if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_canWindowed && CServiceBroker::GetWinSystem()->CanDoWindowed())
    list.push_back(IntegerSettingOption(g_localizeStrings.Get(242), DM_WINDOWED));

  list.push_back(IntegerSettingOption(g_localizeStrings.Get(244), DM_FULLSCREEN));
}

void CDisplaySettings::SettingOptionsStereoscopicModesFiller(SettingConstPtr setting, std::vector<IntegerSettingOption> &list, int &current, void *data)
{
  CGUIComponent *gui = CServiceBroker::GetGUI();
  if (gui != nullptr)
  {
    const CStereoscopicsManager &stereoscopicsManager = gui->GetStereoscopicsManager();

    for (int i = RENDER_STEREO_MODE_OFF; i < RENDER_STEREO_MODE_COUNT; i++)
    {
      RENDER_STEREO_MODE mode = (RENDER_STEREO_MODE) i;
      if (CServiceBroker::GetRenderSystem()->SupportsStereo(mode))
        list.push_back(IntegerSettingOption(stereoscopicsManager.GetLabelForStereoMode(mode), mode));
    }
  }
}

void CDisplaySettings::SettingOptionsPreferredStereoscopicViewModesFiller(SettingConstPtr setting, std::vector<IntegerSettingOption> &list, int &current, void *data)
{
  const CStereoscopicsManager &stereoscopicsManager = CServiceBroker::GetGUI()->GetStereoscopicsManager();

  list.push_back(IntegerSettingOption(stereoscopicsManager.GetLabelForStereoMode(RENDER_STEREO_MODE_AUTO), RENDER_STEREO_MODE_AUTO)); // option for autodetect
  // don't add "off" to the list of preferred modes as this doesn't make sense
  for (int i = RENDER_STEREO_MODE_OFF +1; i < RENDER_STEREO_MODE_COUNT; i++)
  {
    RENDER_STEREO_MODE mode = (RENDER_STEREO_MODE) i;
    // also skip "mono" mode which is no real stereoscopic mode
    if (mode != RENDER_STEREO_MODE_MONO && CServiceBroker::GetRenderSystem()->SupportsStereo(mode))
      list.push_back(IntegerSettingOption(stereoscopicsManager.GetLabelForStereoMode(mode), mode));
  }
}

void CDisplaySettings::SettingOptionsMonitorsFiller(SettingConstPtr setting, std::vector<StringSettingOption> &list, std::string &current, void *data)
{
#if defined(HAVE_X11) || defined(TARGET_DARWIN_OSX) || defined(TARGET_DARWIN_EMBEDDED)
  std::vector<std::string> monitors;

#if defined(HAVE_X11)
  CWinSystemX11 *winSystem = dynamic_cast<CWinSystemX11*>(CServiceBroker::GetWinSystem());
#elif defined(TARGET_DARWIN_OSX)
  CWinSystemOSX *winSystem = dynamic_cast<CWinSystemOSX*>(CServiceBroker::GetWinSystem());
#elif defined(TARGET_DARWIN_IOS)
  CWinSystemIOS *winSystem = dynamic_cast<CWinSystemIOS*>(CServiceBroker::GetWinSystem());
#elif defined(TARGET_DARWIN_TVOS)
  CWinSystemTVOS *winSystem = dynamic_cast<CWinSystemTVOS*>(CServiceBroker::GetWinSystem());
#endif
  winSystem->GetConnectedOutputs(&monitors);
  std::string currentMonitor = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_VIDEOSCREEN_MONITOR);
  for (unsigned int i=0; i<monitors.size(); ++i)
  {
    if(currentMonitor.compare("Default") != 0 &&
       StringUtils::EqualsNoCase(CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP).strOutput, monitors[i]))
    {
      current = monitors[i];
    }
    list.push_back(StringSettingOption(monitors[i], monitors[i]));
  }
#elif defined(HAVE_WAYLAND) || defined(TARGET_WINDOWS_DESKTOP)
  std::vector<std::string> monitors;
#if defined(HAVE_WAYLAND)
  KODI::WINDOWING::WAYLAND::CWinSystemWayland *winSystem = dynamic_cast<KODI::WINDOWING::WAYLAND::CWinSystemWayland*>(CServiceBroker::GetWinSystem());
#elif defined(TARGET_WINDOWS_DESKTOP)
  CWinSystemWin32DX *winSystem = dynamic_cast<CWinSystemWin32DX*>(CServiceBroker::GetWinSystem());
#endif
  winSystem->GetConnectedOutputs(&monitors);
  bool foundMonitor = false;
  std::string currentMonitor = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_VIDEOSCREEN_MONITOR);
  for (auto const& monitor : monitors)
  {
    if(monitor == currentMonitor)
    {
      foundMonitor = true;
    }
    list.push_back(StringSettingOption(monitor, monitor));
  }

  if (!foundMonitor && !current.empty())
  {
    // Add current value so no monitor change is triggered when entering the settings screen and
    // the preferred monitor is preserved
    list.push_back(StringSettingOption(current, current));
  }
#endif
}

void CDisplaySettings::ClearCustomResolutions()
{
  if (m_resolutions.size() > RES_CUSTOM)
  {
    std::vector<RESOLUTION_INFO>::iterator firstCustom = m_resolutions.begin()+RES_CUSTOM;
    m_resolutions.erase(firstCustom, m_resolutions.end());
  }
}

void CDisplaySettings::SettingOptionsCmsModesFiller(SettingConstPtr setting, std::vector<IntegerSettingOption> &list, int &current, void *data)
{
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(36580), CMS_MODE_3DLUT));
#ifdef HAVE_LCMS2
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(36581), CMS_MODE_PROFILE));
#endif
}

void CDisplaySettings::SettingOptionsCmsWhitepointsFiller(SettingConstPtr setting, std::vector<IntegerSettingOption> &list, int &current, void *data)
{
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(36586), CMS_WHITEPOINT_D65));
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(36587), CMS_WHITEPOINT_D93));
}

void CDisplaySettings::SettingOptionsCmsPrimariesFiller(SettingConstPtr setting, std::vector<IntegerSettingOption> &list, int &current, void *data)
{
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(36588), CMS_PRIMARIES_AUTO));
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(36589), CMS_PRIMARIES_BT709));
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(36579), CMS_PRIMARIES_BT2020));
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(36590), CMS_PRIMARIES_170M));
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(36591), CMS_PRIMARIES_BT470M));
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(36592), CMS_PRIMARIES_BT470BG));
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(36593), CMS_PRIMARIES_240M));
}

void CDisplaySettings::SettingOptionsCmsGammaModesFiller(SettingConstPtr setting, std::vector<IntegerSettingOption> &list, int &current, void *data)
{
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(36582), CMS_TRC_BT1886));
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(36583), CMS_TRC_INPUT_OFFSET));
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(36584), CMS_TRC_OUTPUT_OFFSET));
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(36585), CMS_TRC_ABSOLUTE));
}

