/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <set>
#include <string>

namespace KODI
{
namespace KEYMAP
{
/*!
 * \ingroup keymap
 *
 * \brief Action entry in joystick.xml
 */
struct KeymapAction
{
  unsigned int actionId;
  std::string actionString;
  unsigned int holdTimeMs;
  std::set<std::string> hotkeys;

  bool operator<(const KeymapAction& rhs) const { return holdTimeMs < rhs.holdTimeMs; }
};

/*!
 * \ingroup keymap
 *
 * \brief Container that sorts action entries by their holdtime
 */
struct KeymapActionGroup
{
  int windowId = -1;
  std::set<KeymapAction> actions;
};
} // namespace KEYMAP
} // namespace KODI
