/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AchievementRuntime.h"

#include <algorithm>

using namespace KODI::GAME;

void CAchievementRuntime::SetState(const AchievementState& state)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_state = state;
}

void CAchievementRuntime::Clear()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_state = AchievementState{};
}

AchievementState CAchievementRuntime::GetState() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_state;
}

AchievementState CAchievementRuntime::MarkEarned(unsigned int achievementId,
                                                 const CDateTime& unlockedDate,
                                                 bool& newlyEarned)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  newlyEarned = false;
  for (AchievementInfo& info : m_state.achievements)
  {
    if (info.id == achievementId)
    {
      if (!info.earned)
      {
        info.earned = true;
        info.unlockedDate = unlockedDate;

        // Left behind, this would draw a part-filled bar on a completed row
        info.measuredPercent = 0.0f;
        info.measuredProgress.clear();

        ++m_state.unlockedAchievements;
        newlyEarned = true;
      }
      break;
    }
  }
  return m_state;
}

void CAchievementRuntime::SetRichPresence(const std::string& richPresence)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_state.richPresence = richPresence;
}

unsigned int CAchievementRuntime::SetAchievementProgress(
    const std::vector<AchievementProgress>& progress)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  unsigned int applied = 0;

  for (const AchievementProgress& update : progress)
  {
    auto it = std::find_if(m_state.achievements.begin(), m_state.achievements.end(),
                           [&update](const AchievementInfo& achievement)
                           { return achievement.id == update.id; });

    // An update for an achievement we don't know about belongs to a game that
    // has since been unloaded
    if (it == m_state.achievements.end())
      continue;

    it->measuredPercent = update.measuredPercent;
    it->measuredProgress = update.measuredProgress;
    ++applied;
  }

  return applied;
}

std::string CAchievementRuntime::GetRichPresence() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_state.richPresence;
}

unsigned int CAchievementRuntime::GetTotalAchievements() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_state.totalAchievements;
}

unsigned int CAchievementRuntime::GetUnlockedAchievements() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_state.unlockedAchievements;
}
