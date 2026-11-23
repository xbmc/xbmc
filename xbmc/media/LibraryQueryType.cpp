/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LibraryQueryType.h"

#include "Util.h"
#include "utils/ContentNames.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <array>

namespace KODI::MEDIA::LIBRARY_QUERY
{

namespace
{
constexpr Source SONGS{MediaSection::MUSIC, MEDIA::TYPE::SONG};
constexpr Source MUSIC_VIDEOS{MediaSection::VIDEO, MEDIA::TYPE::MUSIC_VIDEO};

struct Description
{
  TYPE type;
  std::string_view name;
  int label;
  std::array<Source, 2> sources;
  size_t sourceCount;
};

// clang-format off
constexpr std::array<Description, 8> DESCRIPTIONS{{
    {TYPE::SONGS,        CONTENT::SONGS,       134,   {SONGS},                                      1},
    {TYPE::ALBUMS,       CONTENT::ALBUMS,      132,   {Source{MediaSection::MUSIC, MEDIA::TYPE::ALBUM}},   1},
    {TYPE::ARTISTS,      CONTENT::ARTISTS,     133,   {Source{MediaSection::MUSIC, MEDIA::TYPE::ARTIST}},  1},
    {TYPE::MOVIES,       CONTENT::MOVIES,      20342, {Source{MediaSection::VIDEO, MEDIA::TYPE::MOVIE}},   1},
    {TYPE::TV_SHOWS,     CONTENT::TVSHOWS,     20343, {Source{MediaSection::VIDEO, MEDIA::TYPE::TV_SHOW}}, 1},
    {TYPE::EPISODES,     CONTENT::EPISODES,    20360, {Source{MediaSection::VIDEO, MEDIA::TYPE::EPISODE}}, 1},
    {TYPE::MUSIC_VIDEOS, CONTENT::MUSICVIDEOS, 20389, {MUSIC_VIDEOS},                               1},
    {TYPE::MIXED,        CONTENT::MIXED,       20395, {SONGS, MUSIC_VIDEOS},                        2},
}};
// clang-format on

const Description& DescriptionOf(TYPE type)
{
  return *std::ranges::find(DESCRIPTIONS, type, &Description::type);
}

struct GroupDescription
{
  GROUP group;
  std::string_view name;
  int label;
  bool canMix;
};

// Each group's own name first; "actors" is the performers' other name.
// clang-format off
constexpr std::array<GroupDescription, 15> GROUP_DESCRIPTIONS{{
    {GROUP::DEFAULT,       "",                     571,   false},
    {GROUP::NONE,          "none",                 231,   false},
    {GROUP::SET,           CONTENT::SETS,          20434, true},
    {GROUP::GENRE,         CONTENT::GENRES,        135,   false},
    {GROUP::YEAR,          CONTENT::YEARS,         652,   false},
    {GROUP::PERFORMER,     CONTENT::ARTISTS,       133,   false},
    {GROUP::DIRECTOR,      CONTENT::DIRECTORS,     20348, false},
    {GROUP::WRITER,        CONTENT::WRITERS,       20418, false},
    {GROUP::STUDIO,        CONTENT::STUDIOS,       20388, false},
    {GROUP::COUNTRY,       CONTENT::COUNTRIES,     20451, false},
    {GROUP::ALBUM,         CONTENT::ALBUMS,        132,   false},
    {GROUP::TAG,           CONTENT::TAGS,          20459, false},
    {GROUP::ORIGINAL_YEAR, "originalyears",        38078, false},
    {GROUP::VIDEO_VERSION, CONTENT::VIDEOVERSIONS, 40000, false},
    {GROUP::PERFORMER,     CONTENT::ACTORS,        344,   false},
}};
// clang-format on

const GroupDescription& DescriptionOf(GROUP group)
{
  return *std::ranges::find(GROUP_DESCRIPTIONS, group, &GroupDescription::group);
}
} // unnamed namespace

std::string_view NameOf(TYPE type)
{
  return DescriptionOf(type).name;
}

std::optional<TYPE> TypeFromName(std::string_view name)
{
  const auto it = std::ranges::find(DESCRIPTIONS, name, &Description::name);
  if (it == DESCRIPTIONS.end())
  {
    return std::nullopt;
  }
  return it->type;
}

int LabelOf(TYPE type)
{
  return DescriptionOf(type).label;
}

std::span<const Source> SourcesOf(TYPE type)
{
  const Description& description{DescriptionOf(type)};
  return std::span{description.sources}.first(description.sourceCount);
}

bool ListsFrom(TYPE type, MediaSection section)
{
  return ItemTypeIn(type, section).has_value();
}

std::optional<MEDIA::TYPE> ItemTypeIn(TYPE type, MediaSection section)
{
  const auto sources{SourcesOf(type)};
  const auto it = std::ranges::find(sources, section, &Source::section);
  if (it == sources.end())
  {
    return std::nullopt;
  }
  return it->itemType;
}

std::optional<TYPE> TypeOf(MEDIA::TYPE itemType)
{
  const auto it = std::ranges::find_if(
      DESCRIPTIONS, [itemType](const Description& description)
      { return description.sourceCount == 1 && description.sources[0].itemType == itemType; });
  if (it == DESCRIPTIONS.end())
  {
    return std::nullopt;
  }
  return it->type;
}

std::string_view SaveFolderOf(TYPE type)
{
  if (type == TYPE::MIXED)
  {
    return CUtil::MIXED_PLAYLISTS_FOLDER;
  }
  return ListsFrom(type, MediaSection::MUSIC) ? CUtil::MUSIC_PLAYLISTS_FOLDER
                                              : CUtil::VIDEO_PLAYLISTS_FOLDER;
}

bool CanReference(TYPE type, TYPE referenced)
{
  const auto mixesWith = [](TYPE mixed, TYPE other)
  { return mixed == TYPE::MIXED && (other == TYPE::SONGS || other == TYPE::MUSIC_VIDEOS); };
  return type == referenced || mixesWith(type, referenced) || mixesWith(referenced, type);
}

std::vector<std::string_view> ReferenceFoldersOf(TYPE type)
{
  std::vector<std::string_view> folders;
  for (const TYPE referenced : TYPES)
  {
    if (!CanReference(type, referenced))
    {
      continue;
    }
    const std::string_view folder{SaveFolderOf(referenced)};
    if (std::ranges::find(folders, folder) == folders.end())
    {
      folders.push_back(folder);
    }
  }
  return folders;
}

PLAYLIST::Type PlayListOf(TYPE type)
{
  return ListsFrom(type, MediaSection::VIDEO) ? PLAYLIST::Video : PLAYLIST::Audio;
}

bool CanFeedPartyMode(TYPE type)
{
  return type == TYPE::SONGS || type == TYPE::MUSIC_VIDEOS || type == TYPE::MIXED;
}

std::optional<GROUP> GroupFromName(std::string_view name)
{
  const auto it = std::ranges::find_if(GROUP_DESCRIPTIONS, [name](const GroupDescription& group)
                                       { return StringUtils::EqualsNoCase(name, group.name); });
  if (it == GROUP_DESCRIPTIONS.end())
  {
    return std::nullopt;
  }
  return it->group;
}

std::string_view NameOf(GROUP group, TYPE type)
{
  if (group == GROUP::PERFORMER &&
      (type == TYPE::MOVIES || type == TYPE::TV_SHOWS || type == TYPE::EPISODES))
  {
    return CONTENT::ACTORS;
  }
  return DescriptionOf(group).name;
}

int LabelOf(GROUP group, TYPE type)
{
  const std::string_view name{NameOf(group, type)};
  return std::ranges::find_if(GROUP_DESCRIPTIONS, [group, name](const GroupDescription& row)
                              { return row.group == group && row.name == name; })
      ->label;
}

bool CanMix(GROUP group)
{
  return DescriptionOf(group).canMix;
}

} // namespace KODI::MEDIA::LIBRARY_QUERY
