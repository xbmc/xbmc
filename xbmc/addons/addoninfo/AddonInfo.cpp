/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonInfo.h"

#include "LangInfo.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "guilib/LocalizeStrings.h"

#include <algorithm>

namespace ADDON
{

typedef struct
{
  std::string name;
  std::string old_name;
  TYPE type;
  int pretty;
  std::string icon;
} TypeMapping;

// clang-format off
static const TypeMapping types[] =
  {{"unknown",                           "", ADDON_UNKNOWN,                 0, "" },
   {"xbmc.metadata.scraper.albums",      "", ADDON_SCRAPER_ALBUMS,      24016, "DefaultAddonAlbumInfo.png" },
   {"xbmc.metadata.scraper.artists",     "", ADDON_SCRAPER_ARTISTS,     24017, "DefaultAddonArtistInfo.png" },
   {"xbmc.metadata.scraper.movies",      "", ADDON_SCRAPER_MOVIES,      24007, "DefaultAddonMovieInfo.png" },
   {"xbmc.metadata.scraper.musicvideos", "", ADDON_SCRAPER_MUSICVIDEOS, 24015, "DefaultAddonMusicVideoInfo.png" },
   {"xbmc.metadata.scraper.tvshows",     "", ADDON_SCRAPER_TVSHOWS,     24014, "DefaultAddonTvInfo.png" },
   {"xbmc.metadata.scraper.library",     "", ADDON_SCRAPER_LIBRARY,     24083, "DefaultAddonInfoLibrary.png" },
   {"xbmc.ui.screensaver",               "", ADDON_SCREENSAVER,         24008, "DefaultAddonScreensaver.png" },
   {"xbmc.player.musicviz",              "", ADDON_VIZ,                 24010, "DefaultAddonVisualization.png" },
   {"xbmc.python.pluginsource",          "", ADDON_PLUGIN,              24005, "" },
   {"xbmc.python.script",                "", ADDON_SCRIPT,              24009, "" },
   {"xbmc.python.weather",               "", ADDON_SCRIPT_WEATHER,      24027, "DefaultAddonWeather.png" },
   {"xbmc.python.lyrics",                "", ADDON_SCRIPT_LYRICS,       24013, "DefaultAddonLyrics.png" },
   {"xbmc.python.library",               "", ADDON_SCRIPT_LIBRARY,      24081, "DefaultAddonHelper.png" },
   {"xbmc.python.module",                "", ADDON_SCRIPT_MODULE,       24082, "DefaultAddonLibrary.png" },
   {"xbmc.subtitle.module",              "", ADDON_SUBTITLE_MODULE,     24012, "DefaultAddonSubtitles.png" },
   {"kodi.context.item",                 "", ADDON_CONTEXT_ITEM,        24025, "DefaultAddonContextItem.png" },
   {"kodi.game.controller",              "", ADDON_GAME_CONTROLLER,     35050, "DefaultAddonGame.png" },
   {"xbmc.gui.skin",                     "", ADDON_SKIN,                  166, "DefaultAddonSkin.png" },
   {"xbmc.webinterface",                 "", ADDON_WEB_INTERFACE,         199, "DefaultAddonWebSkin.png" },
   {"xbmc.addon.repository",             "", ADDON_REPOSITORY,          24011, "DefaultAddonRepository.png" },
   {"kodi.pvrclient",      "xbmc.pvrclient", ADDON_PVRDLL,              24019, "DefaultAddonPVRClient.png" },
   {"kodi.gameclient",                   "", ADDON_GAMEDLL,             35049, "DefaultAddonGame.png" },
   {"kodi.peripheral",                   "", ADDON_PERIPHERALDLL,       35010, "DefaultAddonPeripheral.png" },
   {"xbmc.addon.video",                  "", ADDON_VIDEO,                1037, "DefaultAddonVideo.png" },
   {"xbmc.addon.audio",                  "", ADDON_AUDIO,                1038, "DefaultAddonMusic.png" },
   {"xbmc.addon.image",                  "", ADDON_IMAGE,                1039, "DefaultAddonPicture.png" },
   {"xbmc.addon.executable",             "", ADDON_EXECUTABLE,           1043, "DefaultAddonProgram.png" },
   {"kodi.addon.game",                   "", ADDON_GAME,                35049, "DefaultAddonGame.png" },
   {"kodi.audioencoder",                 "", ADDON_AUDIOENCODER,         200,  "DefaultAddonAudioEncoder.png" },
   {"kodi.audiodecoder",                 "", ADDON_AUDIODECODER,         201,  "DefaultAddonAudioDecoder.png" },
   {"xbmc.service",                      "", ADDON_SERVICE,             24018, "DefaultAddonService.png" },
   {"kodi.resource.images",              "", ADDON_RESOURCE_IMAGES,     24035, "DefaultAddonImages.png" },
   {"kodi.resource.language",            "", ADDON_RESOURCE_LANGUAGE,   24026, "DefaultAddonLanguage.png" },
   {"kodi.resource.uisounds",            "", ADDON_RESOURCE_UISOUNDS,   24006, "DefaultAddonUISounds.png" },
   {"kodi.resource.games",               "", ADDON_RESOURCE_GAMES,      35209, "DefaultAddonGame.png" },
   {"kodi.resource.font",                "", ADDON_RESOURCE_FONT,       13303, "DefaultAddonFont.png" },
   {"kodi.inputstream",                  "", ADDON_INPUTSTREAM,         24048, "DefaultAddonInputstream.png" },
   {"kodi.vfs",                          "", ADDON_VFS,                 39013, "DefaultAddonVfs.png" },
   {"kodi.imagedecoder",                 "", ADDON_IMAGEDECODER,        39015, "DefaultAddonImageDecoder.png" },
  };
// clang-format on

const std::string& CAddonInfo::OriginName() const
{
  if (!m_originName)
  {
    ADDON::AddonPtr origin;
    if (CServiceBroker::GetAddonMgr().GetAddon(m_origin, origin, ADDON::ADDON_UNKNOWN,
                                               ADDON::OnlyEnabled::NO))
      m_originName = std::make_unique<std::string>(origin->Name());
    else
      m_originName = std::make_unique<std::string>(); // remember we tried to fetch the name
  }
  return *m_originName;
}

/**
 * static public helper functions
 *
 */

std::string CAddonInfo::TranslateType(ADDON::TYPE type, bool pretty /*= false*/)
{
  for (const TypeMapping& map : types)
  {
    if (type == map.type)
    {
      if (pretty && map.pretty)
        return g_localizeStrings.Get(map.pretty);
      else
        return map.name;
    }
  }
  return "";
}

TYPE CAddonInfo::TranslateType(const std::string& string)
{
  for (const TypeMapping& map : types)
  {
    if (string == map.name || (!map.old_name.empty() && string == map.old_name))
      return map.type;
  }

  return ADDON_UNKNOWN;
}

std::string CAddonInfo::TranslateIconType(ADDON::TYPE type)
{
  for (const TypeMapping& map : types)
  {
    if (type == map.type)
      return map.icon;
  }
  return "";
}

TYPE CAddonInfo::TranslateSubContent(const std::string& content)
{
  if (content == "audio")
    return ADDON_AUDIO;
  else if (content == "image")
    return ADDON_IMAGE;
  else if (content == "executable")
    return ADDON_EXECUTABLE;
  else if (content == "video")
    return ADDON_VIDEO;
  else if (content == "game")
    return ADDON_GAME;
  else
    return ADDON_UNKNOWN;
}

CAddonInfo::CAddonInfo(std::string id, TYPE type)
  : m_id(std::move(id)),
    m_mainType(type)
{

}

const CAddonType* CAddonInfo::Type(TYPE type) const
{
  static CAddonType dummy;

  if (!m_types.empty())
  {
    if (type == ADDON_UNKNOWN)
      return &m_types[0];

    for (auto& addonType : m_types)
    {
      if (addonType.Type() == type)
        return &addonType;
    }
  }

  return &dummy;
}

bool CAddonInfo::HasType(TYPE type, bool mainOnly /*= false*/) const
{
  return (m_mainType == type || ProvidesSubContent(type, mainOnly ? m_mainType : ADDON_UNKNOWN));
}

bool CAddonInfo::ProvidesSubContent(const TYPE& content, const TYPE& mainType /*= ADDON_UNKNOWN*/) const
{
  if (content == ADDON_UNKNOWN)
    return false;

  for (const auto& addonType : m_types)
  {
    if ((mainType == ADDON_UNKNOWN || addonType.Type() == mainType) && addonType.ProvidesSubContent(content))
      return true;
  }

  return false;
}

bool CAddonInfo::ProvidesSeveralSubContents() const
{
  int contents = 0;
  for (const auto& addonType : m_types)
    contents += addonType.ProvidedSubContents();
  return contents > 0 ? true : false;
}

bool CAddonInfo::MeetsVersion(const AddonVersion& versionMin, const AddonVersion& version) const
{
  return !(versionMin > m_version || version < m_minversion);
}

const AddonVersion& CAddonInfo::DependencyMinVersion(const std::string& dependencyID) const
{
  auto it = std::find_if(m_dependencies.begin(), m_dependencies.end(),
                         [&](const DependencyInfo& other) { return other.id == dependencyID; });

  if (it != m_dependencies.end())
    return it->versionMin;

  static AddonVersion emptyVersion;
  return emptyVersion;
}

const AddonVersion& CAddonInfo::DependencyVersion(const std::string& dependencyID) const
{
  auto it = std::find_if(m_dependencies.begin(), m_dependencies.end(), [&](const DependencyInfo& other) { return other.id == dependencyID; });

  if (it != m_dependencies.end())
    return it->version;

  static AddonVersion emptyVersion;
  return emptyVersion;
}

const std::string& CAddonInfo::GetTranslatedText(const std::unordered_map<std::string, std::string>& locales) const
{
  if (locales.size() == 1)
    return locales.begin()->second;
  else if (locales.empty())
    return StringUtils::Empty;

  // find the language from the list that matches the current locale best
  std::string matchingLanguage = g_langInfo.GetLocale().FindBestMatch(locales);
  if (matchingLanguage.empty())
    matchingLanguage = KODI_ADDON_DEFAULT_LANGUAGE_CODE;

  auto const& translatedValue = locales.find(matchingLanguage);
  if (translatedValue != locales.end())
    return translatedValue->second;
  return StringUtils::Empty;
}

} /* namespace ADDON */
