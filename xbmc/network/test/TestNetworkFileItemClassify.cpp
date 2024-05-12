/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "URL.h"
#include "filesystem/StackDirectory.h"
#include "network/NetworkFileItemClassify.h"

#include <array>
#include <string>

#include <gtest/gtest.h>

using namespace KODI;

namespace
{

struct SimpleDefinition
{
  SimpleDefinition(const std::string& path, bool folder, bool res) : item(path, folder), result(res)
  {
  }

  CFileItem item;
  bool result;
};

} // namespace

class InternetStreamTest : public testing::WithParamInterface<SimpleDefinition>,
                           public testing::Test
{
};

TEST_P(InternetStreamTest, IsInternetStream)
{
  EXPECT_EQ(NETWORK::IsInternetStream(GetParam().item), GetParam().result);
}

const auto inetstream_tests = std::array{
    SimpleDefinition{"/home/user/test.disc", false, false},
    SimpleDefinition{"http://some.where/foo", false, true},
    SimpleDefinition{"http://some.where/foo", true, true},
    SimpleDefinition{"https://some.where/foo", false, true},
    SimpleDefinition{"https://some.where/foo", true, true},
    SimpleDefinition{"tcp://some.where/foo", false, true},
    SimpleDefinition{"tcp://some.where/foo", true, true},
    SimpleDefinition{"udp://some.where/foo", false, true},
    SimpleDefinition{"udp://some.where/foo", true, true},
    SimpleDefinition{"rtp://some.where/foo", false, true},
    SimpleDefinition{"rtp://some.where/foo", true, true},
    SimpleDefinition{"sdp://some.where/foo", false, true},
    SimpleDefinition{"sdp://some.where/foo", true, true},
    SimpleDefinition{"mms://some.where/foo", false, true},
    SimpleDefinition{"mms://some.where/foo", true, true},
    SimpleDefinition{"mmst://some.where/foo", false, true},
    SimpleDefinition{"mmst://some.where/foo", true, true},
    SimpleDefinition{"mmsh://some.where/foo", false, true},
    SimpleDefinition{"mmsh://some.where/foo", true, true},
    SimpleDefinition{"rtsp://some.where/foo", false, true},
    SimpleDefinition{"rtsp://some.where/foo", true, true},
    SimpleDefinition{"rtmp://some.where/foo", false, true},
    SimpleDefinition{"rtmp://some.where/foo", true, true},
    SimpleDefinition{"rtmpt://some.where/foo", false, true},
    SimpleDefinition{"rtmpt://some.where/foo", true, true},
    SimpleDefinition{"rtmpe://some.where/foo", false, true},
    SimpleDefinition{"rtmpe://some.where/foo", true, true},
    SimpleDefinition{"rtmpte://some.where/foo", false, true},
    SimpleDefinition{"rtmpte://some.where/foo", true, true},
    SimpleDefinition{"rtmps://some.where/foo", false, true},
    SimpleDefinition{"rtmps://some.where/foo", true, true},
    SimpleDefinition{"shout://some.where/foo", false, true},
    SimpleDefinition{"shout://some.where/foo", true, true},
    SimpleDefinition{"rss://some.where/foo", false, true},
    SimpleDefinition{"rss://some.where/foo", true, true},
    SimpleDefinition{"rsss://some.where/foo", false, true},
    SimpleDefinition{"rsss://some.where/foo", true, true},
    SimpleDefinition{"upnp://some.where/foo", false, false},
    SimpleDefinition{"ftp://some.where/foo", false, false},
    SimpleDefinition{"sftp://some.where/foo", false, false},
    SimpleDefinition{"ssh://some.where/foo", false, false},
};

INSTANTIATE_TEST_SUITE_P(TestNetworkFileItemClassify,
                         InternetStreamTest,
                         testing::ValuesIn(inetstream_tests));

TEST(TestNetworkWorkFileItemClassify, InternetStreamStacks)
{
  std::string stackPath;
  EXPECT_TRUE(XFILE::CStackDirectory::ConstructStackPath(
      {"/home/foo/somthing.avi", "/home/bar/else.mkv"}, stackPath));
  EXPECT_FALSE(NETWORK::IsInternetStream(CFileItem(stackPath, false)));
  EXPECT_FALSE(NETWORK::IsInternetStream(CFileItem(stackPath, true)));

  EXPECT_TRUE(XFILE::CStackDirectory::ConstructStackPath(
      {"https://home/foo/somthing.avi", "https://home/bar/else.mkv"}, stackPath));
  EXPECT_TRUE(NETWORK::IsInternetStream(CFileItem(stackPath, false)));
  EXPECT_TRUE(NETWORK::IsInternetStream(CFileItem(stackPath, true)));

  EXPECT_TRUE(XFILE::CStackDirectory::ConstructStackPath(
      {"ftp://home/foo/somthing.avi", "ftp://home/bar/else.mkv"}, stackPath));
  EXPECT_FALSE(NETWORK::IsInternetStream(CFileItem(stackPath, false)));
  EXPECT_FALSE(NETWORK::IsInternetStream(CFileItem(stackPath, true)));

  EXPECT_TRUE(XFILE::CStackDirectory::ConstructStackPath(
      {"/home/foo/somthing.avi", "ftp://home/bar/else.mkv"}, stackPath));
  EXPECT_FALSE(NETWORK::IsInternetStream(CFileItem(stackPath, false)));
  EXPECT_FALSE(NETWORK::IsInternetStream(CFileItem(stackPath, true)));

  CFileItem item("https://some.where/", true);
  item.SetProperty("IsHTTPDirectory", true);
  EXPECT_FALSE(NETWORK::IsInternetStream(item));
}

class RemoteTest : public testing::WithParamInterface<SimpleDefinition>, public testing::Test
{
};

TEST_P(RemoteTest, IsRemote)
{
  EXPECT_EQ(NETWORK::IsRemote(GetParam().item), GetParam().result);
}

const auto remote_tests = std::array{
    SimpleDefinition{"cdda://1", false, false},
    SimpleDefinition{"cdda://1", true, false},
    SimpleDefinition{"iso9660://some.file", false, false},
    SimpleDefinition{"cdda://some.file", true, false},
    SimpleDefinition{"special://home/foo.xml", false, false},
    SimpleDefinition{"special://home", true, false},
    SimpleDefinition{"zip://" + CURL::Encode("/home/foo/bar.zip"), true, false},
    SimpleDefinition{"zip://" + CURL::Encode("https://some.where/yo.zip"), true, true},
    SimpleDefinition{"addons://plugins", true, false},
    SimpleDefinition{"sources://music", true, false},
    SimpleDefinition{"videodb://1/2", true, false},
    SimpleDefinition{"musicdb://1/2", true, false},
    SimpleDefinition{"library://movies/titles", true, false},
    SimpleDefinition{"plugin://plugin.video.yo", true, false},
    SimpleDefinition{"androidapp://cool.app", true, false},
    SimpleDefinition{"/home/foo/bar", true, false},
    SimpleDefinition{"https://127.0.0.1/bar", true, false},
    SimpleDefinition{"https://some.where/bar", true, true},
};

INSTANTIATE_TEST_SUITE_P(TestNetworkFileItemClassify, RemoteTest, testing::ValuesIn(remote_tests));

class StreamedFilesystemTest : public testing::WithParamInterface<SimpleDefinition>,
                               public testing::Test
{
};

TEST_P(StreamedFilesystemTest, IsStreamedFilesystem)
{
  EXPECT_EQ(NETWORK::IsStreamedFilesystem(GetParam().item), GetParam().result);
}

const auto streamedfs_tests = std::array{
    SimpleDefinition{"/home/user/test.disc", false, false},
    SimpleDefinition{"/home/user/test.disc", true, false},
    SimpleDefinition{"http://some.where/foo", false, true},
    SimpleDefinition{"http://some.where/foo", true, true},
    SimpleDefinition{"https://some.where/foo", false, true},
    SimpleDefinition{"https://some.where/foo", true, true},
    SimpleDefinition{"ftp://some.where/foo", false, true},
    SimpleDefinition{"ftp://some.where/foo", true, true},
    SimpleDefinition{"sftp://some.where/foo", false, true},
    SimpleDefinition{"sftp://some.where/foo", true, true},
    SimpleDefinition{"ssh://some.where/foo", false, true},
    SimpleDefinition{"ssh://some.where/foo", true, true},
    SimpleDefinition{"ssh://some.where/foo", true, true},
};

INSTANTIATE_TEST_SUITE_P(TestNetworkFileItemClassify,
                         StreamedFilesystemTest,
                         testing::ValuesIn(streamedfs_tests));

TEST(TestNetworkWorkFileItemClassify, StreamedFilesystemStacks)
{
  std::string stackPath;
  EXPECT_TRUE(XFILE::CStackDirectory::ConstructStackPath(
      {"/home/foo/somthing.avi", "/home/bar/else.mkv"}, stackPath));
  EXPECT_FALSE(NETWORK::IsStreamedFilesystem(CFileItem(stackPath, false)));
  EXPECT_FALSE(NETWORK::IsStreamedFilesystem(CFileItem(stackPath, true)));

  EXPECT_TRUE(XFILE::CStackDirectory::ConstructStackPath(
      {"https://home/foo/somthing.avi", "https://home/bar/else.mkv"}, stackPath));
  EXPECT_TRUE(NETWORK::IsStreamedFilesystem(CFileItem(stackPath, false)));
  EXPECT_TRUE(NETWORK::IsStreamedFilesystem(CFileItem(stackPath, true)));

  EXPECT_TRUE(XFILE::CStackDirectory::ConstructStackPath(
      {"shout://home/foo/somthing.avi", "shout://home/bar/else.mkv"}, stackPath));
  EXPECT_TRUE(NETWORK::IsStreamedFilesystem(CFileItem(stackPath, false)));
  EXPECT_TRUE(NETWORK::IsStreamedFilesystem(CFileItem(stackPath, true)));

  EXPECT_TRUE(XFILE::CStackDirectory::ConstructStackPath(
      {"ftp://home/foo/somthing.avi", "ftp://home/bar/else.mkv"}, stackPath));
  EXPECT_TRUE(NETWORK::IsStreamedFilesystem(CFileItem(stackPath, false)));
  EXPECT_TRUE(NETWORK::IsStreamedFilesystem(CFileItem(stackPath, true)));

  EXPECT_TRUE(XFILE::CStackDirectory::ConstructStackPath(
      {"ftp://home/foo/somthing.avi", "/home/bar/else.mkv"}, stackPath));
  EXPECT_TRUE(NETWORK::IsStreamedFilesystem(CFileItem(stackPath, false)));
  EXPECT_TRUE(NETWORK::IsStreamedFilesystem(CFileItem(stackPath, true)));

  EXPECT_TRUE(XFILE::CStackDirectory::ConstructStackPath(
      {"/home/foo/somthing.avi", "ftp://home/bar/else.mkv"}, stackPath));
  EXPECT_FALSE(NETWORK::IsStreamedFilesystem(CFileItem(stackPath, false)));
  EXPECT_FALSE(NETWORK::IsStreamedFilesystem(CFileItem(stackPath, true)));
}
