/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "PlayListPlayer.h"
#include "URL.h"
#include "filesystem/Directory.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "interfaces/json-rpc/PlayerOperations.h"
#include "utils/URIUtils.h"

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace JSONRPC;

namespace
{

class CTestPlayerOperations : public CPlayerOperations
{
public:
  static bool List(const std::string& path,
                   bool recursive,
                   CFileItemList& pictures,
                   CFileItemList& media)
  {
    return ListSlideshowDirectory(path, recursive, pictures, media);
  }
};

class TestPlayerOpenDirectory : public ::testing::Test
{
protected:
  void SetUp() override
  {
    m_root = URIUtils::AddFileToFolder(CSpecialProtocol::TranslatePath("special://temp/"),
                                       "jsonrpc-playeropen-directory/");
    XFILE::CDirectory::RemoveRecursive(m_root);
    ASSERT_TRUE(XFILE::CDirectory::Create(m_root));
  }

  void TearDown() override
  {
    g_directoryCache.ClearSubPaths(CURL(m_root));
    XFILE::CDirectory::RemoveRecursive(m_root);
  }

  void Touch(const std::string& relative)
  {
    const std::string path{URIUtils::AddFileToFolder(m_root, relative)};
    XFILE::CDirectory::Create(URIUtils::GetDirectory(path));
    XFILE::CFile file;
    ASSERT_TRUE(file.OpenForWrite(path, true)) << path;
    file.Close();
  }

  static std::vector<std::string> Names(const CFileItemList& list)
  {
    std::vector<std::string> names;
    for (int i = 0; i < list.Size(); ++i)
      names.push_back(URIUtils::GetFileName(list[i]->GetPath()));
    return names;
  }

  std::string m_root;
};

} // unnamed namespace

TEST_F(TestPlayerOpenDirectory, ADirectoryOfMediaHoldsNoPictures)
{
  Touch("b.mkv");
  Touch("a.mp4");
  Touch("c.mp3");
  Touch("notes.txt");

  CFileItemList pictures;
  CFileItemList media;
  ASSERT_TRUE(CTestPlayerOperations::List(m_root, true, pictures, media));

  EXPECT_EQ(0, pictures.Size());
  EXPECT_EQ((std::vector<std::string>{"a.mp4", "b.mkv", "c.mp3"}), Names(media));
}

TEST_F(TestPlayerOpenDirectory, OnePictureMakesItASlideshowSource)
{
  Touch("a.mp4");
  Touch("x.jpg");

  CFileItemList pictures;
  CFileItemList media;
  ASSERT_TRUE(CTestPlayerOperations::List(m_root, true, pictures, media));

  EXPECT_EQ((std::vector<std::string>{"x.jpg"}), Names(pictures));
  EXPECT_EQ((std::vector<std::string>{"a.mp4"}), Names(media));
}

TEST_F(TestPlayerOpenDirectory, AnArchiveIsNeitherAPictureNorMedia)
{
  Touch("movie.mkv");
  Touch("backup.zip");
  Touch("scans.rar");

  CFileItemList pictures;
  CFileItemList media;
  ASSERT_TRUE(CTestPlayerOperations::List(m_root, true, pictures, media));

  EXPECT_EQ(0, pictures.Size());
  EXPECT_EQ((std::vector<std::string>{"movie.mkv"}), Names(media));
}

TEST_F(TestPlayerOpenDirectory, AFileThatCouldBeAudioOrVideoAsksForTheVideoPlayer)
{
  Touch("recording.wtv");
  Touch("film.mkv");

  CFileItemList pictures;
  CFileItemList media;
  ASSERT_TRUE(CTestPlayerOperations::List(m_root, true, pictures, media));

  ASSERT_EQ((std::vector<std::string>{"film.mkv", "recording.wtv"}), Names(media));
  EXPECT_FALSE(media[0]->HasProperty("playlist_type_hint"));
  EXPECT_EQ(static_cast<int>(KODI::PLAYLIST::Id::TYPE_VIDEO),
            media[1]->GetProperty("playlist_type_hint").asInteger32(-1));
}

TEST_F(TestPlayerOpenDirectory, SubdirectoriesAreOnlyFollowedWhenAsked)
{
  Touch("a.mp4");
  Touch("sub/d.mp4");
  Touch("sub/y.png");

  CFileItemList pictures;
  CFileItemList media;
  ASSERT_TRUE(CTestPlayerOperations::List(m_root, false, pictures, media));
  EXPECT_EQ(0, pictures.Size());
  EXPECT_EQ((std::vector<std::string>{"a.mp4"}), Names(media));

  pictures.Clear();
  media.Clear();
  ASSERT_TRUE(CTestPlayerOperations::List(m_root, true, pictures, media));
  EXPECT_EQ((std::vector<std::string>{"y.png"}), Names(pictures));
  EXPECT_EQ((std::vector<std::string>{"a.mp4", "d.mp4"}), Names(media));
}

// An NFS symlink lists with its target's own path, so a link to an enclosing directory names
// that directory again rather than a deeper path. The listings are seeded into the directory
// cache, which CDirectory::GetDirectory consults before the filesystem.
TEST_F(TestPlayerOpenDirectory, ALinkToAnEnclosingDirectoryIsListedOnce)
{
  const std::string top{URIUtils::AddFileToFolder(m_root, "share/")};
  const std::string sub{URIUtils::AddFileToFolder(top, "sub/")};

  CFileItemList topItems;
  topItems.Add(std::make_shared<CFileItem>(URIUtils::AddFileToFolder(top, "a.mp4"), false));
  topItems.Add(std::make_shared<CFileItem>(sub, true));
  g_directoryCache.SetDirectory(CURL(top), topItems, XFILE::CacheType::ALWAYS);

  CFileItemList subItems;
  subItems.Add(std::make_shared<CFileItem>(URIUtils::AddFileToFolder(sub, "b.mp4"), false));
  subItems.Add(std::make_shared<CFileItem>(top, true));
  g_directoryCache.SetDirectory(CURL(sub), subItems, XFILE::CacheType::ALWAYS);

  CFileItemList pictures;
  CFileItemList media;
  ASSERT_TRUE(CTestPlayerOperations::List(top, true, pictures, media));

  EXPECT_EQ(0, pictures.Size());
  EXPECT_EQ((std::vector<std::string>{"a.mp4", "b.mp4"}), Names(media));
}

TEST_F(TestPlayerOpenDirectory, ADirectoryThatCannotBeListedIsReported)
{
  CFileItemList pictures;
  CFileItemList media;
  EXPECT_FALSE(CTestPlayerOperations::List(URIUtils::AddFileToFolder(m_root, "missing/"), true,
                                           pictures, media));
}
