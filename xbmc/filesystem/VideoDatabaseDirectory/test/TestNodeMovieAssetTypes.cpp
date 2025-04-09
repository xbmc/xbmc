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

TEST(TestNodeMovieAssetTypes, General)
{
  std::unique_ptr<CDirectoryNode> node(CDirectoryNode::ParseURL("videodb://movies/titles/123/456"));

  EXPECT_TRUE(node);

  ASSERT_EQ(node->GetType(), NodeType::MOVIE_ASSET_TYPES);
  ASSERT_EQ(node->GetChildType(), NodeType::MOVIE_ASSETS);
  EXPECT_TRUE(node->GetParent());
  ASSERT_EQ(node->GetParent()->GetType(), NodeType::TITLE_MOVIES);

  CQueryParams params;
  node->CollectQueryParams(params);

  ASSERT_EQ(static_cast<VideoDbContentType>(params.GetContentType()), VideoDbContentType::MOVIES);
  ASSERT_EQ(params.GetMovieId(), 123);
  ASSERT_EQ(params.GetVideoAssetType(), 456);

  params = {};
  CDirectoryNode::GetDatabaseInfo("videodb://movies/titles/123", params);
  ASSERT_EQ(static_cast<VideoDbContentType>(params.GetContentType()), VideoDbContentType::MOVIES);
  ASSERT_EQ(params.GetMovieId(), 123);
  ASSERT_EQ(params.GetVideoAssetType(), -1);
}

TEST(TestNodeMovieAssetTypes, ChildType)
{
  std::unique_ptr<CDirectoryNode> node;

  // special value to return versions + extras virtual folder.
  node.reset(CDirectoryNode::ParseURL("videodb://movies/titles/123/-2"));

  EXPECT_TRUE(node);
  ASSERT_EQ(node->GetChildType(), NodeType::MOVIE_ASSETS_VERSIONS);

  // all types
  node.reset(CDirectoryNode::ParseURL("videodb://movies/titles/123/0"));

  EXPECT_TRUE(node);
  ASSERT_EQ(node->GetChildType(), NodeType::MOVIE_ASSETS);

  // versions
  node.reset(CDirectoryNode::ParseURL("videodb://movies/titles/123/1"));

  EXPECT_TRUE(node);
  ASSERT_EQ(node->GetChildType(), NodeType::MOVIE_ASSETS_VERSIONS);

  // extras
  node.reset(CDirectoryNode::ParseURL("videodb://movies/titles/123/2"));

  EXPECT_TRUE(node);
  ASSERT_EQ(node->GetChildType(), NodeType::MOVIE_ASSETS_EXTRAS);

  // unused value at this time, supposed to default to MOVIE_ASSETS
  node.reset(CDirectoryNode::ParseURL("videodb://movies/titles/123/3"));

  EXPECT_TRUE(node);
  ASSERT_EQ(node->GetChildType(), NodeType::MOVIE_ASSETS);
}
