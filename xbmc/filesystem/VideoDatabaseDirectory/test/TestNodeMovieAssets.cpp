/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/VideoDatabaseDirectory/DirectoryNode.h"
#include "filesystem/VideoDatabaseDirectory/QueryParams.h"
#include "test/TestUtils.h"
#include "video/VideoDatabase.h"

#include <gtest/gtest.h>

using namespace XFILE::VIDEODATABASEDIRECTORY;

TEST(TestNodeMovieAssets, General)
{
  std::unique_ptr<CDirectoryNode> node;
  node.reset(CDirectoryNode::ParseURL("videodb://movies/titles/123/456/789"));

  EXPECT_TRUE(node);

  ASSERT_EQ(node->GetType(), NodeType::MOVIE_ASSETS);
  ASSERT_EQ(node->GetChildType(), NodeType::NONE);
  EXPECT_TRUE(node->GetParent());
  ASSERT_EQ(node->GetParent()->GetType(), NodeType::MOVIE_ASSET_TYPES);

  CQueryParams params;
  node->CollectQueryParams(params);

  ASSERT_EQ(static_cast<VideoDbContentType>(params.GetContentType()), VideoDbContentType::MOVIES);
  ASSERT_EQ(params.GetMovieId(), 123);
  ASSERT_EQ(params.GetVideoAssetType(), 456);
  ASSERT_EQ(params.GetVideoAssetId(), 789);

  params = {};
  CDirectoryNode::GetDatabaseInfo("videodb://movies/titles/123/456", params);
  ASSERT_EQ(static_cast<VideoDbContentType>(params.GetContentType()), VideoDbContentType::MOVIES);
  ASSERT_EQ(params.GetMovieId(), 123);
  ASSERT_EQ(params.GetVideoAssetType(), 456);
  ASSERT_EQ(params.GetVideoAssetId(), -1);

  params = {};
  CDirectoryNode::GetDatabaseInfo("videodb://movies/titles/123", params);
  ASSERT_EQ(static_cast<VideoDbContentType>(params.GetContentType()), VideoDbContentType::MOVIES);
  ASSERT_EQ(params.GetMovieId(), 123);
  ASSERT_EQ(params.GetVideoAssetType(), -1);
  ASSERT_EQ(params.GetVideoAssetId(), -1);
}
