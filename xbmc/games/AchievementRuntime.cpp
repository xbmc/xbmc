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

std::string CAchievementRuntime::GetTrackedAchievementTitle() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  const ProgressIndicator* best = BestProgressIndicator();
  return best != nullptr ? best->title : std::string{};
}

std::string CAchievementRuntime::GetTrackedAchievementProgress() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  const ProgressIndicator* best = BestProgressIndicator();
  return best != nullptr ? best->measuredProgress : std::string{};
}

std::string CAchievementRuntime::GetTrackedAchievementBadge() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  const ProgressIndicator* best = BestProgressIndicator();
  return best != nullptr ? best->badgeUrl : std::string{};
}

float CAchievementRuntime::GetTrackedAchievementPercent() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  const ProgressIndicator* best = BestProgressIndicator();
  return best != nullptr ? best->measuredPercent : 0.0f;
}

const ProgressIndicator* CAchievementRuntime::BestProgressIndicator() const
{
  // Whichever is closest to being earned, so the corner holds the achievement
  // about to unlock rather than whichever of them ticked last.
  const auto& indicators = m_state.progressIndicators;
  const auto best = std::max_element(indicators.begin(), indicators.end(),
                                     [](const ProgressIndicator& a, const ProgressIndicator& b)
                                     { return a.measuredPercent < b.measuredPercent; });

  return best != indicators.end() ? &(*best) : nullptr;
}

void CAchievementRuntime::SetProgressIndicator(const ProgressIndicator& indicator, bool active)
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto& indicators = m_state.progressIndicators;
    auto it = std::find_if(indicators.begin(), indicators.end(),
                           [&indicator](const ProgressIndicator& existing)
                           { return existing.id == indicator.id; });

    if (active)
    {
      // Show and update are the same thing here: one already counting is given
      // its new value rather than added twice
      if (it != indicators.end())
        *it = indicator;
      else
        indicators.emplace_back(indicator);
    }
    else if (indicator.id == 0)
    {
      // The runtime sends no achievement with a hide, so an id of zero means
      // everything currently counting has stopped
      indicators.clear();
    }
    else if (it != indicators.end())
    {
      indicators.erase(it);
    }
  }
}

std::string CAchievementRuntime::GetChallengeAchievementTitle() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_state.challenges.empty() ? std::string{} : m_state.challenges.front().title;
}

std::string CAchievementRuntime::GetChallengeAchievementBadge() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_state.challenges.empty() ? std::string{} : m_state.challenges.front().badgeUrl;
}

void CAchievementRuntime::SetChallengeIndicator(const ChallengeIndicator& indicator, bool active)
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto& challenges = m_state.challenges;
    auto it = std::find_if(challenges.begin(), challenges.end(),
                           [&indicator](const ChallengeIndicator& existing)
                           { return existing.id == indicator.id; });

    if (active)
    {
      // The runtime can re-announce an attempt that is already showing
      if (it == challenges.end())
        challenges.emplace_back(indicator);
    }
    else if (it != challenges.end())
    {
      challenges.erase(it);
    }
  }
}

bool CAchievementRuntime::HasIndicators() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return !m_state.challenges.empty() || !m_state.progressIndicators.empty();
}


