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

  // The leaderboards belong to the game that just went away too, and a stale
  // selection would point the entries dialog at nothing
  m_leaderboards = LeaderboardState{};
  m_trackers.clear();
  m_selectedLeaderboard = 0;
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

void CAchievementRuntime::SetChallenge(const AchievementChallenge& challenge, bool active)
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = std::find_if(m_state.challenges.begin(), m_state.challenges.end(),
                           [&challenge](const AchievementChallenge& existing)
                           { return existing.id == challenge.id; });

    if (active)
    {
      // The runtime can re-announce an attempt that is already showing
      if (it == m_state.challenges.end())
        m_state.challenges.emplace_back(challenge);
    }
    else if (it != m_state.challenges.end())
    {
      m_state.challenges.erase(it);
    }
  }

  NotifyIndicatorsChanged();
}

std::vector<AchievementChallenge> CAchievementRuntime::GetChallenges() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_state.challenges;
}

void CAchievementRuntime::SetProgressIndicator(const AchievementProgressIndicator& indicator,
                                               bool active)
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto& indicators = m_state.progressIndicators;
    auto it = std::find_if(indicators.begin(), indicators.end(),
                           [&indicator](const AchievementProgressIndicator& existing)
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

  NotifyIndicatorsChanged();
}

AchievementProgressIndicator CAchievementRuntime::GetProgressIndicator() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  const auto& indicators = m_state.progressIndicators;

  // A game can count several things at once, and a corner indicator has room
  // for one: the closest to being earned. One with no title is skipped rather
  // than chosen, since a skin draws the title and would show an empty corner.
  AchievementProgressIndicator best;
  for (const AchievementProgressIndicator& indicator : indicators)
  {
    if (indicator.title.empty())
      continue;

    if (best.id == 0 || indicator.measuredPercent > best.measuredPercent)
      best = indicator;
  }

  return best;
}

AchievementChallenge CAchievementRuntime::GetShownChallenge() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  const auto shown =
      std::find_if(m_state.challenges.begin(), m_state.challenges.end(),
                   [](const AchievementChallenge& challenge) { return !challenge.title.empty(); });

  return (shown != m_state.challenges.end()) ? *shown : AchievementChallenge{};
}

LeaderboardTracker CAchievementRuntime::GetShownLeaderboardTracker() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  const auto shown =
      std::find_if(m_trackers.begin(), m_trackers.end(),
                   [](const LeaderboardTracker& tracker) { return !tracker.display.empty(); });

  return (shown != m_trackers.end()) ? *shown : LeaderboardTracker{};
}

void CAchievementRuntime::SetLeaderboardTracker(const LeaderboardTracker& tracker, bool active)
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = std::find_if(m_trackers.begin(), m_trackers.end(),
                           [&tracker](const LeaderboardTracker& existing)
                           { return existing.id == tracker.id; });

    if (active)
    {
      // Show and update are the same thing here: an attempt already on screen is
      // given its new value rather than added twice
      if (it != m_trackers.end())
        it->display = tracker.display;
      else
        m_trackers.emplace_back(tracker);
    }
    else if (it != m_trackers.end())
    {
      m_trackers.erase(it);
    }
  }

  NotifyIndicatorsChanged();
}

std::vector<LeaderboardTracker> CAchievementRuntime::GetLeaderboardTrackers() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_trackers;
}

void CAchievementRuntime::SetLeaderboardState(const LeaderboardState& state)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_leaderboards = state;
}

LeaderboardState CAchievementRuntime::GetLeaderboardState() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_leaderboards;
}

bool CAchievementRuntime::SetLeaderboardEntries(unsigned int leaderboardId,
                                                unsigned int accountGeneration,
                                                const std::vector<LeaderboardEntry>& entries)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if (accountGeneration != m_accountGeneration)
    return false;

  for (LeaderboardInfo& leaderboard : m_leaderboards.leaderboards)
  {
    if (leaderboard.id != leaderboardId)
      continue;

    leaderboard.entries = entries;
    leaderboard.entriesLoaded = true;
    return true;
  }

  // The game changed while the standings were being fetched
  return false;
}

void CAchievementRuntime::ForgetPlayerLeaderboardData()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  for (LeaderboardInfo& leaderboard : m_leaderboards.leaderboards)
  {
    leaderboard.playerRank = 0;
    leaderboard.playerScore.clear();
    leaderboard.entries.clear();
    leaderboard.entriesLoaded = false;
    leaderboard.standingsLoaded = false;
  }

  ++m_accountGeneration;
}

unsigned int CAchievementRuntime::GetAccountGeneration() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_accountGeneration;
}

bool CAchievementRuntime::SetLeaderboardSummary(unsigned int leaderboardId,
                                                unsigned int accountGeneration,
                                                const LeaderboardSummary& summary)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  // Checked against the write rather than before it, so the account cannot
  // move in between
  if (accountGeneration != m_accountGeneration)
    return false;

  for (LeaderboardInfo& leaderboard : m_leaderboards.leaderboards)
  {
    if (leaderboard.id != leaderboardId)
      continue;

    leaderboard.totalEntries = summary.totalEntries;
    leaderboard.playerRank = summary.playerRank;
    leaderboard.playerScore = summary.playerScore;
    leaderboard.topUsername = summary.topUsername;
    leaderboard.topScore = summary.topScore;
    leaderboard.standingsLoaded = true;

    return true;
  }

  return false;
}

bool CAchievementRuntime::SetLeaderboardStanding(unsigned int leaderboardId,
                                                 unsigned int rank,
                                                 const std::string& score,
                                                 unsigned int totalEntries)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  for (LeaderboardInfo& leaderboard : m_leaderboards.leaderboards)
  {
    if (leaderboard.id != leaderboardId)
      continue;

    leaderboard.playerRank = rank;
    leaderboard.playerScore = score;
    if (totalEntries > 0)
      leaderboard.totalEntries = totalEntries;

    // The summary is now stale rather than fetched: a scoreboard says where the
    // player landed and nothing about who leads, so a submission taking first
    // place would leave the old leader on the list until it is asked again
    leaderboard.standingsLoaded = false;

    // The standings that were fetched no longer include this submission, and
    // guessing where it slots in would be wrong as often as right. The flag
    // goes with them: an empty list on its own does not say whether the page
    // still needs fetching.
    leaderboard.entries.clear();
    leaderboard.entriesLoaded = false;

    return true;
  }

  // The game changed between submitting and the server answering
  return false;
}

void CAchievementRuntime::SetSelectedLeaderboard(unsigned int leaderboardId)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_selectedLeaderboard = leaderboardId;
}

unsigned int CAchievementRuntime::GetSelectedLeaderboard() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_selectedLeaderboard;
}

void CAchievementRuntime::SetIndicatorCallback(std::function<void()> callback)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_indicatorCallback = std::move(callback);
}

void CAchievementRuntime::NotifyIndicatorsChanged()
{
  std::function<void()> callback;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    callback = m_indicatorCallback;
  }

  // Outside the lock: what this calls reads the state straight back
  if (callback)
    callback();
}
