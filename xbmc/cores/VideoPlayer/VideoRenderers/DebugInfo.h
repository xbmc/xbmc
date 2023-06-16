/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

struct DEBUG_INFO_PLAYER
{
  std::string audio;
  std::string video;
  std::string player;
  std::string vsync;
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
