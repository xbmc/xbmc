/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "interfaces/json-rpc/FileOperations.h"
#include "utils/Variant.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoTag.h"

#include <initializer_list>
#include <memory>
#include <set>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

using namespace JSONRPC;

namespace
{
class CTestFileItemHandler : public CFileItemHandler
{
public:
  using CFileItemHandler::HandleFileItem;
};

CVariant ParamsWithProperties(std::initializer_list<std::string_view> properties)
{
  CVariant params(CVariant::VariantTypeObject);
  params["properties"] = CVariant(CVariant::VariantTypeArray);
  for (const std::string_view property : properties)
    params["properties"].append(std::string(property));
  return params;
}
} // unnamed namespace

TEST(TestFileOperations, MissingPropertiesDoesNotNeedLibraryLookup)
{
  const CVariant params(CVariant::VariantTypeObject);
  EXPECT_FALSE(CFileOperations::NeedsLibraryLookup(params));
}

TEST(TestFileOperations, BasicFilePropertiesDoNotNeedLibraryLookup)
{
  const CVariant params =
      ParamsWithProperties({"file", "filetype", "label", "mimetype", "size", "lastmodified"});
  EXPECT_FALSE(CFileOperations::NeedsLibraryLookup(params));
}

TEST(TestFileOperations, VideoPropertiesNeedLibraryLookup)
{
  EXPECT_TRUE(CFileOperations::NeedsLibraryLookup(ParamsWithProperties({"thumbnail"})));
  EXPECT_TRUE(CFileOperations::NeedsLibraryLookup(ParamsWithProperties({"cast"})));
  EXPECT_TRUE(CFileOperations::NeedsLibraryLookup(ParamsWithProperties({"file", "streamdetails"})));
}

TEST(TestFileOperations, FolderUsesBrowsedPathWithoutFileTypeProperty)
{
  const std::string browsedPath = "smb://server/videos/movies/";
  const auto item = std::make_shared<CFileItem>(browsedPath, true);
  item->GetVideoInfoTag()->SetPath("videodb://movies/titles/1/");

  CVariant result(CVariant::VariantTypeObject);
  CTestFileItemHandler::HandleFileItem(nullptr, true, "filedetails", item,
                                       CVariant(CVariant::VariantTypeObject),
                                       std::set<std::string>{"file"}, result, false);

  EXPECT_EQ(browsedPath, result["filedetails"]["file"].asString());
}

TEST(TestFileOperations, MatchesVideoDatabaseItemByFilesystemPath)
{
  const auto libraryItem = std::make_shared<CFileItem>("/movies/movie.mkv", false);
  CFileItemList libraryItems;
  libraryItems.Add(libraryItem);
  libraryItems.SetFastLookup(true);

  const CFileItem browsedItem("/movies/movie.mkv", false);
  EXPECT_EQ(libraryItem, CVideoDatabase::GetMatchingItemForPath(browsedItem, libraryItems));
}

TEST(TestFileOperations, MatchesVideoDatabaseItemByFirstMultipathComponent)
{
  const auto libraryItem = std::make_shared<CFileItem>("/movies/one/", true);
  CFileItemList libraryItems;
  libraryItems.Add(libraryItem);
  libraryItems.SetFastLookup(true);

  const CFileItem browsedItem("multipath://%2fmovies%2fone%2f/%2fmovies%2ftwo%2f/", true);
  EXPECT_EQ(libraryItem, CVideoDatabase::GetMatchingItemForPath(browsedItem, libraryItems));
}
