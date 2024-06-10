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
#include "utils/XMLUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoLibraryQueue.h"

#include <mutex>
#include <string>

#include <limits.h>
#include <tinyxml2.h>

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

bool CMediaSettings::Load(const tinyxml2::XMLNode* settings)
{
  if (settings == NULL)
    return false;

  std::unique_lock<CCriticalSection> lock(m_critical);
  const auto* element = settings->FirstChildElement("defaultvideosettings");
  if (element != NULL)
  {
    int interlaceMethod;
    XMLUtils::GetInt(element, "interlacemethod", interlaceMethod, VS_INTERLACEMETHOD_NONE,
                     VS_INTERLACEMETHOD_MAX);

    m_defaultVideoSettings.m_InterlaceMethod = (EINTERLACEMETHOD)interlaceMethod;
    int scalingMethod;
    if (!XMLUtils::GetInt(element, "scalingmethod", scalingMethod, VS_SCALINGMETHOD_NEAREST,
                          VS_SCALINGMETHOD_MAX))
      scalingMethod = (int)VS_SCALINGMETHOD_LINEAR;
    m_defaultVideoSettings.m_ScalingMethod = (ESCALINGMETHOD)scalingMethod;

    XMLUtils::GetInt(element, "viewmode", m_defaultVideoSettings.m_ViewMode, ViewModeNormal,
                     ViewModeZoom110Width);
    if (!XMLUtils::GetFloat(element, "zoomamount", m_defaultVideoSettings.m_CustomZoomAmount, 0.5f,
                            2.0f))
      m_defaultVideoSettings.m_CustomZoomAmount = 1.0f;
    if (!XMLUtils::GetFloat(element, "pixelratio", m_defaultVideoSettings.m_CustomPixelRatio, 0.5f,
                            2.0f))
      m_defaultVideoSettings.m_CustomPixelRatio = 1.0f;
    if (!XMLUtils::GetFloat(element, "verticalshift", m_defaultVideoSettings.m_CustomVerticalShift,
                            -2.0f, 2.0f))
      m_defaultVideoSettings.m_CustomVerticalShift = 0.0f;
    if (!XMLUtils::GetFloat(element, "volumeamplification",
                            m_defaultVideoSettings.m_VolumeAmplification,
                            VOLUME_DRC_MINIMUM * 0.01f, VOLUME_DRC_MAXIMUM * 0.01f))
      m_defaultVideoSettings.m_VolumeAmplification = VOLUME_DRC_MINIMUM * 0.01f;
    if (!XMLUtils::GetFloat(element, "noisereduction", m_defaultVideoSettings.m_NoiseReduction,
                            0.0f, 1.0f))
      m_defaultVideoSettings.m_NoiseReduction = 0.0f;
    XMLUtils::GetBoolean(element, "postprocess", m_defaultVideoSettings.m_PostProcess);
    if (!XMLUtils::GetFloat(element, "sharpness", m_defaultVideoSettings.m_Sharpness, -1.0f, 1.0f))
      m_defaultVideoSettings.m_Sharpness = 0.0f;
    XMLUtils::GetBoolean(element, "showsubtitles", m_defaultVideoSettings.m_SubtitleOn);
    if (!XMLUtils::GetFloat(element, "brightness", m_defaultVideoSettings.m_Brightness, 0, 100))
      m_defaultVideoSettings.m_Brightness = 50;
    if (!XMLUtils::GetFloat(element, "contrast", m_defaultVideoSettings.m_Contrast, 0, 100))
      m_defaultVideoSettings.m_Contrast = 50;
    if (!XMLUtils::GetFloat(element, "gamma", m_defaultVideoSettings.m_Gamma, 0, 100))
      m_defaultVideoSettings.m_Gamma = 20;
    if (!XMLUtils::GetFloat(element, "audiodelay", m_defaultVideoSettings.m_AudioDelay, -10.0f,
                            10.0f))
      m_defaultVideoSettings.m_AudioDelay = 0.0f;
    if (!XMLUtils::GetFloat(element, "subtitledelay", m_defaultVideoSettings.m_SubtitleDelay,
                            -10.0f, 10.0f))
      m_defaultVideoSettings.m_SubtitleDelay = 0.0f;
    XMLUtils::GetBoolean(element, "nonlinstretch", m_defaultVideoSettings.m_CustomNonLinStretch);
    if (!XMLUtils::GetInt(element, "stereomode", m_defaultVideoSettings.m_StereoMode))
      m_defaultVideoSettings.m_StereoMode = 0;
    if (!XMLUtils::GetInt(element, "centermixlevel", m_defaultVideoSettings.m_CenterMixLevel))
      m_defaultVideoSettings.m_CenterMixLevel = 0;

    int toneMapMethod;
    if (!XMLUtils::GetInt(element, "tonemapmethod", toneMapMethod, VS_TONEMAPMETHOD_OFF,
                          VS_TONEMAPMETHOD_MAX))
      toneMapMethod = VS_TONEMAPMETHOD_HABLE;
    m_defaultVideoSettings.m_ToneMapMethod = static_cast<ETONEMAPMETHOD>(toneMapMethod);

    if (!XMLUtils::GetFloat(element, "tonemapparam", m_defaultVideoSettings.m_ToneMapParam, 0.1f,
                            5.0f))
      m_defaultVideoSettings.m_ToneMapParam = 1.0f;
  }

  m_defaultGameSettings.Reset();
  element = settings->FirstChildElement("defaultgamesettings");
  if (element != nullptr)
  {
    std::string videoFilter;
    if (XMLUtils::GetString(element, "videofilter", videoFilter))
      m_defaultGameSettings.SetVideoFilter(videoFilter);

    std::string stretchMode;
    if (XMLUtils::GetString(element, "stretchmode", stretchMode))
    {
      RETRO::STRETCHMODE sm = RETRO::CRetroPlayerUtils::IdentifierToStretchMode(stretchMode);
      m_defaultGameSettings.SetStretchMode(sm);
    }

    int rotation;
    if (XMLUtils::GetInt(element, "rotation", rotation, 0, 270) && rotation >= 0)
      m_defaultGameSettings.SetRotationDegCCW(static_cast<unsigned int>(rotation));
  }

  // mymusic settings
  element = settings->FirstChildElement("mymusic");
  if (element != NULL)
  {
    const auto* child = element->FirstChildElement("playlist");
    if (child != NULL)
    {
      XMLUtils::GetBoolean(child, "repeat", m_musicPlaylistRepeat);
      XMLUtils::GetBoolean(child, "shuffle", m_musicPlaylistShuffle);
    }
    if (!XMLUtils::GetInt(element, "needsupdate", m_musicNeedsUpdate, 0, INT_MAX))
      m_musicNeedsUpdate = 0;
  }

  // Set music playlist player repeat and shuffle from loaded settings
  if (m_musicPlaylistRepeat)
    CServiceBroker::GetPlaylistPlayer().SetRepeat(PLAYLIST::Id::TYPE_MUSIC,
                                                  PLAYLIST::RepeatState::ALL);
  else
    CServiceBroker::GetPlaylistPlayer().SetRepeat(PLAYLIST::Id::TYPE_MUSIC,
                                                  PLAYLIST::RepeatState::NONE);
  CServiceBroker::GetPlaylistPlayer().SetShuffle(PLAYLIST::Id::TYPE_MUSIC, m_musicPlaylistShuffle);

  // Read the watchmode settings for the various media views
  element = settings->FirstChildElement("myvideos");
  if (element != NULL)
  {
    int tmp;
    if (XMLUtils::GetInt(element, "watchmodemovies", tmp, (int)WatchedModeAll,
                         (int)WatchedModeWatched))
      m_watchedModes["movies"] = (WatchedMode)tmp;
    if (XMLUtils::GetInt(element, "watchmodetvshows", tmp, (int)WatchedModeAll,
                         (int)WatchedModeWatched))
      m_watchedModes["tvshows"] = (WatchedMode)tmp;
    if (XMLUtils::GetInt(element, "watchmodemusicvideos", tmp, (int)WatchedModeAll,
                         (int)WatchedModeWatched))
      m_watchedModes["musicvideos"] = (WatchedMode)tmp;
    if (XMLUtils::GetInt(element, "watchmoderecordings", tmp, static_cast<int>(WatchedModeAll),
                         static_cast<int>(WatchedModeWatched)))
      m_watchedModes["recordings"] = static_cast<WatchedMode>(tmp);

    const auto* child = element->FirstChildElement("playlist");
    if (child != NULL)
    {
      XMLUtils::GetBoolean(child, "repeat", m_videoPlaylistRepeat);
      XMLUtils::GetBoolean(child, "shuffle", m_videoPlaylistShuffle);
    }
    if (!XMLUtils::GetInt(element, "needsupdate", m_videoNeedsUpdate, 0, INT_MAX))
      m_videoNeedsUpdate = 0;
  }

  // Set video playlist player repeat and shuffle from loaded settings
  if (m_videoPlaylistRepeat)
    CServiceBroker::GetPlaylistPlayer().SetRepeat(PLAYLIST::Id::TYPE_VIDEO,
                                                  PLAYLIST::RepeatState::ALL);
  else
    CServiceBroker::GetPlaylistPlayer().SetRepeat(PLAYLIST::Id::TYPE_VIDEO,
                                                  PLAYLIST::RepeatState::NONE);
  CServiceBroker::GetPlaylistPlayer().SetShuffle(PLAYLIST::Id::TYPE_VIDEO, m_videoPlaylistShuffle);

  return true;
}

bool CMediaSettings::Save(tinyxml2::XMLNode* settings) const
{
  if (!settings)
    return false;

  std::unique_lock<CCriticalSection> lock(m_critical);
  // default video settings
  auto doc = settings->GetDocument();
  auto* videoSettingsNode = doc->NewElement("defaultvideosettings");
  auto* node = settings->InsertEndChild(videoSettingsNode);
  if (node == NULL)
    return false;

  XMLUtils::SetInt(node, "interlacemethod", m_defaultVideoSettings.m_InterlaceMethod);
  XMLUtils::SetInt(node, "scalingmethod", m_defaultVideoSettings.m_ScalingMethod);
  XMLUtils::SetFloat(node, "noisereduction", m_defaultVideoSettings.m_NoiseReduction);
  XMLUtils::SetBoolean(node, "postprocess", m_defaultVideoSettings.m_PostProcess);
  XMLUtils::SetFloat(node, "sharpness", m_defaultVideoSettings.m_Sharpness);
  XMLUtils::SetInt(node, "viewmode", m_defaultVideoSettings.m_ViewMode);
  XMLUtils::SetFloat(node, "zoomamount", m_defaultVideoSettings.m_CustomZoomAmount);
  XMLUtils::SetFloat(node, "pixelratio", m_defaultVideoSettings.m_CustomPixelRatio);
  XMLUtils::SetFloat(node, "verticalshift", m_defaultVideoSettings.m_CustomVerticalShift);
  XMLUtils::SetFloat(node, "volumeamplification", m_defaultVideoSettings.m_VolumeAmplification);
  XMLUtils::SetBoolean(node, "showsubtitles", m_defaultVideoSettings.m_SubtitleOn);
  XMLUtils::SetFloat(node, "brightness", m_defaultVideoSettings.m_Brightness);
  XMLUtils::SetFloat(node, "contrast", m_defaultVideoSettings.m_Contrast);
  XMLUtils::SetFloat(node, "gamma", m_defaultVideoSettings.m_Gamma);
  XMLUtils::SetFloat(node, "audiodelay", m_defaultVideoSettings.m_AudioDelay);
  XMLUtils::SetFloat(node, "subtitledelay", m_defaultVideoSettings.m_SubtitleDelay);
  XMLUtils::SetBoolean(node, "nonlinstretch", m_defaultVideoSettings.m_CustomNonLinStretch);
  XMLUtils::SetInt(node, "stereomode", m_defaultVideoSettings.m_StereoMode);
  XMLUtils::SetInt(node, "centermixlevel", m_defaultVideoSettings.m_CenterMixLevel);
  XMLUtils::SetInt(node, "tonemapmethod", m_defaultVideoSettings.m_ToneMapMethod);
  XMLUtils::SetFloat(node, "tonemapparam", m_defaultVideoSettings.m_ToneMapParam);

  // default audio settings for dsp addons
  auto* audioSettingsNode = doc->NewElement("defaultaudiosettings");
  node = settings->InsertEndChild(audioSettingsNode);
  if (!node)
    return false;

  // Default game settings
  auto* gameSettingsNode = doc->NewElement("defaultgamesettings");
  node = settings->InsertEndChild(gameSettingsNode);
  if (!node)
    return false;

  XMLUtils::SetString(node, "videofilter", m_defaultGameSettings.VideoFilter());
  std::string sm = RETRO::CRetroPlayerUtils::StretchModeToIdentifier(m_defaultGameSettings.StretchMode());
  XMLUtils::SetString(node, "stretchmode", sm);
  XMLUtils::SetInt(node, "rotation", m_defaultGameSettings.RotationDegCCW());

  // mymusic
  node = settings->FirstChildElement("mymusic");
  if (!node)
  {
    auto* videosNode = doc->NewElement("mymusic");
    node = settings->InsertEndChild(videosNode);
    if (!node)
      return false;
  }

  auto* musicPlaylistNode = doc->NewElement("playlist");
  auto* playlistNode = node->InsertEndChild(musicPlaylistNode);
  if (!playlistNode)
    return false;
  XMLUtils::SetBoolean(playlistNode, "repeat", m_musicPlaylistRepeat);
  XMLUtils::SetBoolean(playlistNode, "shuffle", m_musicPlaylistShuffle);

  XMLUtils::SetInt(node, "needsupdate", m_musicNeedsUpdate);

  // myvideos
  node = settings->FirstChildElement("myvideos");
  if (!node)
  {
    auto* videosNode = doc->NewElement("myvideos");
    node = settings->InsertEndChild(videosNode);
    if (!node)
      return false;
  }

  XMLUtils::SetInt(node, "watchmodemovies", m_watchedModes.find("movies")->second);
  XMLUtils::SetInt(node, "watchmodetvshows", m_watchedModes.find("tvshows")->second);
  XMLUtils::SetInt(node, "watchmodemusicvideos", m_watchedModes.find("musicvideos")->second);
  XMLUtils::SetInt(node, "watchmoderecordings", m_watchedModes.find("recordings")->second);

  auto* videoPlaylistNode = doc->NewElement("playlist");
  playlistNode = node->InsertEndChild(videoPlaylistNode);
  if (!playlistNode)
    return false;
  XMLUtils::SetBoolean(playlistNode, "repeat", m_videoPlaylistRepeat);
  XMLUtils::SetBoolean(playlistNode, "shuffle", m_videoPlaylistShuffle);

  XMLUtils::SetInt(node, "needsupdate", m_videoNeedsUpdate);

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
