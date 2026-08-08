/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "interfaces/json-rpc/VideoLibrarySetSourceContent.h"
#include "utils/Variant.h"

#include <climits>
#include <string>

#include <gtest/gtest.h>

using namespace JSONRPC;

namespace
{
CVariant MakeParams(const std::string& path,
                    const std::string& content,
                    const std::string& scraperId = "")
{
  CVariant params(CVariant::VariantTypeObject);
  params["path"] = path;
  params["content"] = content;
  if (!scraperId.empty())
  {
    params["scraperid"] = scraperId;
  }

  return params;
}

CVariant MovieParams()
{
  return MakeParams("smb://nas/Movies/", "movies", "metadata.themoviedb.org.python");
}

CVariant TvShowParams()
{
  return MakeParams("smb://nas/TV/", "tvshows", "metadata.tvshows.themoviedb.org.python");
}
} // unnamed namespace

TEST(TestSetSourceContentParams, RejectsMissingPath)
{
  CVariant params(CVariant::VariantTypeObject);
  params["content"] = "movies";
  params["scraperid"] = "metadata.themoviedb.org.python";

  ParsedSetSourceContent parsed;
  EXPECT_EQ(InvalidParams, ParseSetSourceContentParams(params, parsed));
}

TEST(TestSetSourceContentParams, RejectsEmptyPath)
{
  ParsedSetSourceContent parsed;
  EXPECT_EQ(InvalidParams, ParseSetSourceContentParams(
                               MakeParams("", "movies", "metadata.themoviedb.org.python"), parsed));
}

TEST(TestSetSourceContentParams, RejectsUnknownContent)
{
  ParsedSetSourceContent parsed;
  EXPECT_EQ(InvalidParams, ParseSetSourceContentParams(
                               MakeParams("smb://nas/Media/", "audiobooks", "metadata.x"), parsed));
}

TEST(TestSetSourceContentParams, RejectsMusicContent)
{
  // "albums" and "artists" are content types, but not ones a video source can be bound to.
  ParsedSetSourceContent parsed;
  EXPECT_EQ(InvalidParams, ParseSetSourceContentParams(
                               MakeParams("smb://nas/Music/", "albums", "metadata.x"), parsed));
}

TEST(TestSetSourceContentParams, RejectsMissingScraperUnlessContentIsNone)
{
  ParsedSetSourceContent parsed;
  EXPECT_EQ(InvalidParams,
            ParseSetSourceContentParams(MakeParams("smb://nas/Movies/", "movies"), parsed));
}

TEST(TestSetSourceContentParams, RejectsEmptyScraperUnlessContentIsNone)
{
  CVariant params = MakeParams("smb://nas/Movies/", "movies");
  params["scraperid"] = "";

  ParsedSetSourceContent parsed;
  EXPECT_EQ(InvalidParams, ParseSetSourceContentParams(params, parsed));
}

TEST(TestSetSourceContentParams, RejectsUnknownClearMode)
{
  CVariant params = MakeParams("smb://nas/Movies/", "none");
  params["clearmode"] = "purge";

  ParsedSetSourceContent parsed;
  EXPECT_EQ(InvalidParams, ParseSetSourceContentParams(params, parsed));
}

TEST(TestSetSourceContentParams, PassesThroughScraperAndFlags)
{
  CVariant params = MovieParams();
  params["scrapersettings"] = "<settings><setting id=\"language\">en</setting></settings>";
  params["noupdate"] = true;
  params["refresh"] = true;

  ParsedSetSourceContent parsed;
  ASSERT_EQ(OK, ParseSetSourceContentParams(params, parsed));
  EXPECT_EQ("smb://nas/Movies/", parsed.path);
  EXPECT_EQ(ADDON::ContentType::MOVIES, parsed.content);
  EXPECT_EQ("metadata.themoviedb.org.python", parsed.scraperId);
  EXPECT_EQ("<settings><setting id=\"language\">en</setting></settings>", parsed.scraperSettings);
  EXPECT_TRUE(parsed.settings.noupdate);
  EXPECT_TRUE(parsed.refresh);
  EXPECT_FALSE(parsed.settings.exclude);
}

TEST(TestSetSourceContentParams, LeavesAllExternalAudioToTheCaller)
{
  // It has no parameter; the handler carries the path's stored value over.
  ParsedSetSourceContent parsed;
  ASSERT_EQ(OK, ParseSetSourceContentParams(MovieParams(), parsed));
  EXPECT_FALSE(parsed.settings.m_allExtAudio);
}

// Movies and music videos: the mapping CGUIDialogContentSettings::Show() applies.

TEST(TestSetSourceContentParams, MoviesDefaultToRecursiveWithoutDirectoryNames)
{
  ParsedSetSourceContent parsed;
  ASSERT_EQ(OK, ParseSetSourceContentParams(MovieParams(), parsed));
  EXPECT_FALSE(parsed.settings.parent_name);
  EXPECT_FALSE(parsed.settings.parent_name_root);
  EXPECT_EQ(INT_MAX, parsed.settings.recurse);
}

TEST(TestSetSourceContentParams, MoviesNonRecursiveWithoutDirectoryNamesDoNotRecurse)
{
  CVariant params = MovieParams();
  params["scanrecursive"] = false;

  ParsedSetSourceContent parsed;
  ASSERT_EQ(OK, ParseSetSourceContentParams(params, parsed));
  EXPECT_FALSE(parsed.settings.parent_name);
  EXPECT_EQ(0, parsed.settings.recurse);
}

TEST(TestSetSourceContentParams, MoviesWithDirectoryNamesRecurseWithoutLimit)
{
  CVariant params = MovieParams();
  params["usedirectorynames"] = true;

  ParsedSetSourceContent parsed;
  ASSERT_EQ(OK, ParseSetSourceContentParams(params, parsed));
  EXPECT_TRUE(parsed.settings.parent_name);
  EXPECT_FALSE(parsed.settings.parent_name_root);
  EXPECT_EQ(INT_MAX, parsed.settings.recurse);
}

TEST(TestSetSourceContentParams, MoviesWithDirectoryNamesStopOneLevelDownWhenNotRecursive)
{
  // One level, not none: a folder per movie is still a level below the source.
  CVariant params = MovieParams();
  params["usedirectorynames"] = true;
  params["scanrecursive"] = false;

  ParsedSetSourceContent parsed;
  ASSERT_EQ(OK, ParseSetSourceContentParams(params, parsed));
  EXPECT_TRUE(parsed.settings.parent_name);
  EXPECT_FALSE(parsed.settings.parent_name_root);
  EXPECT_EQ(1, parsed.settings.recurse);
}

TEST(TestSetSourceContentParams, MoviesContainingASingleItemDoNotRecurse)
{
  CVariant params = MovieParams();
  params["usedirectorynames"] = true;
  params["containssingleitem"] = true;

  ParsedSetSourceContent parsed;
  ASSERT_EQ(OK, ParseSetSourceContentParams(params, parsed));
  EXPECT_TRUE(parsed.settings.parent_name);
  EXPECT_TRUE(parsed.settings.parent_name_root);
  EXPECT_EQ(0, parsed.settings.recurse);
}

TEST(TestSetSourceContentParams, MoviesIgnoreSingleItemWithoutDirectoryNames)
{
  CVariant params = MovieParams();
  params["containssingleitem"] = true;

  ParsedSetSourceContent parsed;
  ASSERT_EQ(OK, ParseSetSourceContentParams(params, parsed));
  EXPECT_FALSE(parsed.settings.parent_name);
  EXPECT_FALSE(parsed.settings.parent_name_root);
  EXPECT_EQ(INT_MAX, parsed.settings.recurse);
}

TEST(TestSetSourceContentParams, MusicVideosMapAsMoviesDo)
{
  CVariant params = MakeParams("smb://nas/MusicVideos/", "musicvideos", "metadata.local");
  params["usedirectorynames"] = true;
  params["scanrecursive"] = false;

  ParsedSetSourceContent parsed;
  ASSERT_EQ(OK, ParseSetSourceContentParams(params, parsed));
  EXPECT_EQ(ADDON::ContentType::MUSICVIDEOS, parsed.content);
  EXPECT_TRUE(parsed.settings.parent_name);
  EXPECT_EQ(1, parsed.settings.recurse);
}

// TV shows: a fixed mapping, driven only by whether the path holds one show.

TEST(TestSetSourceContentParams, TvShowsNeverRecurse)
{
  ParsedSetSourceContent parsed;
  ASSERT_EQ(OK, ParseSetSourceContentParams(TvShowParams(), parsed));
  EXPECT_FALSE(parsed.settings.parent_name);
  EXPECT_FALSE(parsed.settings.parent_name_root);
  EXPECT_EQ(0, parsed.settings.recurse);
}

TEST(TestSetSourceContentParams, TvShowsContainingASingleShowUseTheFolderName)
{
  CVariant params = TvShowParams();
  params["containssingleitem"] = true;

  ParsedSetSourceContent parsed;
  ASSERT_EQ(OK, ParseSetSourceContentParams(params, parsed));
  EXPECT_TRUE(parsed.settings.parent_name);
  EXPECT_TRUE(parsed.settings.parent_name_root);
  EXPECT_EQ(0, parsed.settings.recurse);
}

TEST(TestSetSourceContentParams, TvShowsIgnoreTheMovieOnlyFlags)
{
  CVariant params = TvShowParams();
  params["usedirectorynames"] = true;
  params["scanrecursive"] = true;

  ParsedSetSourceContent parsed;
  ASSERT_EQ(OK, ParseSetSourceContentParams(params, parsed));
  EXPECT_FALSE(parsed.settings.parent_name);
  EXPECT_EQ(0, parsed.settings.recurse);
}

TEST(TestSetSourceContentParams, ClearsWithoutExcludingByDefault)
{
  ParsedSetSourceContent parsed;
  ASSERT_EQ(OK, ParseSetSourceContentParams(MakeParams("smb://nas/Movies/", "none"), parsed));
  EXPECT_EQ(ADDON::ContentType::NONE, parsed.content);
  EXPECT_EQ(SourceContentClearMode::CLEAR, parsed.clearMode);
  EXPECT_FALSE(parsed.settings.exclude);
  EXPECT_TRUE(parsed.scraperId.empty());
}

TEST(TestSetSourceContentParams, ExcludeClearModeExcludesThePath)
{
  CVariant params = MakeParams("smb://nas/Movies/", "none");
  params["clearmode"] = "exclude";

  ParsedSetSourceContent parsed;
  ASSERT_EQ(OK, ParseSetSourceContentParams(params, parsed));
  EXPECT_EQ(SourceContentClearMode::EXCLUDE, parsed.clearMode);
  EXPECT_TRUE(parsed.settings.exclude);
}

TEST(TestSetSourceContentParams, RemoveClearModeDoesNotExcludeThePath)
{
  CVariant params = MakeParams("smb://nas/Movies/", "none");
  params["clearmode"] = "remove";

  ParsedSetSourceContent parsed;
  ASSERT_EQ(OK, ParseSetSourceContentParams(params, parsed));
  EXPECT_EQ(SourceContentClearMode::REMOVE, parsed.clearMode);
  EXPECT_FALSE(parsed.settings.exclude);
}

TEST(TestSetSourceContentParams, ClearingStillHonoursRefresh)
{
  CVariant params = MakeParams("smb://nas/Movies/", "none");
  params["refresh"] = true;

  ParsedSetSourceContent parsed;
  ASSERT_EQ(OK, ParseSetSourceContentParams(params, parsed));
  EXPECT_TRUE(parsed.refresh);
}
