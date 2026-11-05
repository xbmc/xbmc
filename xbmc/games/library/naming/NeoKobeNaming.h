/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 * \brief Read the tags of the Neo Kobe collections of Japanese computer games
 *
 * The collections (PC-88, PC-98, X1, FM-7 and others) have no published
 * convention. What is read here is how their names are written, de facto:
 *
 *   Title (Credit) (Data disk) {loading instructions} [Set 2] [bad sectors]
 */
class CNeoKobeNaming
{
public:
  CNeoKobeNaming() = delete;

  //! \brief "Apploon {V1 mode, MON R G8F00}" -> "Apploon"
  static std::string RemoveLoadingInstructions(std::string_view name);

  //! \brief Whether a flag marks another dump of the same disks: "[Set 2]", "[Alt 1]"
  static bool IsAlternateSet(std::string_view flag);

  //! \brief Whether a flag marks a bad dump: "[bad]", "[bad sectors]"
  static bool IsBadDump(std::string_view flag);

  /*!
   * \brief Whether a tag names a disk the game asks for once it runs
   *
   * "(Data disk)", "(User disk)", "(Scenario disk)". TOSEC's media labels
   * name them the same way: "(Character Disk)".
   */
  static bool IsDataDisk(std::string_view tag);

  //! \brief Whether a tag names a hack: "(Kai hack)", "(Hack)"
  static bool IsHack(std::string_view tag);

  /*!
   * \brief The companies a credit tag names
   *
   * "(Doujin - Noripy)" -> "Noripy", "(D-Photon - Victor)" -> "D-Photon", "Victor"
   */
  static std::vector<std::string> Credits(std::string_view tag);

  /*!
   * \brief Whether a word says what kind of company a name is rather than
   *        which: "Soft", "Ltd", "KK"
   *
   * Not from any convention: what a credit has to be stripped of before it
   * can be compared with a catalogue's company.
   *
   * \param word The word in lower case
   */
  static bool IsCompanyTypeWord(std::string_view word);
};
} // namespace GAME
} // namespace KODI
