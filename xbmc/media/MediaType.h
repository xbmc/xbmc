/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <string_view>

#include <fmt/format.h> //! \todo remove after upgrade to libfmt >= 10.0

namespace KODI::MEDIA
{

//! \brief What a library item is.
enum class TYPE
{
  NONE,
  MUSIC,
  ARTIST,
  ALBUM,
  SONG,
  VIDEO,
  VIDEO_COLLECTION,
  MUSIC_VIDEO,
  MOVIE,
  TV_SHOW,
  SEASON,
  EPISODE,
  VIDEO_VERSION,
};

//! \brief The name a type is stored and exposed under, e.g. "movie". Empty for NONE.
const std::string& NameOf(TYPE type);

//! \brief Formats as its name, for fmt.
inline const std::string& format_as(TYPE type)
{
  return NameOf(type);
}

//! \brief The plural name, e.g. "movies", and "sets" for a video collection. Empty for NONE.
const std::string& PluralNameOf(TYPE type);

//! \brief The type \p name gives, singular or plural, in any case. NONE for any other text.
TYPE MediaTypeFromName(std::string_view name);

//! \brief The type whose stored name is exactly \p name. NONE for any other text.
TYPE MediaTypeOf(std::string_view name);

//! \brief Whether an item of this type holds other items, as an album holds songs.
bool IsContainer(TYPE type);

//! \brief The localized name as a heading, e.g. "Movie". Empty for NONE.
std::string GetCapitalLocalization(TYPE type);

} // namespace KODI::MEDIA

#if FMT_VERSION < 100000
// user-type formatter for libfmt < 10.0
//! \todo remove after libfmt upgrade
template<>
struct fmt::formatter<KODI::MEDIA::TYPE> : fmt::formatter<std::string_view>
{
  auto format(const KODI::MEDIA::TYPE& type, format_context& ctx) const
  {
    return fmt::formatter<std::string_view>::format(KODI::MEDIA::format_as(type), ctx);
  }
};
#endif
