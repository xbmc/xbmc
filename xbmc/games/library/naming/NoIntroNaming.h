/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/library/GameLibraryTypes.h"

#include <optional>
#include <string>
#include <string_view>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 * \brief Read the tags of a name written the No-Intro way
 *
 * No-Intro's convention, last revised 2007-10-30, is published at
 * https://wiki.no-intro.org/index.php?title=Naming_Convention and writes
 *
 *   Title (Region) (Languages) (Version) (Status) (Additional) [b]
 *
 * Redump names discs the same way, adding "(Disc 1)". The convention gives
 * its lists of regions and special tags as examples only; the rest of what is
 * read here is the vocabulary No-Intro's and Redump's DATs use, and says so
 * where it is.
 */
class CNoIntroNaming
{
public:
  CNoIntroNaming() = delete;

  /*!
   * \brief The region a tag's part names, or nothing
   *
   * \param region As written, in any case: "USA", "Europe", "World"
   * \return The region; an empty one for "Unknown", which a set writes when
   *         it doesn't know
   */
  static std::optional<GameRegion> Region(std::string_view region);

  /*!
   * \brief The name No-Intro writes for a region, the reverse of Region()
   *
   * \return "USA", "Europe", "United Kingdom", or empty for a region the
   *         convention doesn't name
   */
  static std::string_view Name(const GameRegion& region);

  /*!
   * \brief Whether a code is a language as No-Intro writes it: "En", "Ja"
   *
   * The codes are ISO 639-1's, with a capital first letter.
   */
  static bool IsLanguage(std::string_view code);

  /*!
   * \brief Whether a tag gives the version or revision: "Rev 1", "v1.1"
   */
  static bool IsRevision(std::string_view tag);

  /*!
   * \brief How finished the release was: "Beta 2", "Proto", "Demo", "Kiosk"
   */
  static std::optional<ReleaseStatus> Status(std::string_view tag);

  /*!
   * \brief Who published the release: "Unl", "Pirate", "Aftermarket", "Homebrew"
   */
  static std::optional<Licence> LicenceOf(std::string_view tag);

  /*!
   * \brief Read which disc of a set this is: "Disc 2 of 3", "Disk B", "Side A"
   *
   * \param[out] disc The disc's number, 1 for "A"
   * \param[out] discs How many there are, 0 where the tag doesn't say
   */
  static bool ParseDisc(std::string_view tag, int& disc, int& discs);

  /*!
   * \brief Whether a tag says this is another dump of the release: "Alt", "Alt 2"
   */
  static bool IsAlternate(std::string_view tag);

  /*!
   * \brief Whether a tag only describes what the cartridge works with
   *
   * "(SGB Enhanced, GB Compatible)", "(Rumble Version)", "(NP)": nothing that
   * tells one release from another, or credits who made it.
   */
  static bool IsFeatureTag(std::string_view tag);

  /*!
   * \brief Whether a bracketed flag marks a system's BIOS: "[BIOS]"
   */
  static bool IsBiosFlag(std::string_view flag);

  /*!
   * \brief Whether a word is an article the conventions move to the end of a
   *        title: "Legend of Zelda, The"
   *
   * \param word The word in any case
   */
  static bool IsArticle(std::string_view word);
};
} // namespace GAME
} // namespace KODI
