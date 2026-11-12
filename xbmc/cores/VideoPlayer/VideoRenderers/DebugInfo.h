/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct DEBUG_INFO_PLAYER
{
  std::string audio;
  std::string video;
  std::string player;
  std::string vsync;
  std::string contentGeometry; //!< served rect, source and live gate state; empty without video
};

struct DEBUG_INFO_VIDEO
{
  std::string videoSource;
  std::string metaPrim;
  std::string metaLight;
  std::string shader;
  std::string render;
};

struct DEBUG_INFO_RENDER
{
  std::string renderFlags;
  std::string videoOutput;
};

constexpr unsigned int DEBUG_INFO_FRAMES_MAX = 3840;

struct DEBUG_INFO_FRAME
{
  uint32_t size;
  char type; //!< 'I', 'P', 'B', or 0 if unknown
};

struct DEBUG_INFO_FRAMES
{
  std::vector<DEBUG_INFO_FRAME> frames; //!< most recent video packets, oldest first
  uint64_t count{0}; //!< packets since the video codec was opened
  uint64_t serial{0}; //!< changes whenever frames changes
  float fps{0.0f};
};
