/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIUserMessages.h"
#include "application/ApplicationPlayLists.h"
#include "application/PlayListsMessageHandler.h"
#include "application/test/PlayListsTestHelpers.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "guilib/GUIMessage.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/ThreadMessage.h"
#include "playlists/PlayList.h"
#include "playlists/PlayListEntryRules.h"
#include "playlists/PlayListFileItemClassify.h"
#include "utils/URIUtils.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace KODI::APPLICATION::TEST;
using namespace KODI::PLAYLIST;

namespace
{
std::unique_ptr<CFileItemList> Items(std::initializer_list<const char*> paths)
{
  auto items = std::make_unique<CFileItemList>();
  for (const char* path : paths)
    items->Add(std::make_shared<CFileItem>(path, false));
  return items;
}

// Two entries, and the Video playlist's cursor on the second.
void FillVideo(CTestPlayLists& playLists)
{
  CPlayList& playList = playLists.EditPlayList(PLAYLIST::Video);
  playList.Add(std::make_shared<CFileItem>("/video/first.mkv", false));
  const EntryId second = playList.Add(std::make_shared<CFileItem>("/video/second.mkv", false));
  playList.SetCurrent(second);
}

constexpr auto EXPANSION_FOLDER = "special://temp/test-playlist-expansion/";

std::string WriteFile(const std::string& name, const std::string& content)
{
  const std::string path = URIUtils::AddFileToFolder(EXPANSION_FOLDER, name);
  XFILE::CFile file;
  if (!file.OpenForWrite(path, true))
    return {};
  file.Write(content.data(), content.size());
  file.Close();
  return path;
}

std::vector<std::string> FileNames(const CFileItemList& items)
{
  std::vector<std::string> names;
  for (const auto& item : items)
    names.push_back(URIUtils::GetFileName(item->GetPath()));
  return names;
}

class TestPlayListExpansion : public ::testing::Test
{
protected:
  void SetUp() override { ASSERT_TRUE(XFILE::CDirectory::Create(EXPANSION_FOLDER)); }
  void TearDown() override { XFILE::CDirectory::RemoveRecursive(EXPANSION_FOLDER); }
};
} // namespace

TEST(TestApplicationPlayLists, BothPlayListsAlwaysExist)
{
  CTestPlayLists playLists;
  EXPECT_TRUE(playLists.EditPlayList(PLAYLIST::Video).IsEmpty());
  EXPECT_TRUE(playLists.EditPlayList(PLAYLIST::Audio).IsEmpty());
  EXPECT_NE(&playLists.EditPlayList(PLAYLIST::Video), &playLists.EditPlayList(PLAYLIST::Audio));
}

TEST(TestApplicationPlayLists, TheRepeatStateIsReadFromTheMarkAndTheWrap)
{
  using enum PLAYLIST::Repeat;
  CTestPlayLists playLists;
  FillVideo(playLists);
  CPlayList& playList = playLists.EditPlayList(PLAYLIST::Video);

  EXPECT_EQ(Off, playLists.GetRepeat(PLAYLIST::Video));

  playList.SetWrap(Wrap::ToStart);
  EXPECT_EQ(All, playLists.GetRepeat(PLAYLIST::Video));

  playList.SetRepeatCurrent(true);
  EXPECT_EQ(One, playLists.GetRepeat(PLAYLIST::Video))
      << "repeating the current entry reads as one";

  playList.SetCurrent(playList.GetEntryId(0));
  EXPECT_EQ(One, playLists.GetRepeat(PLAYLIST::Video)) << "whichever entry is current";
}

TEST(TestApplicationPlayLists, AFilmOnTheVideoPlayListTakesAudioWithIt)
{
  CTestPlayLists playLists;
  FillVideo(playLists);
  playLists.SetPlayingType(PLAYLIST::Video);

  playLists.Started();

  EXPECT_TRUE(playLists.IsAudioFollowingVideo());
  EXPECT_EQ(PLAYLIST::Video, playLists.GetQueueType(PLAYLIST::Audio));

  CGUIMessage stopped(GUI_MSG_PLAYBACK_STOPPED, 0, 0);
  playLists.OnMessage(stopped);
  EXPECT_FALSE(playLists.IsAudioFollowingVideo());
  EXPECT_EQ(PLAYLIST::Audio, playLists.GetQueueType(PLAYLIST::Audio));
}

TEST(TestApplicationPlayLists, APlayListIsPlayingFromTheMomentItIsChosen)
{
  CTestPlayLists playLists;
  FillVideo(playLists);
  playLists.EditPlayList(PLAYLIST::Video)
      .Add(std::make_shared<CFileItem>("/video/third.mkv", false));
  playLists.SetPlayingType(PLAYLIST::Video);

  EXPECT_TRUE(playLists.HasNext(PLAYLIST::Video)) << "while its file is still opening";
  EXPECT_FALSE(playLists.HasNext(PLAYLIST::Audio));
}

TEST(TestApplicationPlayLists, RepeatNamesReadBackAsTheirState)
{
  using enum PLAYLIST::Repeat;
  for (const PLAYLIST::Repeat repeat : {Off, One, All})
    EXPECT_EQ(repeat, PLAYLIST::RepeatFromName(PLAYLIST::NameOf(repeat)));
  EXPECT_FALSE(PLAYLIST::RepeatFromName("cycle").has_value());
}

TEST(TestApplicationPlayLists, TheEntryBeingPlayedCannotBeRemoved)
{
  CTestPlayLists playLists;
  playLists.Queue(PLAYLIST::Audio,
                  *Items({"/music/one.flac", "/music/two.flac", "/music/three.flac"}),
                  CApplicationPlayLists::Placement::End);

  EXPECT_TRUE(playLists.Remove(PLAYLIST::Audio, 0)) << "nothing is being played";
  const CPlayList& playList = playLists.GetPlayList(PLAYLIST::Audio);
  EXPECT_EQ(2, playList.Size());

  ASSERT_TRUE(playLists.PlayFrom(PLAYLIST::Audio, 0));
  EXPECT_FALSE(playLists.Remove(PLAYLIST::Audio, 0));
  EXPECT_EQ(2, playList.Size());
}

TEST(TestApplicationPlayLists, RemovingByPathLeavesTheEntryBeingPlayed)
{
  CTestPlayLists playLists;
  playLists.Queue(PLAYLIST::Audio,
                  *Items({"/music/one.flac", "/music/two.flac", "/music/one.flac"}),
                  CApplicationPlayLists::Placement::End);
  ASSERT_TRUE(playLists.PlayFrom(PLAYLIST::Audio, 0));

  playLists.Remove(PLAYLIST::Audio, "/music/one.flac");
  const CPlayList& playList = playLists.GetPlayList(PLAYLIST::Audio);
  ASSERT_EQ(2, playList.Size());
  EXPECT_EQ(0, playList.GetCurrentPosition());
  EXPECT_EQ("/music/two.flac", playList[1]->GetPath());
}

TEST(TestApplicationPlayLists, ReSortingAFolderKeepsItsEntriesAndItsSourceEachTime)
{
  CTestPlayLists playLists;
  ASSERT_TRUE(playLists.PlayItems(
      PLAYLIST::Video, *Items({"/video/a.mkv", "/video/b.mkv", "/video/c.mkv"}), 1, {}, "/video/"));
  CPlayList& playList = playLists.EditPlayList(PLAYLIST::Video);
  const EntryId playing = playList.GetCurrent();

  std::vector<std::unique_ptr<CFileItemList>> orders;
  orders.push_back(Items({"/video/c.mkv", "/video/b.mkv", "/video/a.mkv"}));
  orders.push_back(Items({"/video/b.mkv", "/video/a.mkv", "/video/c.mkv"}));
  for (const auto& order : orders)
  {
    playLists.Replace(PLAYLIST::Video, *order);
    EXPECT_EQ(playing, playList.GetCurrent()) << "the same entry, not a new one for the file";
    EXPECT_EQ("/video/", playList.GetSourcePath());
  }
  EXPECT_EQ(0, playList.GetCurrentPosition());
}

TEST(TestApplicationPlayLists, AShuffledStartReachesEveryEntry)
{
  CTestPlayLists playLists;
  playLists.Queue(
      PLAYLIST::Video,
      *Items({"/video/0.mkv", "/video/1.mkv", "/video/2.mkv", "/video/3.mkv", "/video/4.mkv"}),
      CApplicationPlayLists::Placement::End);
  playLists.SetShuffle(PLAYLIST::Video, true, CApplicationPlayLists::Persist::No);

  ASSERT_TRUE(playLists.PlayFrom(PLAYLIST::Video, 0));
  for (int i = 0; i < 4; ++i)
    playLists.PlayNext();

  ASSERT_FALSE(playLists.m_opened.empty());
  EXPECT_EQ("/video/0.mkv", playLists.m_opened.front()) << "the chosen entry leads";
  EXPECT_EQ(5u, std::set<std::string>(playLists.m_opened.begin(), playLists.m_opened.end()).size());
}

TEST(TestApplicationPlayLists, PickingAnEntryInAPlayingPlayListMovesWithinTheRun)
{
  CTestPlayLists playLists;
  playLists.Queue(
      PLAYLIST::Video,
      *Items({"/video/0.mkv", "/video/1.mkv", "/video/2.mkv", "/video/3.mkv", "/video/4.mkv"}),
      CApplicationPlayLists::Placement::End);
  playLists.SetShuffle(PLAYLIST::Video, true, CApplicationPlayLists::Persist::No);
  ASSERT_TRUE(playLists.PlayFrom(PLAYLIST::Video, 0));
  playLists.Started();
  const std::vector<EntryId> order = playLists.GetPlayList(PLAYLIST::Video).GetPlayOrder();

  ASSERT_TRUE(playLists.PlayFrom(PLAYLIST::Video, 3));
  EXPECT_EQ(order, playLists.GetPlayList(PLAYLIST::Video).GetPlayOrder()) << "not dealt again";
  EXPECT_EQ(KODI::APPLICATION::StartsRun::No, playLists.m_startsRun.back());
  EXPECT_EQ(3, playLists.GetPlayList(PLAYLIST::Video).GetCurrentPosition());
}

TEST(TestApplicationPlayLists, NewContentsOnThePlayingPlayListLeaveTheScreenAsItIs)
{
  using KODI::APPLICATION::StartsRun;
  CTestPlayLists playLists;
  ASSERT_TRUE(playLists.PlayItems(PLAYLIST::Video, *Items({"/video/a.mkv", "/video/b.mkv"})));
  EXPECT_EQ(StartsRun::Yes, playLists.m_startsRun.back());

  ASSERT_TRUE(
      playLists.PlayItem(PLAYLIST::Video, std::make_shared<CFileItem>("/video/c.mkv", false)));
  EXPECT_EQ(StartsRun::No, playLists.m_startsRun.back()) << "a single item";

  playLists.SetShuffle(PLAYLIST::Video, true, CApplicationPlayLists::Persist::No);
  ASSERT_TRUE(playLists.PlayItems(PLAYLIST::Video, *Items({"/video/d.mkv", "/video/e.mkv"}), 0,
                                  {.inOrder = true}));
  EXPECT_EQ(StartsRun::No, playLists.m_startsRun.back()) << "a new list";
  EXPECT_FALSE(playLists.IsShuffled(PLAYLIST::Video)) << "new contents still play in order";

  ASSERT_TRUE(playLists.PlayItems(PLAYLIST::Audio, *Items({"/music/one.flac"})));
  EXPECT_EQ(StartsRun::Yes, playLists.m_startsRun.back()) << "the other playlist";
}

TEST(TestApplicationPlayLists, ARunPlayedInOrderLeavesTheShuffleOff)
{
  CTestPlayLists playLists;
  playLists.SetShuffle(PLAYLIST::Video, true, CApplicationPlayLists::Persist::No);
  ASSERT_TRUE(playLists.PlayItems(PLAYLIST::Video, *Items({"/video/a.mkv", "/video/b.mkv"}), 0,
                                  {.inOrder = true}));
  EXPECT_FALSE(playLists.IsShuffled(PLAYLIST::Video));

  ASSERT_TRUE(playLists.PlayItems(PLAYLIST::Audio, *Items({"/music/one.flac"})));
  EXPECT_FALSE(playLists.IsShuffled(PLAYLIST::Video)) << "the run on Video has ended";
}

TEST(TestApplicationPlayLists, TheGUIListenerHearsChangesAndTheEndOfARun)
{
  struct CListener : CApplicationPlayLists::IGUIListener
  {
    int changed{0};
    int stopped{0};
    void OnPlayListsChanged() override { ++changed; }
    void OnStopped() override { ++stopped; }
    void OnEntriesFailed() override {}
  } listener;

  CTestPlayLists playLists;
  playLists.SetGUIListener(&listener);
  playLists.Queue(PLAYLIST::Video, *Items({"/video/a.mkv"}), CApplicationPlayLists::Placement::End);
  EXPECT_GT(listener.changed, 0);

  ASSERT_TRUE(playLists.PlayFrom(PLAYLIST::Video, 0));
  EXPECT_EQ(CApplicationPlayLists::Step::NothingThere, playLists.PlayNext())
      << "nothing follows the only entry";
  EXPECT_EQ(1, listener.stopped);
  playLists.SetGUIListener(nullptr);
}

TEST(TestApplicationPlayLists, AnEntryThatFailsHandsOnToTheNext)
{
  CTestPlayLists playLists;
  playLists.Queue(PLAYLIST::Video, *Items({"/video/a.mkv", "/video/b.mkv"}),
                  CApplicationPlayLists::Placement::End);
  playLists.m_failing.insert("/video/a.mkv");

  EXPECT_TRUE(playLists.PlayFrom(PLAYLIST::Video, 0));
  EXPECT_EQ((std::vector<std::string>{"/video/a.mkv", "/video/b.mkv"}), playLists.m_opened);
  EXPECT_EQ(1, playLists.GetPlayList(PLAYLIST::Video).GetCurrentPosition());
}

TEST(TestApplicationPlayLists, AnEntryThatFailsGoingBackGoesFurtherBack)
{
  CTestPlayLists playLists;
  playLists.Queue(PLAYLIST::Video, *Items({"/video/a.mkv", "/video/b.mkv", "/video/c.mkv"}),
                  CApplicationPlayLists::Placement::End);
  ASSERT_TRUE(playLists.PlayFrom(PLAYLIST::Video, 2));
  playLists.m_failing.insert("/video/b.mkv");

  EXPECT_EQ(CApplicationPlayLists::Step::Played, playLists.PlayPrevious());
  EXPECT_EQ((std::vector<std::string>{"/video/c.mkv", "/video/b.mkv", "/video/a.mkv"}),
            playLists.m_opened);
}

TEST(TestApplicationPlayLists, RepeatOneAskedForWithAPlayIsOnForTheRun)
{
  CTestPlayLists playLists;
  playLists.Queue(PLAYLIST::Audio, *Items({"/music/one.flac", "/music/two.flac"}),
                  CApplicationPlayLists::Placement::End);

  KODI::MESSAGING::ThreadMessage message{TMSG_MEDIA_PLAY_PLAYLIST,
                                         static_cast<int>(PLAYLIST::Audio), 1, nullptr};
  message.strParam = "one";
  CPlayListsMessageHandler(playLists).OnApplicationMessage(&message);

  EXPECT_EQ(PLAYLIST::Repeat::One, playLists.GetRepeat(PLAYLIST::Audio));
  EXPECT_EQ(1, playLists.GetPlayList(PLAYLIST::Audio).GetCurrentPosition());
}

TEST(TestApplicationPlayLists, APlayListPlaysInThePlayerAskedForWithThePlay)
{
  CTestPlayLists playLists;
  playLists.Queue(PLAYLIST::Video, *Items({"/video/a.mkv", "/video/b.mkv"}),
                  CApplicationPlayLists::Placement::End);

  KODI::MESSAGING::ThreadMessage message{TMSG_MEDIA_PLAY_PLAYLIST,
                                         static_cast<int>(PLAYLIST::Video), 1,
                                         new APPLICATION::PlayOptions{.player = "VideoPlayer"}};
  CPlayListsMessageHandler(playLists).OnApplicationMessage(&message);

  EXPECT_EQ((std::vector<std::string>{"/video/b.mkv"}), playLists.m_opened);
  EXPECT_EQ((std::vector<std::string>{"VideoPlayer"}), playLists.m_players);
}

TEST(TestApplicationPlayLists, AnEndedTrackLeavesItsPlayListPlayingUntilNothingFollows)
{
  CTestPlayLists playLists;
  playLists.Queue(PLAYLIST::Audio, std::make_shared<CFileItem>("/music/one.flac", false));
  ASSERT_TRUE(playLists.PlayFrom(PLAYLIST::Audio, 0));

  playLists.Started();
  CGUIMessage ended(GUI_MSG_PLAYBACK_ENDED, 0, 0);
  playLists.OnMessage(ended);
  EXPECT_EQ(PLAYLIST::Audio, playLists.GetPlayingType()) << "the next track has yet to be chosen";

  EXPECT_EQ(CApplicationPlayLists::Step::NothingThere,
            playLists.PlayNext(PLAYLIST::Advance::Automatic));
  EXPECT_EQ(std::nullopt, playLists.GetPlayingType());
}

// The stop that eventually ends the item still finds a playback to clean up.
TEST(TestApplicationPlayLists, ClearingThePlayingPlayListLeavesThePlaybackToItsStop)
{
  CTestPlayLists playLists;
  FillVideo(playLists);
  playLists.SetPlayingType(PLAYLIST::Video);

  playLists.Started();

  playLists.EditPlayList(PLAYLIST::Video).Clear();
  EXPECT_EQ(NO_ENTRY, playLists.EditPlayList(PLAYLIST::Video).GetCurrent());
  EXPECT_TRUE(playLists.GetPlayingType() == PLAYLIST::Video);

  CGUIMessage stopped(GUI_MSG_PLAYBACK_STOPPED, 0, 0);
  EXPECT_TRUE(playLists.OnMessage(stopped));
  EXPECT_FALSE(playLists.GetPlayingType());
}

// The cleared playlist is still the playing one, but has no current entry to look at.
TEST(TestApplicationPlayLists, ASingleItemPlaysAfterThePlayingPlayListIsCleared)
{
  CTestPlayLists playLists;
  FillVideo(playLists);
  playLists.SetPlayingType(PLAYLIST::Video);
  playLists.Started();
  playLists.EditPlayList(PLAYLIST::Video).Clear();

  KODI::MESSAGING::ThreadMessage message{TMSG_MEDIA_PLAY_ITEM, 0, 0,
                                         new CFileItem("/video/other.mkv", false)};
  CPlayListsMessageHandler(playLists).OnApplicationMessage(&message);

  ASSERT_FALSE(playLists.m_opened.empty());
  EXPECT_EQ("/video/other.mkv", playLists.m_opened.back());
}

TEST(TestApplicationPlayLists, ThePlayingEntryIsOnlyThePlayingPlayLists)
{
  CTestPlayLists playLists;
  FillVideo(playLists);
  const EntryId current = playLists.EditPlayList(PLAYLIST::Video).GetCurrent();
  EXPECT_EQ(NO_ENTRY, playLists.GetPlayingEntry(PLAYLIST::Video));

  playLists.SetPlayingType(PLAYLIST::Video);
  EXPECT_EQ(current, playLists.GetPlayingEntry(PLAYLIST::Video));
  EXPECT_EQ(NO_ENTRY, playLists.GetPlayingEntry(PLAYLIST::Audio));
}

TEST(TestApplicationPlayLists, RepeatCyclesOffAllOne)
{
  using enum PLAYLIST::Repeat;
  using enum CApplicationPlayLists::Persist;
  CTestPlayLists playLists;
  FillVideo(playLists);

  playLists.CycleRepeat(PLAYLIST::Video, No);
  EXPECT_EQ(All, playLists.GetRepeat(PLAYLIST::Video));
  playLists.CycleRepeat(PLAYLIST::Video, No);
  EXPECT_EQ(One, playLists.GetRepeat(PLAYLIST::Video));
  playLists.CycleRepeat(PLAYLIST::Video, No);
  EXPECT_EQ(Off, playLists.GetRepeat(PLAYLIST::Video));
}

TEST(TestApplicationPlayLists, ShuffleTogglesEachWay)
{
  CTestPlayLists playLists;
  FillVideo(playLists);

  playLists.ToggleShuffle(PLAYLIST::Video, CApplicationPlayLists::Persist::No);
  EXPECT_TRUE(playLists.IsShuffled(PLAYLIST::Video));
  playLists.ToggleShuffle(PLAYLIST::Video, CApplicationPlayLists::Persist::No);
  EXPECT_FALSE(playLists.IsShuffled(PLAYLIST::Video));
}

TEST(TestApplicationPlayLists, ThePlayingPositionIsOnlyThePlayingPlayList)
{
  CTestPlayLists playLists;
  FillVideo(playLists);
  EXPECT_EQ(-1, playLists.GetPlayingPosition(PLAYLIST::Video));

  playLists.SetPlayingType(PLAYLIST::Video);
  EXPECT_EQ(1, playLists.GetPlayingPosition(PLAYLIST::Video));
  EXPECT_EQ(-1, playLists.GetPlayingPosition(PLAYLIST::Audio));
}

TEST(TestApplicationPlayLists, APlayingPositionCanBeCountedFromTheCurrentEntry)
{
  CTestPlayLists playLists;
  FillVideo(playLists);
  EXPECT_EQ(-1, playLists.GetPlayingPosition(PLAYLIST::Video, -1)) << "not playing";

  playLists.SetPlayingType(PLAYLIST::Video);
  EXPECT_EQ(0, playLists.GetPlayingPosition(PLAYLIST::Video, -1));
  EXPECT_EQ(-1, playLists.GetPlayingPosition(PLAYLIST::Video, 1)) << "nothing follows the last";
}

TEST(TestApplicationPlayLists, QueueingReportsWhereTheItemsLanded)
{
  CTestPlayLists playLists;
  FillVideo(playLists);
  CFileItemList items;
  items.Add(std::make_shared<CFileItem>("/video/third.mkv", false));

  EXPECT_EQ(2, playLists.Queue(PLAYLIST::Video, items, CApplicationPlayLists::Placement::Next))
      << "nothing plays, so at the end";

  playLists.SetPlayingType(PLAYLIST::Video);
  playLists.Started();
  playLists.EditPlayList(PLAYLIST::Video)
      .SetCurrent(playLists.EditPlayList(PLAYLIST::Video).GetEntryId(0));

  EXPECT_EQ(1, playLists.Queue(PLAYLIST::Video, items, CApplicationPlayLists::Placement::Next))
      << "straight after the current";
  EXPECT_EQ(4, playLists.Queue(PLAYLIST::Video, items, CApplicationPlayLists::Placement::End));
  EXPECT_EQ(
      -1, playLists.Queue(PLAYLIST::Video, CFileItemList{}, CApplicationPlayLists::Placement::End));
}

TEST(TestApplicationPlayLists, AHandedOnEntryIsWhatStartedAndBecomesCurrent)
{
  CTestPlayLists playLists;
  FillVideo(playLists);
  playLists.SetPlayingType(PLAYLIST::Video);
  CPlayList& playList = playLists.EditPlayList(PLAYLIST::Video);
  const EntryId third = playList.Add(std::make_shared<CFileItem>("/video/third.mkv", false));

  playLists.OnNextQueued(third);
  playLists.Started(std::make_shared<CFileItem>("/resolved/third.mkv", false));

  EXPECT_EQ(third, playList.GetCurrent());
  EXPECT_EQ("/video/third.mkv", playLists.GetCurrentItem()->GetPath());
}

TEST(TestApplicationPlayLists, AHandedOnEntryStillListedAfterARebuildBecomesCurrent)
{
  CTestPlayLists playLists;
  FillVideo(playLists);
  playLists.SetPlayingType(PLAYLIST::Video);
  const CPlayList& playList = playLists.GetPlayList(PLAYLIST::Video);
  const EntryId third = playLists.EditPlayList(PLAYLIST::Video)
                            .Add(std::make_shared<CFileItem>("/video/third.mkv", false));
  playLists.OnNextQueued(third);

  playLists.Replace(PLAYLIST::Video,
                    *Items({"/video/second.mkv", "/video/new.mkv", "/video/third.mkv"}));
  playLists.Started(std::make_shared<CFileItem>("/video/third.mkv", false));

  EXPECT_EQ("/video/third.mkv", playList.GetCurrentItem()->GetPath())
      << "the second file does not play twice";
}

TEST(TestApplicationPlayLists, AHandedOnEntryThatLeftThePlayListIsStillWhatStarted)
{
  CTestPlayLists playLists;
  FillVideo(playLists);
  playLists.SetPlayingType(PLAYLIST::Video);
  CPlayList& playList = playLists.EditPlayList(PLAYLIST::Video);
  const EntryId second = playList.GetCurrent();
  const EntryId third = playList.Add(std::make_shared<CFileItem>("/video/third.mkv", false));

  playLists.OnNextQueued(third);
  playList.Remove(2);
  playLists.Started(std::make_shared<CFileItem>("/video/third.mkv", false));

  EXPECT_EQ("/video/third.mkv", playLists.GetCurrentItem()->GetPath())
      << "what the player reports is what started";
  EXPECT_EQ(second, playList.GetCurrent()) << "the cursor stays where the playlist left it";
}

TEST(TestApplicationPlayLists, TheEntryHandedToThePlayerCannotBeRemoved)
{
  CTestPlayLists playLists;
  FillVideo(playLists);
  playLists.SetPlayingType(PLAYLIST::Video);
  CPlayList& playList = playLists.EditPlayList(PLAYLIST::Video);
  const EntryId third = playList.Add(std::make_shared<CFileItem>("/video/third.mkv", false));

  playLists.OnNextQueued(third);
  EXPECT_FALSE(playLists.Remove(PLAYLIST::Video, 2));
  playLists.Remove(PLAYLIST::Video, "/video/third.mkv");
  EXPECT_EQ(3, playList.Size());
}

TEST(TestApplicationPlayLists, TheCurrentItemOutlivesPlaybackUntilReset)
{
  CTestPlayLists playLists;
  EXPECT_TRUE(playLists.GetCurrentItem()->GetPath().empty());

  playLists.Started(std::make_shared<CFileItem>("/music/one.flac", false));
  const auto current = playLists.GetCurrentItem();
  EXPECT_EQ("/music/one.flac", current->GetPath());

  CGUIMessage stopped(GUI_MSG_PLAYBACK_STOPPED, 0, 0);
  playLists.OnMessage(stopped);
  EXPECT_EQ(current, playLists.GetCurrentItem());

  playLists.ResetCurrentItem();
  EXPECT_TRUE(playLists.GetCurrentItem()->GetPath().empty());
}

TEST(TestApplicationPlayLists, PlayingAnEmptyPlayListChoosesNothing)
{
  CTestPlayLists playLists;
  playLists.SetPlayingType(PLAYLIST::Audio);

  EXPECT_FALSE(playLists.PlayFrom(PLAYLIST::Video));
  EXPECT_TRUE(playLists.GetPlayingType() == PLAYLIST::Audio);
}

TEST(TestApplicationPlayLists, ASongOnTheVideoPlayListIsStillAudio)
{
  CTestPlayLists playLists;
  CPlayList& playList = playLists.EditPlayList(PLAYLIST::Video);
  playList.Add(std::make_shared<CFileItem>("/video/film.mkv", false));
  const EntryId song = playList.Add(std::make_shared<CFileItem>("/music/song.flac", false));
  playList.SetCurrent(song);
  playLists.SetPlayingType(PLAYLIST::Video);

  EXPECT_TRUE(playLists.IsPlayingAsAudio());
  EXPECT_EQ(MEDIA::Streams::VideoAndAudio,
            playLists.GetStreams(PLAYLIST::Video, playList.GetEntryId(0)));
}

TEST(TestApplicationPlayLists, WhatPlaysIsWhatThePlayingEntryHolds)
{
  CTestPlayLists playLists;
  EXPECT_FALSE(playLists.GetPlayingStreams().has_value());

  CPlayList& playList = playLists.EditPlayList(PLAYLIST::Video);
  const EntryId film = playList.Add(std::make_shared<CFileItem>("/video/film.mkv", false));
  const EntryId song = playList.Add(std::make_shared<CFileItem>("/music/song.flac", false));
  playList.SetCurrent(film);
  playLists.SetPlayingType(PLAYLIST::Video);
  EXPECT_EQ(MEDIA::Streams::VideoAndAudio, playLists.GetPlayingStreams());

  playList.SetCurrent(song);
  EXPECT_EQ(MEDIA::Streams::Audio, playLists.GetPlayingStreams());
}

TEST(TestApplicationPlayLists, AnItemThatSaysNothingHoldsWhatItsPlayListClaims)
{
  CTestPlayLists playLists;
  const EntryId entry = playLists.EditPlayList(PLAYLIST::Audio)
                            .Add(std::make_shared<CFileItem>("plugin://plugin.audio.x/1", false));
  EXPECT_EQ(MEDIA::Streams::Audio, playLists.GetStreams(PLAYLIST::Audio, entry));
}
TEST(TestApplicationPlayLists, OnlyWhatCanPlayBecomesAnEntry)
{
  CFileItemList listing;
  listing.Add(std::make_shared<CFileItem>("/music/", true));
  listing.Add(std::make_shared<CFileItem>("/music/one.flac", false));
  listing.Add(std::make_shared<CFileItem>("/music/art.zip", false));
  listing.Add(std::make_shared<CFileItem>("/music/album.nfo", false));
  listing.Add(std::make_shared<CFileItem>("/music/two.flac", false));

  CTestPlayLists playLists;
  EXPECT_EQ(0, playLists.Queue(PLAYLIST::Audio, listing, CApplicationPlayLists::Placement::End));
  ASSERT_EQ(2, playLists.GetPlayList(PLAYLIST::Audio).Size());
  EXPECT_EQ("/music/two.flac", playLists.GetPlayList(PLAYLIST::Audio)[1]->GetPath());

  CFileItemList entries;
  EXPECT_EQ(1, CTestPlayLists::EntriesOf(listing, 4, entries))
      << "the chosen item keeps its place among what is kept";
  EXPECT_EQ(2, entries.Size());
  CFileItemList more;
  EXPECT_EQ(std::nullopt, CTestPlayLists::EntriesOf(listing, 2, more))
      << "a chosen item that cannot play starts nothing";
}

namespace
{
// Numbered items, handed over in order.
class CFakeFeed : public IFeed
{
public:
  explicit CFakeFeed(int total) : m_total(total) {}

  std::vector<std::shared_ptr<CFileItem>> Take(int count) override
  {
    std::vector<std::shared_ptr<CFileItem>> items;
    for (; count > 0 && m_next < m_total; --count, ++m_next)
      items.push_back(
          std::make_shared<CFileItem>("/music/" + std::to_string(m_next) + ".flac", false));
    return items;
  }
  void Restart() override
  {
    m_next = 0;
    ++m_restarts;
  }
  int GetTotal() const override { return m_total; }
  int GetLeft() const override { return m_total - m_next; }

  int m_total;
  int m_next{0};
  int m_restarts{0};
};

// A fed Audio playlist with one entry of its own, current.
std::shared_ptr<CFakeFeed> StartFed(CTestPlayLists& playLists, int total)
{
  auto feed = std::make_shared<CFakeFeed>(total);
  CPlayList& playList = playLists.EditPlayList(PLAYLIST::Audio);
  playList.SetFeed(feed);
  playList.SetCurrent(playList.Add(std::make_shared<CFileItem>("/music/first.flac", false)));
  return feed;
}
} // namespace

TEST(TestApplicationPlayLists, AFedPlayListIsKeptTenAheadAsTheCursorMoves)
{
  CTestPlayLists playLists;
  const auto feed = StartFed(playLists, 50);
  CPlayList& playList = playLists.EditPlayList(PLAYLIST::Audio);
  EXPECT_EQ(11, playList.Size());
  EXPECT_EQ(40, playLists.GetFeedLeft());
  EXPECT_EQ(50, playLists.GetFeedTotal());

  playList.SetCurrent(playList.GetEntryId(5));
  EXPECT_EQ(16, playList.Size()) << "ten ahead of the new current entry";
}

TEST(TestApplicationPlayLists, RemovingFromAFedPlayListTopsItUp)
{
  CTestPlayLists playLists;
  StartFed(playLists, 50);
  EXPECT_TRUE(playLists.Remove(PLAYLIST::Audio, 3));
  EXPECT_EQ(11, playLists.GetPlayList(PLAYLIST::Audio).Size());
}

TEST(TestApplicationPlayLists, AFedPlayListThatWrapsStartsItsFeedOver)
{
  CTestPlayLists playLists;
  playLists.EditPlayList(PLAYLIST::Audio).SetWrap(Wrap::ToStart);
  const auto feed = StartFed(playLists, 3);
  EXPECT_EQ(1, feed->m_restarts);
  EXPECT_EQ(7, playLists.GetPlayList(PLAYLIST::Audio).Size()) << "the three, then the three again";
}

TEST(TestApplicationPlayLists, AFeedThatRunsOutWithoutWrapPlacesNoMore)
{
  CTestPlayLists playLists;
  const auto feed = StartFed(playLists, 3);
  EXPECT_EQ(0, feed->m_restarts);
  EXPECT_EQ(4, playLists.GetPlayList(PLAYLIST::Audio).Size());
}

TEST(TestApplicationPlayLists, AnItemPlayedOnAFedPlayListUsesTheCallersPlayer)
{
  CTestPlayLists playLists;
  StartFed(playLists, 50);
  ASSERT_TRUE(playLists.PlayFrom(PLAYLIST::Audio, 0));
  playLists.Started();

  const auto item = std::make_shared<CFileItem>("/music/asked.flac", false);
  ASSERT_TRUE(playLists.PlayItem(PLAYLIST::Audio, item, {.player = "chosen"}));
  EXPECT_EQ("/music/asked.flac", playLists.m_opened.back());
  EXPECT_EQ("chosen", playLists.m_players.back());
}

TEST(TestApplicationPlayLists, StartingOrRestartingAFeedReportsItOnce)
{
  struct CObserver : CApplicationPlayLists::IObserver
  {
    std::vector<bool> feeds;
    void OnStarted(const std::shared_ptr<CFileItem>&) override {}
    void OnListChanged(PLAYLIST::Type, const PLAYLIST::PlayListChange&) override {}
    void OnShuffled(PLAYLIST::Type, bool) override {}
    void OnRepeat(PLAYLIST::Type, PLAYLIST::Repeat) override {}
    void OnFeed(bool playing) override { feeds.push_back(playing); }
    void OnFailed(const std::shared_ptr<const CFileItem>&, PLAYLIST::FailReason) override {}
  } observer;

  CTestPlayLists playLists;
  playLists.SetObserver(&observer);
  ASSERT_TRUE(
      playLists.PlayFeed(PLAYLIST::Audio, std::make_shared<CFakeFeed>(50), PLAYLIST::Repeat::All));
  EXPECT_EQ(std::vector<bool>{true}, observer.feeds);

  ASSERT_TRUE(
      playLists.PlayFeed(PLAYLIST::Audio, std::make_shared<CFakeFeed>(50), PLAYLIST::Repeat::All));
  EXPECT_EQ((std::vector<bool>{true, true}), observer.feeds) << "a restart is not off then on";
  playLists.SetObserver(nullptr);
}

TEST(TestApplicationPlayLists, StartingAFeedOnThePlayingPlayListLeavesTheScreenAsItIs)
{
  CTestPlayLists playLists;
  ASSERT_TRUE(playLists.PlayItems(PLAYLIST::Audio, *Items({"/music/one.flac"})));
  ASSERT_TRUE(
      playLists.PlayFeed(PLAYLIST::Audio, std::make_shared<CFakeFeed>(50), PLAYLIST::Repeat::All));
  EXPECT_EQ(KODI::APPLICATION::StartsRun::No, playLists.m_startsRun.back());
}

TEST(TestApplicationPlayLists, AShuffleAskedForWithAListOverAPartyIsKept)
{
  CTestPlayLists playLists;
  StartFed(playLists, 50);

  auto list = Items({"/music/one.flac", "/music/two.flac", "/music/three.flac"});
  list->SetProperty("shuffled", true);
  KODI::MESSAGING::ThreadMessage message{TMSG_MEDIA_PLAY_ITEMS, 0, 0, list.release()};
  CPlayListsMessageHandler(playLists).OnApplicationMessage(&message);

  EXPECT_EQ(std::nullopt, playLists.GetFedType()) << "the party has ended";
  EXPECT_TRUE(playLists.IsShuffled(PLAYLIST::Audio));
}

TEST(TestApplicationPlayLists, DroppingTheFeedClearsItsPlayList)
{
  CTestPlayLists playLists;
  StartFed(playLists, 50);
  ASSERT_EQ(PLAYLIST::Audio, playLists.GetFedType());

  playLists.DropFeed();
  EXPECT_EQ(std::nullopt, playLists.GetFedType());
  EXPECT_TRUE(playLists.GetPlayList(PLAYLIST::Audio).IsEmpty());
  EXPECT_EQ(-1, playLists.GetFeedLeft());
}

TEST(TestApplicationPlayLists, AFedPlayListIsKeptTenAheadInPlayOrderWhenShuffled)
{
  CTestPlayLists playLists;
  StartFed(playLists, 500);
  playLists.SetShuffle(PLAYLIST::Audio, true, CApplicationPlayLists::Persist::No);
  CPlayList& playList = playLists.EditPlayList(PLAYLIST::Audio);

  for (int step = 0; step < 20; ++step)
  {
    playList.Next(Advance::Automatic);
    const int ahead = playList.Size() - playList.GetPlayOrderPosition(playList.GetCurrent()) - 1;
    EXPECT_EQ(10, ahead) << "after step " << step;
  }
}

TEST(TestApplicationPlayLists, DroppingTheFeedBringsBackTheSavedShuffleAndRepeat)
{
  CTestPlayLists playLists;
  StartFed(playLists, 50);
  playLists.SetShuffle(PLAYLIST::Audio, true, CApplicationPlayLists::Persist::No);
  playLists.SetRepeat(PLAYLIST::Audio, PLAYLIST::Repeat::All, CApplicationPlayLists::Persist::No);

  playLists.DropFeed();
  EXPECT_FALSE(playLists.IsShuffled(PLAYLIST::Audio));
  EXPECT_EQ(PLAYLIST::Repeat::Off, playLists.GetRepeat(PLAYLIST::Audio));
}

TEST(TestApplicationPlayLists, ReplacingAFedPlayListDropsTheFeed)
{
  CTestPlayLists playLists;
  StartFed(playLists, 50);
  CFileItemList items;
  items.Add(std::make_shared<CFileItem>("/music/other.flac", false));
  playLists.Replace(PLAYLIST::Audio, items);
  EXPECT_EQ(std::nullopt, playLists.GetFedType());
  EXPECT_EQ(1, playLists.GetPlayList(PLAYLIST::Audio).Size());
}

TEST(TestApplicationPlayLists, ThePlayListShownIsThePlayingOneElseThePreferredOneWithEntries)
{
  CTestPlayLists playLists;
  EXPECT_EQ(std::nullopt, playLists.GetTypeToShow(PLAYLIST::Audio)) << "nothing to show";

  playLists.EditPlayList(PLAYLIST::Audio).Add(std::make_shared<CFileItem>("/music/a.flac", false));
  playLists.EditPlayList(PLAYLIST::Video).Add(std::make_shared<CFileItem>("/video/v.mkv", false));
  EXPECT_EQ(PLAYLIST::Audio, playLists.GetTypeToShow(PLAYLIST::Audio));
  EXPECT_EQ(PLAYLIST::Video, playLists.GetTypeToShow(std::nullopt));

  playLists.SetPlayingType(PLAYLIST::Video);
  EXPECT_EQ(PLAYLIST::Video, playLists.GetTypeToShow(PLAYLIST::Audio));
}

TEST(TestApplicationPlayLists, AnEmptyPreferredPlayListGivesWayToOneWithEntries)
{
  CTestPlayLists playLists;
  playLists.EditPlayList(PLAYLIST::Audio).Add(std::make_shared<CFileItem>("/music/a.flac", false));

  EXPECT_EQ(PLAYLIST::Audio, playLists.GetTypeToShow(PLAYLIST::Video));
}

TEST(TestApplicationPlayLists, AnEndedEpgItemHoldsThePlayerOpenAndThePlayListWhereItIs)
{
  CTestPlayLists playLists;
  FillVideo(playLists);
  playLists.SetPlayingType(PLAYLIST::Video);
  const EntryId current = playLists.GetPlayList(PLAYLIST::Video).GetCurrent();

  CFileItem ended("/tv/programme.ts", false);
  ended.SetProperty("epg_playlist_item", true);

  EXPECT_FALSE(playLists.OnEntryEnded(ended));
  EXPECT_EQ(current, playLists.GetPlayList(PLAYLIST::Video).GetCurrent());
  EXPECT_EQ(PLAYLIST::Video, playLists.GetPlayingType());
}

TEST_F(TestPlayListExpansion, APlayListFileThatListsItselfIsOpenedOnce)
{
  const std::string path = WriteFile("self.m3u", "one.mp3\nself.m3u\ntwo.mp3\n");
  ASSERT_FALSE(path.empty());
  CEntriesAsListed rules;
  CFileItemList entries;

  CApplicationPlayLists::ExpandToEntries(std::make_shared<CFileItem>(path, false), rules, nullptr,
                                         entries);

  EXPECT_EQ((std::vector<std::string>{"one.mp3", "two.mp3"}), FileNames(entries));
}

TEST_F(TestPlayListExpansion, APlayListFileInAFolderGivesItsEntries)
{
  ASSERT_FALSE(WriteFile("list.m3u", "one.mp3\ntwo.mp3\n").empty());
  CEntriesAsListed rules;
  CFileItemList entries;

  CApplicationPlayLists::ExpandToEntries(std::make_shared<CFileItem>(EXPANSION_FOLDER, true), rules,
                                         nullptr, entries);

  EXPECT_EQ((std::vector<std::string>{"one.mp3", "two.mp3"}), FileNames(entries));
}

TEST_F(TestPlayListExpansion, TheStartIsWhereTheItemAskedForLanded)
{
  const std::string path = WriteFile("list.m3u", "one.mp3\ntwo.mp3\n");
  ASSERT_FALSE(path.empty());
  CEntriesAsListed rules;
  CFileItemList entries;

  const std::optional<int> start = CApplicationPlayLists::ExpandToEntries(
      std::make_shared<CFileItem>(path, false), rules,
      std::make_shared<CFileItem>(URIUtils::AddFileToFolder(EXPANSION_FOLDER, "two.mp3"), false),
      entries);

  EXPECT_EQ(1, start);
}

TEST(TestApplicationPlayLists, AnEntryHandedOnFromOnePlayListDoesNotProtectTheOthers)
{
  CTestPlayLists playLists;
  FillVideo(playLists);
  playLists.SetPlayingType(PLAYLIST::Video);
  const EntryId handedOn = playLists.GetPlayList(PLAYLIST::Video).GetEntryId(0);
  playLists.OnNextQueued(handedOn);

  CPlayList& audio = playLists.EditPlayList(PLAYLIST::Audio);
  audio.Add(std::make_shared<CFileItem>("/music/a.flac", false));
  ASSERT_EQ(handedOn, audio.GetEntryId(0)) << "both playlists number from 1";

  EXPECT_TRUE(playLists.Remove(PLAYLIST::Audio, 0));
  EXPECT_FALSE(playLists.Remove(PLAYLIST::Video, 0)) << "the handed-on entry itself stays";
}

TEST(TestApplicationPlayLists, ASlideShowDoesNotMakeAnIdleVideoPlayListPlay)
{
  CTestPlayLists playLists;
  FillVideo(playLists);
  playLists.SetPlayingType(PLAYLIST::Video);

  playLists.SetSlideShowRunning(true);

  EXPECT_TRUE(playLists.IsSlideShowRunning());
  EXPECT_EQ(PLAYLIST::Video, playLists.GetPlayingType()) << "the slideshow takes no playlist";
}

TEST(TestApplicationPlayLists, AFileStartedOutsideThePlayListsIsJudgedByItself)
{
  CTestPlayLists playLists;
  FillVideo(playLists);
  playLists.SetPlayingType(PLAYLIST::Video);

  EXPECT_TRUE(playLists.IsStartingAsAudio(CFileItem("/music/elsewhere.flac", false)));
  EXPECT_FALSE(playLists.IsStartingAsAudio(CFileItem("/video/second.mkv", false)))
      << "the current entry answers for itself";
}

TEST_F(TestPlayListExpansion, APlayListFileEntryIsOpenedWhenItPlays)
{
  const std::string path = WriteFile("list.m3u", "one.mp3\ntwo.mp3\n");
  ASSERT_FALSE(path.empty());
  CTestPlayLists playLists;
  playLists.Queue(PLAYLIST::Audio, std::make_shared<CFileItem>(path, false));

  ASSERT_TRUE(playLists.PlayFrom(PLAYLIST::Audio, 0));

  ASSERT_EQ(1u, playLists.m_opened.size());
  EXPECT_EQ("one.mp3", URIUtils::GetFileName(playLists.m_opened.front()));
  EXPECT_EQ(2, playLists.GetPlayList(PLAYLIST::Audio).Size());
}

TEST_F(TestPlayListExpansion, AGamesListOfDiscsReachesThePlayerWhole)
{
  const std::string path = WriteFile("game.m3u", "disc1.cue\ndisc2.cue\n");
  ASSERT_FALSE(path.empty());
  auto game = std::make_shared<CFileItem>(path, false);
  game->GetGameInfoTag();
  CTestPlayLists playLists;
  playLists.Queue(PLAYLIST::Video, game);

  ASSERT_TRUE(playLists.PlayFrom(PLAYLIST::Video, 0));

  EXPECT_EQ((std::vector<std::string>{path}), playLists.m_opened);
  EXPECT_EQ(1, playLists.GetPlayList(PLAYLIST::Video).Size());
}
