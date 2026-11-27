/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "PartyMode.h"
#include "filesystem/Directory.h"
#include "media/LibraryQuery.h"
#include "media/LibraryQueryType.h"
#include "music/tags/MusicInfoTag.h"
#include "playlists/PlayListTypes.h"
#include "test/TestLibrary.h"
#include "test/TestUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoInfoTag.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;

namespace
{
std::shared_ptr<CFileItem> Song(int id)
{
  auto item = std::make_shared<CFileItem>("/music/" + std::to_string(id) + ".flac", false);
  item->GetMusicInfoTag()->SetDatabaseId(id, KODI::MEDIA::TYPE::SONG);
  return item;
}

std::shared_ptr<CFileItem> MusicVideo(int id)
{
  auto item = std::make_shared<CFileItem>("/video/" + std::to_string(id) + ".mkv", false);
  item->GetVideoInfoTag()->m_iDbId = id;
  return item;
}

std::vector<std::string> Paths(const std::vector<std::shared_ptr<CFileItem>>& items)
{
  std::vector<std::string> paths;
  for (const auto& item : items)
    paths.push_back(item->GetPath());
  return paths;
}
} // namespace

TEST(TestPartyMode, ABatchIsQueuedInTheMatchesOrderNotTheDatabases)
{
  constexpr auto SONG = PARTYMODE::Library::Song;
  constexpr auto VIDEO = PARTYMODE::Library::MusicVideo;
  CFileItemList fetched;
  fetched.Add(Song(1));
  fetched.Add(Song(7));
  fetched.Add(MusicVideo(7));

  EXPECT_EQ((std::vector<std::string>{"/video/7.mkv", "/music/1.flac", "/music/7.flac"}),
            Paths(PARTYMODE::InMatchOrder({{VIDEO, 7}, {SONG, 1}, {SONG, 7}}, fetched)))
      << "a song and a music video may share an id";
}

TEST(TestPartyMode, AMatchTheDatabaseNoLongerHoldsIsSkipped)
{
  constexpr auto SONG = PARTYMODE::Library::Song;
  CFileItemList fetched;
  fetched.Add(Song(3));
  fetched.Add(Song(1));

  EXPECT_EQ((std::vector<std::string>{"/music/1.flac", "/music/3.flac"}),
            Paths(PARTYMODE::InMatchOrder({{SONG, 1}, {SONG, 2}, {SONG, 3}}, fetched)));
}

namespace
{
constexpr auto RULES_FOLDER = "special://temp/test-partymode-rules/";

using TypeCanStart = std::pair<std::string, bool>;

class TestPartyModeCanStart : public ::testing::TestWithParam<TypeCanStart>
{
protected:
  void SetUp() override { ASSERT_TRUE(XFILE::CDirectory::Create(RULES_FOLDER)); }
  void TearDown() override { XFILE::CDirectory::RemoveRecursive(RULES_FOLDER); }
};
} // namespace

TEST_P(TestPartyModeCanStart, OnlyTypesItCanFeedFromAreOffered)
{
  const auto& [type, canStart] = GetParam();
  const std::string path{URIUtils::AddFileToFolder(RULES_FOLDER, "rules.xsp")};
  ASSERT_TRUE(WriteTextFile(path, "<smartplaylist type=\"" + type +
                                      "\"><name>Rules</name><match>all</match></smartplaylist>"));

  EXPECT_EQ(canStart, PARTYMODE::CanStart(path));
}

INSTANTIATE_TEST_SUITE_P(Types,
                         TestPartyModeCanStart,
                         ::testing::Values(TypeCanStart{"songs", true},
                                           TypeCanStart{"musicvideos", true},
                                           TypeCanStart{"mixed", true},
                                           TypeCanStart{"music", false},
                                           TypeCanStart{"video", false},
                                           TypeCanStart{"albums", false},
                                           TypeCanStart{"artists", false},
                                           TypeCanStart{"movies", false},
                                           TypeCanStart{"tvshows", false},
                                           TypeCanStart{"episodes", false},
                                           TypeCanStart{"", false}));

TEST(TestPartyMode, AMissingRulesFileCannotStart)
{
  EXPECT_FALSE(PARTYMODE::CanStart("special://temp/test-partymode-missing.xsp"));
}

namespace
{
using PARTYMODE::Library;
using PARTYMODE::Match;

class TestPartyModeMatches : public ::testing::Test
{
protected:
  void SetUp() override
  {
    m_rockSong = m_library.AddSong("Rock song", "Album", "Artist");
    m_popSong = m_library.AddSong("Pop song", "Album", "Artist");
    m_rockClip = m_library.AddMusicVideo("Rock clip", "Band");
    m_popClip = m_library.AddMusicVideo("Pop clip", "Band");
    ASSERT_GT(m_rockSong, 0);
    ASSERT_GT(m_popSong, 0);
    ASSERT_GT(m_rockClip, 0);
    ASSERT_GT(m_popClip, 0);
  }

  //! What rules of this type for titles starting with "Rock" match, in id order
  static std::vector<Match> RockMatches(const std::string& type)
  {
    MEDIA::CLibraryQuery rules;
    EXPECT_TRUE(rules.LoadFromJson(
        R"({"type":")" + type +
        R"(","rules":{"and":[{"field":"title","operator":"startswith","value":"Rock"}]}})"));
    return Sorted(PARTYMODE::FindMatches(&rules, rules.GetType()));
  }

  static std::vector<Match> Sorted(std::optional<std::vector<Match>> matches)
  {
    EXPECT_TRUE(matches);
    std::vector<Match> sorted{matches.value_or(std::vector<Match>{})};
    std::ranges::sort(sorted);
    return sorted;
  }

  CTestLibrary m_library{"TestPartyMode"};
  int m_rockSong{-1};
  int m_popSong{-1};
  int m_rockClip{-1};
  int m_popClip{-1};
};
} // namespace

TEST_F(TestPartyModeMatches, SongRulesMatchSongs)
{
  EXPECT_EQ((std::vector<Match>{{Library::Song, m_rockSong}}), RockMatches("songs"));
}

TEST_F(TestPartyModeMatches, MusicVideoRulesMatchMusicVideos)
{
  EXPECT_EQ((std::vector<Match>{{Library::MusicVideo, m_rockClip}}), RockMatches("musicvideos"));
}

TEST_F(TestPartyModeMatches, MixedRulesMatchSongsAndMusicVideos)
{
  EXPECT_EQ((std::vector<Match>{{Library::Song, m_rockSong}, {Library::MusicVideo, m_rockClip}}),
            RockMatches("mixed"));
}

TEST_F(TestPartyModeMatches, RulesThatCannotBeListedFindNoMatches)
{
  MEDIA::CLibraryQuery rules;
  ASSERT_TRUE(rules.LoadFromJson(
      R"({"type":"mixed","rules":{"and":[{"field":"originaltitle","operator":"is","value":"Title"}]}})"));
  EXPECT_FALSE(PARTYMODE::FindMatches(&rules, rules.GetType()));
}

TEST_F(TestPartyModeMatches, WithoutRulesEverythingOfTheContentMatches)
{
  EXPECT_EQ((std::vector<Match>{{Library::Song, m_rockSong}, {Library::Song, m_popSong}}),
            Sorted(PARTYMODE::FindMatches(nullptr, MEDIA::LIBRARY_QUERY::TYPE::SONGS)));
  EXPECT_EQ((std::vector<Match>{{Library::Song, m_rockSong},
                                {Library::Song, m_popSong},
                                {Library::MusicVideo, m_rockClip},
                                {Library::MusicVideo, m_popClip}}),
            Sorted(PARTYMODE::FindMatches(nullptr, MEDIA::LIBRARY_QUERY::TYPE::MIXED)));
}
