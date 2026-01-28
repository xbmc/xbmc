/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <string>

/*!
 \ingroup guilib
 \brief GUI utility functions
 */
class CGUIUtils
{
public:
  /*!
   * \brief Get a localized string, with support for skin strings
   * \param id The string ID to look up
   * \return The localized string, or empty string if not found
   *
   * This function handles both core strings and skin-specific strings (range 31000-31999).
   * For skin strings, it retrieves them from the active skin's string map.
   * For core strings, it retrieves them from the main localization map.
   */
  static std::string GetLocalizedString(uint32_t id);
};
