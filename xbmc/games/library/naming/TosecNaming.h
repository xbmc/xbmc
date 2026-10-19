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
#include <vector>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 * \brief The fields of a name written to the TOSEC Naming Convention
 *
 * The convention, revision 2015-03-23, is published at
 * https://www.tosecdev.org/tosec-naming-convention and writes
 *
 *   Title version (demo) (date)(publisher)(system)(video)(country)(language)
 *   (copyright)(devstatus)(media type)(media label)
 *   [cr][f][h][m][p][t][tr][o][u][v][b][a][!][more info]
 *
 * The title, date and publisher are always there; every other field is
 * optional and, when present, comes in that order.
 */
namespace TOSEC
{
enum class Demo
{
  NONE,
  DEMO, // demo
  KIOSK, // demo-kiosk
  PLAYABLE, // demo-playable
  ROLLING, // demo-rolling
  SLIDESHOW, // demo-slideshow
};

enum class Copyright
{
  NONE,
  CARDWARE, // CW
  CARDWARE_REGISTERED, // CW-R
  FREEWARE, // FW
  GIFTWARE, // GW
  GIFTWARE_REGISTERED, // GW-R
  LICENCEWARE, // LW
  PUBLIC_DOMAIN, // PD
  SHAREWARE, // SW
  SHAREWARE_REGISTERED, // SW-R
};

enum class DevelopmentStatus
{
  RELEASE,
  ALPHA, // alpha
  BETA, // beta
  PREVIEW, // preview
  PRE_RELEASE, // pre-release
  PROTO, // proto
};

//! The dump info flags, in the order the convention writes them
enum class DumpFlagType
{
  CRACKED, // [cr]
  FIXED, // [f]
  HACKED, // [h]
  MODIFIED, // [m]
  PIRATED, // [p]
  TRAINED, // [t]
  TRANSLATED, // [tr]
  OVER_DUMP, // [o]
  UNDER_DUMP, // [u]
  VIRUS, // [v]
  BAD_DUMP, // [b]
  ALTERNATE_DUMP, // [a]
  VERIFIED, // [!]
};

/*!
 * \brief A date as written, "1986", "198x", "1986-06-21" or "19xx-12-2x"
 *
 * A part the convention leaves unknown with an "x" is 0.
 */
struct Date
{
  int year{0};
  int month{0};
  int day{0};
};

//! "(Disk 1 of 2)", "(Side B)", "(Tape 2 of 2 Side B)", "(Part 1-2 of 3)"
struct Media
{
  std::string type; // Disc, Disk, File, Part, Side or Tape
  std::string number; // "1", "06", "1-2" or, for a side, "A"
  std::string total;
  std::string side; // the side a "Tape 2 of 2 Side B" names after its number
};

//! "[a2]", "[cr PDX]", "[t +2 Group]", "[tr de-partial Translator]"
struct DumpFlag
{
  DumpFlagType type{DumpFlagType::ALTERNATE_DUMP};
  int number{1}; // [a2] is the second alternate
  std::string detail; // the rest of the flag: a group, a fix, a translation's language
};

struct Name
{
  std::string title;
  std::string version; // "v1.03b", "Rev 1"
  Demo demo{Demo::NONE};
  Date date;
  std::vector<std::string> publishers; // none for "(-)"
  std::string system; // "A1200", "+2a"
  std::string video; // "PAL", "NTSC"
  std::vector<std::string> countries; // as written: "DE", "EU"
  std::vector<std::string> languages; // ISO 639-1: "de"
  int languageCount{0}; // "(M3)" says how many without naming them
  Copyright copyright{Copyright::NONE};
  DevelopmentStatus status{DevelopmentStatus::RELEASE};
  Media media;
  std::string mediaLabel; // "Character Disk", "Program", "Bonus Disc"
  std::vector<DumpFlag> dumpFlags;
  std::vector<std::string> moreInfo; // "[aka House of TOSEC]", "[Req TRS-DOS]"
  std::vector<std::string> unknownTags; // parenthesised tags no field takes
};
} // namespace TOSEC

/*!
 * \ingroup games
 * \brief Read a name written to the TOSEC Naming Convention
 */
class CTosecNaming
{
public:
  CTosecNaming() = delete;

  /*!
   * \brief Parse a name, without its extension
   *
   * A name is TOSEC's when its first parenthesised field is a date written
   * against the publisher's, "(1988)(Elite)". A name cut short inside a field,
   * as extracted sets are where they hit a filename length limit, keeps the
   * fields before it.
   *
   * \return The fields, or nothing if the name isn't written this way
   */
  static std::optional<TOSEC::Name> Parse(std::string_view name);

  /*!
   * \brief Read a date field, "1986", "198x", "1986-06-21", "19xx-12-2x"
   */
  static std::optional<TOSEC::Date> ParseDate(std::string_view field);

  /*!
   * \brief The region a country code names, or nothing
   *
   * The codes are ISO 3166-1 alpha-2 in capitals, apart from EU for Europe
   * and AS for Asia, and UK, which sets write for GB. A code in lower case is
   * a language.
   */
  static std::optional<GameRegion> Region(std::string_view code);

  /*!
   * \brief Whether a code is one of ISO 639-1's, as TOSEC writes languages
   *
   * \param code A code in lower case, "de"
   */
  static bool IsLanguage(std::string_view code);
};
} // namespace GAME
} // namespace KODI
