/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/MusicDatabaseDirectory.h"
#include "filesystem/MusicDatabaseDirectory/DirectoryNode.h"
#include "music/MusicDbPaths.h"

#include <utility>

#include <gtest/gtest.h>

using namespace KODI;
using XFILE::CMusicDatabaseDirectory;
using XFILE::MUSICDATABASEDIRECTORY::NodeType;

TEST(TestMusicDbPaths, EveryPathListsTheNodeItNames)
{
  const std::pair<const char*, NodeType> paths[] = {
      {MUSIC::DB_PATH::ROOT, NodeType::OVERVIEW},
      {MUSIC::DB_PATH::GENRES, NodeType::GENRE},
      {MUSIC::DB_PATH::ARTISTS, NodeType::ARTIST},
      {MUSIC::DB_PATH::ALBUMS, NodeType::ALBUM},
      {MUSIC::DB_PATH::BOX_SETS, NodeType::ALBUM},
      {MUSIC::DB_PATH::SINGLES, NodeType::SINGLES},
      {MUSIC::DB_PATH::SONGS, NodeType::SONG},
      {MUSIC::DB_PATH::YEARS, NodeType::YEAR},
      {MUSIC::DB_PATH::ORIGINAL_YEARS, NodeType::YEAR},
      {MUSIC::DB_PATH::TOP100, NodeType::TOP100},
      {MUSIC::DB_PATH::TOP100_ALBUMS, NodeType::ALBUM_TOP100},
      {MUSIC::DB_PATH::TOP100_SONGS, NodeType::SONG_TOP100},
      {MUSIC::DB_PATH::RECENTLY_ADDED_ALBUMS, NodeType::ALBUM_RECENTLY_ADDED},
      {MUSIC::DB_PATH::RECENTLY_PLAYED_ALBUMS, NodeType::ALBUM_RECENTLY_PLAYED},
      {MUSIC::DB_PATH::COMPILATIONS, NodeType::ALBUM},
      {MUSIC::DB_PATH::ROLES, NodeType::ROLE},
      {MUSIC::DB_PATH::SOURCES, NodeType::SOURCE},
      {MUSIC::DB_PATH::DISCS, NodeType::DISC},
  };

  for (const auto& [path, node] : paths)
    EXPECT_EQ(CMusicDatabaseDirectory::GetDirectoryChildType(path), node) << path;
}
