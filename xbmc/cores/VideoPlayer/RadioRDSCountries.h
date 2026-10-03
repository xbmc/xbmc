/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "language/Territory.h"

#include <optional>
#include <string_view>

namespace KODI::RDS
{

/*!
 * \brief The number of PI country codes an extended country code qualifies.
 */
inline constexpr unsigned int COUNTRY_CODE_COUNT{15};

/*!
 * \brief The number of extended country codes each PI country code is qualified by.
 */
inline constexpr unsigned int EXTENDED_COUNTRY_CODE_COUNT{7};

/*!
 * \brief The code the standard assigns to an extended country code and PI country code.
 * \note A broadcaster sends a country as a 4 bit PI nibble crossed with an extended country
 *       code, never as text, so these tables are the whole vocabulary an RDS stream can name a
 *       place in.
 * \param[in] extendedCountryCode The ECC, as the high nibble carries it: 0xA0, 0xD0, 0xE0 or 0xF0.
 * \param[in] countryCode The PI country code, 1 to 15.
 * \param[in] index The ECC's low nibble.
 * \return The code, empty where the standard reserves the cell, or nothing at all where the
 *         arguments name no cell.
 */
std::optional<std::string_view> CountryCode(unsigned int extendedCountryCode,
                                            unsigned int countryCode,
                                            unsigned int index);

/*!
 * \brief The place an extended country code and PI country code name.
 * \param[in] extendedCountryCode The ECC, as the high nibble carries it: 0xA0, 0xD0, 0xE0 or 0xF0.
 * \param[in] countryCode The PI country code, 1 to 15.
 * \param[in] index The ECC's low nibble.
 * \return The territory, naming nowhere where the standard reserves the cell, or nothing at all
 *         where the arguments name no cell.
 */
std::optional<LANGUAGE::CTerritory> Country(unsigned int extendedCountryCode,
                                            unsigned int countryCode,
                                            unsigned int index);

} // namespace KODI::RDS
