/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <algorithm>
#include <cctype>
#include <optional>
#include <string_view>

#include <fmt/format.h>

namespace KODI::PLAYLIST
{

/*!
 * \brief The two playlists: Video and Audio. An entry's playlist is what it claims to hold.
 */
enum class Type
{
  Video,
  Audio
};
using enum Type;

/*!
 * \return The playlist an int carrying a Type names, if it names one.
 */
inline std::optional<Type> TypeFromInt(int value)
{
  if (value != static_cast<int>(Video) && value != static_cast<int>(Audio))
    return std::nullopt;
  return static_cast<Type>(value);
}

//! A playlist's name in the interfaces: "video" or "audio".
constexpr std::string_view NameOf(Type type)
{
  return type == Video ? "video" : "audio";
}

//! The name the interfaces give the slideshow's list, which is not a Type.
constexpr std::string_view PICTURE_NAME = "picture";

/*!
 * \return The playlist a name names, ignoring case: "video", or "audio" or "music".
 */
inline std::optional<Type> TypeFromName(std::string_view name)
{
  const auto is = [name](std::string_view word)
  {
    return std::ranges::equal(name, word, [](char a, char b)
                              { return std::tolower(static_cast<unsigned char>(a)) == b; });
  };
  if (is("video"))
    return Video;
  if (is("audio") || is("music"))
    return Audio;
  return std::nullopt;
}

/*!
 * \brief The three states of the repeat button, composed from the playlist's repeat of the
 * current entry and its wrap.
 */
enum class Repeat
{
  Off,
  One,
  All
};

//! A repeat state's name in the interfaces: "off", "one" or "all".
constexpr std::string_view NameOf(Repeat repeat)
{
  switch (repeat)
  {
    case Repeat::One:
      return "one";
    case Repeat::All:
      return "all";
    case Repeat::Off:
      break;
  }
  return "off";
}

//! \return The repeat state a name names, if it names one.
inline std::optional<Repeat> RepeatFromName(std::string_view name)
{
  for (const Repeat repeat : {Repeat::Off, Repeat::One, Repeat::All})
    if (name == NameOf(repeat))
      return repeat;
  return std::nullopt;
}

//! Why a play that was asked for did not happen.
enum class FailReason
{
  //! No player could play it, or the player refused it.
  Unplayable,
  //! A plugin did not return a playable item.
  Unresolved,
  //! The master or media source lock refused it.
  Locked,
  //! The player reported an error once playback had started.
  Error
};

//! A fail reason's name in the interfaces: "unplayable", "unresolved", "locked" or "error".
constexpr std::string_view NameOf(FailReason reason)
{
  switch (reason)
  {
    case FailReason::Unplayable:
      return "unplayable";
    case FailReason::Unresolved:
      return "unresolved";
    case FailReason::Locked:
      return "locked";
    case FailReason::Error:
      break;
  }
  return "error";
}

/*!
 * \brief Identifies one entry of one playlist. Never reused within that playlist, so a stale id
 * resolves to nothing rather than to a different entry.
 */
using EntryId = unsigned int;
constexpr EntryId NO_ENTRY = 0;

/*!
 * \brief Why the playlist is moving on. A user skip does not repeat the current entry, so a
 * repeating entry can always be left; repeat stays on for whatever becomes current.
 */
enum class Advance
{
  Automatic,
  User
};

/*!
 * \brief What the playlist does once nothing follows the last entry in play order.
 */
enum class Wrap
{
  None,
  ToStart
};

enum class Id : int
{
  TYPE_NONE = -1,
  TYPE_MUSIC = 0,
  TYPE_VIDEO = 1,
  TYPE_PICTURE = 2,
  TYPE_GAME = 3
};

/*!
 * \brief Manages playlist playing.
 */
enum class RepeatState
{
  NONE,
  ONE,
  ALL
};

enum class ExcludeUsedPlaylists : bool
{
  DONT_EXCLUDE_USED_PLAYLISTS,
  EXCLUDE_USED_PLAYLISTS
};

} // namespace KODI::PLAYLIST

template<>
struct fmt::formatter<KODI::PLAYLIST::Type> : fmt::formatter<std::string_view>
{
  template<typename FormatContext>
  constexpr auto format(KODI::PLAYLIST::Type type, FormatContext& ctx) const
  {
    return fmt::formatter<std::string_view>::format(KODI::PLAYLIST::NameOf(type), ctx);
  }
};
