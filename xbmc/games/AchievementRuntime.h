/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"

#include <functional>
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

/*!
 * \brief An achievement the player is currently attempting
 *
 * RetroAchievements calls these "challenge indicators". They appear while an
 * achievement's trigger is primed and disappear when the attempt ends, whether
 * or not it was earned.
 */
struct AchievementChallenge
{
  unsigned int id{0};
  std::string title;
  std::string badgeUrl;
};

/*!
 * \brief One row of a leaderboard's standings
 */
struct LeaderboardEntry
{
  unsigned int rank{0};
  std::string username;

  //! Already formatted by the server for this leaderboard's unit - seconds,
  //! points, frames - so it is shown as given rather than interpreted
  std::string score;

  //! When it was submitted, as sent. Kept raw so that both the date and how
  //! long ago it was can be worked out at display time, in the player's own
  //! locale rather than the one that fetched it.
  std::time_t submitted{0};

  //! True for the signed-in player's own row, so the skin can pick it out
  bool isPlayer{false};
};

/*!
 * \brief A scored challenge within a game
 */
struct LeaderboardInfo
{
  unsigned int id{0};
  std::string title;
  std::string description;

  //! How the server encodes this leaderboard's values - TIME, SCORE, FIXED2
  //! and so on. Needed because the scores arrive as raw integers and only
  //! this says whether 9000 means 9000 points or 2:30.00.
  std::string format;

  //! Whether a smaller value ranks higher, as it does for a time trial
  bool lowerIsBetter{false};

  unsigned int totalEntries{0};

  //! Where the player stands, or 0 if they have never submitted
  unsigned int playerRank{0};
  std::string playerScore;

  std::string topUsername;
  std::string topScore;

  //! Filled in only once the standings have been fetched for this one, which
  //! happens when the player opens it rather than when the game loads
  std::vector<LeaderboardEntry> entries;

  //! Whether the standings have been fetched. An empty list means nothing on
  //! its own: a board nobody has entered and one whose request failed both
  //! arrive with none.
  bool entriesLoaded{false};

  //! Whether the summary above was fetched for the account now signed in. The
  //! entry count is the game's and outlives a sign-out, so it cannot say this.
  bool standingsLoaded{false};
};

/*!
 * \brief How far along a measured achievement is
 *
 * Some achievements are measured rather than simply locked or unlocked -
 * "collect 180 rings" - and the runtime reports progress while the player works
 * towards one. Distinct from AchievementChallenge, which says an attempt is
 * live without saying how far through it is.
 */
struct AchievementProgressIndicator
{
  unsigned int id{0};
  std::string title;
  std::string badgeUrl;

  //! How far along, already formatted by the client - "13/180" - so it is shown
  //! as given rather than interpreted
  std::string measuredProgress;

  //! The same as a percentage, so a skin can draw a bar without parsing the text
  float measuredPercent{0.0f};
};

/*!
 * \brief A leaderboard attempt in progress
 *
 * While an attempt runs the client publishes a live value - the lap time so
 * far, the score so far - for the skin to show in a corner.
 */
struct LeaderboardTracker
{
  //! Identifies the tracker, not the leaderboard. Two leaderboards measuring
  //! the same thing share one, so this cannot be used to look a leaderboard up.
  unsigned int id{0};

  //! The value as it stands, already formatted by the client for this
  //! leaderboard's unit, so it is shown as given rather than interpreted
  std::string display;
};

/*!
 * \brief The leaderboards of the loaded game
 */
/*!
 * \brief What a standings request answers with for one leaderboard
 */
struct LeaderboardSummary
{
  unsigned int totalEntries{0};
  unsigned int playerRank{0};
  std::string playerScore;
  std::string topUsername;
  std::string topScore;
};

struct LeaderboardState
{
  std::string gameTitle;
  std::vector<LeaderboardInfo> leaderboards;
  bool loaded{false};
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

  /*!
   * \brief Achievements the player is currently attempting
   *
   * Usually empty, and rarely more than one or two at a time.
   */
  std::vector<AchievementChallenge> challenges;

  /*!
   * \brief The measured achievements being worked towards
   *
   * More than one can be counting at once - a game will happily track rings
   * collected and time survived together - and the runtime announces each
   * separately. Kept as a set so that one does not overwrite another; which of
   * them is worth a corner indicator is decided on the way out.
   */
  std::vector<AchievementProgressIndicator> progressIndicators;
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

  /*!
   * \brief Add or remove an achievement from the list being attempted
   *
   * Indicators are published to the runtime rather than raised as
   * notifications: they update many times a second during play, which belongs
   * on screen where the skin can show and hide it, not in the notification
   * queue.
   *
   * \param challenge The achievement
   * \param active True while the attempt is live, false once it has ended
   */
  void SetChallenge(const AchievementChallenge& challenge, bool active);

  /*!
   * \brief Get the achievements currently being attempted
   */
  std::vector<AchievementChallenge> GetChallenges() const;

  /*!
   * \brief Show or hide how far along a measured achievement is
   */
  void SetProgressIndicator(const AchievementProgressIndicator& indicator, bool active);

  /*!
   * \brief The attempt worth a corner indicator, if any
   *
   * The first the skin has something to draw for. More than one at a time is
   * rare and a corner has room for one, and the add-on may send an attempt
   * with no title, which would show as an empty corner if it were chosen.
   */
  AchievementChallenge GetShownChallenge() const;

  /*!
   * \brief The leaderboard attempt worth a corner indicator, if any
   *
   * \sa GetShownChallenge()
   */
  LeaderboardTracker GetShownLeaderboardTracker() const;

  /*!
   * \brief The measured achievement worth showing, if any
   *
   * The one closest to completion, so that what is on screen is the one about
   * to be earned rather than whichever happened to tick last.
   */
  AchievementProgressIndicator GetProgressIndicator() const;

  /*!
   * \brief Show or hide a leaderboard attempt's live value
   */
  void SetLeaderboardTracker(const LeaderboardTracker& tracker, bool active);

  std::vector<LeaderboardTracker> GetLeaderboardTrackers() const;

  /*!
   * \brief Replace the leaderboards of the loaded game
   */
  void SetLeaderboardState(const LeaderboardState& state);

  /*!
   * \brief Get a copy of the leaderboards, including their standings
   */
  LeaderboardState GetLeaderboardState() const;

  /*!
   * \brief Store the standings fetched for one leaderboard
   *
   * Kept on the runtime rather than in the dialog so that reopening a
   * leaderboard already looked at costs nothing.
   *
   * \return False if that leaderboard is not in the loaded game
   */
  /*!
   * \brief Which account the runtime's standings belong to
   *
   * Taken when a request is made and handed back when it answers. Signing out
   * leaves the account's name set, so the name cannot say the account moved,
   * and a number lets the check happen inside the write it guards.
   */
  unsigned int GetAccountGeneration() const;

  /*!
   * \brief Store the summary fetched for one leaderboard
   *
   * \param accountGeneration What GetAccountGeneration() said when it was asked
   *
   * \return False if the game or the account moved on while it was in flight
   */
  bool SetLeaderboardSummary(unsigned int leaderboardId,
                             unsigned int accountGeneration,
                             const LeaderboardSummary& summary);

  bool SetLeaderboardEntries(unsigned int leaderboardId,
                             unsigned int accountGeneration,
                             const std::vector<LeaderboardEntry>& entries);

  /*!
   * \brief Record where the player now stands after a submission
   *
   * The server answers a submission with the new standing, which is the same
   * thing the leaderboards list shows. Writing it back means the list is right
   * without asking for it again.
   *
   * \return False if the game changed before the answer arrived
   */
  bool SetLeaderboardStanding(unsigned int leaderboardId,
                              unsigned int rank,
                              const std::string& score,
                              unsigned int totalEntries);

  /*!
   * \brief Which leaderboard the entries dialog should show
   *
   * Passed this way rather than as a window parameter because the entries
   * dialog is opened from the list by the skin, which has no way to hand over
   * anything but a window id.
   */
  void SetSelectedLeaderboard(unsigned int leaderboardId);

  unsigned int GetSelectedLeaderboard() const;

  /*!
   * \brief Be told when anything an on-screen indicator shows has changed
   *
   * Set once by whatever draws them. Every indicator the runtime holds reports
   * through here, so one added later is announced without its author having to
   * remember to wire it up.
   *
   * Called on whichever thread made the change, which is the game's.
   */
  void SetIndicatorCallback(std::function<void()> callback);

  /*!
   * \brief Forget what the standings said about whoever was signed in
   *
   * A leaderboard's definition belongs to the game and stays. Where the player
   * stands, and which row in a fetched page is theirs, belong to the account,
   * so signing in as somebody else has to ask again rather than show them what
   * the last account saw.
   */
  void ForgetPlayerLeaderboardData();

  /*!
   * \brief Tell whoever draws the indicators to look again
   *
   * Every indicator change already reports through here. A setting that
   * decides whether one is drawn is such a change, so flipping it back on
   * while an attempt is live brings the indicator back without waiting for
   * the add-on to send anything.
   */
  void NotifyIndicatorsChanged();

private:
  mutable std::mutex m_mutex;

  //! Whatever draws the on-screen indicators, told whenever one changes. Held
  //! rather than called directly so the runtime keeps knowing nothing about the
  //! GUI, and invoked outside the lock so it may read back safely.
  std::function<void()> m_indicatorCallback;
  AchievementState m_state;
  LeaderboardState m_leaderboards;

  //! Owned by the add-on's event stream rather than by the fetched list, so
  //! that publishing one cannot drop an attempt already on screen. Usually
  //! empty, and rarely more than one at a time.
  std::vector<LeaderboardTracker> m_trackers;
  unsigned int m_selectedLeaderboard{0};

  //! Moved on whenever the signed-in account does, so an answer asked for by
  //! the last one is recognised and dropped
  unsigned int m_accountGeneration{0};
};

} // namespace KODI::GAME
