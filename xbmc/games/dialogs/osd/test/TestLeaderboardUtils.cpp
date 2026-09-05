/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "filesystem/File.h"
#include "games/AchievementRuntime.h"
#include "games/dialogs/osd/LeaderboardUtils.h"
#include "profiles/ProfileManager.h"
#include "settings/SettingsComponent.h"

#include <gtest/gtest.h>

using namespace KODI::GAME;

namespace
{
LeaderboardEntry MakeEntry(unsigned int rank, const std::string& user, std::time_t submitted)
{
  LeaderboardEntry entry;
  entry.rank = rank;
  entry.username = user;
  entry.score = "0:24.13";
  entry.submitted = submitted;
  entry.isPlayer = (user == "chris");
  return entry;
}

std::string CacheFile()
{
  return CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetUserDataItem(
      "gameleaderboards.xml");
}
} // namespace

class TestLeaderboardUtils : public testing::Test
{
protected:
  void SetUp() override { XFILE::CFile::Delete(CacheFile()); }
  void TearDown() override { XFILE::CFile::Delete(CacheFile()); }
};

TEST_F(TestLeaderboardUtils, FormatsAScoreTheWayItsLeaderboardMeasures)
{
  // The same integer means different things on different leaderboards, which is
  // why the format travels with it
  EXPECT_EQ(FormatLeaderboardScore(9000, "TIME"), "2:30.00");
  EXPECT_EQ(FormatLeaderboardScore(9000, "FRAMES"), "2:30.00");
  EXPECT_EQ(FormatLeaderboardScore(9000, "MILLISECS"), "1:30.00");
  EXPECT_EQ(FormatLeaderboardScore(90, "TIMESECS"), "1:30");
  EXPECT_EQ(FormatLeaderboardScore(90, "SECS"), "1:30");
  EXPECT_EQ(FormatLeaderboardScore(90, "MINUTES"), "1h30");
  EXPECT_EQ(FormatLeaderboardScore(5400, "SECS_AS_MINS"), "1h30");
  EXPECT_EQ(FormatLeaderboardScore(9000, "FIXED1"), "900.0");
  EXPECT_EQ(FormatLeaderboardScore(9000, "FIXED2"), "90.00");
  EXPECT_EQ(FormatLeaderboardScore(9000, "FIXED3"), "9.000");
  EXPECT_EQ(FormatLeaderboardScore(5, "FLOAT2"), "5.00");
  EXPECT_EQ(FormatLeaderboardScore(5, "TENS"), "50");
  EXPECT_EQ(FormatLeaderboardScore(5, "HUNDREDS"), "500");

  // Past an hour the hours are split out rather than left in the minutes
  EXPECT_EQ(FormatLeaderboardScore(3661, "SECS"), "1h01:01");

  // A points total is written to six places and never grouped, the way the
  // site has always shown it
  EXPECT_EQ(FormatLeaderboardScore(9000, "SCORE"), "009000");
  EXPECT_EQ(FormatLeaderboardScore(9000, "POINTS"), "009000");

  // Everything else is grouped, so a long value can be read at a glance
  EXPECT_EQ(FormatLeaderboardScore(1234567, "VALUE"), "1,234,567");
  EXPECT_EQ(FormatLeaderboardScore(5, "THOUSANDS"), "5,000");

  // A scaled zero is still zero rather than the trailing zeroes on their own
  EXPECT_EQ(FormatLeaderboardScore(0, "THOUSANDS"), "0");

  // Negative values keep their sign, including below the unit where the whole
  // part is zero and cannot carry it
  EXPECT_EQ(FormatLeaderboardScore(-150, "FIXED2"), "-1.50");
  EXPECT_EQ(FormatLeaderboardScore(-50, "FIXED2"), "-0.50");
  EXPECT_EQ(FormatLeaderboardScore(-5, "FIXED1"), "-0.5");

  // A unit we do not recognise is shown as a plain number: a wrong unit reads
  // worse than none
  EXPECT_EQ(FormatLeaderboardScore(9000, "SOMETHING_NEW"), "9,000");
  EXPECT_EQ(FormatLeaderboardScore(9000, ""), "9,000");
}

TEST_F(TestLeaderboardUtils, KnowsWhichFormatsAreTimes)
{
  // What a board is called comes from this, so every format shown as a
  // duration has to answer true or a time trial is described as a score
  for (const char* format :
       {"TIME", "FRAMES", "MILLISECS", "TIMESECS", "SECS", "SECS_AS_MINS", "MINUTES"})
    EXPECT_TRUE(IsTimeFormat(format)) << format;

  for (const char* format : {"SCORE", "POINTS", "VALUE", "FIXED2", "FLOAT3", "TENS", ""})
    EXPECT_FALSE(IsTimeFormat(format)) << format;
}

TEST_F(TestLeaderboardUtils, ABoardNobodyHasEnteredCountsAsLoaded)
{
  LeaderboardInfo leaderboard;
  leaderboard.id = 7;

  LeaderboardState state;
  state.leaderboards = {leaderboard};

  CAchievementRuntime runtime;
  runtime.SetLeaderboardState(state);

  // An empty answer is still an answer, so reopening does not ask again
  ASSERT_TRUE(runtime.SetLeaderboardEntries(7, runtime.GetAccountGeneration(), {}));
  EXPECT_TRUE(runtime.GetLeaderboardState().leaderboards.front().entriesLoaded);
  EXPECT_TRUE(runtime.GetLeaderboardState().leaderboards.front().entries.empty());
}

TEST_F(TestLeaderboardUtils, AskingForARedrawReachesWhoeverDrawsTheIndicators)
{
  CAchievementRuntime runtime;

  unsigned int redraws = 0;
  runtime.SetIndicatorCallback([&redraws]() { ++redraws; });

  // What the challenge indicator setting calls when it is switched back on,
  // since no add-on event is coming to say the attempt is still running
  runtime.NotifyIndicatorsChanged();
  EXPECT_EQ(redraws, 1u);

  AchievementChallenge challenge;
  challenge.id = 42;
  challenge.title = "Shroom";
  runtime.SetChallenge(challenge, true);
  EXPECT_EQ(redraws, 2u);

  // Still held while it is only the setting that hides it, so switching back
  // on has something to show
  ASSERT_EQ(runtime.GetChallenges().size(), 1u);
  EXPECT_EQ(runtime.GetChallenges().front().title, "Shroom");
}

TEST_F(TestLeaderboardUtils, SigningInAsSomebodyElseDropsTheLastAccountsStandings)
{
  LeaderboardInfo leaderboard;
  leaderboard.id = 7;
  leaderboard.title = "Fastest lap";
  leaderboard.format = "TIME";
  leaderboard.totalEntries = 1483;
  leaderboard.topUsername = "Sonikku";
  leaderboard.topScore = "0:24.11";
  leaderboard.playerRank = 274;
  leaderboard.playerScore = "0:41.88";
  leaderboard.entries = {MakeEntry(1, "Sonikku", std::time(nullptr))};
  leaderboard.entriesLoaded = true;
  leaderboard.standingsLoaded = true;

  LeaderboardState state;
  state.leaderboards = {leaderboard};

  CAchievementRuntime runtime;
  runtime.SetLeaderboardState(state);

  runtime.ForgetPlayerLeaderboardData();

  const LeaderboardInfo kept = runtime.GetLeaderboardState().leaderboards.front();

  // Where the last account stood, and the page marking which row was theirs
  EXPECT_EQ(kept.playerRank, 0u);
  EXPECT_TRUE(kept.playerScore.empty());
  EXPECT_TRUE(kept.entries.empty());
  EXPECT_FALSE(kept.entriesLoaded);

  // What makes the new account's rank get asked for, since the entry count
  // below survives and cannot say whose standings these were
  EXPECT_FALSE(kept.standingsLoaded);

  // The board itself belongs to the game, so it survives and is not refetched
  EXPECT_EQ(kept.id, 7u);
  EXPECT_EQ(kept.title, "Fastest lap");
  EXPECT_EQ(kept.format, "TIME");
  EXPECT_EQ(kept.totalEntries, 1483u);
  EXPECT_EQ(kept.topUsername, "Sonikku");
  EXPECT_EQ(kept.topScore, "0:24.11");
}

TEST_F(TestLeaderboardUtils, AnIndicatorWithNothingToDrawDoesNotHideOneBehindIt)
{
  CAchievementRuntime runtime;

  // The add-on may send any of these with no text, which a skin draws nothing
  // for. The one shown is the first that has text, not simply the first.
  AchievementChallenge nameless;
  nameless.id = 1;
  runtime.SetChallenge(nameless, true);

  AchievementChallenge named;
  named.id = 2;
  named.title = "Shroom";
  runtime.SetChallenge(named, true);

  EXPECT_EQ(runtime.GetShownChallenge().id, 2u);
  EXPECT_EQ(runtime.GetShownChallenge().title, "Shroom");

  LeaderboardTracker blank;
  blank.id = 1;
  runtime.SetLeaderboardTracker(blank, true);

  LeaderboardTracker running;
  running.id = 2;
  running.display = "0:12.44";
  runtime.SetLeaderboardTracker(running, true);

  EXPECT_EQ(runtime.GetShownLeaderboardTracker().display, "0:12.44");

  // Nothing to draw at all answers empty rather than picking one anyway
  CAchievementRuntime silent;
  silent.SetChallenge(nameless, true);
  EXPECT_EQ(silent.GetShownChallenge().id, 0u);
  EXPECT_TRUE(silent.GetShownLeaderboardTracker().display.empty());
}

TEST_F(TestLeaderboardUtils, TheClosestToEarnedIsChosenFromWhatCanBeDrawn)
{
  AchievementProgressIndicator nameless;
  nameless.id = 1;
  nameless.measuredPercent = 90.0f;

  AchievementProgressIndicator named;
  named.id = 2;
  named.title = "Collect 180 rings";
  named.measuredPercent = 20.0f;

  CAchievementRuntime runtime;
  runtime.SetProgressIndicator(nameless, true);
  runtime.SetProgressIndicator(named, true);

  // The nearer one has no title, so the one that can be drawn is shown rather
  // than a corner with a bar and no words
  EXPECT_EQ(runtime.GetProgressIndicator().id, 2u);

  AchievementProgressIndicator nearer;
  nearer.id = 3;
  nearer.title = "Collect 180 coins";
  nearer.measuredPercent = 50.0f;
  runtime.SetProgressIndicator(nearer, true);

  EXPECT_EQ(runtime.GetProgressIndicator().id, 3u);
}

TEST_F(TestLeaderboardUtils, AnAnswerTheLastAccountAskedForIsRefused)
{
  LeaderboardInfo leaderboard;
  leaderboard.id = 7;

  LeaderboardState state;
  state.leaderboards = {leaderboard};

  CAchievementRuntime runtime;
  runtime.SetLeaderboardState(state);

  // Taken when the request goes out
  const unsigned int asked = runtime.GetAccountGeneration();

  LeaderboardSummary summary;
  summary.totalEntries = 1483;
  summary.playerRank = 274;
  summary.playerScore = "0:41.88";
  summary.topUsername = "Sonikku";
  summary.topScore = "0:24.11";

  ASSERT_TRUE(runtime.SetLeaderboardSummary(7, asked, summary));
  EXPECT_TRUE(runtime.GetLeaderboardState().leaderboards.front().standingsLoaded);

  // Somebody else signs in while a second request is in flight
  runtime.ForgetPlayerLeaderboardData();
  EXPECT_NE(runtime.GetAccountGeneration(), asked);

  EXPECT_FALSE(runtime.SetLeaderboardSummary(7, asked, summary));
  EXPECT_FALSE(
      runtime.SetLeaderboardEntries(7, asked, {MakeEntry(1, "Sonikku", std::time(nullptr))}));

  const LeaderboardInfo kept = runtime.GetLeaderboardState().leaderboards.front();
  EXPECT_EQ(kept.playerRank, 0u);
  EXPECT_TRUE(kept.playerScore.empty());
  EXPECT_FALSE(kept.standingsLoaded);
  EXPECT_FALSE(kept.entriesLoaded);

  // What the new account asks for is taken
  EXPECT_TRUE(runtime.SetLeaderboardSummary(7, runtime.GetAccountGeneration(), summary));
}

TEST_F(TestLeaderboardUtils, ASubmissionLeavesTheSummaryToBeAskedForAgain)
{
  LeaderboardInfo leaderboard;
  leaderboard.id = 7;
  leaderboard.topUsername = "Sonikku";
  leaderboard.topScore = "0:24.11";
  leaderboard.standingsLoaded = true;

  LeaderboardState state;
  state.leaderboards = {leaderboard};

  CAchievementRuntime runtime;
  runtime.SetLeaderboardState(state);

  ASSERT_TRUE(runtime.SetLeaderboardStanding(7, 1, "0:19.02", 1484));

  const LeaderboardInfo kept = runtime.GetLeaderboardState().leaderboards.front();

  // A scoreboard says where the player landed and nothing about who leads, so
  // the summary has to be asked for again before the list is right
  EXPECT_FALSE(kept.standingsLoaded);
  EXPECT_EQ(kept.playerRank, 1u);
  EXPECT_EQ(kept.playerScore, "0:19.02");
}

TEST_F(TestLeaderboardUtils, MedalsOnlyGoToTheTopThree)
{
  EXPECT_EQ(RankMedal(1), "gold");
  EXPECT_EQ(RankMedal(2), "silver");
  EXPECT_EQ(RankMedal(3), "bronze");
  EXPECT_TRUE(RankMedal(4).empty());

  // Rank zero means unranked, not first
  EXPECT_TRUE(RankMedal(0).empty());
}

TEST_F(TestLeaderboardUtils, ReadsBackWhatItKept)
{
  const std::time_t now = std::time(nullptr);

  std::vector<LeaderboardEntry> entries{MakeEntry(1, "Sonikku", now - 3600),
                                        MakeEntry(2, "TailsDoll", now - 90000),
                                        MakeEntry(3, "chris", now - 400000)};

  SaveLeaderboardEntries(7, "Sonikku", entries);

  std::vector<LeaderboardEntry> back;
  ASSERT_TRUE(LoadLeaderboardEntries(7, "Sonikku", back));
  ASSERT_EQ(back.size(), 3u);

  EXPECT_EQ(back[0].rank, 1u);
  EXPECT_EQ(back[0].username, "Sonikku");
  EXPECT_EQ(back[0].score, "0:24.13");
  EXPECT_EQ(back[0].submitted, now - 3600);
  EXPECT_FALSE(back[0].isPlayer);

  // The player's own row is what the header is composed from, so losing the
  // flag across a save would quietly break "your best"
  EXPECT_TRUE(back[2].isPlayer);
}

TEST_F(TestLeaderboardUtils, KnowsNothingAboutALeaderboardItNeverSaw)
{
  std::vector<LeaderboardEntry> back;
  EXPECT_FALSE(LoadLeaderboardEntries(999, "Sonikku", back));
  EXPECT_TRUE(back.empty());
}

TEST_F(TestLeaderboardUtils, LookingTwiceLeavesOneRecordNotTwo)
{
  const std::time_t now = std::time(nullptr);

  SaveLeaderboardEntries(7, "Sonikku", {MakeEntry(1, "Sonikku", now)});
  SaveLeaderboardEntries(7, "Sonikku", {MakeEntry(1, "Knux", now), MakeEntry(2, "Sonikku", now)});

  std::vector<LeaderboardEntry> back;
  ASSERT_TRUE(LoadLeaderboardEntries(7, "Sonikku", back));

  // Not four rows from two saves
  ASSERT_EQ(back.size(), 2u);
  EXPECT_EQ(back[0].username, "Knux");
}

TEST_F(TestLeaderboardUtils, KeepsLeaderboardsApart)
{
  const std::time_t now = std::time(nullptr);

  SaveLeaderboardEntries(7, "Sonikku", {MakeEntry(1, "Sonikku", now)});
  SaveLeaderboardEntries(8, "Sonikku", {MakeEntry(1, "Knux", now)});

  std::vector<LeaderboardEntry> seven;
  std::vector<LeaderboardEntry> eight;
  ASSERT_TRUE(LoadLeaderboardEntries(7, "Sonikku", seven));
  ASSERT_TRUE(LoadLeaderboardEntries(8, "Sonikku", eight));

  EXPECT_EQ(seven.front().username, "Sonikku");
  EXPECT_EQ(eight.front().username, "Knux");
}

TEST_F(TestLeaderboardUtils, ForgetsOneLeaderboardWithoutForgettingTheRest)
{
  const std::time_t now = std::time(nullptr);

  SaveLeaderboardEntries(7, "Sonikku", {MakeEntry(1, "Sonikku", now)});
  SaveLeaderboardEntries(8, "Sonikku", {MakeEntry(1, "Knux", now)});

  // What a submission does: the kept page for that one board is now wrong
  ForgetLeaderboardEntries(7);

  std::vector<LeaderboardEntry> gone;
  std::vector<LeaderboardEntry> kept;
  EXPECT_FALSE(LoadLeaderboardEntries(7, "Sonikku", gone));
  EXPECT_TRUE(LoadLeaderboardEntries(8, "Sonikku", kept));
}

TEST_F(TestLeaderboardUtils, ForgettingSomethingNeverKeptIsHarmless)
{
  const std::time_t now = std::time(nullptr);
  SaveLeaderboardEntries(7, "Sonikku", {MakeEntry(1, "Sonikku", now)});

  ForgetLeaderboardEntries(999);

  std::vector<LeaderboardEntry> back;
  EXPECT_TRUE(LoadLeaderboardEntries(7, "Sonikku", back));
}

TEST_F(TestLeaderboardUtils, ClearsEverythingWhenAsked)
{
  const std::time_t now = std::time(nullptr);

  SaveLeaderboardEntries(7, "Sonikku", {MakeEntry(1, "Sonikku", now)});
  SaveLeaderboardEntries(8, "Sonikku", {MakeEntry(1, "Knux", now)});

  ClearLeaderboardEntries();

  std::vector<LeaderboardEntry> back;
  EXPECT_FALSE(LoadLeaderboardEntries(7, "Sonikku", back));
  EXPECT_FALSE(LoadLeaderboardEntries(8, "Sonikku", back));

  // And clearing an empty cache is not an error
  ClearLeaderboardEntries();
}

TEST_F(TestLeaderboardUtils, RefusesStandingsThatHaveGoneStale)
{
  const std::time_t now = std::time(nullptr);

  SaveLeaderboardEntries(7, "Sonikku", {MakeEntry(1, "Sonikku", now)});

  // Age the record past the freshness window by hand, since the test cannot
  // wait a week
  const std::string path = CacheFile();
  std::string xml;
  {
    XFILE::CFile file;
    ASSERT_TRUE(file.Open(path));
    char buffer[8192] = {};
    const ssize_t read = file.Read(buffer, sizeof(buffer) - 1);
    ASSERT_GT(read, 0);
    xml.assign(buffer, static_cast<size_t>(read));
  }

  const std::string fetched = "fetched=\"" + std::to_string(now) + "\"";
  const std::string aged =
      "fetched=\"" + std::to_string(now - (8 * 24 * 60 * 60)) + "\""; // eight days
  ASSERT_NE(xml.find(fetched), std::string::npos);
  xml.replace(xml.find(fetched), fetched.size(), aged);

  {
    XFILE::CFile file;
    ASSERT_TRUE(file.OpenForWrite(path, true));
    file.Write(xml.c_str(), xml.size());
  }

  // A table a week old is asked for again rather than shown as if it were
  // current
  std::vector<LeaderboardEntry> back;
  EXPECT_FALSE(LoadLeaderboardEntries(7, "Sonikku", back));
}

TEST_F(TestLeaderboardUtils, PublishingFetchedLeaderboardsLeavesALiveAttemptAlone)
{
  CAchievementRuntime runtime;

  LeaderboardTracker tracker;
  tracker.id = 3;
  tracker.display = "0:12.44";
  runtime.SetLeaderboardTracker(tracker, true);

  // The list arrives from a request that knows nothing about attempts in
  // progress, and the player may well be inside one while it lands
  LeaderboardInfo leaderboard;
  leaderboard.id = 7;

  LeaderboardState fetched;
  fetched.leaderboards = {leaderboard};
  fetched.loaded = true;
  runtime.SetLeaderboardState(fetched);

  const std::vector<LeaderboardTracker> trackers = runtime.GetLeaderboardTrackers();
  ASSERT_EQ(trackers.size(), 1u);
  EXPECT_EQ(trackers.front().display, "0:12.44");

  // The game going away does take them
  runtime.Clear();
  EXPECT_TRUE(runtime.GetLeaderboardTrackers().empty());
}

TEST_F(TestLeaderboardUtils, ScoreboardWritesTheStandingBackAndDropsTheStalePage)
{
  LeaderboardInfo leaderboard;
  leaderboard.id = 7;
  leaderboard.playerRank = 274;
  leaderboard.playerScore = "0:41.88";
  leaderboard.totalEntries = 1483;
  leaderboard.entries = {MakeEntry(1, "Sonikku", std::time(nullptr))};
  leaderboard.entriesLoaded = true;

  LeaderboardState state;
  state.leaderboards = {leaderboard};

  CAchievementRuntime runtime;
  runtime.SetLeaderboardState(state);

  ASSERT_TRUE(runtime.SetLeaderboardStanding(7, 12, "0:26.11", 1484));

  const LeaderboardInfo updated = runtime.GetLeaderboardState().leaderboards.front();
  EXPECT_EQ(updated.playerRank, 12u);
  EXPECT_EQ(updated.playerScore, "0:26.11");
  EXPECT_EQ(updated.totalEntries, 1484u);

  // The fetched page predates the submission, so it is dropped rather than
  // having the new entry guessed into it, and it has to read as not fetched or
  // the dialog shows an empty table instead of asking again
  EXPECT_TRUE(updated.entries.empty());
  EXPECT_FALSE(updated.entriesLoaded);

  // A board belonging to a game that has since been unloaded
  EXPECT_FALSE(runtime.SetLeaderboardStanding(999, 1, "0:01.00", 2));
}

TEST_F(TestLeaderboardUtils, ATotalOfZeroLeavesTheCountAlone)
{
  LeaderboardInfo leaderboard;
  leaderboard.id = 7;
  leaderboard.totalEntries = 1483;

  LeaderboardState state;
  state.leaderboards = {leaderboard};

  CAchievementRuntime runtime;
  runtime.SetLeaderboardState(state);

  // The server does not always say how many entries there are; keeping the
  // count is better than showing "of 0"
  ASSERT_TRUE(runtime.SetLeaderboardStanding(7, 12, "0:26.11", 0));

  EXPECT_EQ(runtime.GetLeaderboardState().leaderboards.front().totalEntries, 1483u);
}

TEST_F(TestLeaderboardUtils, StandingsKeptForOneAccountAreNotShownToAnother)
{
  const std::time_t now = std::time(nullptr);
  SaveLeaderboardEntries(7, "Sonikku", {MakeEntry(1, "Sonikku", now)});

  // The rows say which one is the player, so another account must refetch
  std::vector<LeaderboardEntry> back;
  EXPECT_FALSE(LoadLeaderboardEntries(7, "Knux", back));

  std::vector<LeaderboardEntry> mine;
  EXPECT_TRUE(LoadLeaderboardEntries(7, "Sonikku", mine));
}
