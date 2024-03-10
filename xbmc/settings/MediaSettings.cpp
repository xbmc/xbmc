/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MediaSettings.h"

#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "cores/RetroPlayer/RetroPlayerUtils.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/builtins/Builtins.h"
#include "messaging/helpers/DialogHelper.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "music/MusicLibraryQueue.h"
#include "settings/Settings.h"
#include "settings/dialogs/GUIDialogLibExportSettings.h"
#include "settings/lib/Setting.h"
#include "storage/MediaManager.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoLibraryQueue.h"

#include <limits.h>
#include <mutex>
#include <string>

using namespace KODI;
using namespace KODI::MESSAGING;

using KODI::MESSAGING::HELPERS::DialogResponse;

CMediaSettings::CMediaSettings()
{
  m_watchedModes["files"] = WatchedModeAll;
  m_watchedModes["movies"] = WatchedModeAll;
  m_watchedModes["tvshows"] = WatchedModeAll;
  m_watchedModes["musicvideos"] = WatchedModeAll;
  m_watchedModes["recordings"] = WatchedModeAll;

  m_musicPlaylistRepeat = false;
  m_musicPlaylistShuffle = false;
  m_videoPlaylistRepeat = false;
  m_videoPlaylistShuffle = false;

  m_mediaStartWindowed = false;
  m_additionalSubtitleDirectoryChecked = 0;

  m_musicNeedsUpdate = 0;
  m_videoNeedsUpdate = 0;
}

CMediaSettings::~CMediaSettings() = default;

CMediaSettings& CMediaSettings::GetInstance()
{
  static CMediaSettings sMediaSettings;
  return sMediaSettings;
}

bool CMediaSettings::Load(const TiXmlNode *settings)
{
  if (settings == NULL)
    return false;

  std::unique_lock<CCriticalSection> lock(m_critical);
  const TiXmlElement *pElement = settings->FirstChildElement("defaultvideosettings");
  if (pElement != NULL)
  {
    int interlaceMethod;
    XMLUtils::GetInt(pElement, "interlacemethod", interlaceMethod, VS_INTERLACEMETHOD_NONE, VS_INTERLACEMETHOD_MAX);

    m_defaultVideoSettings.m_InterlaceMethod = (EINTERLACEMETHOD)interlaceMethod;
    int scalingMethod;
    if (!XMLUtils::GetInt(pElement, "scalingmethod", scalingMethod, VS_SCALINGMETHOD_NEAREST, VS_SCALINGMETHOD_MAX))
      scalingMethod = (int)VS_SCALINGMETHOD_LINEAR;
    m_defaultVideoSettings.m_ScalingMethod = (ESCALINGMETHOD)scalingMethod;

    XMLUtils::GetInt(pElement, "viewmode", m_defaultVideoSettings.m_ViewMode, ViewModeNormal, ViewModeZoom110Width);
    if (!XMLUtils::GetFloat(pElement, "zoomamount", m_defaultVideoSettings.m_CustomZoomAmount, 0.5f, 2.0f))
      m_defaultVideoSettings.m_CustomZoomAmount = 1.0f;
    if (!XMLUtils::GetFloat(pElement, "pixelratio", m_defaultVideoSettings.m_CustomPixelRatio, 0.5f, 2.0f))
      m_defaultVideoSettings.m_CustomPixelRatio = 1.0f;
    if (!XMLUtils::GetFloat(pElement, "verticalshift", m_defaultVideoSettings.m_CustomVerticalShift, -2.0f, 2.0f))
      m_defaultVideoSettings.m_CustomVerticalShift = 0.0f;
    if (!XMLUtils::GetFloat(pElement, "volumeamplification", m_defaultVideoSettings.m_VolumeAmplification, VOLUME_DRC_MINIMUM * 0.01f, VOLUME_DRC_MAXIMUM * 0.01f))
      m_defaultVideoSettings.m_VolumeAmplification = VOLUME_DRC_MINIMUM * 0.01f;
    if (!XMLUtils::GetFloat(pElement, "noisereduction", m_defaultVideoSettings.m_NoiseReduction, 0.0f, 1.0f))
      m_defaultVideoSettings.m_NoiseReduction = 0.0f;
    XMLUtils::GetBoolean(pElement, "postprocess", m_defaultVideoSettings.m_PostProcess);
    if (!XMLUtils::GetFloat(pElement, "sharpness", m_defaultVideoSettings.m_Sharpness, -1.0f, 1.0f))
      m_defaultVideoSettings.m_Sharpness = 0.0f;
    XMLUtils::GetBoolean(pElement, "showsubtitles", m_defaultVideoSettings.m_SubtitleOn);
    if (!XMLUtils::GetFloat(pElement, "brightness", m_defaultVideoSettings.m_Brightness, 0, 100))
      m_defaultVideoSettings.m_Brightness = 50;
    if (!XMLUtils::GetFloat(pElement, "contrast", m_defaultVideoSettings.m_Contrast, 0, 100))
      m_defaultVideoSettings.m_Contrast = 50;
    if (!XMLUtils::GetFloat(pElement, "gamma", m_defaultVideoSettings.m_Gamma, 0, 100))
      m_defaultVideoSettings.m_Gamma = 20;
    if (!XMLUtils::GetFloat(pElement, "audiodelay", m_defaultVideoSettings.m_AudioDelay, -10.0f, 10.0f))
      m_defaultVideoSettings.m_AudioDelay = 0.0f;
    if (!XMLUtils::GetFloat(pElement, "subtitledelay", m_defaultVideoSettings.m_SubtitleDelay, -10.0f, 10.0f))
      m_defaultVideoSettings.m_SubtitleDelay = 0.0f;
    XMLUtils::GetBoolean(pElement, "nonlinstretch", m_defaultVideoSettings.m_CustomNonLinStretch);
    if (!XMLUtils::GetInt(pElement, "stereomode", m_defaultVideoSettings.m_StereoMode))
      m_defaultVideoSettings.m_StereoMode = 0;
    if (!XMLUtils::GetInt(pElement, "centermixlevel", m_defaultVideoSettings.m_CenterMixLevel))
      m_defaultVideoSettings.m_CenterMixLevel = 0;

    int toneMapMethod;
    if (!XMLUtils::GetInt(pElement, "tonemapmethod", toneMapMethod, VS_TONEMAPMETHOD_OFF,
                          VS_TONEMAPMETHOD_MAX))
      toneMapMethod = VS_TONEMAPMETHOD_HABLE;
    m_defaultVideoSettings.m_ToneMapMethod = static_cast<ETONEMAPMETHOD>(toneMapMethod);

    if (!XMLUtils::GetFloat(pElement, "tonemapparam", m_defaultVideoSettings.m_ToneMapParam, 0.1f, 5.0f))
      m_defaultVideoSettings.m_ToneMapParam = 1.0f;
  }

  m_defaultGameSettings.Reset();
  pElement = settings->FirstChildElement("defaultgamesettings");
  if (pElement != nullptr)
  {
    std::string videoFilter;
    if (XMLUtils::GetString(pElement, "videofilter", videoFilter))
      m_defaultGameSettings.SetVideoFilter(videoFilter);

    std::string stretchMode;
    if (XMLUtils::GetString(pElement, "stretchmode", stretchMode))
    {
      RETRO::STRETCHMODE sm = RETRO::CRetroPlayerUtils::IdentifierToStretchMode(stretchMode);
      m_defaultGameSettings.SetStretchMode(sm);
    }

    int rotation;
    if (XMLUtils::GetInt(pElement, "rotation", rotation, 0, 270) && rotation >= 0)
      m_defaultGameSettings.SetRotationDegCCW(static_cast<unsigned int>(rotation));
  }

  // mymusic settings
  pElement = settings->FirstChildElement("mymusic");
  if (pElement != NULL)
  {
    const TiXmlElement *pChild = pElement->FirstChildElement("playlist");
    if (pChild != NULL)
    {
      XMLUtils::GetBoolean(pChild, "repeat", m_musicPlaylistRepeat);
      XMLUtils::GetBoolean(pChild, "shuffle", m_musicPlaylistShuffle);
    }
    if (!XMLUtils::GetInt(pElement, "needsupdate", m_musicNeedsUpdate, 0, INT_MAX))
      m_musicNeedsUpdate = 0;
  }

  // Set music playlist player repeat and shuffle from loaded settings
  if (m_musicPlaylistRepeat)
    CServiceBroker::GetPlaylistPlayer().SetRepeat(PLAYLIST::TYPE_MUSIC, PLAYLIST::RepeatState::ALL);
  else
    CServiceBroker::GetPlaylistPlayer().SetRepeat(PLAYLIST::TYPE_MUSIC,
                                                  PLAYLIST::RepeatState::NONE);
  CServiceBroker::GetPlaylistPlayer().SetShuffle(PLAYLIST::TYPE_MUSIC, m_musicPlaylistShuffle);

  // Read the watchmode settings for the various media views
  pElement = settings->FirstChildElement("myvideos");
  if (pElement != NULL)
  {
    int tmp;
    if (XMLUtils::GetInt(pElement, "watchmodemovies", tmp, (int)WatchedModeAll, (int)WatchedModeWatched))
      m_watchedModes["movies"] = (WatchedMode)tmp;
    if (XMLUtils::GetInt(pElement, "watchmodetvshows", tmp, (int)WatchedModeAll, (int)WatchedModeWatched))
      m_watchedModes["tvshows"] = (WatchedMode)tmp;
    if (XMLUtils::GetInt(pElement, "watchmodemusicvideos", tmp, (int)WatchedModeAll, (int)WatchedModeWatched))
      m_watchedModes["musicvideos"] = (WatchedMode)tmp;
    if (XMLUtils::GetInt(pElement, "watchmoderecordings", tmp, static_cast<int>(WatchedModeAll), static_cast<int>(WatchedModeWatched)))
      m_watchedModes["recordings"] = static_cast<WatchedMode>(tmp);

    const TiXmlElement *pChild = pElement->FirstChildElement("playlist");
    if (pChild != NULL)
    {
      XMLUtils::GetBoolean(pChild, "repeat", m_videoPlaylistRepeat);
      XMLUtils::GetBoolean(pChild, "shuffle", m_videoPlaylistShuffle);
    }
    if (!XMLUtils::GetInt(pElement, "needsupdate", m_videoNeedsUpdate, 0, INT_MAX))
      m_videoNeedsUpdate = 0;
  }

  // Set video playlist player repeat and shuffle from loaded settings
  if (m_videoPlaylistRepeat)
    CServiceBroker::GetPlaylistPlayer().SetRepeat(PLAYLIST::TYPE_VIDEO, PLAYLIST::RepeatState::ALL);
  else
    CServiceBroker::GetPlaylistPlayer().SetRepeat(PLAYLIST::TYPE_VIDEO,
                                                  PLAYLIST::RepeatState::NONE);
  CServiceBroker::GetPlaylistPlayer().SetShuffle(PLAYLIST::TYPE_VIDEO, m_videoPlaylistShuffle);

  return true;
}

bool CMediaSettings::Save(TiXmlNode *settings) const
{
  if (settings == NULL)
    return false;

  std::unique_lock<CCriticalSection> lock(m_critical);
  // default video settings
  TiXmlElement videoSettingsNode("defaultvideosettings");
  TiXmlNode *pNode = settings->InsertEndChild(videoSettingsNode);
  if (pNode == NULL)
    return false;

  XMLUtils::SetInt(pNode, "interlacemethod", m_defaultVideoSettings.m_InterlaceMethod);
  XMLUtils::SetInt(pNode, "scalingmethod", m_defaultVideoSettings.m_ScalingMethod);
  XMLUtils::SetFloat(pNode, "noisereduction", m_defaultVideoSettings.m_NoiseReduction);
  XMLUtils::SetBoolean(pNode, "postprocess", m_defaultVideoSettings.m_PostProcess);
  XMLUtils::SetFloat(pNode, "sharpness", m_defaultVideoSettings.m_Sharpness);
  XMLUtils::SetInt(pNode, "viewmode", m_defaultVideoSettings.m_ViewMode);
  XMLUtils::SetFloat(pNode, "zoomamount", m_defaultVideoSettings.m_CustomZoomAmount);
  XMLUtils::SetFloat(pNode, "pixelratio", m_defaultVideoSettings.m_CustomPixelRatio);
  XMLUtils::SetFloat(pNode, "verticalshift", m_defaultVideoSettings.m_CustomVerticalShift);
  XMLUtils::SetFloat(pNode, "volumeamplification", m_defaultVideoSettings.m_VolumeAmplification);
  XMLUtils::SetBoolean(pNode, "showsubtitles", m_defaultVideoSettings.m_SubtitleOn);
  XMLUtils::SetFloat(pNode, "brightness", m_defaultVideoSettings.m_Brightness);
  XMLUtils::SetFloat(pNode, "contrast", m_defaultVideoSettings.m_Contrast);
  XMLUtils::SetFloat(pNode, "gamma", m_defaultVideoSettings.m_Gamma);
  XMLUtils::SetFloat(pNode, "audiodelay", m_defaultVideoSettings.m_AudioDelay);
  XMLUtils::SetFloat(pNode, "subtitledelay", m_defaultVideoSettings.m_SubtitleDelay);
  XMLUtils::SetBoolean(pNode, "nonlinstretch", m_defaultVideoSettings.m_CustomNonLinStretch);
  XMLUtils::SetInt(pNode, "stereomode", m_defaultVideoSettings.m_StereoMode);
  XMLUtils::SetInt(pNode, "centermixlevel", m_defaultVideoSettings.m_CenterMixLevel);
  XMLUtils::SetInt(pNode, "tonemapmethod", m_defaultVideoSettings.m_ToneMapMethod);
  XMLUtils::SetFloat(pNode, "tonemapparam", m_defaultVideoSettings.m_ToneMapParam);

  // default audio settings for dsp addons
  TiXmlElement audioSettingsNode("defaultaudiosettings");
  pNode = settings->InsertEndChild(audioSettingsNode);
  if (pNode == NULL)
    return false;

  // Default game settings
  TiXmlElement gameSettingsNode("defaultgamesettings");
  pNode = settings->InsertEndChild(gameSettingsNode);
  if (pNode == nullptr)
    return false;

  XMLUtils::SetString(pNode, "videofilter", m_defaultGameSettings.VideoFilter());
  std::string sm = RETRO::CRetroPlayerUtils::StretchModeToIdentifier(m_defaultGameSettings.StretchMode());
  XMLUtils::SetString(pNode, "stretchmode", sm);
  XMLUtils::SetInt(pNode, "rotation", m_defaultGameSettings.RotationDegCCW());

  // mymusic
  pNode = settings->FirstChild("mymusic");
  if (pNode == NULL)
  {
    TiXmlElement videosNode("mymusic");
    pNode = settings->InsertEndChild(videosNode);
    if (pNode == NULL)
      return false;
  }

  TiXmlElement musicPlaylistNode("playlist");
  TiXmlNode *playlistNode = pNode->InsertEndChild(musicPlaylistNode);
  if (playlistNode == NULL)
    return false;
  XMLUtils::SetBoolean(playlistNode, "repeat", m_musicPlaylistRepeat);
  XMLUtils::SetBoolean(playlistNode, "shuffle", m_musicPlaylistShuffle);

  XMLUtils::SetInt(pNode, "needsupdate", m_musicNeedsUpdate);

  // myvideos
  pNode = settings->FirstChild("myvideos");
  if (pNode == NULL)
  {
    TiXmlElement videosNode("myvideos");
    pNode = settings->InsertEndChild(videosNode);
    if (pNode == NULL)
      return false;
  }

  XMLUtils::SetInt(pNode, "watchmodemovies", m_watchedModes.find("movies")->second);
  XMLUtils::SetInt(pNode, "watchmodetvshows", m_watchedModes.find("tvshows")->second);
  XMLUtils::SetInt(pNode, "watchmodemusicvideos", m_watchedModes.find("musicvideos")->second);
  XMLUtils::SetInt(pNode, "watchmoderecordings", m_watchedModes.find("recordings")->second);

  TiXmlElement videoPlaylistNode("playlist");
  playlistNode = pNode->InsertEndChild(videoPlaylistNode);
  if (playlistNode == NULL)
    return false;
  XMLUtils::SetBoolean(playlistNode, "repeat", m_videoPlaylistRepeat);
  XMLUtils::SetBoolean(playlistNode, "shuffle", m_videoPlaylistShuffle);

  XMLUtils::SetInt(pNode, "needsupdate", m_videoNeedsUpdate);

  return true;
}

void CMediaSettings::OnSettingAction(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_MUSICLIBRARY_CLEANUP)
  {
    if (HELPERS::ShowYesNoDialogText(CVariant{313}, CVariant{333}) == DialogResponse::CHOICE_YES)
    {
      if (CMusicLibraryQueue::GetInstance().IsRunning())
        HELPERS::ShowOKDialogText(CVariant{700}, CVariant{703});
      else
        CMusicLibraryQueue::GetInstance().CleanLibrary(true);
    }
  }
  else if (settingId == CSettings::SETTING_MUSICLIBRARY_EXPORT)
  {
    CLibExportSettings m_musicExportSettings;
    if (CGUIDialogLibExportSettings::Show(m_musicExportSettings))
    {
      // Export music library showing progress dialog
      CMusicLibraryQueue::GetInstance().ExportLibrary(m_musicExportSettings, true);
    }
  }
  else if (settingId == CSettings::SETTING_MUSICLIBRARY_IMPORT)
  {
    std::string path;
    VECSOURCES shares;
    CServiceBroker::GetMediaManager().GetLocalDrives(shares);
    CServiceBroker::GetMediaManager().GetNetworkLocations(shares);
    CServiceBroker::GetMediaManager().GetRemovableDrives(shares);

    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, "musicdb.xml", g_localizeStrings.Get(651) , path))
    {
      // Import data to music library showing progress dialog
      CMusicLibraryQueue::GetInstance().ImportLibrary(path, true);
    }
  }
  else if (settingId == CSettings::SETTING_VIDEOLIBRARY_CLEANUP)
  {
    if (HELPERS::ShowYesNoDialogText(CVariant{313}, CVariant{333}) == DialogResponse::CHOICE_YES)
    {
      if (!CVideoLibraryQueue::GetInstance().CleanLibraryModal())
        HELPERS::ShowOKDialogText(CVariant{700}, CVariant{703});
    }
  }
  else if (settingId == CSettings::SETTING_VIDEOLIBRARY_EXPORT)
    CBuiltins::GetInstance().Execute("exportlibrary(video)");
  else if (settingId == CSettings::SETTING_VIDEOLIBRARY_IMPORT)
  {
    std::string path;
    VECSOURCES shares;
    CServiceBroker::GetMediaManager().GetLocalDrives(shares);
    CServiceBroker::GetMediaManager().GetNetworkLocations(shares);
    CServiceBroker::GetMediaManager().GetRemovableDrives(shares);

    if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(651) , path))
    {
      CVideoDatabase videodatabase;
      videodatabase.Open();
      videodatabase.ImportFromXML(path);
      videodatabase.Close();
    }
  }
}

void CMediaSettings::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == nullptr)
    return;

  if (setting->GetId() == CSettings::SETTING_VIDEOLIBRARY_SHOWUNWATCHEDPLOTS)
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::VideoLibrary, "OnRefresh");
}

int CMediaSettings::GetWatchedMode(const std::string &content) const
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  WatchedModes::const_iterator it = m_watchedModes.find(GetWatchedContent(content));
  if (it != m_watchedModes.end())
    return it->second;

  return WatchedModeAll;
}

void CMediaSettings::SetWatchedMode(const std::string &content, WatchedMode mode)
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  WatchedModes::iterator it = m_watchedModes.find(GetWatchedContent(content));
  if (it != m_watchedModes.end())
    it->second = mode;
}

void CMediaSettings::CycleWatchedMode(const std::string &content)
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  WatchedModes::iterator it = m_watchedModes.find(GetWatchedContent(content));
  if (it != m_watchedModes.end())
  {
    it->second = (WatchedMode)((int)it->second + 1);
    if (it->second > WatchedModeWatched)
      it->second = WatchedModeAll;
  }
}

std::string CMediaSettings::GetWatchedContent(const std::string &content)
{
  if (content == "seasons" || content == "episodes")
    return "tvshows";

  return content;
}
