/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string_view>

/*!
 * \brief Dolby Vision enhancement layer type, derived from the RPU data header.
 */
enum class DoviElType
{
  NONE, ///< The frame carries no enhancement layer
  MEL, ///< Minimal enhancement layer
  FEL, ///< Full enhancement layer
};

/*!
 * \brief Get the display representation of a Dolby Vision enhancement layer type.
 * \param elType The enhancement layer type
 * \return "MEL", "FEL", or an empty string when there is no enhancement layer
 */
constexpr std::string_view DoviElTypeToString(DoviElType elType)
{
  switch (elType)
  {
    case DoviElType::MEL:
      return "MEL";
    case DoviElType::FEL:
      return "FEL";
    default:
      return "";
  }
}
