/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "filesystem/Directory.h"
#include "filesystem/SmartPlaylistDirectory.h"
#include "media/LibraryQuery.h"
#include "settings/Settings.h"
#include "test/TestLibrary.h"
#include "test/TestUtils.h"
#include "utils/ContentNames.h"
#include "utils/ItemProperties.h"
#include "utils/SortUtils.h"
#include "utils/URIUtils.h"

#include <algorithm>
#include <ostream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;

namespace
{
constexpr auto PLAYLISTS_FOLDER = "special://temp/test-smartplaylist-directory/";

//! Lists from a music and a video library that each hold two items of every kind.
class TestSmartPlaylistDirectory : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_GT(m_library.AddMovie("Movie one"), 0);
    ASSERT_GT(m_library.AddMovie("Movie two"), 0);
    for (const std::string show : {"Show one", "Show two"})
    {
      const int idShow{m_library.AddTvShow(show)};
      ASSERT_GT(idShow, 0);
      ASSERT_GT(m_library.AddEpisode(idShow, show, "Episode of " + show, 1), 0);
    }
    ASSERT_GT(m_library.AddMusicVideo("Clip one", "Band one"), 0);
    ASSERT_GT(m_library.AddMusicVideo("Clip two", "Band two"), 0);
    ASSERT_GT(m_library.AddSong("Song one", "Album one", "Artist one"), 0);
    ASSERT_GT(m_library.AddSong("Song two", "Album two", "Artist two"), 0);
  }

  static MEDIA::CLibraryQuery Playlist(const std::string& type, const std::string& rest = "")
  {
    MEDIA::CLibraryQuery playlist;
    EXPECT_TRUE(playlist.LoadFromJson("{\"type\":\"" + type + "\"" + rest + "}"));
    return playlist;
  }

  //! A rule matching the items whose title or name ends in "one"
  static std::string OneRule(const std::string& field)
  {
    return ",\"rules\":{\"and\":[{\"field\":\"" + field +
           "\",\"operator\":\"endswith\",\"value\":\"one\"}]}";
  }

  static std::vector<std::string> SortedLabels(const CFileItemList& items)
  {
    std::vector<std::string> labels;
    for (const auto& item : items)
    {
      labels.push_back(item->GetLabel());
    }
    std::ranges::sort(labels);
    return labels;
  }

  static std::string DbPath(const CFileItemList& items)
  {
    return items.GetProperty(ITEM::PROPERTY::PATH_DB).asString();
  }

  CTestLibrary m_library{"TestSmartPlaylistDirectory"};
};

struct TypeListing
{
  std::string type;
  std::string titleField;
  std::string dbPath;
  std::vector<std::string> labels;
  std::vector<std::string> labelsEndingOne;
};

void PrintTo(const TypeListing& listing, std::ostream* os)
{
  *os << listing.type;
}

class TestSmartPlaylistDirectoryTypes : public TestSmartPlaylistDirectory,
                                        public ::testing::WithParamInterface<TypeListing>
{
};
} // namespace

TEST_P(TestSmartPlaylistDirectoryTypes, APlaylistWithoutRulesListsEverythingOfItsType)
{
  CFileItemList items;
  ASSERT_TRUE(XFILE::CSmartPlaylistDirectory::GetDirectory(Playlist(GetParam().type), items));

  EXPECT_EQ(GetParam().labels, SortedLabels(items));
  EXPECT_EQ(GetParam().type, items.GetContent());
  EXPECT_EQ(GetParam().dbPath, DbPath(items));
}

TEST_P(TestSmartPlaylistDirectoryTypes, APlaylistListsWhatItsRulesMatch)
{
  CFileItemList items;
  ASSERT_TRUE(XFILE::CSmartPlaylistDirectory::GetDirectory(
      Playlist(GetParam().type, OneRule(GetParam().titleField)), items));

  EXPECT_EQ(GetParam().labelsEndingOne, SortedLabels(items));
  const CURL dbUrl{DbPath(items)};
  EXPECT_EQ(GetParam().dbPath, dbUrl.GetWithoutOptions());
  EXPECT_TRUE(dbUrl.HasOption("xsp"));
  EXPECT_FALSE(dbUrl.HasOption("filter"));
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    Types,
    TestSmartPlaylistDirectoryTypes,
    ::testing::Values(
        TypeListing{"movies", "title", "videodb://movies/titles/",
                    {"Movie one", "Movie two"}, {"Movie one"}},
        TypeListing{"tvshows", "title", "videodb://tvshows/titles/",
                    {"Show one", "Show two"}, {"Show one"}},
        TypeListing{"episodes", "tvshow", "videodb://tvshows/titles/-1/-1/",
                    {"1x01. Episode of Show one", "1x01. Episode of Show two"},
                    {"1x01. Episode of Show one"}},
        TypeListing{"musicvideos", "title", "videodb://musicvideos/titles/",
                    {"Clip one", "Clip two"}, {"Clip one"}},
        TypeListing{"songs", "title", "musicdb://songs/",
                    {"Song one", "Song two"}, {"Song one"}},
        TypeListing{"albums", "album", "musicdb://albums/",
                    {"Album one", "Album two"}, {"Album one"}},
        TypeListing{"artists", "artist", "musicdb://artists/",
                    {"Artist one", "Artist two"}, {"Artist one"}},
        // The music videos' path is recorded over the songs'
        TypeListing{"mixed", "title", "videodb://musicvideos/titles/",
                    {"Clip one", "Clip two", "Song one", "Song two"}, {"Clip one", "Song one"}}));
// clang-format on

TEST_F(TestSmartPlaylistDirectory, AMixedPlaylistIsOrderedAcrossBothLibraries)
{
  CFileItemList items;
  ASSERT_TRUE(XFILE::CSmartPlaylistDirectory::GetDirectory(
      Playlist(MEDIA::CONTENT::MIXED, R"(,"order":{"method":"title","direction":"ascending"})"),
      items));

  std::vector<std::string> labels;
  for (const auto& item : items)
  {
    labels.push_back(item->GetLabel());
  }
  EXPECT_EQ((std::vector<std::string>{"Clip one", "Clip two", "Song one", "Song two"}), labels);
}

TEST_F(TestSmartPlaylistDirectory, AMixedPlaylistAsksEachLibraryForNoMoreThanItsLimit)
{
  CFileItemList items;
  ASSERT_TRUE(XFILE::CSmartPlaylistDirectory::GetDirectory(
      Playlist(MEDIA::CONTENT::MIXED,
               R"(,"limit":1,"order":{"method":"title","direction":"descending"})"),
      items));

  ASSERT_EQ(1, items.Size()) << "the merged list is cut to the limit";
  EXPECT_EQ("Song two", items[0]->GetLabel());
  const std::string xsp{CURL{DbPath(items)}.GetOption("xsp")};
  EXPECT_NE(std::string::npos, xsp.find(R"("limit":1)")) << xsp;
}

TEST_F(TestSmartPlaylistDirectory, AnUnorderedMixedPlaylistTakesItsLimitFromEachLibrary)
{
  CFileItemList items;
  ASSERT_TRUE(XFILE::CSmartPlaylistDirectory::GetDirectory(
      Playlist(MEDIA::CONTENT::MIXED, R"(,"limit":1)"), items));

  ASSERT_EQ(2, items.Size());
  EXPECT_TRUE(items[0]->HasMusicInfoTag());
  EXPECT_TRUE(items[1]->HasVideoInfoTag());
}

TEST_F(TestSmartPlaylistDirectory, AMusicVideoPlaylistGroupedByArtistListsArtists)
{
  CFileItemList items;
  ASSERT_TRUE(XFILE::CSmartPlaylistDirectory::GetDirectory(
      Playlist(MEDIA::CONTENT::MUSICVIDEOS, OneRule("title") + R"(,"group":{"type":"artists"})"),
      items));

  EXPECT_EQ(std::vector<std::string>{"Band one"}, SortedLabels(items));
  EXPECT_EQ("artists", items.GetContent());
  EXPECT_EQ("artists", items.GetProperty(ITEM::PROPERTY::GROUP_BY).asString());
  EXPECT_EQ("videodb://musicvideos/artists/", CURL{DbPath(items)}.GetWithoutOptions());
}

TEST_F(TestSmartPlaylistDirectory, AGroupTheLibraryHasNoListOfIsNotListed)
{
  CFileItemList items;
  EXPECT_FALSE(XFILE::CSmartPlaylistDirectory::GetDirectory(
      Playlist(MEDIA::CONTENT::MOVIES, R"(,"group":{"type":"writers"})"), items));
}

TEST_F(TestSmartPlaylistDirectory, AGroupedPlaylistListsTheGroupsOfWhatItMatches)
{
  CFileItemList items;
  ASSERT_TRUE(XFILE::CSmartPlaylistDirectory::GetDirectory(
      Playlist(MEDIA::CONTENT::ALBUMS, OneRule("album") + R"(,"group":{"type":"artists"})"),
      items));

  EXPECT_EQ(std::vector<std::string>{"Artist one"}, SortedLabels(items));
  EXPECT_EQ("artists", items.GetContent());
  EXPECT_EQ("artists", items.GetProperty(ITEM::PROPERTY::GROUP_BY).asString());
  EXPECT_FALSE(items.GetProperty(ITEM::PROPERTY::GROUP_MIXED).asBoolean());
  EXPECT_EQ("musicdb://artists/", CURL{DbPath(items)}.GetWithoutOptions());
}

TEST_F(TestSmartPlaylistDirectory, AGroupThatMixesOrIsNoneListsTheItems)
{
  for (const std::string group :
       {R"(,"group":{"type":"sets","mixed":true})", R"(,"group":{"type":"none"})"})
  {
    CFileItemList items;
    ASSERT_TRUE(XFILE::CSmartPlaylistDirectory::GetDirectory(
        Playlist(MEDIA::CONTENT::MOVIES, group), items))
        << group;

    EXPECT_EQ((std::vector<std::string>{"Movie one", "Movie two"}), SortedLabels(items)) << group;
    EXPECT_EQ("movies", items.GetContent()) << group;
    EXPECT_EQ("videodb://movies/titles/", CURL{DbPath(items)}.GetWithoutOptions()) << group;
  }
}

TEST_F(TestSmartPlaylistDirectory, AFilterIsAppliedToTheListingItIsGiven)
{
  CFileItemList items;
  ASSERT_TRUE(XFILE::CSmartPlaylistDirectory::GetDirectory(
      Playlist(MEDIA::CONTENT::MOVIES, OneRule("title") + R"(,"limit":1)"), items,
      "videodb://movies/titles/", true));

  EXPECT_EQ(std::vector<std::string>{"Movie one"}, SortedLabels(items));
  const CURL dbUrl{DbPath(items)};
  EXPECT_FALSE(dbUrl.HasOption("xsp"));
  EXPECT_EQ(R"({"rules":{"and":[{"field":"title","operator":"endswith","value":["one"]}]},)"
            R"("type":"movies"})",
            dbUrl.GetOption("filter"));
}

TEST_F(TestSmartPlaylistDirectory, APlaylistsOrderLimitAndWatchedModeReachTheList)
{
  CFileItemList items;
  ASSERT_TRUE(XFILE::CSmartPlaylistDirectory::GetDirectory(
      Playlist(MEDIA::CONTENT::MOVIES,
               R"(,"limit":1,"order":{"method":"title","direction":"descending"},)"
               R"("watchedmode":1)"),
      items));

  EXPECT_EQ(std::vector<std::string>{"Movie two"}, SortedLabels(items));
  EXPECT_EQ(static_cast<int>(SortBy::TITLE),
            items.GetProperty(ITEM::PROPERTY::SORT_ORDER).asInteger());
  EXPECT_FALSE(items.GetProperty(ITEM::PROPERTY::SORT_ASCENDING).asBoolean());
  EXPECT_EQ(1, items.GetProperty(ITEM::PROPERTY::WATCHED_MODE).asInteger());
}

TEST_F(TestSmartPlaylistDirectory, AVirtualFolderRuleListsThatPlaylistAsAFolderFirst)
{
  const CScopedSetting playlistsPath{CSettings::SETTING_SYSTEM_PLAYLISTSPATH, PLAYLISTS_FOLDER};
  const std::string folder{URIUtils::AddFileToFolder(PLAYLISTS_FOLDER, "video")};
  ASSERT_TRUE(XFILE::CDirectory::Create(PLAYLISTS_FOLDER));
  ASSERT_TRUE(XFILE::CDirectory::Create(folder));
  const std::string other{URIUtils::AddFileToFolder(folder, "Other.xsp")};
  ASSERT_TRUE(WriteTextFile(
      other,
      "<smartplaylist type=\"movies\"><name>Other</name><match>all</match></smartplaylist>"));

  CFileItemList items;
  const bool listed{XFILE::CSmartPlaylistDirectory::GetDirectory(
      Playlist(MEDIA::CONTENT::MOVIES,
               R"(,"rules":{"and":[{"field":"virtualfolder","operator":"is","value":"Other"}]})"),
      items)};
  XFILE::CDirectory::RemoveRecursive(PLAYLISTS_FOLDER);

  ASSERT_TRUE(listed);
  ASSERT_EQ(3, items.Size());
  EXPECT_EQ(other, items[0]->GetPath());
  EXPECT_TRUE(items[0]->IsFolder());
}

TEST_F(TestSmartPlaylistDirectory, APlaylistNamingAMissingPlaylistIsNotListed)
{
  const CScopedSetting playlistsPath{CSettings::SETTING_SYSTEM_PLAYLISTSPATH, PLAYLISTS_FOLDER};

  for (const std::string field : {"playlist", "virtualfolder"})
  {
    CFileItemList items;
    EXPECT_FALSE(XFILE::CSmartPlaylistDirectory::GetDirectory(
        Playlist(MEDIA::CONTENT::MOVIES, R"(,"rules":{"and":[{"field":")" + field +
                                             R"(","operator":"is","value":"Gone"}]})"),
        items))
        << field;
    EXPECT_TRUE(items.IsEmpty()) << field;
  }
}

TEST_F(TestSmartPlaylistDirectory, APlaylistWithARuleOnAFieldItsTypeDoesNotHaveIsNotListed)
{
  for (const std::string type : {MEDIA::CONTENT::SONGS, MEDIA::CONTENT::MIXED})
  {
    CFileItemList items;
    EXPECT_FALSE(XFILE::CSmartPlaylistDirectory::GetDirectory(
        Playlist(type, R"(,"rules":{"and":[{"field":"videoresolution","operator":"is",)"
                       R"("value":"1080"}]})"),
        items))
        << type;
    EXPECT_EQ(0, items.GetObjectCount()) << type;
  }
}
