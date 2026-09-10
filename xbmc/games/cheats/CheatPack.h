/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>

namespace KODI::GAME
{
/*!
 * \ingroup games
 *
 * \brief One cheat out of a cheat file
 */
struct Cheat
{
  //! What the cheat does, as the file describes it
  std::string description;

  //! The code, in whatever form the emulated system uses. Kodi does not read
  //! it: the game client hands it to the core, which is the only thing that
  //! knows how to decode it.
  std::string code;

  //! A fuller explanation, where the file carries one. Cheat files often put
  //! a sentence of instructions here and keep the description a short name.
  std::string longDescription;

  //! Whether the file shipped it switched on
  bool enabled{false};
};

/*!
 * \ingroup games
 *
 * \brief The cheats libretro's cheat files declare for a game
 *
 * The format is the one RetroArch writes and the libretro cheat database is
 * published in: a count, then a numbered block per cheat.
 *
 *     cheats = 2
 *
 *     cheat0_desc = "Infinite lives"
 *     cheat0_code = "SXIOPO"
 *     cheat0_enable = false
 */
class CCheatPack
{
public:
  /*!
   * \brief Read the cheats out of a cheat file
   *
   * A file that cannot be read, or that declares no cheats Kodi can pass on,
   * gives back an empty pack rather than an error: a missing or unusable cheat
   * file is an ordinary thing, not a fault.
   */
  static CCheatPack Load(const std::string& path);

  const std::vector<Cheat>& Cheats() const { return m_cheats; }
  bool IsEmpty() const { return m_cheats.empty(); }

private:
  std::vector<Cheat> m_cheats;
};
} // namespace KODI::GAME
