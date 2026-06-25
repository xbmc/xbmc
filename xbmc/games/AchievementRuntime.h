/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <mutex>
#include <string>
#include <vector>

namespace KODI::GAME
{

struct AchievementInfo
{
  unsigned int id{0};
  std::string title;
  std::string description;
  std::string badgeUrl;
  std::string lockedBadgeUrl;
  std::string rarity;
  std::string unlockedDate;
  unsigned int points{0};
  bool earned{false};
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
 */
class CAchievementRuntime
{
public:
  void SetState(const AchievementState& state);
  void Clear();
  AchievementState GetState() const;
  AchievementState MarkEarned(unsigned int achievementId, bool& newlyEarned);
  void SetRichPresence(const std::string& richPresence);
  std::string GetRichPresence() const;

private:
  mutable std::mutex m_mutex;
  AchievementState m_state;
};

} // namespace KODI::GAME
