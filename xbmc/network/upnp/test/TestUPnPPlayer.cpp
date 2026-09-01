/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "network/upnp/UPnPPlayer.h"

#include <gtest/gtest.h>

using namespace UPNP;

TEST(TestUPnPPlaybackState, NothingIsWatchedBeforeAFileStarts)
{
  CPlaybackState state;

  EXPECT_FALSE(state.IsStarted());
  EXPECT_FALSE(state.HasEnded("STOPPED"));
  EXPECT_FALSE(state.Finish());
}

TEST(TestUPnPPlaybackState, AStoppedRendererEndsTheFileItWasPlaying)
{
  CPlaybackState state;

  state.Opening();
  state.Started();

  EXPECT_FALSE(state.HasEnded("PLAYING"));
  EXPECT_FALSE(state.HasEnded("TRANSITIONING"));
  EXPECT_TRUE(state.HasEnded("STOPPED"));
}

TEST(TestUPnPPlaybackState, PlaybackEndsOnlyOnce)
{
  CPlaybackState state;

  state.Opening();
  state.Started();

  ASSERT_TRUE(state.HasEnded("STOPPED"));
  EXPECT_TRUE(state.Finish());

  EXPECT_FALSE(state.HasEnded("STOPPED"));
  EXPECT_FALSE(state.Finish());
}

//! \brief Opening a file stops a renderer that is already playing, and that STOPPED belongs to the
//!        file being replaced. Reading it as the end of playback advances the playlist, so the
//!        renderer is handed a file the user did not pick.
TEST(TestUPnPPlaybackState, TheStopThatStartsTheNextFileDoesNotEndPlayback)
{
  CPlaybackState state;

  state.Opening();
  state.Started();
  ASSERT_FALSE(state.HasEnded("PLAYING"));

  // the user picks another file while that one is playing
  state.Opening();

  EXPECT_FALSE(state.HasEnded("STOPPED"));

  state.Started();

  EXPECT_FALSE(state.HasEnded("PLAYING"));
  EXPECT_TRUE(state.HasEnded("STOPPED"));
}

//! \brief An open that never reaches PLAYING leaves the previous file unwatched rather than
//!        ending it a second time.
TEST(TestUPnPPlaybackState, AnOpenThatFailsLeavesNothingWatched)
{
  CPlaybackState state;

  state.Opening();
  state.Started();
  ASSERT_TRUE(state.IsStarted());

  state.Opening();

  EXPECT_FALSE(state.IsStarted());
  EXPECT_FALSE(state.HasEnded("STOPPED"));
  EXPECT_FALSE(state.Finish());
}
