/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "GUIUserMessages.h"
#include "application/ApplicationPlayLists.h"
#include "application/PlaybackAnnouncer.h"
#include "application/test/PlayListsTestHelpers.h"
#include "guilib/GUIMessage.h"
#include "playlists/PlayList.h"

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace KODI::APPLICATION::TEST;
using namespace KODI::PLAYLIST;

namespace
{
struct Published
{
  ANNOUNCEMENT::AnnouncementFlag flag;
  std::string message;
  std::shared_ptr<const CFileItem> item;
  // Temporary: the event itself rather than the data JSON-RPC and Python are given; removed with
  // JSON-RPC v15.
  ANNOUNCEMENT::Announcement announcement;
};

template<class Event, class Flag>
const Event* EventOf(const Published* published)
{
  const auto* flag = published ? std::get_if<Flag>(&published->announcement) : nullptr;
  return flag ? std::get_if<Event>(flag) : nullptr;
}

class TestPlaybackAnnouncer : public ::testing::Test
{
protected:
  // The playlists see a message before the announcer, as they are registered.
  void Send(int message, const std::shared_ptr<CFileItem>& item = nullptr)
  {
    CGUIMessage msg(message, 0, 0, 0, 0, item);
    m_playLists->OnMessage(msg);
    m_announcer.OnMessage(msg);
  }

  const Published* Find(const std::string& message) const
  {
    for (const Published& published : m_published)
      if (published.message == message)
        return &published;
    return nullptr;
  }

  std::shared_ptr<CTestPlayLists> m_playLists = std::make_shared<CTestPlayLists>();
  std::vector<Published> m_published;
  CPlaybackAnnouncer m_announcer{m_playLists, [this](const ANNOUNCEMENT::Announcement& announcement)
                                 {
                                   m_published.push_back({ANNOUNCEMENT::FlagOf(announcement),
                                                          ANNOUNCEMENT::MessageOf(announcement),
                                                          ANNOUNCEMENT::ItemOf(announcement),
                                                          announcement});
                                 }};
};
} // namespace

TEST_F(TestPlaybackAnnouncer, AStartingEntryHoldsWhatItsPlayListClaims)
{
  m_playLists->SetPlayingType(PLAYLIST::Audio);
  EXPECT_EQ(MEDIA::Streams::Audio, m_announcer.GetStreams(nullptr, true));

  m_playLists->SetPlayingType(PLAYLIST::Video);
  EXPECT_EQ(MEDIA::Streams::VideoAndAudio, m_announcer.GetStreams(nullptr, true));
}

TEST_F(TestPlaybackAnnouncer, ASongOnTheVideoPlayListStartsAsAudio)
{
  CPlayList& playList = m_playLists->EditPlayList(PLAYLIST::Video);
  playList.Add(std::make_shared<CFileItem>("/video/film.mkv", false));
  playList.SetCurrent(playList.Add(std::make_shared<CFileItem>("/music/song.flac", false)));
  m_playLists->SetPlayingType(PLAYLIST::Video);

  EXPECT_EQ(MEDIA::Streams::Audio, m_announcer.GetStreams(nullptr, true));
}

TEST_F(TestPlaybackAnnouncer, WhatStartedIsACopyOfWhatThePlayerReports)
{
  const auto reported = std::make_shared<CFileItem>("/video/film.mkv", false);
  Send(GUI_MSG_PLAYBACK_STARTED, reported);

  const auto item = m_playLists->GetCurrentItem();
  ASSERT_NE(nullptr, item);
  EXPECT_NE(reported, item);
  EXPECT_EQ("/video/film.mkv", item->GetPath());
}

TEST_F(TestPlaybackAnnouncer, WhatStartedIsPublishedWithWhatItClaimsAndForgottenOnStop)
{
  m_playLists->EditPlayList(PLAYLIST::Audio)
      .SetCurrent(m_playLists->EditPlayList(PLAYLIST::Audio)
                      .Add(std::make_shared<CFileItem>("/music/one.flac", false)));
  m_playLists->SetPlayingType(PLAYLIST::Audio);
  Send(GUI_MSG_PLAYBACK_STARTED, std::make_shared<CFileItem>("/music/one.flac", false));

  const Published* play = Find("OnPlay");
  ASSERT_NE(nullptr, play);
  EXPECT_EQ(ANNOUNCEMENT::Player, play->flag);
  const auto* played = EventOf<ANNOUNCEMENT::EVENT::PLAYER::Play, ANNOUNCEMENT::PlayerEvent>(play);
  ASSERT_NE(nullptr, played);
  EXPECT_EQ(MEDIA::Streams::Audio, played->streams);
  EXPECT_EQ(1, played->speed);

  Send(GUI_MSG_PLAYBACK_STOPPED);
  const Published* stop = Find("OnStop");
  ASSERT_NE(nullptr, stop);
  const auto* stopped = EventOf<ANNOUNCEMENT::EVENT::PLAYER::Stop, ANNOUNCEMENT::PlayerEvent>(stop);
  ASSERT_NE(nullptr, stopped);
  EXPECT_FALSE(stopped->end);
  ASSERT_NE(nullptr, stop->item);
  EXPECT_EQ("/music/one.flac", stop->item->GetPath());
}

TEST_F(TestPlaybackAnnouncer, NothingIsPublishedWhenThePlayerReportsNoItem)
{
  Send(GUI_MSG_PLAYBACK_STARTED);
  EXPECT_EQ(nullptr, Find("OnPlay"));
}

TEST_F(TestPlaybackAnnouncer, AListChangeNamesItsPlayList)
{
  m_playLists->Queue(PLAYLIST::Audio, std::make_shared<CFileItem>("/music/one.flac", false));

  const Published* added = Find("OnAdd");
  ASSERT_NE(nullptr, added);
  EXPECT_EQ(ANNOUNCEMENT::Playlist, added->flag);
  const auto* add = EventOf<ANNOUNCEMENT::EVENT::PLAYLIST::Add, ANNOUNCEMENT::PlaylistEvent>(added);
  ASSERT_NE(nullptr, add);
  EXPECT_EQ(PLAYLIST::Audio, add->playList);
  EXPECT_EQ(0, add->position);

  m_playLists->Clear(PLAYLIST::Audio);
  const auto* clear =
      EventOf<ANNOUNCEMENT::EVENT::PLAYLIST::Clear, ANNOUNCEMENT::PlaylistEvent>(Find("OnClear"));
  ASSERT_NE(nullptr, clear);
  EXPECT_EQ(PLAYLIST::Audio, clear->playList);

  m_announcer.OnSlideShowListChanged({PlayListChange::Type::Cleared});
  const auto* slides = EventOf<ANNOUNCEMENT::EVENT::PLAYLIST::Clear, ANNOUNCEMENT::PlaylistEvent>(
      &m_published.back());
  ASSERT_NE(nullptr, slides);
  EXPECT_FALSE(slides->playList) << "the slideshow's pictures are no playlist";
}

TEST_F(TestPlaybackAnnouncer, ASlideShowHoldsVideoUnderMusic)
{
  const auto slide = std::make_shared<CFileItem>("/pictures/one.jpg", false);

  m_announcer.OnSlideShow(CPlaybackAnnouncer::SlideShowEvent::Play, slide, true);
  EXPECT_TRUE(m_playLists->IsSlideShowRunning());
  const auto* played =
      EventOf<ANNOUNCEMENT::EVENT::PLAYER::Play, ANNOUNCEMENT::PlayerEvent>(Find("OnPlay"));
  ASSERT_NE(nullptr, played);
  EXPECT_EQ(MEDIA::Streams::Video, played->streams);

  CPlayList& audio = m_playLists->EditPlayList(PLAYLIST::Audio);
  audio.SetCurrent(audio.Add(std::make_shared<CFileItem>("/music/one.flac", false)));
  m_playLists->SetPlayingType(PLAYLIST::Audio);
  Send(GUI_MSG_PLAYBACK_STARTED);

  EXPECT_EQ(PLAYLIST::Audio, m_playLists->GetPlayingType());
  EXPECT_TRUE(m_playLists->IsSlideShowRunning()) << "music does not end the slideshow";

  m_announcer.OnSlideShow(CPlaybackAnnouncer::SlideShowEvent::Pause, slide, false);
  EXPECT_TRUE(m_playLists->IsSlideShowRunning());

  m_announcer.OnSlideShow(CPlaybackAnnouncer::SlideShowEvent::Stop, slide, false);
  EXPECT_FALSE(m_playLists->IsSlideShowRunning());
  const auto* stopped =
      EventOf<ANNOUNCEMENT::EVENT::PLAYER::Stop, ANNOUNCEMENT::PlayerEvent>(&m_published.back());
  ASSERT_NE(nullptr, stopped);
  EXPECT_TRUE(stopped->end);
}

TEST_F(TestPlaybackAnnouncer, ASlideIsAnnouncedAsAPictureBeforeItsTagIsRead)
{
  const auto slide = std::make_shared<CFileItem>("/pictures/untagged.jpg", false);

  m_announcer.OnSlideShow(CPlaybackAnnouncer::SlideShowEvent::Stop, slide, false);

  ASSERT_NE(nullptr, Find("OnStop"));
  ASSERT_NE(nullptr, Find("OnStop")->item);
  EXPECT_TRUE(Find("OnStop")->item->HasPictureInfoTag());
}

TEST_F(TestPlaybackAnnouncer, ShuffleAndRepeatAreAnnouncedOnTheirPlayListOnly)
{
  m_playLists->SetPlayingType(PLAYLIST::Audio);
  CApplicationPlayLists::IObserver& observer = m_announcer;

  observer.OnShuffled(PLAYLIST::Audio, true);
  observer.OnRepeat(PLAYLIST::Audio, PLAYLIST::Repeat::All);
  m_announcer.OnSlideShowShuffled();

  int onPlayList = 0;
  for (const Published& published : m_published)
  {
    EXPECT_NE(ANNOUNCEMENT::Player, published.flag) << published.message;
    if (published.flag == ANNOUNCEMENT::Playlist && published.message == "OnPropertiesChanged")
      ++onPlayList;
  }
  EXPECT_EQ(3, onPlayList);
}
