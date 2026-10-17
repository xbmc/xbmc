/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GameLibraryTypes.h"

#include <string>
#include <string_view>
#include <vector>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 * \brief What a ROM's file name says about it
 */
struct ParsedGameName
{
  std::string title; // as written, tags removed: "Legend of Zelda, The"
  std::string displayTitle; // article restored: "The Legend of Zelda"
  std::vector<std::string> regions; // full names, e.g. "USA", "Europe"
  std::vector<std::string> languages; // lower-case ISO 639-1
  std::string revision;
  ReleaseStatus status{ReleaseStatus::RETAIL};
  Licence licence{Licence::LICENSED};
  bool alternate{false};
  bool bad{false};
  bool verified{false};
  bool hack{false};
  bool translated{false}; // a fan translation
  std::string translation; // the language it is translated into, when the tag names one
  int disc{0};
  int discs{0};
  bool dataDisk{false}; // one the game asks for once it runs: user, data, scenario...
  int year{0};
  std::string publisher; // TOSEC names the publisher in the file name
  bool tosec{false}; // named the TOSEC way: a date, then the publisher
  std::vector<std::string> unknownTags;
};

/*!
 * \ingroup games
 *
 * \brief Read the conventions ROM sets name their files by
 *
 * No-Intro and Redump write "Title (Region) (Languages) (Rev 1) (Beta)",
 * TOSEC writes "Title (1985)(Publisher)[cr Group]", and GoodTools writes
 * "Title [!]" and "Title [h1]". All three are read; everything recognised
 * becomes a field and what is left is the title. Nothing is guessed: a tag
 * that is not understood is kept as it is, so a later version can read it.
 */
class CGameNameParser
{
public:
  /*!
   * \brief Parse a file or folder name
   *
   * \param fileName The base name, with or without an extension
   * \param hasExtension False for a folder's name, whose last dot is part of
   *        the title: "G.R" is a game, not "G" with an extension
   */
  static ParsedGameName Parse(std::string_view fileName, bool hasExtension = true);

  /*!
   * \brief Turn a WHDLoad install name into something a catalogue can match
   *
   * Amiga collections are named for the WHDLoad slave rather than the game:
   * "SecretOfMonkeyIsland_v3.4_1625". The version, the hardware flags and the
   * slave number say nothing about which game it is, and the words are run
   * together, so nothing matches by name until they are separated.
   *
   * \return The spaced title, or an empty string if this is not such a name
   */
  static std::string ParseWhdLoadName(std::string_view fileName);

  /*!
   * \brief The companies a name credits in a tag of its own
   *
   * Collections of Japanese computer games give one tag the parser doesn't
   * otherwise know, sometimes marked as doujin: "Tetris (Doujin - Noripy)",
   * "Teki wa Kaizoku (D-Photon - Victor)". A TOSEC name credits nothing: its
   * publisher may be a budget label reissuing the game, and its other fields
   * say what the release runs on.
   */
  static std::vector<std::string> Credits(const ParsedGameName& name);

  /*!
   * \brief Whether two credited names could be one company
   *
   * They share a word, or one's words run together inside the other's
   * ("Micro Cabin", "Microcabin"). Words like "Soft" say what kind of company
   * it is rather than which.
   */
  static bool SameCompany(std::string_view a, std::string_view b);

  /*!
   * \brief "Legend of Zelda, The" -> "The Legend of Zelda"
   */
  static std::string DisplayTitle(std::string_view title);

  /*!
   * \brief The region a file name's tag names, written in full, or empty
   *
   * "USA", "Europe", "Japan", "World" and the rest, as No-Intro writes them,
   * whatever spelling or abbreviation the tag used.
   */
  static std::string RegionName(std::string_view region);
};
} // namespace GAME
} // namespace KODI
