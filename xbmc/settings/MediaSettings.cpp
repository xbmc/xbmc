/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
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

#include <string>

#include <limits.h>

#include "MediaSettings.h"
#include "Application.h"
#include "PlayListPlayer.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "interfaces/builtins/Builtins.h"
#include "music/MusicDatabase.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogHelper.h"
#include "profiles/ProfilesManager.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "storage/MediaManager.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "utils/Variant.h"
#include "video/VideoDatabase.h"
#include "cores/AudioEngine/Engines/ActiveAE/AudioDSPAddons/ActiveAEDSP.h"

using namespace KODI::MESSAGING;

using KODI::MESSAGING::HELPERS::DialogResponse;

CMediaSettings::CMediaSettings()
{
  m_watchedModes["files"] = WatchedModeAll;
  m_watchedModes["movies"] = WatchedModeAll;
  m_watchedModes["tvshows"] = WatchedModeAll;
  m_watchedModes["musicvideos"] = WatchedModeAll;

  m_musicPlaylistRepeat = false;
  m_musicPlaylistShuffle = false;
  m_videoPlaylistRepeat = false;
  m_videoPlaylistShuffle = false;

  m_videoStartWindowed = false;
  m_additionalSubtitleDirectoryChecked = 0;

  m_musicNeedsUpdate = 0;
  m_videoNeedsUpdate = 0;
}

CMediaSettings::~CMediaSettings()
{ }

CMediaSettings& CMediaSettings::GetInstance()
{
  static CMediaSettings sMediaSettings;
  return sMediaSettings;
}

bool CMediaSettings::Load(const TiXmlNode *settings)
{
  if (settings == NULL)
    return false;

  CSingleLock lock(m_critical);
  const TiXmlElement *pElement = settings->FirstChildElement("defaultvideosettings");
  if (pElement != NULL)
  {
    int interlaceMethod;
    bool interlaceMethodPresent = XMLUtils::GetInt(pElement, "interlacemethod", interlaceMethod, VS_INTERLACEMETHOD_AUTO, VS_INTERLACEMETHOD_MAX);

    m_defaultVideoSettings.m_InterlaceMethod = (EINTERLACEMETHOD)interlaceMethod;
    int scalingMethod;
    if (!XMLUtils::GetInt(pElement, "scalingmethod", scalingMethod, VS_SCALINGMETHOD_NEAREST, VS_SCALINGMETHOD_MAX))
      scalingMethod = (int)VS_SCALINGMETHOD_LINEAR;
    m_defaultVideoSettings.m_ScalingMethod = (ESCALINGMETHOD)scalingMethod;

    XMLUtils::GetInt(pElement, "viewmode", m_defaultVideoSettings.m_ViewMode, ViewModeNormal, ViewModeCustom);
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
    XMLUtils::GetBoolean(pElement, "outputtoallspeakers", m_defaultVideoSettings.m_OutputToAllSpeakers);
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

    m_defaultVideoSettings.m_SubtitleCached = false;
  }

  pElement = settings->FirstChildElement("defaultaudiosettings");
  if (pElement != NULL)
  {
    if (!XMLUtils::GetInt(pElement, "masterstreamtype", m_defaultAudioSettings.m_MasterStreamType))
      m_defaultAudioSettings.m_MasterStreamType = AE_DSP_ASTREAM_AUTO;
    if (!XMLUtils::GetInt(pElement, "masterstreamtypesel", m_defaultAudioSettings.m_MasterStreamTypeSel))
      m_defaultAudioSettings.m_MasterStreamTypeSel = AE_DSP_ASTREAM_AUTO;
    if (!XMLUtils::GetInt(pElement, "masterstreambase", m_defaultAudioSettings.m_MasterStreamBase))
      m_defaultAudioSettings.m_MasterStreamBase = AE_DSP_ABASE_STEREO;

    std::string strTag;
    for (int type = AE_DSP_ASTREAM_BASIC; type < AE_DSP_ASTREAM_MAX; type++)
    {
      for (int base = AE_DSP_ABASE_STEREO; base < AE_DSP_ABASE_MAX; base++)
      {
        int tmp;
        strTag = StringUtils::Format("masterprocess_%i_%i", type, base);
        if (XMLUtils::GetInt(pElement, strTag.c_str(), tmp))
          m_defaultAudioSettings.m_MasterModes[type][base] = tmp;
        else
          m_defaultAudioSettings.m_MasterModes[type][base] = 0;
      }
    }
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

    const TiXmlElement *pChild = pElement->FirstChildElement("playlist");
    if (pChild != NULL)
    {
      XMLUtils::GetBoolean(pChild, "repeat", m_videoPlaylistRepeat);
      XMLUtils::GetBoolean(pChild, "shuffle", m_videoPlaylistShuffle);
    }
    if (!XMLUtils::GetInt(pElement, "needsupdate", m_videoNeedsUpdate, 0, INT_MAX))
      m_videoNeedsUpdate = 0;
  }

  return true;
}

void CMediaSettings::OnSettingsLoaded()
{
  g_playlistPlayer.SetRepeat(PLAYLIST_MUSIC, m_musicPlaylistRepeat ? PLAYLIST::REPEAT_ALL : PLAYLIST::REPEAT_NONE);
  g_playlistPlayer.SetShuffle(PLAYLIST_MUSIC, m_musicPlaylistShuffle);
  g_playlistPlayer.SetRepeat(PLAYLIST_VIDEO, m_videoPlaylistRepeat ? PLAYLIST::REPEAT_ALL : PLAYLIST::REPEAT_NONE);
  g_playlistPlayer.SetShuffle(PLAYLIST_VIDEO, m_videoPlaylistShuffle);
}

bool CMediaSettings::Save(TiXmlNode *settings) const
{
  if (settings == NULL)
    return false;

  CSingleLock lock(m_critical);
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
  XMLUtils::SetBoolean(pNode, "outputtoallspeakers", m_defaultVideoSettings.m_OutputToAllSpeakers);
  XMLUtils::SetBoolean(pNode, "showsubtitles", m_defaultVideoSettings.m_SubtitleOn);
  XMLUtils::SetFloat(pNode, "brightness", m_defaultVideoSettings.m_Brightness);
  XMLUtils::SetFloat(pNode, "contrast", m_defaultVideoSettings.m_Contrast);
  XMLUtils::SetFloat(pNode, "gamma", m_defaultVideoSettings.m_Gamma);
  XMLUtils::SetFloat(pNode, "audiodelay", m_defaultVideoSettings.m_AudioDelay);
  XMLUtils::SetFloat(pNode, "subtitledelay", m_defaultVideoSettings.m_SubtitleDelay);
  XMLUtils::SetBoolean(pNode, "nonlinstretch", m_defaultVideoSettings.m_CustomNonLinStretch);
  XMLUtils::SetInt(pNode, "stereomode", m_defaultVideoSettings.m_StereoMode);

  // default audio settings for dsp addons
  TiXmlElement audioSettingsNode("defaultaudiosettings");
  pNode = settings->InsertEndChild(audioSettingsNode);
  if (pNode == NULL)
    return false;

  XMLUtils::SetInt(pNode, "masterstreamtype", m_defaultAudioSettings.m_MasterStreamType);
  XMLUtils::SetInt(pNode, "masterstreamtypesel", m_defaultAudioSettings.m_MasterStreamTypeSel);
  XMLUtils::SetInt(pNode, "masterstreambase", m_defaultAudioSettings.m_MasterStreamBase);

  std::string strTag;
  for (int type = AE_DSP_ASTREAM_BASIC; type < AE_DSP_ASTREAM_MAX; type++)
  {
    for (int base = AE_DSP_ABASE_STEREO; base < AE_DSP_ABASE_MAX; base++)
    {
      strTag = StringUtils::Format("masterprocess_%i_%i", type, base);
      XMLUtils::SetInt(pNode, strTag.c_str(), m_defaultAudioSettings.m_MasterModes[type][base]);
    }
  }

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

  TiXmlElement videoPlaylistNode("playlist");
  playlistNode = pNode->InsertEndChild(videoPlaylistNode);
  if (playlistNode == NULL)
    return false;
  XMLUtils::SetBoolean(playlistNode, "repeat", m_videoPlaylistRepeat);
  XMLUtils::SetBoolean(playlistNode, "shuffle", m_videoPlaylistShuffle);

  XMLUtils::SetInt(pNode, "needsupdate", m_videoNeedsUpdate);

  return true;
}

void CMediaSettings::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_MUSICLIBRARY_CLEANUP)
  {
    if (HELPERS::ShowYesNoDialogText(CVariant{313}, CVariant{333}) == DialogResponse::YES)
      g_application.StartMusicCleanup(true);
  }
  else if (settingId == CSettings::SETTING_MUSICLIBRARY_EXPORT)
    CBuiltins::GetInstance().Execute("exportlibrary(music)");
  else if (settingId == CSettings::SETTING_MUSICLIBRARY_IMPORT)
  {
    std::string path;
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    g_mediaManager.GetNetworkLocations(shares);
    g_mediaManager.GetRemovableDrives(shares);

    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, "musicdb.xml", g_localizeStrings.Get(651) , path))
    {
      CMusicDatabase musicdatabase;
      musicdatabase.Open();
      musicdatabase.ImportFromXML(path);
      musicdatabase.Close();
    }
  }
  else if (settingId == CSettings::SETTING_VIDEOLIBRARY_CLEANUP)
  {
    if (HELPERS::ShowYesNoDialogText(CVariant{313}, CVariant{333}) == DialogResponse::YES)
      g_application.StartVideoCleanup(true);
  }
  else if (settingId == CSettings::SETTING_VIDEOLIBRARY_EXPORT)
    CBuiltins::GetInstance().Execute("exportlibrary(video)");
  else if (settingId == CSettings::SETTING_VIDEOLIBRARY_IMPORT)
  {
    std::string path;
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    g_mediaManager.GetNetworkLocations(shares);
    g_mediaManager.GetRemovableDrives(shares);

    if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(651) , path))
    {
      CVideoDatabase videodatabase;
      videodatabase.Open();
      videodatabase.ImportFromXML(path);
      videodatabase.Close();
    }
  }
}

int CMediaSettings::GetWatchedMode(const std::string &content) const
{
  CSingleLock lock(m_critical);
  WatchedModes::const_iterator it = m_watchedModes.find(GetWatchedContent(content));
  if (it != m_watchedModes.end())
    return it->second;

  return WatchedModeAll;
}

void CMediaSettings::SetWatchedMode(const std::string &content, WatchedMode mode)
{
  CSingleLock lock(m_critical);
  WatchedModes::iterator it = m_watchedModes.find(GetWatchedContent(content));
  if (it != m_watchedModes.end())
    it->second = mode;
}

void CMediaSettings::CycleWatchedMode(const std::string &content)
{
  CSingleLock lock(m_critical);
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
