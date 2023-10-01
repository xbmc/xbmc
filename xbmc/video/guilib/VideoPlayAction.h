/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace VIDEO
{
namespace GUILIB
{
// Note: Do not change the numerical values of the elements. Some of them are used as values for
//       the integer setting SETTING_MYVIDEOS_PLAYACTION.
enum PlayAction
{
  PLAY_ACTION_PLAY_OR_RESUME = 1, // if resume is possible, ask user. play from beginning otherwise
  PLAY_ACTION_RESUME = 2, // resume if possibly, play from beginning otherwise
  PLAY_ACTION_PLAY_FROM_BEGINNING = 5, // play from beginning, also if resume would be possible
};
} // namespace GUILIB
} // namespace VIDEO
