/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace KODI
{
namespace GAME
{
class CGameInfoTag;
struct ParsedGameName;

/*!
 * \ingroup games
 *
 * \brief What a game file's name tells the player
 */
class CGameNameInfo
{
public:
  /*!
   * \brief Fill in a game's info from its file name
   *
   * "Legend of Zelda, The - A Link to the Past (USA, Europe).sfc" is "The Legend
   * of Zelda - A Link to the Past", released in the USA and Europe.
   */
  static void SetInfoTag(CGameInfoTag& tag, const ParsedGameName& name);

  /*!
   * \brief What sets one version of a game apart, in words: "Hack · Rev 1 · USA"
   */
  static std::string VersionLabel(const ParsedGameName& name);
};
} // namespace GAME
} // namespace KODI
