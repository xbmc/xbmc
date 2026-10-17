/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/library/GameLibraryTypes.h"

#include <array>
#include <string>
#include <vector>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 * \brief The age a classification from any board stands for
 *
 * Boards classify differently ("T", "B", "15", "+12 ans"), so a library is
 * browsed by one set of ages instead: PEGI's, which every board lines up
 * with closely enough for a person choosing what to play.
 */
class CGameAgeRatings
{
public:
  //! The ages games are grouped by, youngest first
  static constexpr std::array<int, 5> AGES{3, 7, 12, 16, 18};

  /*!
   * \brief The age in AGES a board's classification fits best
   *
   * A classification this doesn't know is read for the age written in it,
   * so "MA15+" or "+12 ans" from a board it has never seen still fits.
   *
   * \return The age, or 0 for one that isn't an age, such as ESRB "RP"
   */
  static int AgeFor(const std::string& board, const std::string& value);

  /*!
   * \brief The youngest of a board's classifications that covers an age
   *
   * \return The classification, e.g. "15" for BBFC and 16, or empty if the
   *         board isn't known
   */
  static std::string ClassificationFor(const std::string& board, int age);

  /*!
   * \brief The age of the first of a game's classifications that is one
   *
   * \param ratings The classifications, the board a person prefers first
   */
  static int AgeOf(const std::vector<GameAgeRating>& ratings);
};
} // namespace GAME
} // namespace KODI
