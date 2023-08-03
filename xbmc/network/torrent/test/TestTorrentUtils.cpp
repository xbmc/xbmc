/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "network/torrent/TorrentUtils.h"

#include <gtest/gtest.h>

using namespace KODI::NETWORK;

TEST(TestTorrentUtils, SplitMagnetURLPreservesQueryIdentity)
{
  const std::string query = "?xt=urn:btih:0123456789abcdef0123456789abcdef01234567"
                            "&tr=http%3a%2f%2fone&tr=http%3A%2F%2Ftwo&dn=C++&empty=&flag";
  for (const std::string path : {"", "/"})
  {
    const CURL url("MAGNET:" + path + query + "#part");
    const auto [magnet, filename] = CTorrentUtils::SplitMagnetURL(url);
    EXPECT_EQ("magnet:" + query, magnet);
    EXPECT_TRUE(filename.empty());
    EXPECT_EQ("MAGNET:" + path + query + "#part", url.Get());
  }
}

TEST(TestTorrentUtils, SplitMagnetURLDecodesMemberPath)
{
  const CURL url("magnet:/C++/100%25%20done%3f%23%2520.mkv?xt=urn:btih:hash&tr=one&tr=two#part");
  const auto [magnet, filename] = CTorrentUtils::SplitMagnetURL(url);
  EXPECT_EQ("magnet:?xt=urn:btih:hash&tr=one&tr=two", magnet);
#ifdef TARGET_WINDOWS
  EXPECT_EQ("C++\\100% done?#%20.mkv", filename);
#else
  EXPECT_EQ("C++/100% done?#%20.mkv", filename);
#endif
}

TEST(TestTorrentUtils, SplitMagnetURLRejectsUnsupportedPaths)
{
  for (const auto* input :
       {"magnet://host/file?xt=urn:btih:hash", "magnet:relative/file?xt=urn:btih:hash",
        "magnet:/bad%?xt=urn:btih:hash", "magnet:?xt=bad%", "http://host/file?xt=urn:btih:hash"})
  {
    SCOPED_TRACE(input);
    const auto [magnet, filename] = CTorrentUtils::SplitMagnetURL(CURL(input));
    EXPECT_TRUE(magnet.empty());
    EXPECT_TRUE(filename.empty());
  }
}

TEST(TestTorrentUtils, SetMagnetPathEncodesNativeNamesAndPreservesIdentity)
{
  CURL url("MAGNET:?xt=urn:btih:hash&tr=one&tr=two#part");
#ifdef TARGET_WINDOWS
  const std::string filename = "C++\\100% done?#%20.mkv";
#else
  const std::string filename = "C++/100% done?#%20.mkv";
#endif
  ASSERT_TRUE(CTorrentUtils::SetMagnetPath(url, filename));
  EXPECT_EQ("MAGNET:/C%2b%2b/100%25%20done%3f%23%2520.mkv?xt=urn:btih:hash&tr=one&tr=two#part",
            url.Get());
  const auto [magnet, member] = CTorrentUtils::SplitMagnetURL(url);
  EXPECT_EQ("magnet:?xt=urn:btih:hash&tr=one&tr=two", magnet);
  EXPECT_EQ(filename, member);
}

TEST(TestTorrentUtils, SetMagnetPathTreatsColonNamesAsNative)
{
  CURL url("magnet:?xt=urn:btih:hash");
  ASSERT_TRUE(CTorrentUtils::SetMagnetPath(url, "urn:series"));
  EXPECT_EQ("magnet:/urn%3aseries?xt=urn:btih:hash", url.Get());
  EXPECT_EQ("urn:series", CTorrentUtils::SplitMagnetURL(url).second);
}
