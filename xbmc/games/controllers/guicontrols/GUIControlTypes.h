/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 *
 * \brief Types of button controls that can populate the feature list
 */
enum class BUTTON_TYPE
{
  UNKNOWN,
  BUTTON,
  ANALOG_STICK,
  RELATIVE_POINTER,
  WHEEL,
  THROTTLE,
  SELECT_KEY,
};
} // namespace GAME
} // namespace KODI
