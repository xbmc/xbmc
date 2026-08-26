/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"

#include <mutex>
#include <string>
#include <vector>

namespace KODI::GAME
{

/*!
 * \brief Progress towards a single measured achievement
 */
struct AchievementProgress
{
  unsigned int id{0};
  float measuredPercent{0.0f};
  std::string measuredProgress;
};

struct AchievementInfo
{
  unsigned int id{0};
  std::string title;
  std::string description;
  std::string badgeUrl;
  std::string lockedBadgeUrl;

  /*!
   * \brief Percentage of players who have earned this achievement, or 0.0 if
   *        the rarity is unknown
   */
  float rarity{0.0f};

  /*!
   * \brief When the achievement was earned, invalid if it was not
   */
  CDateTime unlockedDate;

  unsigned int points{0};
  bool earned{false};

  /*!
   * \brief Progress towards the achievement from 0.0 to 100.0, or 0.0 if the
   *        achievement doesn't count anything
   *
   * \sa measuredProgress
   */
  float measuredPercent{0.0f};

  /*!
   * \brief Human-readable progress such as "45/100", empty if not measured
   *
   * Formatted by the add-on, which is the only side that knows what is being
   * counted.
   */
  std::string measuredProgress;
};

struct AchievementState
{
  std::string gameTitle;
  unsigned int gameId{0};
  unsigned int totalAchievements{0};
  unsigned int unlockedAchievements{0};
  std::string richPresence;
  std::vector<AchievementInfo> achievements;
  bool loaded{false};
};

/*!
 * \brief Thread-safe runtime achievement state for the current game
 *
 * This state is published by RetroPlayer and consumed by GUI info providers.
 * It is not persisted as a game setting.
 *
 * Mutating accessors are targeted rather than read-modify-write, so that
 * concurrent updates from the game thread and the GUI thread cannot clobber
 * each other.
 */
class CAchievementRuntime
{
public:
  void SetState(const AchievementState& state);
  void Clear();

  /*!
   * \brief Get a copy of the whole state, including the achievement list
   *
   * Copies every achievement and its strings. GUI info providers must use the
   * targeted
   * accessors below, which are queried on every rendered frame.
   */
  AchievementState GetState() const;

  /*!
   * \brief Mark an achievement as earned
   *
   * \param achievementId The achievement
   * \param unlockedDate When it was earned. Carried as a date rather than
   *        formatted text so the runtime stays free of locale handling and
   *        the formatting happens where it is displayed.
   * \param[out] newlyEarned True if this changed the achievement's state
   */
  AchievementState MarkEarned(unsigned int achievementId,
                              const CDateTime& unlockedDate,
                              bool& newlyEarned);
  void SetRichPresence(const std::string& richPresence);
  std::string GetRichPresence() const;

  /*!
   * \brief Update progress for the measured achievements of the current game
   *
   * IDs that don't belong to the loaded game are ignored, so a late update
   * arriving after the game changed can't corrupt the new game's list.
   */
  unsigned int SetAchievementProgress(const std::vector<AchievementProgress>& progress);

  /*!
   * \name Targeted accessors for the progress info label
   *
   * The skin queries this once per frame per control, so these read a single
   * field under the lock rather than copying the achievement list.
   */
  //@{
  unsigned int GetTotalAchievements() const;
  unsigned int GetUnlockedAchievements() const;
  //@}

private:
  mutable std::mutex m_mutex;
  AchievementState m_state;
};

} // namespace KODI::GAME
