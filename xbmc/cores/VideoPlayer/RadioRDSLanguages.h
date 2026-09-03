/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "language/LanguageTag.h"

#include <string_view>

namespace KODI::RDS
{

/*!
 * \brief The number of language indices the RDS slow labelling codes can name.
 */
inline constexpr unsigned int LANGUAGE_INDEX_COUNT{128};

/*!
 * \brief The code EBU Tech 3244 Annex J assigns to a slow labelling language index.
 * \note A broadcaster sends the index, never text, so this table is the whole vocabulary an RDS
 *       stream can name a language in.
 * \param[in] index The index, as carried by slow labelling variant 3.
 * \return The code, empty where the standard reserves the index or the index is out of range.
 */
std::string_view LanguageCode(unsigned int index);

/*!
 * \brief The language a slow labelling language index names.
 * \note The table holds ISO 639-2/B codes where the standard has one, and ISO 639-3 for the few
 *       languages it does not.
 * \param[in] index The index, as carried by slow labelling variant 3.
 * \return The language, undetermined where the index names none.
 */
LANGUAGE::CLanguageTag Language(unsigned int index);

} // namespace KODI::RDS
