/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GameLibraryTypes.h"

#include <vector>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 *
 * \brief Which of a game's versions to offer
 *
 * The "one game, one ROM" rule: a bad dump never wins when there's another,
 * then a finished release beats a beta, prototype or hack, the player's
 * regions in order of preference, a newer revision beats an older, and a
 * verified dump beats an unverified one.
 */
class CReleasePolicy
{
public:
  explicit CReleasePolicy(std::vector<GameRegion> regionPriority);

  /*!
   * \brief The regions a player gets releases from, best first
   *
   * The regions they chose, in order, each with the wider region it belongs to,
   * a release for the whole world straight after the first, then Europe, USA
   * and Japan, the regions most games were released in.
   */
  static std::vector<GameRegion> RegionPriority(const std::vector<GameRegion>& choices);

  /*!
   * \brief The index of the release to offer, or -1 for an empty list
   */
  int PickDefault(const std::vector<GameRelease>& releases) const;

  /*!
   * \brief Whether the first release is preferred over the second
   */
  bool Prefers(const GameRelease& a, const GameRelease& b) const;

private:
  int RegionRank(const GameRelease& release) const;
  static int StatusRank(const GameRelease& release);
  static std::vector<int> RevisionRank(const GameRelease& release);
  static int DumpRank(const GameRelease& release);

  std::vector<GameRegion> m_regionPriority;
};
} // namespace GAME
} // namespace KODI
