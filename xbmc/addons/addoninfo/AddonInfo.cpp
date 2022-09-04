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
#include "filesystem/Directory.h"
#include "guilib/LocalizeStrings.h"
#include "utils/URIUtils.h"

#include <algorithm>
#include <array>
#include <string_view>

namespace ADDON
{

typedef struct
{
  std::string_view name;
  std::string_view old_name;
  TYPE type;
  int pretty;
  AddonInstanceSupport instance_support;
  std::string_view icon;
} TypeMapping;

// clang-format off
static constexpr const std::array<TypeMapping, 40> types =
  {{
   {"unknown",                           "", ADDON_UNKNOWN,                 0, AddonInstanceSupport::SUPPORT_NONE,      "" },
   {"xbmc.metadata.scraper.albums",      "", ADDON_SCRAPER_ALBUMS,      24016, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonAlbumInfo.png" },
   {"xbmc.metadata.scraper.artists",     "", ADDON_SCRAPER_ARTISTS,     24017, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonArtistInfo.png" },
   {"xbmc.metadata.scraper.movies",      "", ADDON_SCRAPER_MOVIES,      24007, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonMovieInfo.png" },
   {"xbmc.metadata.scraper.musicvideos", "", ADDON_SCRAPER_MUSICVIDEOS, 24015, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonMusicVideoInfo.png" },
   {"xbmc.metadata.scraper.tvshows",     "", ADDON_SCRAPER_TVSHOWS,     24014, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonTvInfo.png" },
   {"xbmc.metadata.scraper.library",     "", ADDON_SCRAPER_LIBRARY,     24083, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonInfoLibrary.png" },
   {"xbmc.ui.screensaver",               "", ADDON_SCREENSAVER,         24008, AddonInstanceSupport::SUPPORT_OPTIONAL,  "DefaultAddonScreensaver.png" },
   {"xbmc.player.musicviz",              "", ADDON_VIZ,                 24010, AddonInstanceSupport::SUPPORT_OPTIONAL,  "DefaultAddonVisualization.png" },
   {"xbmc.python.pluginsource",          "", ADDON_PLUGIN,              24005, AddonInstanceSupport::SUPPORT_NONE,      "" },
   {"xbmc.python.script",                "", ADDON_SCRIPT,              24009, AddonInstanceSupport::SUPPORT_NONE,      "" },
   {"xbmc.python.weather",               "", ADDON_SCRIPT_WEATHER,      24027, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonWeather.png" },
   {"xbmc.python.lyrics",                "", ADDON_SCRIPT_LYRICS,       24013, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonLyrics.png" },
   {"xbmc.python.library",               "", ADDON_SCRIPT_LIBRARY,      24081, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonHelper.png" },
   {"xbmc.python.module",                "", ADDON_SCRIPT_MODULE,       24082, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonLibrary.png" },
   {"xbmc.subtitle.module",              "", ADDON_SUBTITLE_MODULE,     24012, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonSubtitles.png" },
   {"kodi.context.item",                 "", ADDON_CONTEXT_ITEM,        24025, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonContextItem.png" },
   {"kodi.game.controller",              "", ADDON_GAME_CONTROLLER,     35050, AddonInstanceSupport::SUPPORT_OPTIONAL,  "DefaultAddonGame.png" },
   {"xbmc.gui.skin",                     "", ADDON_SKIN,                  166, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonSkin.png" },
   {"xbmc.webinterface",                 "", ADDON_WEB_INTERFACE,         199, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonWebSkin.png" },
   {"xbmc.addon.repository",             "", ADDON_REPOSITORY,          24011, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonRepository.png" },
   {"kodi.pvrclient",      "xbmc.pvrclient", ADDON_PVRDLL,              24019, AddonInstanceSupport::SUPPORT_SETTINGS,  "DefaultAddonPVRClient.png" },
   {"kodi.gameclient",                   "", ADDON_GAMEDLL,             35049, AddonInstanceSupport::SUPPORT_OPTIONAL,  "DefaultAddonGame.png" },
   {"kodi.peripheral",                   "", ADDON_PERIPHERALDLL,       35010, AddonInstanceSupport::SUPPORT_MANDATORY, "DefaultAddonPeripheral.png" },
   {"xbmc.addon.video",                  "", ADDON_VIDEO,                1037, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonVideo.png" },
   {"xbmc.addon.audio",                  "", ADDON_AUDIO,                1038, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonMusic.png" },
   {"xbmc.addon.image",                  "", ADDON_IMAGE,                1039, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonPicture.png" },
   {"xbmc.addon.executable",             "", ADDON_EXECUTABLE,           1043, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonProgram.png" },
   {"kodi.addon.game",                   "", ADDON_GAME,                35049, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonGame.png" },
   {"kodi.audioencoder",                 "", ADDON_AUDIOENCODER,          200, AddonInstanceSupport::SUPPORT_MANDATORY, "DefaultAddonAudioEncoder.png" },
   {"kodi.audiodecoder",                 "", ADDON_AUDIODECODER,          201, AddonInstanceSupport::SUPPORT_MANDATORY, "DefaultAddonAudioDecoder.png" },
   {"xbmc.service",                      "", ADDON_SERVICE,             24018, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonService.png" },
   {"kodi.resource.images",              "", ADDON_RESOURCE_IMAGES,     24035, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonImages.png" },
   {"kodi.resource.language",            "", ADDON_RESOURCE_LANGUAGE,   24026, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonLanguage.png" },
   {"kodi.resource.uisounds",            "", ADDON_RESOURCE_UISOUNDS,   24006, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonUISounds.png" },
   {"kodi.resource.games",               "", ADDON_RESOURCE_GAMES,      35209, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonGame.png" },
   {"kodi.resource.font",                "", ADDON_RESOURCE_FONT,       13303, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonFont.png" },
   {"kodi.inputstream",                  "", ADDON_INPUTSTREAM,         24048, AddonInstanceSupport::SUPPORT_MANDATORY, "DefaultAddonInputstream.png" },
   {"kodi.vfs",                          "", ADDON_VFS,                 39013, AddonInstanceSupport::SUPPORT_MANDATORY, "DefaultAddonVfs.png" },
   {"kodi.imagedecoder",                 "", ADDON_IMAGEDECODER,        39015, AddonInstanceSupport::SUPPORT_MANDATORY, "DefaultAddonImageDecoder.png" },
  }};
// clang-format on

const std::string& CAddonInfo::OriginName() const
{
  if (!m_originName)
  {
    ADDON::AddonPtr origin;
    if (CServiceBroker::GetAddonMgr().GetAddon(m_origin, origin, ADDON::ADDON_UNKNOWN,
                                               ADDON::OnlyEnabled::CHOICE_NO))
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
        return std::string(map.name.data(), map.name.size());
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
      return std::string(map.icon.data(), map.icon.size());
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

AddonInstanceSupport CAddonInfo::InstanceSupportType(TYPE type)
{
  const auto it = std::find_if(types.begin(), types.end(),
                               [type](const TypeMapping& entry) { return entry.type == type; });
  if (it != types.end())
    return it->instance_support;

  return AddonInstanceSupport::SUPPORT_NONE;
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

bool CAddonInfo::SupportsMultipleInstances() const
{
  switch (m_addonInstanceSupportType)
  {
    case AddonInstanceSupport::SUPPORT_MANDATORY:
    case AddonInstanceSupport::SUPPORT_OPTIONAL:
      return true;
    case AddonInstanceSupport::SUPPORT_SETTINGS:
      return m_supportsInstanceSettings;
    case AddonInstanceSupport::SUPPORT_NONE:
    default:
      return false;
  }
}

std::vector<AddonInstanceId> CAddonInfo::GetKnownInstanceIds() const
{
  static const std::vector<AddonInstanceId> singletonInstance = {ADDON_SINGLETON_INSTANCE_ID};

  if (!m_supportsInstanceSettings)
    return singletonInstance;

  const std::string searchPath = StringUtils::Format("special://profile/addon_data/{}/", m_id);
  CFileItemList items;
  XFILE::CDirectory::GetDirectory(searchPath, items, ".xml", XFILE::DIR_FLAG_NO_FILE_DIRS);

  std::vector<AddonInstanceId> ret;

  for (const auto& item : items)
  {
    const std::string startName = "instance-settings-";
    std::string filename = URIUtils::GetFileName(item->GetPath());
    if (StringUtils::StartsWithNoCase(URIUtils::GetFileName(item->GetPath()), startName))
    {
      URIUtils::RemoveExtension(filename);
      const std::string uid = filename.substr(startName.length());
      if (!uid.empty() && StringUtils::IsInteger(uid))
        ret.emplace_back(std::atoi(uid.c_str()));
    }
  }

  // If no instances are used, create first as default.
  if (ret.empty())
    ret.emplace_back(ADDON_FIRST_INSTANCE_ID);

  return ret;
}

} /* namespace ADDON */
