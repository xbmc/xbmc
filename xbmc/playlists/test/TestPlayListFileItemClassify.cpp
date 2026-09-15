/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "playlists/PlayListFileItemClassify.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/recordings/PVRRecording.h"
#include "utils/Variant.h"

#include <array>

#include <gtest/gtest.h>

using namespace KODI;

struct PlayListClassifyTest
{
  PlayListClassifyTest(std::string path, bool res, std::string mime = "")
    : path(std::move(path)),
      result(res),
      mime(std::move(mime))
  {
  }

  std::string path;
  bool result;
  std::string mime;
};

class PlayListTest : public testing::WithParamInterface<PlayListClassifyTest>, public testing::Test
{
};

TEST_P(PlayListTest, IsPlayList)
{
  const PlayListClassifyTest& param = GetParam();

  CFileItem item(param.path, false);
  if (!param.mime.empty())
    item.SetMimeType(param.mime);

  EXPECT_EQ(PLAYLIST::IsPlayList(item), param.result);
}

const auto playlist_tests = std::array{
    PlayListClassifyTest{"/home/user/video.avi", false},
    PlayListClassifyTest{"/home/user/video.avi", false, "video/avi"},
    PlayListClassifyTest{"https://some.where/foo.m3u8", false},
    PlayListClassifyTest{"https://some.where/something", true, "audio/x-pn-realaudio"},
    PlayListClassifyTest{"https://some.where/something", true, "playlist"},
    PlayListClassifyTest{"https://some.where/something", true, "audio/x-mpegurl"},
    PlayListClassifyTest{"/home/user/video.m3u", true},
    PlayListClassifyTest{"/home/user/video.m3u8", true},
    PlayListClassifyTest{"/home/user/video.b4s", true},
    PlayListClassifyTest{"/home/user/video.pls", true},
    PlayListClassifyTest{"/home/user/video.strm", true},
    PlayListClassifyTest{"/home/user/video.wpl", true},
    PlayListClassifyTest{"/home/user/video.asx", true},
    PlayListClassifyTest{"/home/user/video.ram", true},
    PlayListClassifyTest{"/home/user/video.url", true},
    PlayListClassifyTest{"/home/user/video.pxml", true},
    PlayListClassifyTest{"/home/user/video.xspf", true},
};

INSTANTIATE_TEST_SUITE_P(TestPlayListFileItemClassify,
                         PlayListTest,
                         testing::ValuesIn(playlist_tests));

TEST(TestPlayListFileItemClassify, IsSmartPlayList)
{
  CFileItem item("/some/where.avi", false);
  EXPECT_FALSE(PLAYLIST::IsSmartPlayList(item));
  item.SetProperty("library.smartplaylist", true);
  EXPECT_TRUE(PLAYLIST::IsSmartPlayList(item));

  CFileItem item2("/some/where.xsp", false);
  EXPECT_TRUE(PLAYLIST::IsSmartPlayList(item2));
  CFileItem item3("/some/where.xsp", true);
  EXPECT_TRUE(PLAYLIST::IsSmartPlayList(item3));
}

namespace
{

std::shared_ptr<PVR::CPVRRecording> MakeRecording(bool radio)
{
  PVR_RECORDING recording{};
  recording.channelType = radio ? PVR_RECORDING_CHANNEL_TYPE_RADIO : PVR_RECORDING_CHANNEL_TYPE_TV;
  return std::make_shared<PVR::CPVRRecording>(recording, 1);
}

std::shared_ptr<PVR::CPVRChannelGroupMember> MakeChannel(bool radio)
{
  return std::make_shared<PVR::CPVRChannelGroupMember>("group", 1, 0,
                                                       std::make_shared<PVR::CPVRChannel>(radio));
}

} // unnamed namespace

TEST(TestPlayListFileItemClassify, PlaylistIdOfRecognisesTheOrdinaryCases)
{
  EXPECT_EQ(PLAYLIST::Id::TYPE_VIDEO, PLAYLIST::PlaylistIdOf(CFileItem("/home/user/a.avi", false)));
  EXPECT_EQ(PLAYLIST::Id::TYPE_MUSIC, PLAYLIST::PlaylistIdOf(CFileItem("/home/user/a.mp3", false)));
}

// CFileItem's PVR constructors reach CServiceBroker::GetPVRManager(), which the test environment
// does not stand up, so these fault rather than fail. Disabled and skipped so that neither plain
// runs nor --gtest_also_run_disabled_tests can execute them until it does.
TEST(TestPlayListFileItemClassify, DISABLED_APvrItemAnswersFromItsTagNotItsStreams)
{
  GTEST_SKIP() << "constructing a CFileItem from a PVR tag needs a PVR manager";

  EXPECT_EQ(PLAYLIST::Id::TYPE_MUSIC, PLAYLIST::PlaylistIdOf(CFileItem(MakeRecording(true))));
  EXPECT_EQ(PLAYLIST::Id::TYPE_VIDEO, PLAYLIST::PlaylistIdOf(CFileItem(MakeRecording(false))));
  EXPECT_EQ(PLAYLIST::Id::TYPE_MUSIC, PLAYLIST::PlaylistIdOf(CFileItem(MakeChannel(true))));
  EXPECT_EQ(PLAYLIST::Id::TYPE_VIDEO, PLAYLIST::PlaylistIdOf(CFileItem(MakeChannel(false))));
}

TEST(TestPlayListFileItemClassify, AnItemThatSaysNothingHasNoPlaylist)
{
  EXPECT_EQ(PLAYLIST::Id::TYPE_NONE, PLAYLIST::PlaylistIdOf(CFileItem("/home/user/a", false)));
}

TEST(TestPlayListFileItemClassify, TheHintDecidesWhatTheClassifiersCannot)
{
  // A generic path answers neither video nor audio, so only the hint can say.
  CFileItem item("/home/user/a", false);
  item.SetProperty("playlist_type_hint", static_cast<int>(PLAYLIST::Id::TYPE_VIDEO));
  EXPECT_EQ(PLAYLIST::Id::TYPE_VIDEO, PLAYLIST::PlaylistIdOf(item));

  item.SetProperty("playlist_type_hint", static_cast<int>(PLAYLIST::Id::TYPE_MUSIC));
  EXPECT_EQ(PLAYLIST::Id::TYPE_MUSIC, PLAYLIST::PlaylistIdOf(item));
}

TEST(TestPlayListFileItemClassify, DISABLED_APvrItemIgnoresTheHint)
{
  GTEST_SKIP() << "constructing a CFileItem from a PVR tag needs a PVR manager";

  CFileItem item(MakeRecording(true));
  item.SetProperty("playlist_type_hint", static_cast<int>(PLAYLIST::Id::TYPE_VIDEO));
  EXPECT_EQ(PLAYLIST::Id::TYPE_MUSIC, PLAYLIST::PlaylistIdOf(item));
}
