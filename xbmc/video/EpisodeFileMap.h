/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "video/Bookmark.h"

#include <functional>
#include <map>
#include <string>

struct EpisodeInformation
{
  int index{0};
  int season{-1};
  int episode{-1};
  unsigned int duration{0};
  CBookmark bookmark{};
};

using EpisodeFileMap = std::multimap<std::string, EpisodeInformation, std::less<>>;
using EpisodeFileMapEntry = EpisodeFileMap::value_type;
