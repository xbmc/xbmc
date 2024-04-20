/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonInfo.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "LangInfo.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "addons/addoninfo/AddonType.h"
#include "filesystem/Directory.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
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
  AddonType type;
  int pretty;
  AddonInstanceSupport instance_support;
  std::string_view icon;
} TypeMapping;

// clang-format off
static constexpr const std::array<TypeMapping, 40> types =
  {{
   {"unknown",                           "", AddonType::UNKNOWN,                 0, AddonInstanceSupport::SUPPORT_NONE,      "" },
   {"xbmc.metadata.scraper.albums",      "", AddonType::SCRAPER_ALBUMS,      24016, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonAlbumInfo.png" },
   {"xbmc.metadata.scraper.artists",     "", AddonType::SCRAPER_ARTISTS,     24017, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonArtistInfo.png" },
   {"xbmc.metadata.scraper.movies",      "", AddonType::SCRAPER_MOVIES,      24007, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonMovieInfo.png" },
   {"xbmc.metadata.scraper.musicvideos", "", AddonType::SCRAPER_MUSICVIDEOS, 24015, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonMusicVideoInfo.png" },
   {"xbmc.metadata.scraper.tvshows",     "", AddonType::SCRAPER_TVSHOWS,     24014, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonTvInfo.png" },
   {"xbmc.metadata.scraper.library",     "", AddonType::SCRAPER_LIBRARY,     24083, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonInfoLibrary.png" },
   {"xbmc.ui.screensaver",               "", AddonType::SCREENSAVER,         24008, AddonInstanceSupport::SUPPORT_OPTIONAL,  "DefaultAddonScreensaver.png" },
   {"xbmc.player.musicviz",              "", AddonType::VISUALIZATION,       24010, AddonInstanceSupport::SUPPORT_OPTIONAL,  "DefaultAddonVisualization.png" },
   {"xbmc.python.pluginsource",          "", AddonType::PLUGIN,              24005, AddonInstanceSupport::SUPPORT_NONE,      "" },
   {"xbmc.python.script",                "", AddonType::SCRIPT,              24009, AddonInstanceSupport::SUPPORT_NONE,      "" },
   {"xbmc.python.weather",               "", AddonType::SCRIPT_WEATHER,      24027, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonWeather.png" },
   {"xbmc.python.lyrics",                "", AddonType::SCRIPT_LYRICS,       24013, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonLyrics.png" },
   {"xbmc.python.library",               "", AddonType::SCRIPT_LIBRARY,      24081, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonHelper.png" },
   {"xbmc.python.module",                "", AddonType::SCRIPT_MODULE,       24082, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonLibrary.png" },
   {"xbmc.subtitle.module",              "", AddonType::SUBTITLE_MODULE,     24012, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonSubtitles.png" },
   {"kodi.context.item",                 "", AddonType::CONTEXTMENU_ITEM,    24025, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonContextItem.png" },
   {"kodi.game.controller",              "", AddonType::GAME_CONTROLLER,     35050, AddonInstanceSupport::SUPPORT_OPTIONAL,  "DefaultAddonGame.png" },
   {"xbmc.gui.skin",                     "", AddonType::SKIN,                  166, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonSkin.png" },
   {"xbmc.webinterface",                 "", AddonType::WEB_INTERFACE,         199, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonWebSkin.png" },
   {"xbmc.addon.repository",             "", AddonType::REPOSITORY,          24011, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonRepository.png" },
   {"kodi.pvrclient",      "xbmc.pvrclient", AddonType::PVRDLL,              24019, AddonInstanceSupport::SUPPORT_SETTINGS,  "DefaultAddonPVRClient.png" },
   {"kodi.gameclient",                   "", AddonType::GAMEDLL,             35049, AddonInstanceSupport::SUPPORT_OPTIONAL,  "DefaultAddonGame.png" },
   {"kodi.peripheral",                   "", AddonType::PERIPHERALDLL,       35010, AddonInstanceSupport::SUPPORT_MANDATORY, "DefaultAddonPeripheral.png" },
   {"xbmc.addon.video",                  "", AddonType::VIDEO,                1037, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonVideo.png" },
   {"xbmc.addon.audio",                  "", AddonType::AUDIO,                1038, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonMusic.png" },
   {"xbmc.addon.image",                  "", AddonType::IMAGE,                1039, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonPicture.png" },
   {"xbmc.addon.executable",             "", AddonType::EXECUTABLE,           1043, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonProgram.png" },
   {"kodi.addon.game",                   "", AddonType::GAME,                35049, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonGame.png" },
   {"kodi.audioencoder",                 "", AddonType::AUDIOENCODER,          200, AddonInstanceSupport::SUPPORT_MANDATORY, "DefaultAddonAudioEncoder.png" },
   {"kodi.audiodecoder",                 "", AddonType::AUDIODECODER,          201, AddonInstanceSupport::SUPPORT_MANDATORY, "DefaultAddonAudioDecoder.png" },
   {"xbmc.service",                      "", AddonType::SERVICE,             24018, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonService.png" },
   {"kodi.resource.images",              "", AddonType::RESOURCE_IMAGES,     24035, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonImages.png" },
   {"kodi.resource.language",            "", AddonType::RESOURCE_LANGUAGE,   24026, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonLanguage.png" },
   {"kodi.resource.uisounds",            "", AddonType::RESOURCE_UISOUNDS,   24006, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonUISounds.png" },
   {"kodi.resource.games",               "", AddonType::RESOURCE_GAMES,      35209, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonGame.png" },
   {"kodi.resource.font",                "", AddonType::RESOURCE_FONT,       13303, AddonInstanceSupport::SUPPORT_NONE,      "DefaultAddonFont.png" },
   {"kodi.inputstream",                  "", AddonType::INPUTSTREAM,         24048, AddonInstanceSupport::SUPPORT_MANDATORY, "DefaultAddonInputstream.png" },
   {"kodi.vfs",                          "", AddonType::VFS,                 39013, AddonInstanceSupport::SUPPORT_MANDATORY, "DefaultAddonVfs.png" },
   {"kodi.imagedecoder",                 "", AddonType::IMAGEDECODER,        39015, AddonInstanceSupport::SUPPORT_MANDATORY, "DefaultAddonImageDecoder.png" },
  }};
// clang-format on

const std::string& CAddonInfo::OriginName() const
{
  if (!m_originName)
  {
    ADDON::AddonPtr origin;
    if (CServiceBroker::GetAddonMgr().GetAddon(m_origin, origin, ADDON::OnlyEnabled::CHOICE_NO))
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

std::string CAddonInfo::TranslateType(AddonType type, bool pretty /*= false*/)
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

AddonType CAddonInfo::TranslateType(const std::string& string)
{
  for (const TypeMapping& map : types)
  {
    if (string == map.name || (!map.old_name.empty() && string == map.old_name))
      return map.type;
  }

  return AddonType::UNKNOWN;
}

std::string CAddonInfo::TranslateIconType(AddonType type)
{
  for (const TypeMapping& map : types)
  {
    if (type == map.type)
      return std::string(map.icon.data(), map.icon.size());
  }
  return "";
}

AddonType CAddonInfo::TranslateSubContent(const std::string& content)
{
  if (content == "audio")
    return AddonType::AUDIO;
  else if (content == "image")
    return AddonType::IMAGE;
  else if (content == "executable")
    return AddonType::EXECUTABLE;
  else if (content == "video")
    return AddonType::VIDEO;
  else if (content == "game")
    return AddonType::GAME;
  else
    return AddonType::UNKNOWN;
}

AddonInstanceSupport CAddonInfo::InstanceSupportType(AddonType type)
{
  const auto it = std::find_if(types.begin(), types.end(),
                               [type](const TypeMapping& entry) { return entry.type == type; });
  if (it != types.end())
    return it->instance_support;

  return AddonInstanceSupport::SUPPORT_NONE;
}

CAddonInfo::CAddonInfo(std::string id, AddonType type) : m_id(std::move(id)), m_mainType(type)
{

}

const CAddonType* CAddonInfo::Type(AddonType type) const
{
  static CAddonType dummy;

  if (!m_types.empty())
  {
    if (type == AddonType::UNKNOWN)
      return &m_types[0];

    for (auto& addonType : m_types)
    {
      if (addonType.Type() == type)
        return &addonType;
    }
  }

  return &dummy;
}

bool CAddonInfo::HasType(AddonType type, bool mainOnly /*= false*/) const
{
  return (m_mainType == type ||
          ProvidesSubContent(type, mainOnly ? m_mainType : AddonType::UNKNOWN));
}

bool CAddonInfo::ProvidesSubContent(AddonType content, AddonType mainType) const
{
  if (content == AddonType::UNKNOWN)
    return false;

  for (const auto& addonType : m_types)
  {
    if ((mainType == AddonType::UNKNOWN || addonType.Type() == mainType) &&
        addonType.ProvidesSubContent(content))
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

bool CAddonInfo::MeetsVersion(const CAddonVersion& versionMin, const CAddonVersion& version) const
{
  return !(versionMin > m_version || version < m_minversion);
}

const CAddonVersion& CAddonInfo::DependencyMinVersion(const std::string& dependencyID) const
{
  auto it = std::find_if(m_dependencies.begin(), m_dependencies.end(),
                         [&](const DependencyInfo& other) { return other.id == dependencyID; });

  if (it != m_dependencies.end())
    return it->versionMin;

  static CAddonVersion emptyVersion;
  return emptyVersion;
}

const CAddonVersion& CAddonInfo::DependencyVersion(const std::string& dependencyID) const
{
  auto it = std::find_if(m_dependencies.begin(), m_dependencies.end(), [&](const DependencyInfo& other) { return other.id == dependencyID; });

  if (it != m_dependencies.end())
    return it->version;

  static CAddonVersion emptyVersion;
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

  std::vector<AddonInstanceId> ret;
  const std::string searchPath = StringUtils::Format("special://profile/addon_data/{}/", m_id);

  if (XFILE::CDirectory::Exists(searchPath))
  {
    CFileItemList items;
    XFILE::CDirectory::GetDirectory(searchPath, items, ".xml", XFILE::DIR_FLAG_NO_FILE_DIRS);

    static const std::string startName = "instance-settings-";

    for (const auto& item : items)
    {
      std::string filename = URIUtils::GetFileName(item->GetPath());
      if (StringUtils::StartsWithNoCase(filename, startName))
      {
        URIUtils::RemoveExtension(filename);
        const std::string_view uid(filename.data() + startName.length());
        if (!uid.empty() && StringUtils::IsInteger(uid.data()))
          ret.emplace_back(std::atoi(uid.data()));
      }
    }
  }

  // If no instances are used, create first as default.
  if (ret.empty())
    ret.emplace_back(ADDON_FIRST_INSTANCE_ID);

  return ret;
}

} /* namespace ADDON */
