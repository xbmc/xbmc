/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "GUIUserMessages.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "guilib/GUIMessage.h"
#include "interfaces/AnnouncementManager.h"
#include "playlists/PlayList.h"

#include <memory>

#include <gtest/gtest.h>

using namespace KODI;

namespace
{

// CPlayListPlayer reaches the GUI and the application's components on most of its paths, and
// neither exists under InitForTesting. The index arithmetic under test reaches neither, so the
// repeat state it reads is set here instead of through SetRepeat(), which ends in
// AnnouncePropertyChanged() asking the application components for CApplicationPlayer; that
// component is not registered in a test, and the container throws rather than answering null.
//
// m_repeatState, RepeatedOne() and m_bPlaybackStarted are protected, so a subclass reaches them.
class TestablePlayListPlayer : public PLAYLIST::CPlayListPlayer
{
public:
  void SetRepeatDirectly(PLAYLIST::Id playlistId, PLAYLIST::RepeatState state)
  {
    m_repeatState[playlistId] = state;
  }

  bool IsRepeatedOne(PLAYLIST::Id playlistId) const { return RepeatedOne(playlistId); }

  bool PlaybackStarted() const { return m_bPlaybackStarted; }
};

} // namespace

class TestPlayListPlayer : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // CPlayList::Add() announces every addition, and nothing registers an announcement manager
    // in the test environment: the accessor hands back a null shared_ptr and Announce()
    // dereferences it. An unstarted manager is enough, because Announce() only appends to a
    // queue - without the worker thread nothing drains it, which for a couple of items costs
    // nothing. Registered per test and taken away again so no other test sees it.
    m_previous = CServiceBroker::GetAnnouncementManager();
    CServiceBroker::RegisterAnnouncementManager(
        std::make_shared<ANNOUNCEMENT::CAnnouncementManager>());
  }

  void TearDown() override
  {
    CServiceBroker::UnregisterAnnouncementManager();
    if (m_previous)
      CServiceBroker::RegisterAnnouncementManager(m_previous);
  }

  // Two items, so an index of 0 or 1 is in range and -1 is unambiguously "no current item".
  static void FillWithTwoItems(TestablePlayListPlayer& player, PLAYLIST::Id playlistId)
  {
    PLAYLIST::CPlayList& playlist = player.GetPlaylist(playlistId);
    playlist.Add(std::make_shared<CFileItem>("/video/first.mkv", false));
    playlist.Add(std::make_shared<CFileItem>("/video/second.mkv", false));
    ASSERT_EQ(2, playlist.size());
  }

private:
  std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> m_previous;
};

// A playlist that has been cleared leaves the current index at -1, and items queued afterwards
// have to be reachable. GetNextItemIdx() is what PlayNext() asks, so -1 in has to give the
// first item out.
TEST_F(TestPlayListPlayer, GetNextItemIdxFromNoCurrentItemGivesTheFirstItem)
{
  TestablePlayListPlayer player;
  player.SetCurrentPlaylist(PLAYLIST::Id::TYPE_VIDEO);
  FillWithTwoItems(player, PLAYLIST::Id::TYPE_VIDEO);

  player.SetCurrentItemIdx(-1);
  ASSERT_EQ(-1, player.GetCurrentItemIdx()) << "the no-current-item state under test was not set";

  EXPECT_EQ(0, player.GetNextItemIdx(1));
}

TEST_F(TestPlayListPlayer, GetNextItemIdxFromNoCurrentItemGivesTheFirstItemWithRepeatOne)
{
  TestablePlayListPlayer player;
  player.SetCurrentPlaylist(PLAYLIST::Id::TYPE_VIDEO);
  FillWithTwoItems(player, PLAYLIST::Id::TYPE_VIDEO);

  player.SetCurrentItemIdx(-1);
  ASSERT_EQ(-1, player.GetCurrentItemIdx()) << "the no-current-item state under test was not set";

  player.SetRepeatDirectly(PLAYLIST::Id::TYPE_VIDEO, PLAYLIST::RepeatState::ONE);
  ASSERT_TRUE(player.IsRepeatedOne(PLAYLIST::Id::TYPE_VIDEO)) << "repeat one was not set";

  // Without the fix this answers -1: the repeat-one branch returns the current index unchanged,
  // and PlayNext() treats a negative index as nothing to play.
  EXPECT_EQ(0, player.GetNextItemIdx(1));
}

// Repeat One still repeats, once there is a current item to repeat.
TEST_F(TestPlayListPlayer, GetNextItemIdxWithRepeatOneRepeatsTheCurrentItem)
{
  TestablePlayListPlayer player;
  player.SetCurrentPlaylist(PLAYLIST::Id::TYPE_VIDEO);
  FillWithTwoItems(player, PLAYLIST::Id::TYPE_VIDEO);

  player.SetCurrentItemIdx(1);
  ASSERT_EQ(1, player.GetCurrentItemIdx()) << "the current item under test was not set";

  player.SetRepeatDirectly(PLAYLIST::Id::TYPE_VIDEO, PLAYLIST::RepeatState::ONE);
  ASSERT_TRUE(player.IsRepeatedOne(PLAYLIST::Id::TYPE_VIDEO)) << "repeat one was not set";

  EXPECT_EQ(1, player.GetNextItemIdx(1));
}

// Clearing the playlist the playing item came from does not end playback. The item keeps playing,
// and the stop that eventually ends it cleans up only if the playback-started state survived the
// clear, so clearing drops the position and nothing else.
TEST_F(TestPlayListPlayer, ClearingTheCurrentPlaylistKeepsThePlaybackStartedState)
{
  TestablePlayListPlayer player;
  player.SetCurrentPlaylist(PLAYLIST::Id::TYPE_VIDEO);
  FillWithTwoItems(player, PLAYLIST::Id::TYPE_VIDEO);
  player.SetCurrentItemIdx(0);

  CGUIMessage started(GUI_MSG_PLAYBACK_STARTED, 0, 0);
  player.OnMessage(started);
  ASSERT_TRUE(player.PlaybackStarted()) << "the playing state under test was not set";

  player.ClearPlaylist(PLAYLIST::Id::TYPE_VIDEO);

  EXPECT_EQ(-1, player.GetCurrentItemIdx());
  EXPECT_TRUE(player.PlaybackStarted());
  EXPECT_EQ(PLAYLIST::Id::TYPE_VIDEO, player.GetCurrentPlaylist());
}
