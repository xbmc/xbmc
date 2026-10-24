/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 * \brief The codes GoodTools writes into the names of the sets it builds
 *
 * GoodTools has no published convention. Its codes are de facto, documented
 * by the GoodCodes.txt that ships with each tool and collected at
 * https://pleasuredome.miraheze.org/wiki/GoodTools. They read
 *
 *   Title (Country) [flag][flag]
 *
 * with a country such as "(U)" or "(E)" and flags such as "[!]", "[a1]" and
 * "[T+Ger]".
 */
namespace GOODTOOLS
{
//! The standard codes, written in brackets
enum class FlagType
{
  ALTERNATE_DUMP, // [a]
  BAD_DUMP, // [b]
  FIXED, // [f]
  HACK, // [h]
  OVER_DUMP, // [o]
  PIRATE, // [p]
  TRAINED, // [t]
  TRANSLATION, // [T+Ger], the newest translation
  OLD_TRANSLATION, // [T-Ger], one a newer translation replaces
  VERIFIED, // [!]
  PENDING, // [!p], a dump not yet verified
};

//! "[a1]", "[h2]", "[T+Ger1.00_Translator]"
struct Flag
{
  FlagType type{FlagType::ALTERNATE_DUMP};
  int number{0}; // [a1] is the first alternate; 0 where none is written
  std::string language; // a translation's language, ISO 639-1
};
} // namespace GOODTOOLS

/*!
 * \ingroup games
 * \brief Read the codes in a GoodTools name
 */
class CGoodToolsNaming
{
public:
  CGoodToolsNaming() = delete;

  /*!
   * \brief The country a code names, or empty
   *
   * \param code The code as GoodTools writes it: "U", "E", "UK", "As"
   * \return The name the game library gives the country: "USA", "Europe"
   */
  static std::string_view CountryName(std::string_view code);

  /*!
   * \brief Whether a tag only says how many languages there are: "(M3)"
   */
  static bool IsLanguageCount(std::string_view tag);

  /*!
   * \brief Read a flag, without its brackets: "!", "a1", "T+Ger"
   */
  static std::optional<GOODTOOLS::Flag> ParseFlag(std::string_view flag);

  /*!
   * \brief The ISO 639-1 code of a translation's language, or empty
   *
   * GoodTools names it in three letters, mostly ISO 639-2's own: "Ger", "Fre".
   */
  static std::string TranslationLanguage(std::string_view code);
};
} // namespace GAME
} // namespace KODI
