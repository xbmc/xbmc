/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace XFILE
{
/*! \brief What a disc says a playlist holds, where it says anything. */
enum class PlaylistRole : uint8_t
{
  UNKNOWN,
  FEATURE, //!< the whole movie, or an edition of it
  EPISODE, //!< one episode of a series
  SPECIAL, //!< an extra
  FRONT_MATTER, //!< a studio ident, warning or certificate shown before anything is played
  MENU //!< a menu background or transition rather than content
};

constexpr std::string_view GetPlaylistRoleName(PlaylistRole role)
{
  switch (role)
  {
    case PlaylistRole::FEATURE:
      return "feature";
    case PlaylistRole::EPISODE:
      return "episode";
    case PlaylistRole::SPECIAL:
      return "special";
    case PlaylistRole::FRONT_MATTER:
      return "front matter";
    case PlaylistRole::MENU:
      return "menu";
    default:
      return "unknown";
  }
}

/*! \brief What a disc says about one of its playlists. */
struct PlaylistHint
{
  PlaylistRole role{PlaylistRole::UNKNOWN};

  //! What the disc calls the playlist, shown alongside its number in a listing
  std::string name;

  //! What the playlist holds, in words, for comparison with a scraped title. Empty where the disc
  //! offers nothing comparable.
  std::string title;

  //! Where the disc numbers its episodes, the number it gives this one. The numbering is the
  //! disc's own and need not agree with the season or episode number a scraper would use.
  std::optional<unsigned int> ordinal;

  //! Whether this is the plain presentation of what it holds, rather than the same content
  //! dubbed, audio described or otherwise presented differently
  bool basePresentation{true};
};

using PlaylistHintMap = std::map<unsigned int, PlaylistHint>;

/*!
 \brief A source of hints about what a disc's playlists hold.

 The disc directory helper identifies playlists by their structure - durations, clips and streams.
 A disc sometimes carries something that says outright what a playlist is, and an implementation
 of this presents that to the helper without the helper knowing where it came from or how it was
 read.
 */
class IPlaylistHints
{
public:
  virtual ~IPlaylistHints() = default;

  /*!
   \brief Whether the disc offered anything at all.
   A disc that offered hints for none of its playlists still did, so this can be true when
   GetHints() is empty.
   */
  virtual bool HasHints() const = 0;

  /*! \brief The hints, by playlist. Playlists the disc says nothing about are absent. */
  virtual const PlaylistHintMap& GetHints() const = 0;
};
} // namespace XFILE
