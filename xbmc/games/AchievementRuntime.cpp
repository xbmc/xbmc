/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AchievementRuntime.h"

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

AchievementState CAchievementRuntime::MarkEarned(unsigned int achievementId, bool& newlyEarned)
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

std::string CAchievementRuntime::GetRichPresence() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_state.richPresence;
}
