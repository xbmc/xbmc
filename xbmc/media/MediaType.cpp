/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MediaType.h"

#include "ServiceBroker.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/ContentNames.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <array>

namespace KODI::MEDIA
{

namespace
{
struct MediaTypeInfo
{
  TYPE type;
  std::string_view name;
  std::string_view plural;
  bool container;
  int localizationSingularCapital;
};

// clang-format off
constexpr std::array<MediaTypeInfo, 12> MEDIA_TYPES{{
    {TYPE::MUSIC,            "music",        "music",              true,    249},
    {TYPE::ARTIST,           "artist",       CONTENT::ARTISTS,     true,    557},
    {TYPE::ALBUM,            "album",        CONTENT::ALBUMS,      true,    558},
    {TYPE::SONG,             "song",         CONTENT::SONGS,       false,   179},
    {TYPE::VIDEO,            "video",        "videos",             true,    291},
    {TYPE::VIDEO_COLLECTION, "set",          CONTENT::SETS,        true,  20141},
    {TYPE::MUSIC_VIDEO,      "musicvideo",   CONTENT::MUSICVIDEOS, false, 20391},
    {TYPE::MOVIE,            "movie",        CONTENT::MOVIES,      false, 20338},
    {TYPE::TV_SHOW,          "tvshow",       CONTENT::TVSHOWS,     true,  36902},
    {TYPE::SEASON,           "season",       CONTENT::SEASONS,     true,  20373},
    {TYPE::EPISODE,          "episode",      CONTENT::EPISODES,    false, 20359},
    {TYPE::VIDEO_VERSION,    "videoversion", CONTENT::VIDEOVERSIONS, false, 40012},
}};
// clang-format on

const MediaTypeInfo* Find(TYPE type)
{
  const auto it = std::ranges::find(MEDIA_TYPES, type, &MediaTypeInfo::type);
  return it != MEDIA_TYPES.end() ? &*it : nullptr;
}

//! Every type's Field as a string, indexed by type, so a name can be handed out by reference.
template<std::string_view MediaTypeInfo::* Field>
const std::string& Text(TYPE type)
{
  static const std::array<std::string, MEDIA_TYPES.size() + 1> texts = []
  {
    std::array<std::string, MEDIA_TYPES.size() + 1> result;
    for (const MediaTypeInfo& info : MEDIA_TYPES)
      result[static_cast<size_t>(info.type)] = std::string{info.*Field};
    return result;
  }();
  return texts[static_cast<size_t>(type)];
}

} // namespace

const std::string& NameOf(TYPE type)
{
  return Text<&MediaTypeInfo::name>(type);
}

const std::string& PluralNameOf(TYPE type)
{
  return Text<&MediaTypeInfo::plural>(type);
}

TYPE MediaTypeFromName(std::string_view name)
{
  const auto it = std::ranges::find_if(MEDIA_TYPES,
                                       [name](const MediaTypeInfo& info)
                                       {
                                         return StringUtils::EqualsNoCase(name, info.name) ||
                                                StringUtils::EqualsNoCase(name, info.plural);
                                       });
  return it != MEDIA_TYPES.end() ? it->type : TYPE::NONE;
}

TYPE MediaTypeOf(std::string_view name)
{
  const auto it = std::ranges::find(MEDIA_TYPES, name, &MediaTypeInfo::name);
  return it != MEDIA_TYPES.end() ? it->type : TYPE::NONE;
}

bool IsContainer(TYPE type)
{
  const MediaTypeInfo* info = Find(type);
  return info && info->container;
}

std::string GetCapitalLocalization(TYPE type)
{
  const MediaTypeInfo* info = Find(type);
  if (!info)
    return {};
  return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(
      info->localizationSingularCapital);
}

} // namespace KODI::MEDIA
