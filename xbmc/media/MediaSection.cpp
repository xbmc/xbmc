/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MediaSection.h"

#include <algorithm>
#include <array>

namespace KODI::MEDIA
{

namespace
{
struct SectionName
{
  MediaSection section;
  std::string_view name;
};

// Each section's own name first; the later rows are aliases the parser also accepts.
constexpr std::array<SectionName, 8> SECTION_NAMES{{
    {MediaSection::VIDEO, "video"},
    {MediaSection::MUSIC, "music"},
    {MediaSection::PICTURES, "pictures"},
    {MediaSection::FILES, "files"},
    {MediaSection::PROGRAMS, "programs"},
    {MediaSection::GAMES, "games"},
    {MediaSection::VIDEO, "videos"},
    {MediaSection::PROGRAMS, "myprograms"},
}};
} // unnamed namespace

std::string_view NameOf(MediaSection section)
{
  const auto it = std::ranges::find(SECTION_NAMES, section, &SectionName::section);
  return it == SECTION_NAMES.end() ? std::string_view{} : it->name;
}

std::optional<MediaSection> MediaSectionFromName(std::string_view name)
{
  const auto it = std::ranges::find(SECTION_NAMES, name, &SectionName::name);
  if (it == SECTION_NAMES.end())
    return std::nullopt;
  return it->section;
}

} // namespace KODI::MEDIA
