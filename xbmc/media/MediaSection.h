/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <array>
#include <optional>
#include <string_view>

namespace KODI::MEDIA
{

//! \brief A section of Kodi, each with sources of its own.
enum class MediaSection
{
  VIDEO,
  MUSIC,
  PICTURES,
  FILES,
  PROGRAMS,
  GAMES,
};

inline constexpr std::array<MediaSection, 6> MEDIA_SECTIONS{
    MediaSection::VIDEO, MediaSection::MUSIC,    MediaSection::PICTURES,
    MediaSection::FILES, MediaSection::PROGRAMS, MediaSection::GAMES};

//! \brief The name a section has in sources.xml and the APIs: "video", "music", "pictures",
//! "files", "programs" or "games".
std::string_view NameOf(MediaSection section);

//! \brief The section \p name gives, taking "videos" and "myprograms" as well. Nullopt for any
//! other text.
std::optional<MediaSection> MediaSectionFromName(std::string_view name);

} // namespace KODI::MEDIA
