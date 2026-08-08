/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>

namespace MUSIC_INFO
{
class CMusicInfoTag;

/*!
 * \brief Maps Matroska SimpleTag names onto CMusicInfoTag fields.
 *
 * Deliberately free of any TagLib dependency, so that a reader of Matroska tags can use it
 * whatever it parsed the file with. CMusicInfoTagLoaderMatroska handles single files,
 * CAudioBookFileDirectory handles chaptered ones; keeping the mapping here is what stops the two
 * disagreeing about what a tag name means.
 *
 * Tag names are as written in the file (see https://www.matroska.org/technical/tagging.html),
 * upper-cased by the caller. The comma delimited variants (INVOLVEDPEOPLE, INSTRUMENTS) are
 * outside the spec, which says to use one SimpleTag per value, but taggers write them anyway.
 */
namespace MatroskaTagMapping
{
/*!
 * \brief Apply one Matroska tag to a music tag.
 * \param key Upper-cased Matroska SimpleTag name.
 * \param value The tag value.
 * \param separators Item separators used to split multi-value tags.
 * \param musicsep Separator to re-join values with, from advancedsettings.
 * \param tag Music tag to write to. Unknown keys leave it untouched.
 */
void MapTag(const std::string& key,
            const std::string& value,
            const std::vector<std::string>& separators,
            const std::string& musicsep,
            CMusicInfoTag& tag);

} // namespace MatroskaTagMapping
} // namespace MUSIC_INFO
