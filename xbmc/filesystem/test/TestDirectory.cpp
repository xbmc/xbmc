/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "filesystem/Directory.h"
#include "filesystem/DirectoryFactory.h"
#include "filesystem/File.h"
#include "filesystem/IDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "test/TestUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoInfoTag.h"

#include <memory>
#include <string>

#include <gtest/gtest.h>

TEST(TestDirectory, General)
{
  std::string tmppath1, tmppath2, tmppath3;
  CFileItemList items;
  CFileItemPtr itemptr;
  tmppath1 = CSpecialProtocol::TranslatePath("special://temp/");
  tmppath1 = URIUtils::AddFileToFolder(tmppath1, "TestDirectory");
  tmppath2 = tmppath1;
  tmppath2 = URIUtils::AddFileToFolder(tmppath2, "subdir");
  EXPECT_TRUE(XFILE::CDirectory::Create(tmppath1));
  EXPECT_TRUE(XFILE::CDirectory::Exists(tmppath1));
  EXPECT_FALSE(XFILE::CDirectory::Exists(tmppath2));
  EXPECT_TRUE(XFILE::CDirectory::Create(tmppath2));
  EXPECT_TRUE(XFILE::CDirectory::Exists(tmppath2));
  EXPECT_TRUE(XFILE::CDirectory::GetDirectory(tmppath1, items, "", XFILE::DIR_FLAG_DEFAULTS));
  XFILE::CDirectory::FilterFileDirectories(items, "");
  tmppath3 = tmppath2;
  URIUtils::AddSlashAtEnd(tmppath3);
  itemptr = items[0];
  EXPECT_STREQ(tmppath3.c_str(), itemptr->GetPath().c_str());
  EXPECT_TRUE(XFILE::CDirectory::Remove(tmppath2));
  EXPECT_FALSE(XFILE::CDirectory::Exists(tmppath2));
  EXPECT_TRUE(XFILE::CDirectory::Exists(tmppath1));
  EXPECT_TRUE(XFILE::CDirectory::Remove(tmppath1));
  EXPECT_FALSE(XFILE::CDirectory::Exists(tmppath1));
}

TEST(TestDirectory, ListingOrderIsKeptForPlayListOrder)
{
  const std::string dir =
      URIUtils::AddFileToFolder(CSpecialProtocol::TranslatePath("special://temp/"), "listing");
  ASSERT_TRUE(XFILE::CDirectory::Create(dir));
  const std::string m3u = URIUtils::AddFileToFolder(dir, "order.m3u");
  {
    XFILE::CFile file;
    ASSERT_TRUE(file.OpenForWrite(m3u, true));
    const std::string body = "#EXTM3U\nc.mp3\na.mp3\nb.mp3\n";
    file.Write(body.data(), body.size());
  }

  CFileItemList items;
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(m3u, items, "", XFILE::DIR_FLAG_DEFAULTS));
  ASSERT_EQ(3, items.Size());
  items.Sort(SortBy::LABEL, SortOrder::ASCENDING);
  items.Sort(SortBy::PLAYLIST_ORDER, SortOrder::ASCENDING);

  EXPECT_EQ("c.mp3", URIUtils::GetFileName(items[0]->GetPath()));
  EXPECT_EQ("a.mp3", URIUtils::GetFileName(items[1]->GetPath()));
  EXPECT_EQ("b.mp3", URIUtils::GetFileName(items[2]->GetPath()));
  EXPECT_TRUE(XFILE::CDirectory::RemoveRecursive(dir));
}

TEST(TestDirectory, ExistsAfterCreateInListedFolder)
{
  const std::string parent{
      URIUtils::AddFileToFolder(CSpecialProtocol::TranslatePath("special://temp/"), "listed")};
  const std::string child{URIUtils::AddFileToFolder(parent, "child")};
  ASSERT_TRUE(XFILE::CDirectory::Create(parent));

  // Listing the parent puts it in the directory cache
  CFileItemList items;
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(parent, items, "", XFILE::DIR_FLAG_DEFAULTS));

  EXPECT_TRUE(XFILE::CDirectory::Create(child));
  EXPECT_TRUE(XFILE::CDirectory::Exists(child));
  EXPECT_TRUE(XFILE::CDirectory::RemoveRecursive(parent));
}

TEST(TestDirectory, CreateRecursive)
{
  auto path1 = URIUtils::AddFileToFolder(
    CSpecialProtocol::TranslatePath("special://temp/"),
    "level1");
  auto path2 = URIUtils::AddFileToFolder(path1,
    "level2",
    "level3");

  EXPECT_TRUE(XFILE::CDirectory::Create(path2));
  EXPECT_TRUE(XFILE::CDirectory::RemoveRecursive(path1));
}

TEST(TestDirectory, CreateForDotM3uFolder)
{
  const std::string dir{
      URIUtils::AddFileToFolder(CSpecialProtocol::TranslatePath("special://temp/"), "game.m3u")};
  ASSERT_TRUE(XFILE::CDirectory::Create(dir));
  std::string dirWithSlash{dir};
  URIUtils::AddSlashAtEnd(dirWithSlash);

  for (const std::string& path : {dir, dirWithSlash})
  {
    const CURL url{path};
    const std::unique_ptr<XFILE::IDirectory> pDir{XFILE::CDirectoryFactory::Create(url)};
    ASSERT_NE(nullptr, pDir) << path;
    EXPECT_TRUE(pDir->Exists(url)) << path;
  }
  EXPECT_TRUE(XFILE::CDirectory::RemoveRecursive(dir));
}

#ifdef HAVE_LIBBLURAY
TEST(TestDirectory, BlurayResolve)
{
  CFileItem item;
  CVideoInfoTag* tag{item.GetVideoInfoTag()};

  // Emulate first play of removable bluray disc
  item.SetPath("E:\\BDMV\\index.bdmv");
  std::string pathToResolve{
      "bluray://removable%3a%2f%2fsometitle_0000000000000000000000000000000000000000%00%cc"
      "/BDMV/index.bdmv"};
  item.SetDynPath(pathToResolve);
  tag->m_strFileNameAndPath = pathToResolve;

  if (const std::unique_ptr<XFILE::IDirectory> dir{XFILE::CDirectoryFactory::Create(item)}; dir)
  {
    EXPECT_TRUE(dir->Resolve(item));
    std::string correctResolvedPath{"E:\\BDMV\\index.bdmv"};
    EXPECT_STREQ(item.GetDynPath().c_str(), correctResolvedPath.c_str());

    // Emulate second/resume play of removable bluray disc
    pathToResolve =
        "bluray://removable%3a%2f%2fsometitle_0000000000000000000000000000000000000000%00%cc"
        "/BDMV/PLAYLIST/00800.mpls";
    item.SetDynPath(pathToResolve);
    tag->m_strFileNameAndPath = pathToResolve;

    EXPECT_TRUE(dir->Resolve(item));
    correctResolvedPath = "bluray://E%3a%5c/BDMV/PLAYLIST/00800.mpls";
    EXPECT_STREQ(item.GetDynPath().c_str(), correctResolvedPath.c_str());
  }
}
#endif
