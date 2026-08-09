/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "URL.h"
#include "Util.h"
#include "filesystem/Directory.h"
#include "platform/Filesystem.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoUtils.h"

#include <array>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
namespace fs = KODI::PLATFORM::FILESYSTEM;

namespace
{

using OptDef = std::pair<std::string, bool>;

class OpticalMediaPathTest : public testing::WithParamInterface<OptDef>, public testing::Test
{
};

using TrailerDef = std::pair<std::string, std::string>;

class TrailerTest : public testing::WithParamInterface<TrailerDef>, public testing::Test
{
};

} // namespace

TEST_P(OpticalMediaPathTest, GetOpticalMediaPath)
{
  std::error_code ec;
  const std::string temp_path = fs::create_temp_directory(ec);
  EXPECT_FALSE(ec);
  const std::string file_path = URIUtils::AddFileToFolder(temp_path, GetParam().first);
  EXPECT_TRUE(CUtil::CreateDirectoryEx(URIUtils::GetDirectory(file_path)));
  {
    std::ofstream of(file_path);
  }
  CFileItem item(temp_path, true);
  if (GetParam().second)
    EXPECT_EQ(VIDEO::UTILS::GetOpticalMediaPath(item), file_path);
  else
    EXPECT_EQ(VIDEO::UTILS::GetOpticalMediaPath(item), "");

  XFILE::CDirectory::RemoveRecursive(temp_path);
}

const auto mediapath_tests = std::array{
    OptDef{"VIDEO_TS.IFO", true},    OptDef{"VIDEO_TS/VIDEO_TS.IFO", true},
    OptDef{"some.file", false},
#ifdef HAVE_LIBBLURAY
    OptDef{"index.bdmv", true},      OptDef{"INDEX.BDM", true},
    OptDef{"BDMV/index.bdmv", true}, OptDef{"BDMV/INDEX.BDM", true},
#endif
};

INSTANTIATE_TEST_SUITE_P(TestVideoUtils, OpticalMediaPathTest, testing::ValuesIn(mediapath_tests));

TEST_P(TrailerTest, FindTrailer)
{
  std::string temp_path;
  if (!GetParam().second.empty())
  {
    std::error_code ec;
    temp_path = fs::create_temp_directory(ec);
    EXPECT_FALSE(ec);
    XFILE::CDirectory::Create(temp_path);
    const std::string file_path = URIUtils::AddFileToFolder(temp_path, GetParam().second);
    {
      std::ofstream of(file_path);
    }
    URIUtils::AddSlashAtEnd(temp_path);
  }

  std::string input_path = GetParam().first;
  if (!temp_path.empty())
  {
    StringUtils::Replace(input_path, "#DIRECTORY#", temp_path);
    StringUtils::Replace(input_path, "#URLENCODED_DIRECTORY#", CURL::Encode(temp_path));
  }

  CFileItem item(input_path, false);
  EXPECT_EQ(VIDEO::UTILS::FindTrailer(item),
            GetParam().second.empty() ? ""
                                      : URIUtils::AddFileToFolder(temp_path, GetParam().second));

  if (!temp_path.empty())
    XFILE::CDirectory::RemoveRecursive(temp_path);
}

const auto trailer_tests = std::array{
    TrailerDef{"https://some.where/foo", ""},
    TrailerDef{"upnp://1/2/3", ""},
    TrailerDef{"bluray://1", ""},
    TrailerDef{"pvr://foobar.pvr", ""},
    TrailerDef{"plugin://plugin.video.foo/foo?param=1", ""},
    TrailerDef{"dvd://1", ""},
    TrailerDef{"stack://#DIRECTORY#foo-cd1.avi , #DIRECTORY#foo-cd2.avi", "foo-trailer.mkv"},
    TrailerDef{"stack://#DIRECTORY#foo-cd1.avi , #DIRECTORY#foo-cd2.avi", "foo-cd1-trailer.avi"},
    TrailerDef{"stack://#DIRECTORY#foo-cd1.avi , #DIRECTORY#foo-cd2.avi", "movie-trailer.mp4"},
    TrailerDef{"zip://#URLENCODED_DIRECTORY#bar.zip/bar.avi", "bar-trailer.mov"},
    TrailerDef{"zip://#URLENCODED_DIRECTORY#bar.zip/bar.mkv", "movie-trailer.ogm"},
    TrailerDef{"#DIRECTORY#bar.mkv", "bar-trailer.mkv"},
    TrailerDef{"#DIRECTORY#bar.mkv", "movie-trailer.avi"},
};

INSTANTIATE_TEST_SUITE_P(TestVideoUtils, TrailerTest, testing::ValuesIn(trailer_tests));

namespace
{

using NormaliseDef = std::pair<std::string, std::string>;

class NormaliseEditionNameTest : public testing::WithParamInterface<NormaliseDef>,
                                 public testing::Test
{
};

using EditionDef = std::pair<std::string, std::string>;

class FindEditionInNameTest : public testing::WithParamInterface<EditionDef>, public testing::Test
{
};

// A stand in for the editions held in the videoversiontype table, including a deliberately short
// one to check that matching does not run into the middle of a word
const std::vector<std::string> editions{"Extended Edition",
                                        "Director's Cut",
                                        "Theatrical Cut",
                                        "The Final Cut",
                                        "Uncut Version",
                                        "Collector's Edition",
                                        "Ultimate Collector's Edition",
                                        "SE"};

} // namespace

TEST_P(NormaliseEditionNameTest, NormaliseEditionName)
{
  EXPECT_EQ(VIDEO::UTILS::NormaliseEditionName(GetParam().first), GetParam().second);
}

const auto normalise_tests = std::array{
    // Wrapped in spaces so that a substring search only matches whole words
    NormaliseDef{"Extended Edition", " extended edition "},
    NormaliseDef{"", " "},
    // Apostrophes are dropped, so the same name spelt without one still matches
    NormaliseDef{"Director's Cut", " directors cut "},
    NormaliseDef{"Directors Cut", " directors cut "},
    NormaliseDef{"Director\xE2\x80\x99s Cut", " directors cut "}, // Right single quotation mark
    // Every other punctuation character separates words, however many there are
    NormaliseDef{"Directors.Cut", " directors cut "},
    NormaliseDef{"DIRECTORS_CUT", " directors cut "},
    NormaliseDef{"Directors - Cut", " directors cut "},
    NormaliseDef{"[Directors.Cut]", " directors cut "},
    // Non-ascii characters are kept, so a localised name still matches itself
    NormaliseDef{"\xC3\x89"
                 "dition Sp\xC3\xA9"
                 "ciale",
                 " \xC3\x89"
                 "dition sp\xC3\xA9"
                 "ciale "},
};

INSTANTIATE_TEST_SUITE_P(TestVideoUtils,
                         NormaliseEditionNameTest,
                         testing::ValuesIn(normalise_tests));

TEST_P(FindEditionInNameTest, FindEditionInName)
{
  EXPECT_EQ(VIDEO::UTILS::FindEditionInName(GetParam().first, editions), GetParam().second);
}

const auto edition_tests = std::array{
    // The folder names of a disc and of a folder stack's parts once reduced to the one they share
    EditionDef{"LORD_OF_THE_RINGS_THE_FELLOWSHIP_OF_THE_RING_EXTENDED_EDITION", "Extended Edition"},
    EditionDef{"Blade Runner (1982) Directors.Cut", "Director's Cut"},
    EditionDef{"Blade Runner The Final Cut", "The Final Cut"},
    // Nothing to find
    EditionDef{"LORD_OF_THE_RINGS_THE_FELLOWSHIP_OF_THE_RING", ""},
    EditionDef{"The Matrix (1999)", ""},
    EditionDef{"", ""},
    // A name that starts with an edition is a movie titled after one, not a version
    EditionDef{"The Final Cut (2004)", ""},
    EditionDef{"Extended Edition", ""},
    // The longest match wins over the shorter ones it contains
    EditionDef{"Some Movie Ultimate Collector's Edition", "Ultimate Collector's Edition"},
    EditionDef{"Some Movie Collector's Edition", "Collector's Edition"},
    // Only whole words match, so a short edition is not found inside a longer word
    EditionDef{"These Venues", ""},
    EditionDef{"Some Movie SE", "SE"},
    // Matching ignores case
    EditionDef{"some movie theatrical cut", "Theatrical Cut"},
    EditionDef{"SOME MOVIE UNCUT VERSION", "Uncut Version"},
};

INSTANTIATE_TEST_SUITE_P(TestVideoUtils, FindEditionInNameTest, testing::ValuesIn(edition_tests));
