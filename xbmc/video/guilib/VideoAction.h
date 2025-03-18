/*
 *  Copyright (C) 2023-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI::VIDEO::GUILIB
{
// Note: Do not change the numerical values of the elements. Some of them are used as values for
//       the integer settings SETTING_MYVIDEOS_SELECTACTION and SETTING_MYVIDEOS_PLAYACTION.
enum Action
{
  ACTION_CHOOSE = 0, // open context menu to pick an action
  ACTION_PLAY_OR_RESUME = 1, // if resume is possible, ask user. play from beginning otherwise
  ACTION_RESUME = 2, // resume if possibly, play from beginning otherwise
  ACTION_INFO = 3, // show info dialog
  // 4 unused
  ACTION_PLAY_FROM_BEGINNING = 5, // play from beginning, also if resume would be possible
  // 6 unused
  ACTION_QUEUE = 7, // add item to the video playlist
  ACTION_PLAY = 8, // play according to SETTING_MYVIDEOS_PLAYACTION value
};
} // namespace KODI::VIDEO::GUILIB
