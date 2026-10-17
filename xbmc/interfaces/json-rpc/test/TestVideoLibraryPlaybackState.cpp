/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "interfaces/json-rpc/VideoLibrary.h"
#include "video/VideoInfoTag.h"

#include <gtest/gtest.h>

using namespace JSONRPC;

namespace
{
class CTestVideoLibrary : public CVideoLibrary
{
public:
  using CVideoLibrary::EpisodePlaybackUpdate;
};

CVideoInfoTag Show(int playCount, const char* lastPlayed)
{
  CVideoInfoTag show;
  show.SetPlayCount(playCount);
  show.m_lastPlayed.SetFromDBDateTime(lastPlayed);
  return show;
}

CVideoInfoTag Episode(int playCount, const char* lastPlayed)
{
  CVideoInfoTag episode;
  episode.SetPlayCount(playCount);
  if (lastPlayed)
    episode.m_lastPlayed.SetFromDBDateTime(lastPlayed);
  return episode;
}
} // unnamed namespace

TEST(TestVideoLibraryEpisodePlaybackUpdate, APlaycountOnlyUpdateKeepsTheEpisodesLastPlayed)
{
  const auto update = CTestVideoLibrary::EpisodePlaybackUpdate(
      Show(1, "2026-09-02 20:00:00"), true, false, Episode(0, "2026-08-10 21:49:28"));

  ASSERT_TRUE(update);
  EXPECT_EQ(1, update->playCount);
  EXPECT_EQ("2026-08-10 21:49:28", update->lastPlayed.GetAsDBDateTime());
}

TEST(TestVideoLibraryEpisodePlaybackUpdate, ANeverPlayedEpisodeHasNoTimeToKeep)
{
  const auto update = CTestVideoLibrary::EpisodePlaybackUpdate(Show(1, "2026-09-02 20:00:00"), true,
                                                               false, Episode(0, nullptr));

  ASSERT_TRUE(update);
  EXPECT_FALSE(update->lastPlayed.IsValid());
}

TEST(TestVideoLibraryEpisodePlaybackUpdate, AnEpisodeAlreadyAtThePlaycountIsLeftAlone)
{
  EXPECT_FALSE(CTestVideoLibrary::EpisodePlaybackUpdate(Show(2, "2026-09-02 20:00:00"), true, false,
                                                        Episode(2, "2026-08-10 21:49:28")));
}

TEST(TestVideoLibraryEpisodePlaybackUpdate, ALastPlayedUpdateAppliesTheShowsTime)
{
  const auto update = CTestVideoLibrary::EpisodePlaybackUpdate(
      Show(0, "2026-09-02 20:00:00"), false, true, Episode(2, "2026-08-10 21:49:28"));

  ASSERT_TRUE(update);
  EXPECT_EQ(2, update->playCount);
  EXPECT_EQ("2026-09-02 20:00:00", update->lastPlayed.GetAsDBDateTime());
}
