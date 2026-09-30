/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "URL.h"
#include "filesystem/Directory.h"
#include "platform/Filesystem.h"
#include "utils/URIUtils.h"
#include "video/VideoInfoTag.h"
#include "video/tags/VideoTagLoaderNFO.h"

#include <fstream>
#include <random>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <gtest/gtest.h>

namespace
{
struct ArchiveNfoTest
{
  int season;
  int episode;
  std::vector<std::string> nfoFiles; // created next to archive.rar
  std::string expected;
};

const ArchiveNfoTest archive_nfo_tests[] = {
    {3, 4, {"archive-S03E04.nfo"}, "archive-S03E04.nfo"},
    {3, 4, {"archive.nfo"}, "archive.nfo"},
    {3, 4, {"archive-S03E04.nfo", "archive.nfo"}, "archive-S03E04.nfo"},
    {3, 4, {"foo.nfo", "archive-S03E04.nfo"}, "foo.nfo"},
    {3, 4, {"archive-S03E05.nfo"}, ""},
    {-1, -1, {"archive.nfo"}, "archive.nfo"},
};
} // namespace

class ArchiveNfoTestFixture : public testing::WithParamInterface<ArchiveNfoTest>,
                              public testing::Test
{
};

TEST_P(ArchiveNfoTestFixture, FindNFO)
{
  std::error_code ec;
  const std::string tmpdir{KODI::PLATFORM::FILESYSTEM::temp_directory_path(ec)};
  ASSERT_FALSE(ec);
  std::string dir{URIUtils::AddFileToFolder(
      tmpdir, fmt::format("NfoArchiveTest{:08x}", std::random_device{}()))};
  URIUtils::AddSlashAtEnd(dir);
  ASSERT_TRUE(XFILE::CDirectory::Create(dir));
  for (const std::string& nfoFile : GetParam().nfoFiles)
    std::ofstream(URIUtils::AddFileToFolder(dir, nfoFile), std::ios::out);

  CFileItem item(
      fmt::format("rar://{}/foo.mkv", CURL::Encode(URIUtils::AddFileToFolder(dir, "archive.rar"))),
      false);
  item.GetVideoInfoTag()->m_iSeason = GetParam().season;
  item.GetVideoInfoTag()->m_iEpisode = GetParam().episode;
  const CVideoTagLoaderNFO loader(item, nullptr, false);

  EXPECT_EQ(URIUtils::GetFileName(loader.GetNFOPath()), GetParam().expected);

  XFILE::CDirectory::RemoveRecursive(dir);
}

INSTANTIATE_TEST_SUITE_P(TestVideoTagLoaderNFO,
                         ArchiveNfoTestFixture,
                         testing::ValuesIn(archive_nfo_tests));
