/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GameLibraryTypes.h"

#include <cstddef>
#include <vector>

namespace KODI
{
namespace GAME
{
class CReleasePolicy;
struct ParsedGameName;

/*!
 * \ingroup games
 *
 * \brief Tells which file names are versions of the same game
 */
class CGameVersions
{
public:
  /*!
   * \brief The version a file name describes, as the release policy sees it
   */
  static GameRelease Release(const ParsedGameName& name);

  /*!
   * \brief Group the names that give the same title, best version first
   *
   * Each disc of a game is grouped on its own, and a name without a title or
   * for a game's data disk isn't grouped. Only groups of two or more are
   * returned, as indices into names.
   */
  static std::vector<std::vector<size_t>> Group(const std::vector<ParsedGameName>& names,
                                                const CReleasePolicy& policy);
};
} // namespace GAME
} // namespace KODI
