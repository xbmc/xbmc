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

TEST(TestNodeTitleMovies, General)
{
  std::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL("videodb://movies/titles/123"));

  EXPECT_TRUE(pNode);

  ASSERT_EQ(pNode->GetType(), NodeType::TITLE_MOVIES);
  ASSERT_EQ(pNode->GetChildType(), NodeType::MOVIE_ASSET_TYPES);
  EXPECT_TRUE(pNode->GetParent());
  ASSERT_EQ(pNode->GetParent()->GetType(), NodeType::MOVIES_OVERVIEW);

  CQueryParams params;
  pNode->CollectQueryParams(params);

  ASSERT_EQ(static_cast<VideoDbContentType>(params.GetContentType()), VideoDbContentType::MOVIES);
  ASSERT_EQ(params.GetMovieId(), 123);

  params = {};
  CDirectoryNode::GetDatabaseInfo("videodb://movies/titles", params);
  ASSERT_EQ(static_cast<VideoDbContentType>(params.GetContentType()), VideoDbContentType::MOVIES);
  ASSERT_EQ(params.GetMovieId(), -1);
}
