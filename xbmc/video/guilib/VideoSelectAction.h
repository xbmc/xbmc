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
//       the integer setting SETTING_MYVIDEOS_SELECTACTION.
enum SelectAction
{
  SELECT_ACTION_CHOOSE = 0,
  SELECT_ACTION_PLAY_OR_RESUME = 1,
  SELECT_ACTION_RESUME = 2,
  SELECT_ACTION_INFO = 3,
  SELECT_ACTION_MORE = 4,
  SELECT_ACTION_PLAY = 5,
  SELECT_ACTION_PLAYPART = 6,
  SELECT_ACTION_QUEUE = 7,
};
} // namespace GUILIB
} // namespace VIDEO
