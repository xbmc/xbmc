/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <array>
#include <string_view>

namespace EDL
{

constexpr int EDL_ACTION_NONE = -1;

enum class Action
{
  CUT = 0,
  MUTE = 1,
  SCENE = 2,
  COMM_BREAK = 3
};

constexpr std::array<std::string_view, 4> edlActionDescription{
    {"Cut", "Mute", "Scene Marker", "Commercial Break"}};

struct Edit
{
  int start = 0; // ms
  int end = 0; // ms
  Action action = Action::CUT;
};

} // namespace EDL
