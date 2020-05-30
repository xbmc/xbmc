/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace KODI
{
namespace RETRO
{
class CSavestateUtils
{
public:
  /*!
   * \brief Calculate a savestate path for the specified game
   *
   * The savestate path is the game path with the extension replaced
   * by ".sav".
   */
  static std::string MakePath(const std::string& gamePath);
};
} // namespace RETRO
} // namespace KODI
