/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/AnnouncementV14.h"
#include "threads/Event.h"
#include "utils/Variant.h"
#include "video/VideoInfoTag.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

#include <gtest/gtest.h>

using namespace ANNOUNCEMENT;
using namespace std::chrono_literals;

namespace
{
constexpr auto TIMEOUT = 5s;

OtherEvent TestEvent()
{
  return {CAnnouncementManager::ANNOUNCEMENT_SENDER, "Test", {}};
}

//! Stands in for an announcer that waits on another thread, as closing a window waits on the GUI
//! thread.
class CBlockingAnnouncer : public IAnnouncer
{
public:
  void OnAnnouncement(const Announcement& announcement) override
  {
    m_entered.Set();
    m_released = m_release.Wait(TIMEOUT);
  }

  CEvent m_entered;
  CEvent m_release;
  std::atomic<bool> m_released{false};
};

class CRecordingAnnouncer : public IAnnouncer
{
public:
  void OnAnnouncement(const Announcement& announcement) override
  {
    if (m_removed)
      m_calledAfterRemoval = true;
    m_data = V14::NotificationsOf(announcement).at(0).data;
    m_called.Set();
  }

  std::atomic<bool> m_removed{false};
  std::atomic<bool> m_calledAfterRemoval{false};
  CVariant m_data; //!< Of the last announcement
  CEvent m_called;
};

class CSelfRemovingAnnouncer : public IAnnouncer
{
public:
  explicit CSelfRemovingAnnouncer(CAnnouncementManager& manager) : m_manager(manager) {}

  void OnAnnouncement(const Announcement& announcement) override
  {
    m_manager.RemoveAnnouncer(this);
    m_removed.Set();
  }

  CAnnouncementManager& m_manager;
  CEvent m_removed;
};

class CThrowingAnnouncer : public IAnnouncer
{
public:
  void OnAnnouncement(const Announcement& announcement) override
  {
    m_called.Set();
    throw std::runtime_error("announcer failed");
  }

  CEvent m_called;
};
} // namespace

class TestAnnouncementManager : public ::testing::Test
{
protected:
  void SetUp() override { m_manager.Start(); }

  // Declared before the manager so they outlive its thread.
  CBlockingAnnouncer m_blocking;
  CRecordingAnnouncer m_recording;
  CAnnouncementManager m_manager;
};

TEST_F(TestAnnouncementManager, AnAnnouncerCanBeAddedWhileAnotherIsBeingCalled)
{
  m_manager.AddAnnouncer(&m_blocking);
  m_manager.Announce(TestEvent());
  ASSERT_TRUE(m_blocking.m_entered.Wait(TIMEOUT));

  m_manager.AddAnnouncer(&m_recording);
  m_blocking.m_release.Set();
  m_manager.Deinitialize();

  EXPECT_TRUE(m_blocking.m_released);
}

TEST_F(TestAnnouncementManager, AnAnnouncerRemovedDuringADispatchIsNotCalledAfterwards)
{
  m_manager.AddAnnouncer(&m_blocking);
  m_manager.AddAnnouncer(&m_recording);
  m_manager.Announce(TestEvent());
  ASSERT_TRUE(m_blocking.m_entered.Wait(TIMEOUT));

  m_manager.RemoveAnnouncer(&m_recording);
  m_recording.m_removed = true;
  m_blocking.m_release.Set();
  m_manager.Deinitialize();

  EXPECT_TRUE(m_blocking.m_released);
  EXPECT_FALSE(m_recording.m_calledAfterRemoval);
}

TEST_F(TestAnnouncementManager, RemovingAnAnnouncerWaitsForItsCallInProgress)
{
  m_manager.AddAnnouncer(&m_blocking);
  m_manager.Announce(TestEvent());
  ASSERT_TRUE(m_blocking.m_entered.Wait(TIMEOUT));

  std::atomic<bool> removed{false};
  std::thread remover(
      [this, &removed]
      {
        m_manager.RemoveAnnouncer(&m_blocking);
        removed = true;
      });
  std::this_thread::sleep_for(200ms);
  EXPECT_FALSE(removed);

  m_blocking.m_release.Set();
  remover.join();
  EXPECT_TRUE(removed);
  EXPECT_TRUE(m_blocking.m_released);
}

TEST_F(TestAnnouncementManager, AnAnnouncerCanRemoveItselfWhileBeingCalled)
{
  CSelfRemovingAnnouncer announcer(m_manager);
  m_manager.AddAnnouncer(&announcer);
  m_manager.Announce(TestEvent());

  EXPECT_TRUE(announcer.m_removed.Wait(TIMEOUT));
  m_manager.Deinitialize();
}

// A lent item is copied as it is announced, so that it may change, or go, at once
TEST_F(TestAnnouncementManager, ALentItemIsAnnouncedAsItWasThen)
{
  // The announcement thread is held until the item has changed and gone
  m_manager.AddAnnouncer(&m_blocking, Other);
  m_manager.AddAnnouncer(&m_recording, VideoLibrary);
  m_manager.Announce(TestEvent());
  ASSERT_TRUE(m_blocking.m_entered.Wait(TIMEOUT));
  {
    CFileItem item{"smb://host/Show/Show S01E01.mkv", false};
    item.GetVideoInfoTag()->m_iDbId = 7;
    item.GetVideoInfoTag()->SetMediaType(KODI::MEDIA::TYPE::EPISODE);
    m_manager.Announce(VideoLibraryEvent{EVENT::LIBRARY::Update{
        .item = std::shared_ptr<const CFileItem>(&item, [](const CFileItem*) {}), .added = true}});
    item.GetVideoInfoTag()->m_iDbId = 8;
  }
  m_blocking.m_release.Set();
  const bool called{m_recording.m_called.Wait(TIMEOUT)};
  m_manager.RemoveAnnouncer(&m_recording);

  ASSERT_TRUE(called);
  EXPECT_EQ(7, m_recording.m_data["item"]["id"].asInteger());
  EXPECT_EQ("episode", m_recording.m_data["item"]["type"].asString());
  EXPECT_TRUE(m_recording.m_data["added"].asBoolean());
}

TEST(TestAnnouncementManagerFailure, RemovingAnAnnouncerWhoseCallThrewDoesNotWait)
{
  // Leaked if the removal never returns, so the thread left waiting in it touches nothing freed
  auto manager = std::make_unique<CAnnouncementManager>();
  auto announcer = std::make_unique<CThrowingAnnouncer>();
  manager->Start();
  manager->AddAnnouncer(announcer.get());
  manager->Announce(TestEvent());
  ASSERT_TRUE(announcer->m_called.Wait(TIMEOUT));

  auto removed = std::make_shared<CEvent>();
  std::thread remover(
      [m = manager.get(), a = announcer.get(), removed]
      {
        m->RemoveAnnouncer(a);
        removed->Set();
      });
  if (!removed->Wait(TIMEOUT))
  {
    remover.detach();
    manager.release();
    announcer.release();
    FAIL() << "RemoveAnnouncer is still waiting on a call that threw";
  }
  remover.join();
}

namespace
{
class CTypedAnnouncer : public IAnnouncer
{
public:
  void OnPlayerEvent(const PlayerEvent& event) override
  {
    m_event = event;
    m_typed.Set();
  }

  void OnAnnouncement(const Announcement& announcement) override
  {
    m_message = MessageOf(announcement);
    m_data = V14::NotificationsOf(announcement).at(0).data;
    IAnnouncer::OnAnnouncement(announcement);
    m_legacy.Set();
  }

  PlayerEvent m_event;
  CEvent m_typed;
  std::string m_message;
  CVariant m_data;
  CEvent m_legacy;
};
} // namespace

TEST_F(TestAnnouncementManager, ATypedEventReachesListenersAsItselfAndAsData)
{
  CTypedAnnouncer announcer;
  m_manager.AddAnnouncer(&announcer, Player);
  const auto item = std::make_shared<CFileItem>("/music/song.flac", false);
  m_manager.Announce(PlayerEvent{EVENT::PLAYER::Play{item, 1, KODI::MEDIA::Streams::Audio}});
  ASSERT_TRUE(announcer.m_typed.Wait(TIMEOUT));
  ASSERT_TRUE(announcer.m_legacy.Wait(TIMEOUT));
  m_manager.RemoveAnnouncer(&announcer);
  m_manager.Deinitialize();

  const auto* play = std::get_if<EVENT::PLAYER::Play>(&announcer.m_event);
  ASSERT_NE(nullptr, play);
  ASSERT_NE(nullptr, play->item);
  EXPECT_NE(item, play->item) << "a listener must receive the copy taken when it was announced";
  EXPECT_EQ("/music/song.flac", play->item->GetPath());

  EXPECT_EQ("OnPlay", announcer.m_message);
  EXPECT_EQ(1, announcer.m_data["player"]["speed"].asInteger());
  EXPECT_TRUE(announcer.m_data["item"].isMember("type"));
}

namespace
{
class CLibraryAnnouncer : public IAnnouncer
{
public:
  void OnVideoLibraryEvent(const VideoLibraryEvent& event) override
  {
    m_video = event;
    m_videoReceived.Set();
  }

  void OnAudioLibraryEvent(const AudioLibraryEvent& event) override
  {
    m_audio = event;
    m_audioReceived.Set();
  }

  void OnAnnouncement(const Announcement& announcement) override
  {
    if (FlagOf(announcement) == VideoLibrary)
      m_videoData = V14::NotificationsOf(announcement).at(0).data;
    IAnnouncer::OnAnnouncement(announcement);
  }

  VideoLibraryEvent m_video;
  CEvent m_videoReceived;
  AudioLibraryEvent m_audio;
  CEvent m_audioReceived;
  CVariant m_videoData;
};
} // namespace

TEST_F(TestAnnouncementManager, ALibraryEventReachesTheHandlerForItsLibrary)
{
  CLibraryAnnouncer announcer;
  m_manager.AddAnnouncer(&announcer, VideoLibrary | AudioLibrary);
  const auto item = std::make_shared<CFileItem>("/movies/film.mkv", false);
  m_manager.Announce(AudioLibraryEvent{EVENT::LIBRARY::ScanStarted{}});
  m_manager.Announce(VideoLibraryEvent{EVENT::LIBRARY::Update{.item = item, .added = true}});
  ASSERT_TRUE(announcer.m_audioReceived.Wait(TIMEOUT));
  ASSERT_TRUE(announcer.m_videoReceived.Wait(TIMEOUT));
  m_manager.RemoveAnnouncer(&announcer);
  m_manager.Deinitialize();

  EXPECT_TRUE(std::holds_alternative<EVENT::LIBRARY::ScanStarted>(announcer.m_audio));
  const auto* update = std::get_if<EVENT::LIBRARY::Update>(&announcer.m_video);
  ASSERT_NE(nullptr, update);
  ASSERT_NE(nullptr, update->item);
  EXPECT_NE(item, update->item) << "a listener must receive the copy taken when it was announced";
  EXPECT_TRUE(update->added);

  EXPECT_TRUE(announcer.m_videoData["added"].asBoolean());
  EXPECT_TRUE(announcer.m_videoData["item"].isMember("type"));
}

TEST_F(TestAnnouncementManager, ATypedEventIsNotDeliveredForAnotherFlag)
{
  CTypedAnnouncer announcer;
  m_manager.AddAnnouncer(&announcer, Playlist);
  m_manager.Announce(PlayerEvent{EVENT::PLAYER::Menu{}});
  m_manager.Announce(PlaylistEvent{EVENT::PLAYLIST::Clear{}});
  ASSERT_TRUE(announcer.m_legacy.Wait(TIMEOUT));
  m_manager.RemoveAnnouncer(&announcer);
  m_manager.Deinitialize();

  EXPECT_FALSE(announcer.m_typed.Wait(0ms));
  EXPECT_EQ("OnClear", announcer.m_message);
}
