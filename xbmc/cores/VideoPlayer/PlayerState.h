/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/MenuType.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"

#include <cstdint>
#include <string>
#include <vector>

struct SPlayerState
{
  SPlayerState() { Clear(); }
  void Clear()
  {
    timestamp = 0;
    time = 0;
    startTime = 0;
    timeMin = 0;
    timeMax = 0;
    time_offset = 0;
    dts = DVD_NOPTS_VALUE;
    player_state = "";
    isInMenu = false;
    menuType = MenuType::NONE;
    chapter = 0;
    chapters.clear();
    canpause = false;
    canseek = false;
    cantempo = false;
    caching = false;
    cache_bytes = 0;
    cache_level = 0.0;
    cache_offset = 0.0;
    lastSeek = 0;
    streamsReady = false;
  }

  double timestamp; // last time of update
  double lastSeek; // time of last seek
  double time_offset; // difference between time and pts

  double time; // current playback time
  double timeMax;
  double timeMin;
  time_t startTime;
  double dts; // last known dts

  std::string player_state; // full player state
  bool isInMenu;
  MenuType menuType;
  bool streamsReady;

  int chapter; // current chapter
  std::vector<std::pair<std::string, int64_t>> chapters; // name and position for chapters

  bool canpause; // pvr: can pause the current playing item
  bool canseek; // pvr: can seek in the current playing item
  bool cantempo;
  bool caching;

  int64_t cache_bytes; // number of bytes current's cached
  double cache_level; // current cache level
  double cache_offset; // percentage of file ahead of current position
  double cache_time; // estimated playback time of current cached bytes
};
