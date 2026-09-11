/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JSONRPCTestUtils.h"
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

/*! A video playing while the music playlist is still the current one. Opening a disc leaves
    Kodi here: a bluray:// path does not classify as video, so the playlist player files it
    under music while the video player goes on to play it. */
constexpr PlayerState VIDEO_UNDER_THE_MUSIC_PLAYLIST{Video, MUSIC_PLAYLIST, VIDEO_PLAYLIST};

} // unnamed namespace

TEST(TestPlayerIds, EachPlayerHasItsOwnId)
{
  EXPECT_EQ(VIDEO_PLAYLIST, PlayerIdOf(Video));
  EXPECT_EQ(MUSIC_PLAYLIST, PlayerIdOf(Audio));
  EXPECT_EQ(PICTURE_PLAYLIST, PlayerIdOf(Picture));
}

TEST(TestPlayerIds, TwoPlayersNeverShareAPlayerid)
{
  // Two can run at once: a radio recording counts as a video player for PVR while the app
  // player reports audio, as the stream carries no video.
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

namespace
{
constexpr PlayerState PLAYING_MUSIC{Audio, MUSIC_PLAYLIST, MUSIC_PLAYLIST};
constexpr PlayerState PLAYING_NOTHING{None, NO_PLAYLIST, NO_PLAYLIST};

PlayerType Resolve(PLAYLIST::Id playerid, PLAYLIST::Id playlistid, const PlayerState& state)
{
  PlayerType player{None};
  EXPECT_EQ(OK, ResolvePlayer(playerid, playlistid, state, player));
  return player;
}
} // unnamed namespace

TEST(TestResolvePlayer, APlayeridNamesTheRunningPlayerHoweverItWasStarted)
{
  EXPECT_EQ(Video, Resolve(VIDEO_PLAYLIST, NO_PLAYLIST, VIDEO_UNDER_THE_MUSIC_PLAYLIST));
  EXPECT_EQ(Audio, Resolve(MUSIC_PLAYLIST, NO_PLAYLIST, PLAYING_MUSIC));
}

TEST(TestResolvePlayer, APlayerThatIsNotRunningIsUnavailable)
{
  PlayerType player{None};
  EXPECT_EQ(Unavailable, ResolvePlayer(VIDEO_PLAYLIST, NO_PLAYLIST, PLAYING_MUSIC, player));
  EXPECT_EQ(Unavailable, ResolvePlayer(VIDEO_PLAYLIST, NO_PLAYLIST, PLAYING_NOTHING, player));
}

TEST(TestResolvePlayer, APlaylistidNamesThePlayerWorkingThroughIt)
{
  EXPECT_EQ(Video, Resolve(NO_PLAYLIST, MUSIC_PLAYLIST, VIDEO_UNDER_THE_MUSIC_PLAYLIST));
  EXPECT_EQ(Audio, Resolve(NO_PLAYLIST, MUSIC_PLAYLIST, PLAYING_MUSIC));
  EXPECT_EQ(Picture,
            Resolve(NO_PLAYLIST, PICTURE_PLAYLIST, PlayerState{Picture, NO_PLAYLIST, NO_PLAYLIST}));
}

TEST(TestResolvePlayer, APlaylistNothingPlaysThroughIsUnavailable)
{
  PlayerType player{None};
  EXPECT_EQ(Unavailable,
            ResolvePlayer(NO_PLAYLIST, VIDEO_PLAYLIST, VIDEO_UNDER_THE_MUSIC_PLAYLIST, player));
  EXPECT_EQ(Unavailable, ResolvePlayer(NO_PLAYLIST, MUSIC_PLAYLIST, PLAYING_NOTHING, player));
}

TEST(TestResolvePlayer, ExactlyOneIdIsGiven)
{
  PlayerType player{None};
  EXPECT_EQ(InvalidParams, ResolvePlayer(NO_PLAYLIST, NO_PLAYLIST, PLAYING_MUSIC, player));
  EXPECT_EQ(InvalidParams, ResolvePlayer(MUSIC_PLAYLIST, MUSIC_PLAYLIST, PLAYING_MUSIC, player));
  EXPECT_EQ(InvalidParams,
            ResolvePlayer(PLAYLIST::Id::TYPE_GAME, NO_PLAYLIST, PLAYING_MUSIC, player));
}

TEST(TestDescribePlayer, CarriesThePlayersIdAndThePlaylistItWorksThrough)
{
  CVariant player;
  DescribePlayer(player, Video, MUSIC_PLAYLIST);
  EXPECT_EQ(1, player["playerid"].asInteger());
  EXPECT_EQ(0, player["playlistid"].asInteger());
}

TEST(TestDescribePlayer, WithNoPlaylistInForceThePlayersOwnIsNamed)
{
  CVariant player;
  DescribePlayer(player, Audio, NO_PLAYLIST);
  EXPECT_EQ(0, player["playerid"].asInteger());
  EXPECT_EQ(0, player["playlistid"].asInteger());
}

//! \brief Every method addressed by playerid takes playlistid in its place, and requires neither
TEST(TestPlayerIdSchema, EveryPlayerMethodTakesEitherId)
{
  for (const auto& [name, method] : ShippedMethods())
  {
    const std::map<std::string, CVariant> params{Params(method)};
    if (!params.contains("playerid"))
      continue;

    EXPECT_TRUE(params.contains("playlistid")) << name;
    EXPECT_FALSE(params.at("playerid")["required"].asBoolean(false)) << name;
    EXPECT_FALSE(params.at("playlistid")["required"].asBoolean(false)) << name;
    EXPECT_EQ("playlistid", method["params"][method["params"].size() - 1]["name"].asString())
        << name << " must keep playlistid last for positional callers";
  }
}

TEST(TestPlayerIdSchema, NotificationsAndActivePlayersCarryBothIds)
{
  const CVariant player{ShippedType("Player.Notifications.Player")};
  EXPECT_TRUE(RequiredMembers(player).contains("playlistid"));

  const CVariant entry{ShippedMethod("Player.GetActivePlayers")["returns"]["items"]};
  EXPECT_TRUE(RequiredMembers(entry).contains("playlistid"));
}
