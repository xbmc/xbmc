/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "interfaces/AnnouncementEvents.h"
#include "settings/lib/SettingLevel.h"
#include "video/VideoInfoTag.h"

#include <memory>

#include <gtest/gtest.h>

using namespace ANNOUNCEMENT;

namespace
{
constexpr auto VIDEO = KODI::MEDIA::Streams::Video;
} // unnamed namespace

TEST(TestAnnouncementEvents, PlayerEventsKeepTheirMessages)
{
  EXPECT_STREQ("OnPlay", MessageOf(PlayerEvent{EVENT::PLAYER::Play{}}));
  EXPECT_STREQ("OnStop", MessageOf(PlayerEvent{EVENT::PLAYER::Stop{}}));
  EXPECT_STREQ("OnSeek", MessageOf(PlayerEvent{EVENT::PLAYER::Seek{}}));
  EXPECT_STREQ("OnPropertiesChanged", MessageOf(PlayerEvent{EVENT::PLAYER::PropertiesChanged{}}));
  EXPECT_STREQ("OnContentGeometryChange",
               MessageOf(PlayerEvent{EVENT::PLAYER::ContentGeometryChange{}}));
  EXPECT_STREQ("OnAudioFormatHold", MessageOf(PlayerEvent{EVENT::PLAYER::AudioFormatHold{}}));
  EXPECT_EQ(Player, FlagOf(PlayerEvent{EVENT::PLAYER::Play{}}));
  EXPECT_EQ(Playlist, FlagOf(PlaylistEvent{EVENT::PLAYLIST::Clear{}}));
}

TEST(TestAnnouncementEvents, GUIEvents)
{
  using namespace EVENT::GUI;
  EXPECT_EQ(GUI, FlagOf(GUIEvent{SkinLoaded{}}));
  EXPECT_STREQ("OnScreensaverActivated", MessageOf(GUIEvent{ScreensaverActivated{}}));
  EXPECT_STREQ("OnScreensaverDeactivated", MessageOf(GUIEvent{ScreensaverDeactivated{}}));
  EXPECT_STREQ("OnDPMSActivated", MessageOf(GUIEvent{DPMSActivated{}}));
  EXPECT_STREQ("OnDPMSDeactivated", MessageOf(GUIEvent{DPMSDeactivated{}}));
  EXPECT_STREQ("OnSkinUnloading", MessageOf(GUIEvent{SkinUnloading{}}));
  EXPECT_STREQ("OnSkinLoaded", MessageOf(GUIEvent{SkinLoaded{}}));
  EXPECT_STREQ("OnSkinLoadFailed", MessageOf(GUIEvent{SkinLoadFailed{}}));
  EXPECT_STREQ("WindowFocused", MessageOf(GUIEvent{WindowFocused{}}));
  EXPECT_STREQ("WindowUnfocused", MessageOf(GUIEvent{WindowUnfocused{}}));
}

TEST(TestAnnouncementEvents, SystemEvents)
{
  using namespace EVENT::SYSTEM;
  EXPECT_EQ(System, FlagOf(SystemEvent{Wake{}}));
  EXPECT_STREQ("OnQuit", MessageOf(SystemEvent{Quit{}}));
  EXPECT_STREQ("OnRestart", MessageOf(SystemEvent{Restart{}}));
  EXPECT_STREQ("OnSleep", MessageOf(SystemEvent{EVENT::SYSTEM::Sleep{}}));
  EXPECT_STREQ("OnWake", MessageOf(SystemEvent{Wake{}}));
  EXPECT_STREQ("OnLowBattery", MessageOf(SystemEvent{LowBattery{}}));
}

TEST(TestAnnouncementEvents, LibraryEventsKeepTheirMessages)
{
  using namespace EVENT::LIBRARY;
  EXPECT_EQ(VideoLibrary, FlagOf(VideoLibraryEvent{ScanStarted{}}));
  EXPECT_EQ(AudioLibrary, FlagOf(AudioLibraryEvent{ScanStarted{}}));
  EXPECT_STREQ("OnScanStarted", MessageOf(VideoLibraryEvent{ScanStarted{}}));
  EXPECT_STREQ("OnScanFinished", MessageOf(AudioLibraryEvent{ScanFinished{}}));
  EXPECT_STREQ("OnCleanStarted", MessageOf(VideoLibraryEvent{CleanStarted{}}));
  EXPECT_STREQ("OnCleanFinished", MessageOf(AudioLibraryEvent{CleanFinished{}}));
  EXPECT_STREQ("OnUpdate", MessageOf(VideoLibraryEvent{Update{}}));
  EXPECT_STREQ("OnRemove", MessageOf(AudioLibraryEvent{Remove{}}));
  EXPECT_STREQ("OnExport", MessageOf(VideoLibraryEvent{Export{}}));
  EXPECT_STREQ("OnRefresh", MessageOf(VideoLibraryEvent{Refresh{}}));
}

TEST(TestAnnouncementEvents, OnlyScansAndCleansAreTransactions)
{
  using namespace EVENT::LIBRARY;
  EXPECT_TRUE(IsTransaction(VideoLibraryEvent{Update{.transaction = true}}));
  EXPECT_TRUE(IsTransaction(AudioLibraryEvent{Remove{.transaction = true}}));
  EXPECT_FALSE(IsTransaction(VideoLibraryEvent{Update{}}));
  EXPECT_FALSE(IsTransaction(VideoLibraryEvent{ScanFinished{}}));
}

TEST(TestAnnouncementEvents, InputEvents)
{
  using namespace EVENT::INPUT;
  EXPECT_EQ(Input, FlagOf(InputEvent{Finished{}}));
  EXPECT_STREQ("OnInputRequested", MessageOf(InputEvent{Requested{}}));
  EXPECT_STREQ("OnInputFinished", MessageOf(InputEvent{Finished{}}));
}

TEST(TestAnnouncementEvents, PVREvents)
{
  using namespace EVENT::PVR;
  EXPECT_EQ(ANNOUNCEMENT::PVR, FlagOf(PVREvent{RadioClock{}}));
  EXPECT_STREQ("RDSRadioTA", MessageOf(PVREvent{RadioTrafficAnnouncement{}}));
  EXPECT_STREQ("RDSRadioRTC", MessageOf(PVREvent{RadioClock{}}));
  EXPECT_STREQ("RDSRadioTMC", MessageOf(PVREvent{RadioTrafficMessage{}}));
}

TEST(TestAnnouncementEvents, InfoSourcesAndSettingsEvents)
{
  EXPECT_EQ(Info, FlagOf(InfoEvent{EVENT::INFO::Changed{}}));
  EXPECT_STREQ("OnChanged", MessageOf(InfoEvent{EVENT::INFO::Changed{}}));

  using namespace EVENT::SOURCES;
  EXPECT_EQ(Sources, FlagOf(SourcesEvent{Added{}}));
  EXPECT_STREQ("OnAdded", MessageOf(SourcesEvent{Added{}}));
  EXPECT_STREQ("OnRemoved", MessageOf(SourcesEvent{Removed{}}));
  EXPECT_STREQ("OnUpdated", MessageOf(SourcesEvent{Updated{}}));

  EXPECT_EQ(Settings, FlagOf(SettingsEvent{EVENT::SETTINGS::LevelChanged{}}));
  EXPECT_STREQ("OnLevelChanged", MessageOf(SettingsEvent{EVENT::SETTINGS::LevelChanged{}}));
}

TEST(TestAnnouncementEvents, AnItemTheLibraryAlreadyNamesIsNotLookedUp)
{
  CVideoInfoTag tag;
  tag.m_iDbId = 7;
  tag.SetMediaType(KODI::MEDIA::TYPE::MOVIE);
  const auto movie = std::make_shared<CFileItem>(tag);
  movie->SetPath("/movies/film.mkv");
  const Announcement announcement{PlayerEvent{EVENT::PLAYER::Play{movie, 1, VIDEO}}};

  EXPECT_EQ(movie, ItemOf(WithLibraryDetails(announcement)));
  EXPECT_EQ(nullptr, ItemOf(WithLibraryDetails(PlayerEvent{EVENT::PLAYER::Menu{}})));
}

TEST(TestAnnouncementEvents, AnItemCanBeReplaced)
{
  const auto original = std::make_shared<CFileItem>("/music/song.flac", false);
  const auto copy = std::make_shared<CFileItem>("/music/song.flac", false);
  const Announcement announcement{PlayerEvent{EVENT::PLAYER::Play{original, 1, VIDEO}}};

  EXPECT_EQ(original, ItemOf(announcement));
  EXPECT_EQ(copy, ItemOf(WithItem(announcement, copy)));
  EXPECT_EQ(nullptr, ItemOf(PlayerEvent{EVENT::PLAYER::Menu{}}));
}
