/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/VideoDatabaseDirectory.h"
#include "filesystem/VideoDatabaseDirectory/DirectoryNode.h"
#include "video/VideoDbPaths.h"

#include <utility>

#include <gtest/gtest.h>

using namespace KODI;
using XFILE::CVideoDatabaseDirectory;
using XFILE::VIDEODATABASEDIRECTORY::NodeType;

TEST(TestVideoDbPaths, EveryPathListsTheNodeItNames)
{
  const std::pair<const char*, NodeType> paths[] = {
      {VIDEO::DB_PATH::ROOT, NodeType::OVERVIEW},
      {VIDEO::DB_PATH::MOVIES, NodeType::MOVIES_OVERVIEW},
      {VIDEO::DB_PATH::MOVIE_GENRES, NodeType::GENRE},
      {VIDEO::DB_PATH::MOVIE_TITLES, NodeType::TITLE_MOVIES},
      {VIDEO::DB_PATH::MOVIE_YEARS, NodeType::YEAR},
      {VIDEO::DB_PATH::MOVIE_ACTORS, NodeType::ACTOR},
      {VIDEO::DB_PATH::MOVIE_DIRECTORS, NodeType::DIRECTOR},
      {VIDEO::DB_PATH::MOVIE_STUDIOS, NodeType::STUDIO},
      {VIDEO::DB_PATH::MOVIE_SETS, NodeType::SETS},
      {VIDEO::DB_PATH::MOVIE_COUNTRIES, NodeType::COUNTRY},
      {VIDEO::DB_PATH::MOVIE_TAGS, NodeType::TAGS},
      {VIDEO::DB_PATH::TVSHOWS, NodeType::TVSHOWS_OVERVIEW},
      {VIDEO::DB_PATH::TVSHOW_GENRES, NodeType::GENRE},
      {VIDEO::DB_PATH::TVSHOW_TITLES, NodeType::TITLE_TVSHOWS},
      {VIDEO::DB_PATH::TVSHOW_YEARS, NodeType::YEAR},
      {VIDEO::DB_PATH::TVSHOW_ACTORS, NodeType::ACTOR},
      {VIDEO::DB_PATH::TVSHOW_STUDIOS, NodeType::STUDIO},
      {VIDEO::DB_PATH::TVSHOW_TAGS, NodeType::TAGS},
      {VIDEO::DB_PATH::MUSICVIDEOS, NodeType::MUSICVIDEOS_OVERVIEW},
      {VIDEO::DB_PATH::MUSICVIDEO_GENRES, NodeType::GENRE},
      {VIDEO::DB_PATH::MUSICVIDEO_TITLES, NodeType::TITLE_MUSICVIDEOS},
      {VIDEO::DB_PATH::MUSICVIDEO_YEARS, NodeType::YEAR},
      {VIDEO::DB_PATH::MUSICVIDEO_ARTISTS, NodeType::ACTOR},
      {VIDEO::DB_PATH::MUSICVIDEO_ALBUMS, NodeType::MUSICVIDEOS_ALBUM},
      {VIDEO::DB_PATH::MUSICVIDEO_DIRECTORS, NodeType::DIRECTOR},
      {VIDEO::DB_PATH::MUSICVIDEO_STUDIOS, NodeType::STUDIO},
      {VIDEO::DB_PATH::MUSICVIDEO_TAGS, NodeType::TAGS},
      {VIDEO::DB_PATH::RECENTLY_ADDED_MOVIES, NodeType::RECENTLY_ADDED_MOVIES},
      {VIDEO::DB_PATH::RECENTLY_ADDED_EPISODES, NodeType::RECENTLY_ADDED_EPISODES},
      {VIDEO::DB_PATH::RECENTLY_ADDED_MUSICVIDEOS, NodeType::RECENTLY_ADDED_MUSICVIDEOS},
      {VIDEO::DB_PATH::INPROGRESS_TVSHOWS, NodeType::INPROGRESS_TVSHOWS},
  };

  for (const auto& [path, node] : paths)
    EXPECT_EQ(CVideoDatabaseDirectory::GetDirectoryChildType(path), node) << path;
}
