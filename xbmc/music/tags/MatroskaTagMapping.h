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
 * Deliberately free of any TagLib dependency. The same key/value pairs reach Kodi from two
 * readers - TagLib's Matroska API where available (see HAS_TAGLIB_MATROSKA in TagLibVersion.h),
 * FFmpeg's Matroska demuxer otherwise - and both have to agree on what a tag name means. Keeping
 * the mapping in one always-compiled place is what stops the two readers drifting apart, and it
 * is why gating the TagLib reader costs fidelity rather than whole tags.
 *
 * Tag names are as written in the file (see https://www.matroska.org/technical/tagging.html),
 * upper-cased by the caller. The comma delimited variants (INVOLVEDPEOPLE, INSTRUMENTS) are
 * outside the spec, which says to use one SimpleTag per value, but FFmpeg returns only the last
 * of a repeated set (https://trac.ffmpeg.org/ticket/9641), so taggers write them anyway.
 */
namespace MatroskaTagMapping
{
/*!
 * \brief The level a tag applies to, which is half of what it means.
 *
 * A name says what a tag is and its level says what it is about; only both together say which
 * field it means. An ARTIST at album level is the album's, at track level the song's. Matroska
 * writes the level as a TargetTypeValue, FFmpeg's demuxer as a prefix on the name - neither
 * reaches here, because a reader has resolved it before a name is looked up.
 */
enum class TagLevel
{
  File, //!< The file itself: no TargetTypeValue. Describes the album and, failing better, the song.
  Album, //!< TargetTypeValue 50 or 60.
  Track, //!< TargetTypeValue 30.
};

/*!
 * \brief What a reader joins the repeats of one SimpleTag with before MapTag() sees them.
 *
 * The spec writes several values as the same SimpleTag repeated; Kodi holds them in one delimited
 * string. MapTag() splits most fields on the caller's whole separator list, which absorbs this
 * along the way. A field holding names it splits on this and the delimiters that cannot occur
 * inside one, because that list holds "/" and "&" and in an artist either is as likely inside a
 * name as between two - AC/DC, Simon & Garfunkel. So join with exactly this: it is the spacing
 * that tells the delimiter from the name.
 */
inline constexpr const char* MultiValueSeparator = " / ";

/*!
 * \brief Apply one Matroska tag to a music tag.
 * \param key Upper-cased Matroska SimpleTag name, with no level prefix.
 * \param level What the tag is about.
 * \param value The tag value.
 * \param separators Item separators used to split multi-value tags.
 * \param musicsep Separator to re-join values with, from advancedsettings.
 * \param tag Music tag to write to. Unknown keys leave it untouched.
 */
void MapTag(const std::string& key,
            const std::string& value,
            TagLevel level,
            const std::vector<std::string>& separators,
            const std::string& musicsep,
            CMusicInfoTag& tag);

/*!
 * \brief Whether a name is one whose repeats are values of one tag rather than replacements.
 *
 * The Matroska spec writes several values as the same SimpleTag repeated, while Kodi holds them in
 * one delimited string. A reader that keeps repeats has to know which names to concatenate, and it
 * has to agree with MapTag() on what a name is: every spelling MapTag() accepts for one of these
 * fields answers true here, or the same tag would behave differently spelt two ways.
 *
 * \param name Upper-cased Matroska SimpleTag name, without any ALBUM/ prefix.
 */
bool HoldsSeveralValues(const std::string& name);

} // namespace MatroskaTagMapping
} // namespace MUSIC_INFO
