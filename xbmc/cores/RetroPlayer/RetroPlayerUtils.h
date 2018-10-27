/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/GameSettings.h"

#include <string>

namespace KODI
{
namespace RETRO
{
  class CRetroPlayerUtils
  {
  public:
    /*!
     * \brief Convert a stretch mode enum to a short string identifier
     *
     * \param stretchMode The stretch mode
     *
     * \return A short string identifier specified by GameSettings.h, or an
     *         empty string if the stretch mode is invalid
     */
    static const char* StretchModeToIdentifier(STRETCHMODE stretchMode);

    /*!
     * \brief Convert a stretch mode identifier to an enum
     *
     * \param stretchMode The short string identifier, from GameSettings.h,
     *        representing the stretch mode
     *
     * \return The stretch mode enum, or STRETCHMODE::Normal if the identifier
     *         is invalid
     */
    static STRETCHMODE IdentifierToStretchMode(const std::string &stretchMode);
  };
}
}
