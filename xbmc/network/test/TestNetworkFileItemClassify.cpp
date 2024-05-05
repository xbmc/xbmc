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

struct InternetStreamDefinition
{
  InternetStreamDefinition(const std::string& path, bool folder, bool strict, bool res)
    : item(path, folder), strictCheck(strict), result(res)
  {
  }

  CFileItem item;
  bool strictCheck;
  bool result;
};

class InternetStreamTest : public testing::WithParamInterface<InternetStreamDefinition>,
                           public testing::Test
{
};

TEST_P(InternetStreamTest, IsInternetStream)
{
  EXPECT_EQ(NETWORK::IsInternetStream(GetParam().item, GetParam().strictCheck), GetParam().result);
}

const auto inetstream_tests = std::array{
    InternetStreamDefinition{"/home/user/test.disc", false, false, false},
    InternetStreamDefinition{"/home/user/test.disc", true, true, false},
    InternetStreamDefinition{"http://some.where/foo", false, false, true},
    InternetStreamDefinition{"http://some.where/foo", false, true, true},
    InternetStreamDefinition{"http://some.where/foo", true, false, true},
    InternetStreamDefinition{"http://some.where/foo", true, true, true},
    InternetStreamDefinition{"https://some.where/foo", false, false, true},
    InternetStreamDefinition{"https://some.where/foo", false, true, true},
    InternetStreamDefinition{"https://some.where/foo", true, false, true},
    InternetStreamDefinition{"https://some.where/foo", true, true, true},
    InternetStreamDefinition{"tcp://some.where/foo", false, false, true},
    InternetStreamDefinition{"tcp://some.where/foo", false, true, true},
    InternetStreamDefinition{"tcp://some.where/foo", true, false, true},
    InternetStreamDefinition{"tcp://some.where/foo", true, true, true},
    InternetStreamDefinition{"udp://some.where/foo", false, false, true},
    InternetStreamDefinition{"udp://some.where/foo", false, true, true},
    InternetStreamDefinition{"udp://some.where/foo", true, false, true},
    InternetStreamDefinition{"udp://some.where/foo", true, true, true},
    InternetStreamDefinition{"rtp://some.where/foo", false, false, true},
    InternetStreamDefinition{"rtp://some.where/foo", false, false, true},
    InternetStreamDefinition{"rtp://some.where/foo", true, false, true},
    InternetStreamDefinition{"rtp://some.where/foo", true, true, true},
    InternetStreamDefinition{"sdp://some.where/foo", false, false, true},
    InternetStreamDefinition{"sdp://some.where/foo", false, true, true},
    InternetStreamDefinition{"sdp://some.where/foo", true, false, true},
    InternetStreamDefinition{"sdp://some.where/foo", true, true, true},
    InternetStreamDefinition{"mms://some.where/foo", false, false, true},
    InternetStreamDefinition{"mms://some.where/foo", false, true, true},
    InternetStreamDefinition{"mms://some.where/foo", true, false, true},
    InternetStreamDefinition{"mms://some.where/foo", true, true, true},
    InternetStreamDefinition{"mmst://some.where/foo", false, false, true},
    InternetStreamDefinition{"mmst://some.where/foo", false, true, true},
    InternetStreamDefinition{"mmst://some.where/foo", true, false, true},
    InternetStreamDefinition{"mmst://some.where/foo", true, true, true},
    InternetStreamDefinition{"mmsh://some.where/foo", false, false, true},
    InternetStreamDefinition{"mmsh://some.where/foo", false, true, true},
    InternetStreamDefinition{"mmsh://some.where/foo", true, false, true},
    InternetStreamDefinition{"mmsh://some.where/foo", true, true, true},
    InternetStreamDefinition{"rtsp://some.where/foo", false, false, true},
    InternetStreamDefinition{"rtsp://some.where/foo", false, true, true},
    InternetStreamDefinition{"rtsp://some.where/foo", true, false, true},
    InternetStreamDefinition{"rtsp://some.where/foo", true, true, true},
    InternetStreamDefinition{"rtmp://some.where/foo", false, false, true},
    InternetStreamDefinition{"rtmp://some.where/foo", false, true, true},
    InternetStreamDefinition{"rtmp://some.where/foo", true, false, true},
    InternetStreamDefinition{"rtmp://some.where/foo", true, true, true},
    InternetStreamDefinition{"rtmpt://some.where/foo", false, false, true},
    InternetStreamDefinition{"rtmpt://some.where/foo", false, true, true},
    InternetStreamDefinition{"rtmpt://some.where/foo", true, false, true},
    InternetStreamDefinition{"rtmpt://some.where/foo", true, true, true},
    InternetStreamDefinition{"rtmpe://some.where/foo", false, false, true},
    InternetStreamDefinition{"rtmpe://some.where/foo", false, true, true},
    InternetStreamDefinition{"rtmpe://some.where/foo", true, false, true},
    InternetStreamDefinition{"rtmpe://some.where/foo", true, true, true},
    InternetStreamDefinition{"rtmpte://some.where/foo", false, false, true},
    InternetStreamDefinition{"rtmpte://some.where/foo", false, true, true},
    InternetStreamDefinition{"rtmpte://some.where/foo", true, false, true},
    InternetStreamDefinition{"rtmpte://some.where/foo", true, true, true},
    InternetStreamDefinition{"rtmps://some.where/foo", false, false, true},
    InternetStreamDefinition{"rtmps://some.where/foo", false, true, true},
    InternetStreamDefinition{"rtmps://some.where/foo", true, false, true},
    InternetStreamDefinition{"rtmps://some.where/foo", true, true, true},
    InternetStreamDefinition{"shout://some.where/foo", false, false, true},
    InternetStreamDefinition{"shout://some.where/foo", false, true, true},
    InternetStreamDefinition{"shout://some.where/foo", true, false, true},
    InternetStreamDefinition{"shout://some.where/foo", true, true, true},
    InternetStreamDefinition{"rss://some.where/foo", false, false, true},
    InternetStreamDefinition{"rss://some.where/foo", false, true, true},
    InternetStreamDefinition{"rss://some.where/foo", true, false, true},
    InternetStreamDefinition{"rss://some.where/foo", true, true, true},
    InternetStreamDefinition{"rsss://some.where/foo", false, false, true},
    InternetStreamDefinition{"rsss://some.where/foo", false, true, true},
    InternetStreamDefinition{"rsss://some.where/foo", true, false, true},
    InternetStreamDefinition{"rsss://some.where/foo", true, true, true},
    InternetStreamDefinition{"upnp://some.where/foo", false, false, false},
    InternetStreamDefinition{"upnp://some.where/foo", true, false, false},
    InternetStreamDefinition{"upnp://some.where/foo", false, true, true},
    InternetStreamDefinition{"upnp://some.where/foo", true, true, true},
    InternetStreamDefinition{"ftp://some.where/foo", false, false, false},
    InternetStreamDefinition{"ftp://some.where/foo", true, false, false},
    InternetStreamDefinition{"ftp://some.where/foo", false, true, true},
    InternetStreamDefinition{"ftp://some.where/foo", true, true, true},
    InternetStreamDefinition{"sftp://some.where/foo", false, false, false},
    InternetStreamDefinition{"sftp://some.where/foo", true, false, false},
    InternetStreamDefinition{"sftp://some.where/foo", false, true, true},
    InternetStreamDefinition{"sftp://some.where/foo", true, true, true},
    InternetStreamDefinition{"ssh://some.where/foo", false, false, false},
    InternetStreamDefinition{"ssh://some.where/foo", true, false, false},
    InternetStreamDefinition{"ssh://some.where/foo", false, true, true},
    InternetStreamDefinition{"ssh://some.where/foo", true, true, true},
    InternetStreamDefinition{"ssh://some.where/foo", true, true, true},
};

INSTANTIATE_TEST_SUITE_P(TestNetworkFileItemClassify,
                         InternetStreamTest,
                         testing::ValuesIn(inetstream_tests));

TEST(TestNetworkWorkFileItemClassify, InternetStreamStacks)
{
  std::string stackPath;
  EXPECT_TRUE(XFILE::CStackDirectory::ConstructStackPath(
      {"/home/foo/somthing.avi", "/home/bar/else.mkv"}, stackPath));
  EXPECT_FALSE(NETWORK::IsInternetStream(CFileItem(stackPath, false), false));
  EXPECT_FALSE(NETWORK::IsInternetStream(CFileItem(stackPath, true), false));
  EXPECT_FALSE(NETWORK::IsInternetStream(CFileItem(stackPath, false), true));
  EXPECT_FALSE(NETWORK::IsInternetStream(CFileItem(stackPath, true), true));

  EXPECT_TRUE(XFILE::CStackDirectory::ConstructStackPath(
      {"https://home/foo/somthing.avi", "https://home/bar/else.mkv"}, stackPath));
  EXPECT_TRUE(NETWORK::IsInternetStream(CFileItem(stackPath, false), false));
  EXPECT_TRUE(NETWORK::IsInternetStream(CFileItem(stackPath, true), false));
  EXPECT_TRUE(NETWORK::IsInternetStream(CFileItem(stackPath, false), true));
  EXPECT_TRUE(NETWORK::IsInternetStream(CFileItem(stackPath, true), true));

  EXPECT_TRUE(XFILE::CStackDirectory::ConstructStackPath(
      {"ftp://home/foo/somthing.avi", "ftp://home/bar/else.mkv"}, stackPath));
  EXPECT_FALSE(NETWORK::IsInternetStream(CFileItem(stackPath, false), false));
  EXPECT_FALSE(NETWORK::IsInternetStream(CFileItem(stackPath, true), false));
  EXPECT_TRUE(NETWORK::IsInternetStream(CFileItem(stackPath, false), true));
  EXPECT_TRUE(NETWORK::IsInternetStream(CFileItem(stackPath, true), true));

  EXPECT_TRUE(XFILE::CStackDirectory::ConstructStackPath(
      {"ftp://home/foo/somthing.avi", "/home/bar/else.mkv"}, stackPath));
  EXPECT_FALSE(NETWORK::IsInternetStream(CFileItem(stackPath, false), false));
  EXPECT_FALSE(NETWORK::IsInternetStream(CFileItem(stackPath, true), false));
  EXPECT_TRUE(NETWORK::IsInternetStream(CFileItem(stackPath, false), true));
  EXPECT_TRUE(NETWORK::IsInternetStream(CFileItem(stackPath, true), true));

  EXPECT_TRUE(XFILE::CStackDirectory::ConstructStackPath(
      {"/home/foo/somthing.avi", "ftp://home/bar/else.mkv"}, stackPath));
  EXPECT_FALSE(NETWORK::IsInternetStream(CFileItem(stackPath, false), false));
  EXPECT_FALSE(NETWORK::IsInternetStream(CFileItem(stackPath, true), false));
  EXPECT_FALSE(NETWORK::IsInternetStream(CFileItem(stackPath, false), true));
  EXPECT_FALSE(NETWORK::IsInternetStream(CFileItem(stackPath, true), true));
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
