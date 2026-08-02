/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace KODI::RETRO
{

class CCheevosAwardQueue
{
public:
  /*!
   * \brief Retry pending achievement awards and retain any that still fail
   */
  static void Flush();

  /*!
   * \brief Persist an achievement award URL for a later retry
   */
  static void Queue(const std::string& url);
};

} // namespace KODI::RETRO
