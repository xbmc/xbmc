/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "filesystem/Directory.h"
#include "filesystem/DirectoryFactory.h"
#include "filesystem/IDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "test/TestUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoInfoTag.h"

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
