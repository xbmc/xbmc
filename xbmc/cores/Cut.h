/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace EDL
{

enum class Action
{
  CUT = 0,
  MUTE = 1,
  // SCENE = 2,
  COMM_BREAK = 3
};

struct Cut
{
  int start = 0; // ms
  int end = 0;   // ms
  Action action = Action::CUT;
};

}
