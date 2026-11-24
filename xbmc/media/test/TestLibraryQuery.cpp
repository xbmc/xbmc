/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "dbwrappers/test/DatabaseTestUtils.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "media/LibraryQuery.h"
#include "media/LibraryQueryReferences.h"
#include "media/MediaSection.h"
#include "music/MusicDatabase.h"
#include "settings/Settings.h"
#include "test/TestUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML2.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;

namespace
{
constexpr auto FILES_FOLDER = "special://temp/test-smartplaylist-files/";

constexpr auto FULL_XSP = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<smartplaylist type="movies">
    <name>Everything</name>
    <match>one</match>
    <rule field="genre" operator="is">
        <value>Drama</value>
        <value>Comedy</value>
    </rule>
    <rule field="inprogress" operator="true"/>
    <rule field="year" operator="greaterthan">
        <value>1999</value>
    </rule>
    <group mixed="true">sets</group>
    <limit>25</limit>
    <order direction="descending" ignorefolders="true">rating</order>
    <watchedmode>1</watchedmode>
</smartplaylist>
)";

constexpr auto FULL_JSON =
    R"({"group":{"mixed":true,"type":"sets"},"limit":25,)"
    R"("order":{"direction":"descending","ignorefolders":true,"method":"rating"},)"
    R"("rules":{"or":[{"field":"genre","operator":"is","value":["Drama","Comedy"]},)"
    R"({"field":"inprogress","operator":"true","value":[]},)"
    R"({"field":"year","operator":"greaterthan","value":["1999"]}]},)"
    R"("type":"movies","watchedmode":1})";

class TestLibraryQueryFile : public ::testing::Test
{
protected:
  void SetUp() override { ASSERT_TRUE(XFILE::CDirectory::Create(FILES_FOLDER)); }
  void TearDown() override { XFILE::CDirectory::RemoveRecursive(FILES_FOLDER); }

  static std::string PathOf(const std::string& name)
  {
    return URIUtils::AddFileToFolder(FILES_FOLDER, name);
  }

  static void Write(const std::string& name, const std::string& text)
  {
    ASSERT_TRUE(WriteTextFile(PathOf(name), text));
  }

  static std::string Read(const std::string& name)
  {
    XFILE::CFile file;
    std::vector<uint8_t> content;
    EXPECT_GT(file.LoadFile(PathOf(name), content), 0);
    return {content.begin(), content.end()};
  }
};
} // namespace

TEST_F(TestLibraryQueryFile, AnXspFileLoadsAndSavesAsItWas)
{
  Write("in.xsp", FULL_XSP);

  MEDIA::CLibraryQuery playlist;
  ASSERT_TRUE(playlist.Load(PathOf("in.xsp")));
  EXPECT_EQ(MEDIA::LIBRARY_QUERY::TYPE::MOVIES, playlist.GetType());
  EXPECT_EQ("Everything", playlist.GetName());
  EXPECT_EQ(25u, playlist.GetLimit());
  EXPECT_EQ(SortBy::RATING, playlist.GetOrder());
  EXPECT_EQ(SortOrder::DESCENDING, playlist.GetOrderDirection());
  EXPECT_EQ(SortAttributeIgnoreFolders, playlist.GetOrderAttributes());
  EXPECT_EQ(MEDIA::LIBRARY_QUERY::GROUP::SET, playlist.GetGroup());
  EXPECT_TRUE(playlist.IsGroupMixed());
  ASSERT_TRUE(playlist.GetWatchedMode());
  EXPECT_EQ(1, static_cast<int>(*playlist.GetWatchedMode()));

  ASSERT_TRUE(playlist.Save(PathOf("out.xsp")));
  EXPECT_EQ(FULL_XSP, Read("out.xsp"));

  std::string json;
  ASSERT_TRUE(playlist.SaveAsJson(json));
  EXPECT_EQ(FULL_JSON, json) << "JSON has no name";
}

TEST_F(TestLibraryQueryFile, AnXspFileWithOnlyATypeSavesItsDefaults)
{
  Write("in.xsp", R"(<smartplaylist type="songs"></smartplaylist>)");

  MEDIA::CLibraryQuery playlist;
  ASSERT_TRUE(playlist.Load(PathOf("in.xsp")));
  EXPECT_EQ("in", playlist.GetName()) << "a playlist without a name is named after its file";

  ASSERT_TRUE(playlist.Save(PathOf("out.xsp")));
  EXPECT_EQ(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<smartplaylist type="songs">
    <name>in</name>
    <match>all</match>
</smartplaylist>
)",
            Read("out.xsp"));
}

TEST_F(TestLibraryQueryFile, ARuleValueWrittenAsTextIsSavedAsAValue)
{
  Write("in.xsp", R"(<smartplaylist type="songs"><name>Text</name><match>all</match>)"
                  R"(<rule field="genre" operator="contains">Rock</rule></smartplaylist>)");

  MEDIA::CLibraryQuery playlist;
  ASSERT_TRUE(playlist.Load(PathOf("in.xsp")));
  ASSERT_TRUE(playlist.Save(PathOf("out.xsp")));
  EXPECT_EQ(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<smartplaylist type="songs">
    <name>Text</name>
    <match>all</match>
    <rule field="genre" operator="contains">
        <value>Rock</value>
    </rule>
</smartplaylist>
)",
            Read("out.xsp"));
}

TEST(TestLibraryQuery, JsonLoadsAndSavesAsItWas)
{
  MEDIA::CLibraryQuery playlist;
  ASSERT_TRUE(playlist.LoadFromJson(FULL_JSON));

  std::string json;
  ASSERT_TRUE(playlist.SaveAsJson(json));
  EXPECT_EQ(FULL_JSON, json);

  ASSERT_TRUE(playlist.SaveAsJson(json, false));
  EXPECT_EQ(R"({"group":{"mixed":true,"type":"sets"},)"
            R"("rules":{"or":[{"field":"genre","operator":"is","value":["Drama","Comedy"]},)"
            R"({"field":"inprogress","operator":"true","value":[]},)"
            R"({"field":"year","operator":"greaterthan","value":["1999"]}]},)"
            R"("type":"movies","watchedmode":1})",
            json)
      << "a filter has no limit and order";
}

TEST(TestLibraryQuery, NestedRuleGroupsSurviveAJsonRoundTrip)
{
  const std::string nested{
      R"({"rules":{"and":[{"or":[{"field":"genre","operator":"is","value":["Rock"]},)"
      R"({"field":"genre","operator":"is","value":["Pop"]}]},)"
      R"({"field":"year","operator":"greaterthan","value":["1990"]}]},"type":"songs"})"};
  MEDIA::CLibraryQuery playlist;
  ASSERT_TRUE(playlist.LoadFromJson(nested));

  std::string json;
  ASSERT_TRUE(playlist.SaveAsJson(json));
  EXPECT_EQ(nested, json);
}

TEST(TestLibraryQuery, APlaylistWithoutRulesOrOrderIsEmpty)
{
  MEDIA::CLibraryQuery playlist;
  EXPECT_TRUE(playlist.IsEmpty());
  EXPECT_TRUE(playlist.IsEmpty(false));

  playlist.SetLimit(10);
  EXPECT_TRUE(playlist.IsEmpty()) << "a limit alone does not filter";
  EXPECT_FALSE(playlist.IsEmpty(false));
}

namespace
{
class TestLibraryQueryTypes : public TestLibraryQueryFile,
                              public ::testing::WithParamInterface<std::string>
{
};
} // namespace

TEST_P(TestLibraryQueryTypes, ATypeSurvivesXmlAndJson)
{
  const std::string& type{GetParam()};
  Write("in.xsp", "<smartplaylist type=\"" + type + "\"><name>Typed</name></smartplaylist>");

  MEDIA::CLibraryQuery fromXml;
  ASSERT_TRUE(fromXml.Load(PathOf("in.xsp")));
  EXPECT_EQ(type, MEDIA::LIBRARY_QUERY::NameOf(fromXml.GetType()));
  ASSERT_TRUE(fromXml.Save(PathOf("out.xsp")));
  EXPECT_NE(std::string::npos, Read("out.xsp").find("<smartplaylist type=\"" + type + "\">"));

  std::string json;
  ASSERT_TRUE(fromXml.SaveAsJson(json));
  EXPECT_EQ("{\"type\":\"" + type + "\"}", json);
  MEDIA::CLibraryQuery fromJson;
  ASSERT_TRUE(fromJson.LoadFromJson(json));
  EXPECT_EQ(type, MEDIA::LIBRARY_QUERY::NameOf(fromJson.GetType()));
}

INSTANTIATE_TEST_SUITE_P(
    Types,
    TestLibraryQueryTypes,
    ::testing::Values(
        "songs", "albums", "artists", "movies", "tvshows", "episodes", "musicvideos", "mixed"));

TEST_F(TestLibraryQueryFile, AnXspFileWithoutATypeItCanBeDoesNotLoad)
{
  for (const std::string root : {
           R"(<smartplaylist>)",
           R"(<smartplaylist type="">)",
           R"(<smartplaylist type="movie">)",
           R"(<smartplaylist type="Songs">)",
           R"(<smartplaylist type="music">)",
           R"(<smartplaylist type="video">)",
       })
  {
    Write("in.xsp", root + "<name>Typed</name><match>all</match></smartplaylist>");

    MEDIA::CLibraryQuery playlist;
    EXPECT_FALSE(playlist.Load(PathOf("in.xsp"))) << root;
    EXPECT_FALSE(playlist.OpenAndReadName(CURL{PathOf("in.xsp")})) << root;
  }
}

TEST(TestLibraryQuery, JsonWithoutATypeItCanBeDoesNotLoad)
{
  for (const std::string json : {
           R"({})",
           R"({"rules":{"and":[]}})",
           R"({"type":""})",
           R"({"type":7})",
           R"({"type":"movie"})",
           R"({"type":"Songs"})",
           R"({"type":"music"})",
           R"({"type":"video"})",
       })
  {
    MEDIA::CLibraryQuery playlist;
    EXPECT_FALSE(playlist.LoadFromJson(json)) << json;
  }
}

namespace
{
constexpr auto PLAYLISTS_FOLDER = "special://temp/test-smartplaylist-references/";

class TestLibraryQueryReferences : public ::testing::Test
{
protected:
  void SetUp() override
  {
    for (const char* folder : {"music", "video", "mixed"})
    {
      ASSERT_TRUE(XFILE::CDirectory::Create(URIUtils::AddFileToFolder(PLAYLISTS_FOLDER, folder)));
    }
  }
  void TearDown() override { XFILE::CDirectory::RemoveRecursive(PLAYLISTS_FOLDER); }

  static std::string PathOf(const std::string& folder, const std::string& name)
  {
    return URIUtils::AddFileToFolder(PLAYLISTS_FOLDER, folder, name + ".xsp");
  }

  //! Writes a playlist of this type into this folder of the playlists path
  static void WritePlaylist(const std::string& name,
                            const std::string& rules,
                            const std::string& type = "songs",
                            const std::string& folder = "music")
  {
    const std::string xml{"<smartplaylist type=\"" + type + "\"><name>" + name +
                          "</name><match>all</match>" + rules + "</smartplaylist>"};
    ASSERT_TRUE(WriteTextFile(PathOf(folder, name), xml));
  }

  //! The file names of a playlist's virtual folders; nullopt if a playlist it names is refused
  static std::optional<std::vector<std::string>> VirtualFolderNamesOf(
      const std::string& name, const std::string& folder = "music")
  {
    MEDIA::CLibraryQuery playlist;
    EXPECT_TRUE(playlist.Load(PathOf(folder, name)));
    const std::optional<MEDIA::CLibraryQueryReferences> references{
        MEDIA::CLibraryQueryReferences::Resolve(playlist)};
    if (!references)
    {
      return std::nullopt;
    }
    std::vector<std::string> folders{references->GetVirtualFolders()};
    for (auto& virtualFolder : folders)
    {
      virtualFolder = URIUtils::GetFileName(virtualFolder);
    }
    return folders;
  }

  //! A loop that is not stopped recurses until the stack overflows, which ends the whole test
  //! run, so the playlists are first resolved in a process of its own.
  static void ExpectResolutionEnds(const std::string& name)
  {
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    EXPECT_EXIT(
        {
          VirtualFolderNamesOf(name);
          std::_Exit(0);
        },
        ::testing::ExitedWithCode(0), "");
  }

  //! The clause a playlist gives for songs; nullopt if it is refused
  std::optional<std::string> ClauseOf(const std::string& name, const std::string& folder = "music")
  {
    MEDIA::CLibraryQuery playlist;
    EXPECT_TRUE(playlist.Load(PathOf(folder, name)));
    return playlist.GetWhereClause(m_db, MEDIA::TYPE::SONG, false);
  }

  std::string WhereOf(const std::string& name) { return ClauseOf(name).value_or("no clause"); }

  static std::string Rule(const std::string& field,
                          const std::string& value,
                          const std::string& oper = "is")
  {
    return "<rule field=\"" + field + "\" operator=\"" + oper + "\"><value>" + value +
           "</value></rule>";
  }

  CConnectedTestDatabase<CMusicDatabase> m_connection{"TestLibraryQueryReferences"};
  CMusicDatabase& m_db{m_connection.Get()};

  const CScopedSetting m_playlistsPath{CSettings::SETTING_SYSTEM_PLAYLISTSPATH, PLAYLISTS_FOLDER};
};
} // namespace

TEST_F(TestLibraryQueryReferences, APlaylistNamingItselfIsRefused)
{
  WritePlaylist("A", Rule("playlist", "A"));

  ExpectResolutionEnds("A");
  if (HasFailure())
  {
    return;
  }
  EXPECT_FALSE(VirtualFolderNamesOf("A"));
  EXPECT_FALSE(ClauseOf("A"));
}

TEST_F(TestLibraryQueryReferences, PlaylistsNamingEachOtherAreRefused)
{
  WritePlaylist("A", Rule("playlist", "B"));
  WritePlaylist("B", Rule("playlist", "A") + Rule("virtualfolder", "C"));
  WritePlaylist("C", Rule("genre", "Rock"));

  ExpectResolutionEnds("A");
  if (HasFailure())
  {
    return;
  }
  EXPECT_FALSE(VirtualFolderNamesOf("A"));
  EXPECT_FALSE(ClauseOf("A"));
  EXPECT_FALSE(ClauseOf("B"));
}

TEST_F(TestLibraryQueryReferences, APlaylistNamingAMissingPlaylistIsRefused)
{
  WritePlaylist("In", Rule("genre", "Rock") + Rule("playlist", "Gone"));
  WritePlaylist("Out", Rule("genre", "Rock") + Rule("playlist", "Gone", "isnot"));
  WritePlaylist("Folder", Rule("genre", "Rock") + Rule("virtualfolder", "Gone"));

  EXPECT_FALSE(ClauseOf("In"));
  EXPECT_FALSE(ClauseOf("Out"));
  EXPECT_FALSE(ClauseOf("Folder"));
  EXPECT_FALSE(VirtualFolderNamesOf("Folder"));
}

TEST_F(TestLibraryQueryReferences, APlaylistNamingOneWhoseRulesItCannotTakeIsRefused)
{
  WritePlaylist("Albums", Rule("genre", "Rock"), "albums");
  WritePlaylist("In", Rule("playlist", "Albums"));

  EXPECT_FALSE(ClauseOf("In"));
}

TEST_F(TestLibraryQueryReferences, APlaylistNamedOnTwoBranchesGivesItsRulesOnBoth)
{
  WritePlaylist("Rock", Rule("genre", "Rock"));
  WritePlaylist("B", Rule("playlist", "Rock"));
  WritePlaylist("C", Rule("playlist", "Rock"));
  WritePlaylist("A", Rule("playlist", "B") + Rule("playlist", "C"));

  const std::string rock{WhereOf("Rock")};
  ASSERT_NE(std::string::npos, rock.find("genre.strGenre LIKE 'Rock'")) << rock;
  EXPECT_EQ("((" + rock + ")) AND ((" + rock + "))", WhereOf("A"));
}

TEST_F(TestLibraryQueryReferences, APlaylistWithoutRulesHoldsEverything)
{
  WritePlaylist("Everything", "");
  WritePlaylist("In", Rule("playlist", "Everything"));
  WritePlaylist("Out", Rule("playlist", "Everything", "isnot"));

  EXPECT_EQ("(1)", WhereOf("In"));
  EXPECT_EQ("(NOT (1))", WhereOf("Out"));
}

TEST_F(TestLibraryQueryReferences, AMixedPlaylistTakesTheRulesOfOneInTheMixedFolder)
{
  WritePlaylist("Rock", Rule("genre", "Rock"));
  WritePlaylist("Base", Rule("genre", "Rock"), "mixed", "mixed");
  WritePlaylist("Mixed", Rule("playlist", "Base"), "mixed", "mixed");

  const std::optional<std::string> mixed{ClauseOf("Mixed", "mixed")};
  ASSERT_TRUE(mixed);
  EXPECT_EQ("(" + WhereOf("Rock") + ")", *mixed);
}

TEST_F(TestLibraryQueryReferences, AMixedPlaylistTakesASongsPlaylistsRulesForItsSongsOnly)
{
  WritePlaylist("Rock", Rule("genre", "Rock"));
  WritePlaylist("In", Rule("playlist", "Rock"), "mixed", "mixed");
  WritePlaylist("Out", Rule("playlist", "Rock", "isnot"), "mixed", "mixed");

  EXPECT_EQ("(" + WhereOf("Rock") + ")", ClauseOf("In", "mixed"));

  // No music video is in a playlist of songs
  MEDIA::CLibraryQuery in;
  ASSERT_TRUE(in.Load(PathOf("mixed", "In")));
  EXPECT_EQ("(0)", in.GetWhereClause(m_db, MEDIA::TYPE::MUSIC_VIDEO, false));
  MEDIA::CLibraryQuery out;
  ASSERT_TRUE(out.Load(PathOf("mixed", "Out")));
  EXPECT_EQ("(1)", out.GetWhereClause(m_db, MEDIA::TYPE::MUSIC_VIDEO, false));
}

TEST_F(TestLibraryQueryReferences,
       AMixedPlaylistTakesAMusicVideoPlaylistsRulesForItsMusicVideosOnly)
{
  WritePlaylist("Clips", Rule("genre", "Rock"), "musicvideos", "video");
  WritePlaylist("In", Rule("playlist", "Clips"), "mixed", "mixed");
  WritePlaylist("Out", Rule("playlist", "Clips", "isnot"), "mixed", "mixed");

  MEDIA::CLibraryQuery clips;
  ASSERT_TRUE(clips.Load(PathOf("video", "Clips")));
  const std::optional<std::string> rock{
      clips.GetWhereClause(m_db, MEDIA::TYPE::MUSIC_VIDEO, false)};
  ASSERT_TRUE(rock);
  ASSERT_NE(std::string::npos, rock->find("Rock")) << *rock;
  MEDIA::CLibraryQuery in;
  ASSERT_TRUE(in.Load(PathOf("mixed", "In")));
  EXPECT_EQ("(" + *rock + ")", in.GetWhereClause(m_db, MEDIA::TYPE::MUSIC_VIDEO, false));
  MEDIA::CLibraryQuery out;
  ASSERT_TRUE(out.Load(PathOf("mixed", "Out")));
  EXPECT_EQ("(NOT (" + *rock + "))", out.GetWhereClause(m_db, MEDIA::TYPE::MUSIC_VIDEO, false));

  // No song is in a playlist of music videos
  EXPECT_EQ("(0)", ClauseOf("In", "mixed"));
  EXPECT_EQ("(1)", ClauseOf("Out", "mixed"));
}

TEST_F(TestLibraryQueryReferences, AVirtualFolderRuleAddsNothingToTheClause)
{
  WritePlaylist("Rock", Rule("genre", "Rock"));
  WritePlaylist("Other", Rule("genre", "Pop"));
  WritePlaylist("Both", Rule("genre", "Rock") + Rule("virtualfolder", "Other"));
  WritePlaylist("Folder", Rule("virtualfolder", "Other"));

  EXPECT_EQ(WhereOf("Rock"), WhereOf("Both"));
  EXPECT_EQ("", WhereOf("Folder"));
}

TEST_F(TestLibraryQueryReferences, TheVirtualFoldersOfAPlaylistNamedTwiceAreListedOnce)
{
  WritePlaylist("Folder", Rule("genre", "Rock"));
  WritePlaylist("Shared", Rule("genre", "Rock") + Rule("virtualfolder", "Folder"));
  WritePlaylist("B", Rule("playlist", "Shared"));
  WritePlaylist("C", Rule("playlist", "Shared"));
  WritePlaylist("A", Rule("playlist", "B") + Rule("playlist", "C"));

  const std::optional<std::vector<std::string>> folders{VirtualFolderNamesOf("A")};
  ASSERT_TRUE(folders);
  EXPECT_EQ(std::vector<std::string>{"Folder.xsp"}, *folders);
}

TEST_F(TestLibraryQueryReferences, AMixedPlaylistFindsAMusicVideoPlaylistAsAVirtualFolder)
{
  WritePlaylist("V", Rule("genre", "Rock"), "musicvideos", "video");
  WritePlaylist("M", Rule("virtualfolder", "V"), "mixed", "mixed");

  const std::optional<std::vector<std::string>> folders{VirtualFolderNamesOf("M", "mixed")};
  ASSERT_TRUE(folders);
  EXPECT_EQ(std::vector<std::string>{"V.xsp"}, *folders);
}

TEST_F(TestLibraryQueryReferences, APlaylistRuleGivesTheClauseOfThePlaylistItNames)
{
  WritePlaylist("Rock", Rule("genre", "Rock"));
  WritePlaylist("In", Rule("playlist", "Rock"));
  WritePlaylist("Out", Rule("playlist", "Rock", "isnot"));

  const std::string rock{WhereOf("Rock")};
  ASSERT_NE(std::string::npos, rock.find("genre.strGenre LIKE 'Rock'")) << rock;
  EXPECT_EQ("(" + rock + ")", WhereOf("In"));
  EXPECT_EQ("(NOT (" + rock + "))", WhereOf("Out"));
}

TEST_F(TestLibraryQueryReferences, ASongsPlaylistTakesAMixedPlaylistsRulesAsSongRules)
{
  WritePlaylist("Rock", Rule("genre", "Rock"));
  WritePlaylist("Mixed", Rule("genre", "Rock"), "mixed");
  WritePlaylist("In", Rule("playlist", "Mixed"));

  EXPECT_EQ("(" + WhereOf("Rock") + ")", WhereOf("In"));
}

TEST_F(TestLibraryQueryReferences, AMixedPlaylistNamedByASongsPlaylistFindsWhatItNames)
{
  WritePlaylist("Clips", Rule("genre", "Rock"), "musicvideos", "video");
  WritePlaylist("Party", Rule("playlist", "Clips", "isnot"), "mixed", "mixed");
  WritePlaylist("In", Rule("playlist", "Party"));

  const std::optional<std::string> party{ClauseOf("Party", "mixed")};
  ASSERT_TRUE(party);
  EXPECT_EQ("(" + *party + ")", ClauseOf("In"));
}

TEST(TestLibraryQuery, ThePerformersAreSavedAsTheTypeCallsThem)
{
  MEDIA::CLibraryQuery clips;
  ASSERT_TRUE(clips.LoadFromJson(R"({"type":"musicvideos","group":{"type":"actors"}})"));
  EXPECT_EQ(MEDIA::LIBRARY_QUERY::GROUP::PERFORMER, clips.GetGroup());
  std::string json;
  ASSERT_TRUE(clips.SaveAsJson(json));
  EXPECT_EQ(R"({"group":{"mixed":false,"type":"artists"},"type":"musicvideos"})", json);

  MEDIA::CLibraryQuery movies;
  ASSERT_TRUE(movies.LoadFromJson(R"({"type":"movies","group":{"type":"artists"}})"));
  EXPECT_EQ(MEDIA::LIBRARY_QUERY::GROUP::PERFORMER, movies.GetGroup());
  ASSERT_TRUE(movies.SaveAsJson(json));
  EXPECT_EQ(R"({"group":{"mixed":false,"type":"actors"},"type":"movies"})", json);
}

TEST(TestLibraryQuery, JsonGroupedByWhatIsNoGroupDoesNotLoad)
{
  MEDIA::CLibraryQuery playlist;
  EXPECT_FALSE(playlist.LoadFromJson(R"({"type":"movies","group":{"type":"sources"}})"));
}

TEST_F(TestLibraryQueryFile, AnXspFileGroupedByWhatIsNoGroupDoesNotLoad)
{
  Write("in.xsp", R"(<smartplaylist type="movies"><group>sources</group></smartplaylist>)");

  MEDIA::CLibraryQuery playlist;
  EXPECT_FALSE(playlist.Load(PathOf("in.xsp")));
}

TEST(TestLibraryQuery, AQueryPutsOnlyTheLimitAndOrderItStatesIntoASorting)
{
  SortDescription callers;
  callers.sortBy = SortBy::TITLE;
  callers.sortOrder = SortOrder::DESCENDING;
  callers.sortAttributes = SortAttributeIgnoreFolders;
  callers.limitEnd = 7;

  MEDIA::CLibraryQuery silent;
  ASSERT_TRUE(silent.LoadFromJson(R"({"type":"songs"})"));
  SortDescription sorting{callers};
  silent.ApplyOrder(sorting, true);
  EXPECT_EQ(SortBy::TITLE, sorting.sortBy);
  EXPECT_EQ(SortOrder::DESCENDING, sorting.sortOrder);
  EXPECT_EQ(7, sorting.limitEnd);
  EXPECT_EQ(SortAttributeIgnoreFolders | SortAttributeIgnoreArticle,
            static_cast<int>(sorting.sortAttributes));

  MEDIA::CLibraryQuery stating;
  ASSERT_TRUE(stating.LoadFromJson(
      R"({"type":"songs","limit":3,"order":{"method":"artist","direction":"ascending"}})"));
  sorting = callers;
  stating.ApplyOrder(sorting, false);
  EXPECT_EQ(SortBy::ARTIST, sorting.sortBy);
  EXPECT_EQ(SortOrder::ASCENDING, sorting.sortOrder);
  EXPECT_EQ(3, sorting.limitEnd);
  EXPECT_EQ(static_cast<int>(SortAttributeIgnoreFolders), static_cast<int>(sorting.sortAttributes));
}

namespace
{
struct Listing
{
  //! The query as JSON without its braces
  std::string_view query;
  MEDIA::MediaSection section;
  std::string_view content;
  bool asPlaylist;
  bool asFilter;
};

void PrintTo(const Listing& listing, std::ostream* os)
{
  *os << listing.query << " on a " << NameOf(listing.section) << " list of " << listing.content;
}

class TestLibraryQueryAppliesTo : public ::testing::TestWithParam<Listing>
{
};

constexpr auto MUSIC_LIBRARY = MEDIA::MediaSection::MUSIC;
constexpr auto VIDEO_LIBRARY = MEDIA::MediaSection::VIDEO;

// clang-format off
const Listing LISTINGS[] = {
    {R"("type":"songs")", MUSIC_LIBRARY, "songs", true, true},
    {R"("type":"songs")", MUSIC_LIBRARY, "albums", false, false},
    {R"("type":"songs")", VIDEO_LIBRARY, "songs", false, false},
    {R"("type":"albums","group":{"type":"years"})", MUSIC_LIBRARY, "years", true, false},
    {R"("type":"albums","group":{"type":"originalyears"})", MUSIC_LIBRARY, "years", true, false},
    {R"("type":"albums","group":{"type":"years"})", MUSIC_LIBRARY, "albums", true, true},
    {R"("type":"albums","group":{"type":"years"})", MUSIC_LIBRARY, "genres", false, false},
    {R"("type":"albums","group":{"type":"artists"})", MUSIC_LIBRARY, "artists", true, false},
    {R"("type":"artists","group":{"type":"genres"})", MUSIC_LIBRARY, "genres", true, false},
    {R"("type":"movies")", VIDEO_LIBRARY, "movies", true, true},
    {R"("type":"movies")", VIDEO_LIBRARY, "sets", false, false},
    {R"("type":"movies","group":{"type":"sets"})", VIDEO_LIBRARY, "sets", true, false},
    {R"("type":"movies","group":{"type":"sets","mixed":true})", VIDEO_LIBRARY, "sets", false, false},
    {R"("type":"movies","group":{"type":"sets","mixed":true})", VIDEO_LIBRARY, "movies", true, true},
    {R"("type":"movies","group":{"type":"actors"})", VIDEO_LIBRARY, "actors", true, false},
    {R"("type":"movies","group":{"type":"videoversions"})", VIDEO_LIBRARY, "videoversions", true, false},
    {R"("type":"movies","group":{"type":"none"})", VIDEO_LIBRARY, "movies", true, true},
    {R"("type":"movies","group":{"type":"none"})", VIDEO_LIBRARY, "none", false, false},
    {R"("type":"musicvideos","group":{"type":"artists"})", VIDEO_LIBRARY, "actors", true, false},
    {R"("type":"musicvideos","group":{"type":"albums"})", VIDEO_LIBRARY, "albums", true, false},
    {R"("type":"tvshows")", VIDEO_LIBRARY, "tvshows", true, true},
    {R"("type":"tvshows")", VIDEO_LIBRARY, "episodes", false, false},
    {R"("type":"episodes")", VIDEO_LIBRARY, "episodes", true, true},
    {R"("type":"episodes")", VIDEO_LIBRARY, "tvshows", true, false},
    {R"("type":"mixed")", MUSIC_LIBRARY, "songs", true, true},
    {R"("type":"mixed")", VIDEO_LIBRARY, "musicvideos", true, true},
};
// clang-format on
} // namespace

TEST_P(TestLibraryQueryAppliesTo, AQueryAppliesToTheListingsOfItsItemsAndItsGroups)
{
  const Listing& listing{GetParam()};
  MEDIA::CLibraryQuery query;
  ASSERT_TRUE(query.LoadFromJson("{" + std::string{listing.query} + "}"));

  EXPECT_EQ(listing.asPlaylist, query.AppliesTo(listing.section, listing.content, false));
  EXPECT_EQ(listing.asFilter, query.AppliesTo(listing.section, listing.content, true));
}

INSTANTIATE_TEST_SUITE_P(Listings, TestLibraryQueryAppliesTo, ::testing::ValuesIn(LISTINGS));

TEST(TestLibraryQuery, WhatSongsAndAlbumsOfferDoesNotFollowTheOriginalDateSetting)
{
  using MEDIA::LIBRARY_QUERY::GROUP;
  using MEDIA::LIBRARY_QUERY::TYPE;
  for (const bool useOriginalDate : {false, true})
  {
    const CScopedSetting setting{CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE, useOriginalDate};
    for (const TYPE type : {TYPE::SONGS, TYPE::ALBUMS})
    {
      std::vector<std::string> fields;
      MEDIA::CLibraryQuery::GetAvailableFields(type, fields);
      EXPECT_NE(fields.end(), std::ranges::find(fields, std::string{"year"})) << useOriginalDate;
      EXPECT_NE(fields.end(), std::ranges::find(fields, std::string{"originalyear"}))
          << useOriginalDate;

      const std::vector<SortBy> orders{MEDIA::CLibraryQueryRule::GetOrders(type)};
      EXPECT_NE(orders.end(), std::ranges::find(orders, SortBy::YEAR)) << useOriginalDate;
      EXPECT_NE(orders.end(), std::ranges::find(orders, SortBy::ORIG_DATE)) << useOriginalDate;
    }
    const std::vector<GROUP> groups{MEDIA::CLibraryQueryRule::GetGroups(TYPE::ALBUMS)};
    EXPECT_NE(groups.end(), std::ranges::find(groups, GROUP::YEAR)) << useOriginalDate;
    EXPECT_NE(groups.end(), std::ranges::find(groups, GROUP::ORIGINAL_YEAR)) << useOriginalDate;
  }
}

TEST(TestLibraryQuery, AMixedPlaylistOffersTheFieldsSongsAndMusicVideosBothHave)
{
  std::vector<std::string> fields;
  MEDIA::CLibraryQuery::GetAvailableFields(MEDIA::LIBRARY_QUERY::TYPE::MIXED, fields);
  EXPECT_EQ(
      (std::vector<std::string>{"genre", "album", "artist", "albumartist", "title", "year", "time",
                                "tracknumber", "filename", "path", "playcount", "lastplayed",
                                "rating", "userrating", "dateadded", "playlist", "virtualfolder"}),
      fields);

  for (const Field field : MEDIA::CLibraryQueryRule::GetFields(MEDIA::LIBRARY_QUERY::TYPE::MIXED))
  {
    EXPECT_TRUE(MEDIA::CLibraryQueryRule::HasField(MEDIA::LIBRARY_QUERY::TYPE::SONGS, field));
  }
}

TEST_F(TestLibraryQueryFile, AnXspFileSkipsCommentsAndRulesWithoutAField)
{
  Write("in.xsp", R"(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<smartplaylist type="movies">
  <name>Test &amp; Playlist</name>
  <match>all</match>
  <rule field="genre" operator="is">
    <value>Action</value>
    <!-- comment -->
    <value>Comedy</value>
  </rule>
  <rule field="title" operator="contains">Star</rule>
  <rule field="rating" operator="greaterthan"><!-- comment --><value>5</value></rule>
  <rule operator="is">no field</rule>
</smartplaylist>)");

  MEDIA::CLibraryQuery playlist;
  ASSERT_TRUE(playlist.Load(PathOf("in.xsp")));
  EXPECT_EQ("Test & Playlist", playlist.GetName());
  std::string json;
  ASSERT_TRUE(playlist.SaveAsJson(json));
  EXPECT_EQ(R"({"rules":{"and":[{"field":"genre","operator":"is","value":["Action","Comedy"]},)"
            R"({"field":"title","operator":"contains","value":["Star"]}]},"type":"movies"})",
            json);
}

TEST_F(TestLibraryQueryFile, AFileThatIsNotAnXspDoesNotLoad)
{
  Write("empty.xsp", "");
  Write("other.xsp", R"(<playlist type="movies"/>)");
  Write("broken.xsp", "<smartplaylist");

  for (const char* name : {"empty.xsp", "other.xsp", "broken.xsp"})
  {
    MEDIA::CLibraryQuery playlist;
    EXPECT_FALSE(playlist.Load(PathOf(name))) << name;
  }
}

TEST_F(TestLibraryQueryFile, AnUnescapedAmpersandIsKept)
{
  Write("in.xsp", R"(<smartplaylist type="songs"><name>Rock & Roll</name>)"
                  R"(<rule field="genre" operator="is">R&B</rule></smartplaylist>)");

  MEDIA::CLibraryQuery playlist;
  ASSERT_TRUE(playlist.Load(PathOf("in.xsp")));
  EXPECT_EQ("Rock & Roll", playlist.GetName());
  std::string json;
  ASSERT_TRUE(playlist.SaveAsJson(json));
  EXPECT_EQ(
      R"({"rules":{"and":[{"field":"genre","operator":"is","value":["R&B"]}]},"type":"songs"})",
      json);
}

TEST_F(TestLibraryQueryFile, ALatin1FileIsReadAsUtf8)
{
  Write("in.xsp", "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>"
                  "<smartplaylist type=\"songs\"><name>Caf\xE9</name>"
                  "<rule field=\"genre\" operator=\"is\">Caf\xE9</rule></smartplaylist>");

  MEDIA::CLibraryQuery playlist;
  ASSERT_TRUE(playlist.Load(PathOf("in.xsp")));
  EXPECT_EQ("Caf\xC3\xA9", playlist.GetName());
  std::string json;
  ASSERT_TRUE(playlist.SaveAsJson(json));
  EXPECT_EQ("{\"rules\":{\"and\":[{\"field\":\"genre\",\"operator\":\"is\","
            "\"value\":[\"Caf\xC3\xA9\"]}]},\"type\":\"songs\"}",
            json);
}

TEST_F(TestLibraryQueryFile, OpeningAnXspReadsItsNameAndType)
{
  Write("in.xsp", FULL_XSP);

  MEDIA::CLibraryQuery playlist;
  ASSERT_TRUE(playlist.OpenAndReadName(CURL(PathOf("in.xsp"))));
  EXPECT_EQ("Everything", playlist.GetName());
  EXPECT_EQ(MEDIA::LIBRARY_QUERY::TYPE::MOVIES, playlist.GetType());
}

TEST(TestLibraryQuery, ALibraryNodeLoadsAsAQueryOfItsType)
{
  CXBMCTinyXML2 doc;
  ASSERT_TRUE(doc.Parse(std::string{R"(<node order="1" type="filter">
    <label>Drama</label>
    <content>movies</content>
    <match>one</match>
    <rule field="genre" operator="is"><value>Drama</value></rule>
    <rule field="playcount" operator="is">0</rule>
    <limit>10</limit>
  </node>)"}));

  MEDIA::CLibraryQuery playlist;
  playlist.SetType(MEDIA::LIBRARY_QUERY::TYPE::MOVIES);
  ASSERT_TRUE(playlist.LoadFromXML(doc.RootElement()));
  std::string json;
  ASSERT_TRUE(playlist.SaveAsJson(json));
  EXPECT_EQ(R"({"limit":10,"rules":{"or":[{"field":"genre","operator":"is","value":["Drama"]},)"
            R"({"field":"playcount","operator":"is","value":["0"]}]},"type":"movies"})",
            json);
}
