/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "MPLSParser.h"

#include <chrono>
#include <string>
#include <vector>

class CFileItem;
class CFileItemList;

namespace XFILE
{
using namespace std::chrono_literals;

struct BlurayPlaylistInformation
{
  unsigned int playlist{0};
  std::string version;
  std::chrono::milliseconds duration{0ms};
  BLURAY_PLAYBACK_TYPE playbackType{0};
  unsigned int playbackCount{0};
  std::vector<PlayItemInformation> playItems;
  std::vector<ClipInformation> clips;
  std::vector<SubPlayItemInformation> subPlayItems;
  std::vector<SubPlayItemInformation> extensionSubPlayItems;
  std::vector<PlaylistMarkInformation> playlistMarks;
  std::vector<ChapterInformation> chapters;
};
} // namespace XFILE
