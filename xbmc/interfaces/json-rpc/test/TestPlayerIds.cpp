/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "interfaces/json-rpc/PlayerIds.h"
#include "utils/Variant.h"

#include <ostream>

#include <gtest/gtest.h>

using namespace JSONRPC;
using namespace KODI;

namespace KODI::PLAYLIST
{

/*! Without this gtest reports a mismatched playerid as its raw bytes. */
void PrintTo(const Id& id, std::ostream* os)
{
  *os << static_cast<int>(id);
}

} // namespace KODI::PLAYLIST

namespace
{

constexpr auto NO_PLAYLIST = PLAYLIST::Id::TYPE_NONE;
constexpr auto MUSIC_PLAYLIST = PLAYLIST::Id::TYPE_MUSIC;
constexpr auto VIDEO_PLAYLIST = PLAYLIST::Id::TYPE_VIDEO;
constexpr auto PICTURE_PLAYLIST = PLAYLIST::Id::TYPE_PICTURE;

constexpr PlayerState VIDEO_UNDER_THE_MUSIC_PLAYLIST{Video, MUSIC_PLAYLIST, VIDEO_PLAYLIST};

} // unnamed namespace

TEST(TestPlayerIds, EachPlayerHasItsOwnId)
{
  EXPECT_EQ(VIDEO_PLAYLIST, PlayerIdOf(Video));
  EXPECT_EQ(MUSIC_PLAYLIST, PlayerIdOf(Audio));
  EXPECT_EQ(PICTURE_PLAYLIST, PlayerIdOf(Picture));
}

TEST(TestPlayerIds, AVideoIsPlayerOneEvenWhenTheMusicPlaylistIsCurrent)
{
  EXPECT_EQ(VIDEO_PLAYLIST, PlayerIdOf(Video));
}

TEST(TestPlayerIds, PlayeridOneReachesTheVideoPlayerWhicheverPlaylistIsCurrent)
{
  EXPECT_EQ(Video, PlayerForId(VIDEO_PLAYLIST));
}

TEST(TestPlayerIds, TwoPlayersNeverShareAPlayerid)
{
  // Two can run at once: PVR reports a recording, which counts as a video player, while the
  // app player reports audio because the stream carries no video.
  EXPECT_NE(PlayerIdOf(Video), PlayerIdOf(Audio));
  EXPECT_NE(PlayerIdOf(Audio), PlayerIdOf(Picture));
  EXPECT_NE(PlayerIdOf(Picture), PlayerIdOf(Video));
}

TEST(TestPlayerIds, EveryPlayerAnswersToTheIdItIsPublishedUnder)
{
  for (const PlayerType player : {Video, Audio, Picture})
  {
    const PLAYLIST::Id playerid = PlayerIdOf(player);
    EXPECT_EQ(player, PlayerForId(playerid))
        << "player " << player << " is published as playerid " << static_cast<int>(playerid);
  }
}

TEST(TestPlayerIds, AnIdOutsideTheRangeNamesNoPlayer)
{
  EXPECT_EQ(None, PlayerForId(NO_PLAYLIST));
  EXPECT_EQ(None, PlayerForId(PLAYLIST::Id::TYPE_GAME));
}

TEST(TestPlayerIds, ThePlaylistAPlayerWorksThroughIsTheCurrentOne)
{
  EXPECT_EQ(MUSIC_PLAYLIST, PlaylistOf(Video, VIDEO_UNDER_THE_MUSIC_PLAYLIST));
  EXPECT_EQ(MUSIC_PLAYLIST, PlaylistOf(Audio, VIDEO_UNDER_THE_MUSIC_PLAYLIST));
  EXPECT_EQ(PICTURE_PLAYLIST, PlaylistOf(Picture, VIDEO_UNDER_THE_MUSIC_PLAYLIST));
}

TEST(TestPlayerIds, WithNoPlaylistInForceThePlayerDecidesWhichItWorksThrough)
{
  constexpr PlayerState playingVideo{Video, NO_PLAYLIST, VIDEO_PLAYLIST};
  constexpr PlayerState playingNothing{None, NO_PLAYLIST, NO_PLAYLIST};

  EXPECT_EQ(VIDEO_PLAYLIST, PlaylistOf(Video, playingVideo));
  EXPECT_EQ(VIDEO_PLAYLIST, PlaylistOf(Video, playingNothing));
  EXPECT_EQ(MUSIC_PLAYLIST, PlaylistOf(Audio, playingNothing));
}

TEST(TestPlayerIds, APlayeridForAPlayerThatIsNotRunningNamesNone)
{
  constexpr PlayerState playingMusic{Audio, MUSIC_PLAYLIST, MUSIC_PLAYLIST};

  EXPECT_EQ(Audio, RunningPlayerForId(MUSIC_PLAYLIST, playingMusic));
  EXPECT_EQ(None, RunningPlayerForId(VIDEO_PLAYLIST, playingMusic));
  EXPECT_EQ(Video, RunningPlayerForId(VIDEO_PLAYLIST, VIDEO_UNDER_THE_MUSIC_PLAYLIST));
}

TEST(TestPlayerIds, ANotificationCarriesThePlayersOwnId)
{
  CVariant player;
  DescribePlayer(player, Video);
  EXPECT_EQ(1, player["playerid"].asInteger());
}
