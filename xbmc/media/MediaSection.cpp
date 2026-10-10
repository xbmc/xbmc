/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MediaSection.h"

namespace KODI::MEDIA
{

std::string_view NameOf(MediaSection section)
{
  switch (section)
  {
    case MediaSection::VIDEO:
      return "video";
    case MediaSection::MUSIC:
      return "music";
    case MediaSection::PICTURES:
      return "pictures";
    case MediaSection::FILES:
      return "files";
    case MediaSection::PROGRAMS:
      return "programs";
    case MediaSection::GAMES:
      return "games";
  }
  return {};
}

std::optional<MediaSection> MediaSectionFromName(std::string_view name)
{
  if (name == "video" || name == "videos")
    return MediaSection::VIDEO;
  if (name == "music")
    return MediaSection::MUSIC;
  if (name == "pictures")
    return MediaSection::PICTURES;
  if (name == "files")
    return MediaSection::FILES;
  if (name == "programs" || name == "myprograms")
    return MediaSection::PROGRAMS;
  if (name == "games")
    return MediaSection::GAMES;
  return std::nullopt;
}

} // namespace KODI::MEDIA
