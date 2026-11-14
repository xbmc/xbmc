/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/addoninfo/AddonType.h"
#include "media/MediaSection.h"
#include "utils/DefaultArt.h"

#include <array>
#include <cstdint>
#include <string_view>

//! \brief The addons:// paths of the add-on browser's nodes, as XFILE::CAddonsDirectory parses
//! them, and of the entries it offers that stand for an action.
namespace KODI::ADDONS
{

inline constexpr char ROOT[] = "addons://";

inline constexpr char ALL[] = "addons://all/";
inline constexpr char USER[] = "addons://user/";
inline constexpr char DEPENDENCIES[] = "addons://dependencies/";
inline constexpr char DISABLED[] = "addons://disabled/";
inline constexpr char OUTDATED[] = "addons://outdated/";
inline constexpr char RUNNING[] = "addons://running/";
inline constexpr char REPOS[] = "addons://repos/";
inline constexpr char SOURCES[] = "addons://sources/";
inline constexpr char SEARCH[] = "addons://search/";
inline constexpr char RECENTLY_UPDATED[] = "addons://recently_updated/";
inline constexpr char DOWNLOADING[] = "addons://downloading/";
inline constexpr char MORE[] = "addons://more/";
inline constexpr char DEFAULT_BINARY_ADDONS_SOURCE[] = "addons://default_binary_addons_source/";

inline constexpr char INSTALL[] = "addons://install/";
inline constexpr char UPDATE_ALL[] = "addons://update_all/";
inline constexpr char UPDATE_ALLOWED[] = "addons://update_allowed/";

//! \brief The host name of the node at \p path, which is how CAddonsDirectory tells nodes apart.
constexpr std::string_view EndpointOf(std::string_view path)
{
  path.remove_prefix(std::string_view{ROOT}.size());
  path.remove_suffix(1);
  return path;
}

//! \brief The source at the root of a section listing the add-ons that provide its content.
struct SectionSource
{
  MEDIA::MediaSection section;
  ADDON::AddonType type;
  uint32_t label;
  std::string_view icon;
};

// clang-format off
inline constexpr std::array<SectionSource, 5> SECTION_SOURCES{{
    {MEDIA::MediaSection::VIDEO,    ADDON::AddonType::VIDEO,      1037,  ART::DEFAULT::ADDON_VIDEO},
    {MEDIA::MediaSection::PROGRAMS, ADDON::AddonType::EXECUTABLE, 1043,  ART::DEFAULT::ADDON_PROGRAM},
    {MEDIA::MediaSection::PICTURES, ADDON::AddonType::IMAGE,      1039,  ART::DEFAULT::ADDON_PICTURE},
    {MEDIA::MediaSection::MUSIC,    ADDON::AddonType::AUDIO,      1038,  ART::DEFAULT::ADDON_MUSIC},
    {MEDIA::MediaSection::GAMES,    ADDON::AddonType::GAME,       35049, ART::DEFAULT::ADDON_GAME},
}};
// clang-format on

} // namespace KODI::ADDONS
