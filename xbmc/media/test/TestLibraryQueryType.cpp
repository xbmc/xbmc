/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "media/LibraryQueryType.h"
#include "playlists/PlayListTypes.h"

#include <string_view>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace KODI::MEDIA::LIBRARY_QUERY;

TEST(TestLibraryQueryType, APlaylistReferencesItsOwnTypeAndMixedGoesWithSongsAndMusicVideos)
{
  EXPECT_TRUE(CanReference(TYPE::SONGS, TYPE::MIXED));
  EXPECT_TRUE(CanReference(TYPE::MIXED, TYPE::MUSIC_VIDEOS));
  EXPECT_FALSE(CanReference(TYPE::SONGS, TYPE::MUSIC_VIDEOS));
  EXPECT_FALSE(CanReference(TYPE::MIXED, TYPE::ALBUMS));
}

TEST(TestLibraryQueryType, APlaylistIsLookedForInTheFoldersOfTheTypesItCanReference)
{
  using Folders = std::vector<std::string_view>;
  EXPECT_EQ((Folders{"music", "mixed"}), ReferenceFoldersOf(TYPE::SONGS));
  EXPECT_EQ((Folders{"music"}), ReferenceFoldersOf(TYPE::ALBUMS));
  EXPECT_EQ((Folders{"music"}), ReferenceFoldersOf(TYPE::ARTISTS));
  EXPECT_EQ((Folders{"video"}), ReferenceFoldersOf(TYPE::MOVIES));
  EXPECT_EQ((Folders{"video"}), ReferenceFoldersOf(TYPE::TV_SHOWS));
  EXPECT_EQ((Folders{"video"}), ReferenceFoldersOf(TYPE::EPISODES));
  EXPECT_EQ((Folders{"video", "mixed"}), ReferenceFoldersOf(TYPE::MUSIC_VIDEOS));
  EXPECT_EQ((Folders{"music", "video", "mixed"}), ReferenceFoldersOf(TYPE::MIXED));
}

TEST(TestLibraryQueryType, MusicPlaysOnTheAudioPlaylistAndEverythingElseOnVideo)
{
  for (const TYPE type : {TYPE::SONGS, TYPE::ALBUMS, TYPE::ARTISTS})
  {
    EXPECT_EQ(PLAYLIST::Audio, PlayListOf(type)) << NameOf(type);
  }
  for (const TYPE type :
       {TYPE::MOVIES, TYPE::TV_SHOWS, TYPE::EPISODES, TYPE::MUSIC_VIDEOS, TYPE::MIXED})
  {
    EXPECT_EQ(PLAYLIST::Video, PlayListOf(type)) << NameOf(type);
  }
}

TEST(TestLibraryQueryType, AGroupIsReadFromTheNameItIsWrittenAs)
{
  for (const GROUP group :
       {GROUP::DEFAULT, GROUP::NONE, GROUP::SET, GROUP::GENRE, GROUP::YEAR, GROUP::PERFORMER,
        GROUP::DIRECTOR, GROUP::WRITER, GROUP::STUDIO, GROUP::COUNTRY, GROUP::ALBUM, GROUP::TAG,
        GROUP::ORIGINAL_YEAR, GROUP::VIDEO_VERSION})
  {
    for (const TYPE type : TYPES)
    {
      EXPECT_EQ(group, GroupFromName(NameOf(group, type))) << NameOf(group, type);
    }
  }
  EXPECT_EQ("", NameOf(GROUP::DEFAULT, TYPE::MOVIES));
  EXPECT_EQ("none", NameOf(GROUP::NONE, TYPE::MOVIES));
  EXPECT_EQ("sets", NameOf(GROUP::SET, TYPE::MOVIES));
  EXPECT_EQ("originalyears", NameOf(GROUP::ORIGINAL_YEAR, TYPE::ALBUMS));
  EXPECT_EQ("videoversions", NameOf(GROUP::VIDEO_VERSION, TYPE::MOVIES));
  EXPECT_EQ(GROUP::GENRE, GroupFromName("Genres"));
  EXPECT_FALSE(GroupFromName("genre"));
  EXPECT_FALSE(GroupFromName("sources"));
}

TEST(TestLibraryQueryType, ThePerformersAreActorsForMoviesAndShowsAndArtistsOtherwise)
{
  EXPECT_EQ(GROUP::PERFORMER, GroupFromName("actors"));
  EXPECT_EQ(GROUP::PERFORMER, GroupFromName("artists"));
  for (const TYPE type : {TYPE::MOVIES, TYPE::TV_SHOWS, TYPE::EPISODES})
  {
    EXPECT_EQ("actors", NameOf(GROUP::PERFORMER, type)) << NameOf(type);
  }
  for (const TYPE type :
       {TYPE::SONGS, TYPE::ALBUMS, TYPE::ARTISTS, TYPE::MUSIC_VIDEOS, TYPE::MIXED})
  {
    EXPECT_EQ("artists", NameOf(GROUP::PERFORMER, type)) << NameOf(type);
  }
}
