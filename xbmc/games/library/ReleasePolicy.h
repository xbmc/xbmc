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
#include <vector>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 *
 * \brief Which of a game's releases plays by default
 *
 * The "one game, one ROM" rule: the player's regions in order of
 * preference, then whether a finished release beats a beta or prototype, a
 * newer revision beats an older, and a verified dump beats an unverified
 * one.
 */
class CReleasePolicy
{
public:
  CReleasePolicy(std::vector<std::string> regionPriority,
                 bool preferRetail,
                 bool preferNewestRevision,
                 bool preferVerified);

  /*!
   * \brief The regions a player in one region gets releases from, best first
   *
   * Their own region, the wider region it belongs to, the whole world, then
   * Europe, USA and Japan, the regions most games were released in.
   */
  static std::vector<std::string> RegionPriority(const std::string& region);

  /*!
   * \brief The release that should play, or -1 for an empty list
   */
  int PickDefault(const std::vector<GameRelease>& releases) const;

  /*!
   * \brief Whether the first release is preferred over the second
   */
  bool Prefers(const GameRelease& a, const GameRelease& b) const;

  const std::vector<std::string>& GetRegionPriority() const { return m_regionPriority; }

private:
  int RegionRank(const GameRelease& release) const;
  static int StatusRank(const GameRelease& release);
  static int RevisionRank(const GameRelease& release);
  static int DumpRank(const GameRelease& release);

  std::vector<std::string> m_regionPriority;
  bool m_preferRetail{true};
  bool m_preferNewestRevision{true};
  bool m_preferVerified{true};
};
} // namespace GAME
} // namespace KODI
