/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DatabaseManager.h"
#include "FileItem.h"
#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "interfaces/AnnouncementManager.h"
#include "platform/Filesystem.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "test/TestUtils.h"
#include "utils/Artwork.h"
#include "utils/ItemProperties.h"
#include "utils/SaveFileStateJob.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/Bookmark.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoTag.h"
#include "video/VideoUtils.h"
#include "video/dialogs/GUIDialogVideoManagerExtras.h"

#include <array>
#include <fstream>
#include <memory>
#include <set>
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
  std::string temp_root;
  std::string temp_path;
  std::string trailer{GetParam().second};
  if (!trailer.empty())
  {
    std::error_code ec;
    temp_root = fs::create_temp_directory(ec);
    ASSERT_FALSE(ec);
    temp_path = URIUtils::AddFileToFolder(temp_root, "trailer_test");
    XFILE::CDirectory::Create(temp_path);
    // A disc folder rip's trailer is named after the folder holding the disc
    StringUtils::Replace(trailer, "#FOLDER#", URIUtils::GetFileName(temp_path));
    const std::string file_path = URIUtils::AddFileToFolder(temp_path, trailer);
    XFILE::CDirectory::Create(URIUtils::GetDirectory(file_path));
    {
      std::ofstream of(file_path);
    }
    URIUtils::AddSlashAtEnd(temp_path);
  }

  std::string input_path = GetParam().first;
  if (!temp_path.empty())
  {
    // A path in a folder is built as Kodi builds one, with the platform's separators
    if (constexpr std::string_view directory{"#DIRECTORY#"}; input_path.starts_with(directory))
      input_path = URIUtils::AddFileToFolder(temp_path, input_path.substr(directory.size()));
    StringUtils::Replace(input_path, "#DIRECTORY#", temp_path);
    StringUtils::Replace(input_path, "#URLENCODED_DIRECTORY#", CURL::Encode(temp_path));
  }

  CFileItem item(input_path, false);
  EXPECT_EQ(VIDEO::UTILS::FindTrailer(item, true),
            trailer.empty() ? "" : URIUtils::AddFileToFolder(temp_path, trailer));

  if (!temp_root.empty())
    XFILE::CDirectory::RemoveRecursive(temp_root);
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
    TrailerDef{"bluray://#URLENCODED_DIRECTORY#/BDMV/PLAYLIST/00800.mpls", "#FOLDER#-trailer.mov"},
    TrailerDef{"bluray://#URLENCODED_DIRECTORY#/BDMV/PLAYLIST/00800.mpls", "movie-trailer.mkv"},
    TrailerDef{"#DIRECTORY#VIDEO_TS/VIDEO_TS.IFO", "#FOLDER#-trailer.mkv"},
    // As up to v21, inside BDMV/VIDEO_TS when there is none beside it
    TrailerDef{"bluray://#URLENCODED_DIRECTORY#/BDMV/PLAYLIST/00800.mpls",
               "BDMV/index-trailer.mov"},
    TrailerDef{"bluray://#URLENCODED_DIRECTORY#/BDMV/PLAYLIST/00800.mpls",
               "BDMV/movie-trailer.mov"},
    TrailerDef{"#DIRECTORY#VIDEO_TS/VIDEO_TS.IFO", "VIDEO_TS/VIDEO_TS-trailer.mkv"},
};

INSTANTIATE_TEST_SUITE_P(TestVideoUtils, TrailerTest, testing::ValuesIn(trailer_tests));

TEST(TestVideoUtils, FindTrailerOfADiscFolderRipPrefersTheOneBesideBDMV)
{
  std::error_code ec;
  const std::string temp_path{fs::create_temp_directory(ec)};
  ASSERT_FALSE(ec);
  const std::string beside{
      URIUtils::AddFileToFolder(temp_path, URIUtils::GetFileName(temp_path) + "-trailer.mkv")};
  const std::string bdmv{URIUtils::AddFileToFolder(temp_path, "BDMV")};
  XFILE::CDirectory::Create(bdmv);
  for (const std::string& path : {beside, URIUtils::AddFileToFolder(bdmv, "index-trailer.mkv")})
  {
    std::ofstream of(path);
  }

  const CFileItem item(URIUtils::AddFileToFolder(bdmv, "index.bdmv"), false);
  EXPECT_EQ(beside, VIDEO::UTILS::FindTrailer(item, true));

  XFILE::CDirectory::RemoveRecursive(temp_path);
}

namespace
{
//! A disc's trailer, found in a folder of the test's own
class TestDiscTrailer : public testing::Test
{
protected:
  void SetUp() override
  {
    std::error_code ec;
    m_root = fs::create_temp_directory(ec);
    ASSERT_FALSE(ec);
  }

  void TearDown() override { XFILE::CDirectory::RemoveRecursive(m_root); }

  //! A file, and the folders holding it, in the test's folder
  std::string File(const std::string& name) const
  {
    const std::string path{URIUtils::AddFileToFolder(m_root, name)};
    EXPECT_TRUE(CUtil::CreateDirectoryEx(URIUtils::GetDirectory(path)));
    std::ofstream of(path);
    return path;
  }

  //! \param movieFolder whether the movie has a folder of its own
  static std::string Trailer(const std::string& video, bool movieFolder = true)
  {
    return VIDEO::UTILS::FindTrailer(CFileItem(video, false), movieFolder);
  }

  std::string m_root;
};
} // namespace

// A disc in a folder of its own within the movie's has the trailer beside it, named after the
// movie, as its subtitles are
TEST_F(TestDiscTrailer, ADiscFolderRipInADiscFolderHasTheMoviesTrailer)
{
  const std::string disc{File("M3GAN/disc 1/BDMV/index.bdmv")};
  const std::string trailer{File("M3GAN/disc 1/M3GAN-trailer.mkv")};
  EXPECT_EQ(trailer, Trailer(URIUtils::GetBlurayPlaylistPath(disc, 800)));
}

TEST_F(TestDiscTrailer, ADiscFolderRipsTrailerIsNotNamedAfterItsDiscFolder)
{
  const std::string disc{File("M3GAN/disc 1/BDMV/index.bdmv")};
  File("M3GAN/disc 1/disc 1-trailer.mkv");
  EXPECT_EQ("", Trailer(URIUtils::GetBlurayPlaylistPath(disc, 800)));
}

TEST_F(TestDiscTrailer, ADiscFolderRipsTrailerKeepsTheDotsOfTheMoviesName)
{
  const std::string disc{File("The.Matrix.1999/BDMV/index.bdmv")};
  const std::string trailer{File("The.Matrix.1999/The.Matrix.1999-trailer.mkv")};
  EXPECT_EQ(trailer, Trailer(URIUtils::GetBlurayPlaylistPath(disc, 800)));
}

TEST_F(TestDiscTrailer, ADvdFolderRipInADiscFolderHasTheMoviesTrailer)
{
  const std::string disc{File("M3GAN/disc 1/VIDEO_TS/VIDEO_TS.IFO")};
  const std::string trailer{File("M3GAN/disc 1/M3GAN-trailer.mkv")};
  EXPECT_EQ(trailer, Trailer(disc));
}

// As up to v21, inside BDMV when there is none beside it
TEST_F(TestDiscTrailer, ADiscFolderRipInADiscFolderHasTheTrailerInsideBDMV)
{
  const std::string disc{File("M3GAN/disc 1/BDMV/index.bdmv")};
  const std::string trailer{File("M3GAN/disc 1/BDMV/index-trailer.mkv")};
  EXPECT_EQ(trailer, Trailer(URIUtils::GetBlurayPlaylistPath(disc, 800)));
}

// An ISO's trailer is named after the ISO, wherever it is
TEST_F(TestDiscTrailer, AnIsoHasTheTrailerNamedAfterIt)
{
  const std::string disc{File("M3GAN/M3GAN.iso")};
  const std::string trailer{File("M3GAN/M3GAN-trailer.mkv")};
  EXPECT_EQ(trailer, Trailer(URIUtils::GetBlurayPlaylistPath(disc, 800)));
  EXPECT_EQ(trailer, Trailer(disc));
}

TEST_F(TestDiscTrailer, AnIsoInADiscFolderHasTheTrailerNamedAfterIt)
{
  const std::string disc{File("M3GAN/disc 1/M3GAN.iso")};
  const std::string trailer{File("M3GAN/disc 1/M3GAN-trailer.mkv")};
  EXPECT_EQ(trailer, Trailer(URIUtils::GetBlurayPlaylistPath(disc, 800)));
}

TEST_F(TestDiscTrailer, ADvdFolderRipPrefersTheTrailerBesideVIDEO_TS)
{
  const std::string disc{File("M3GAN/VIDEO_TS/VIDEO_TS.IFO")};
  const std::string beside{File("M3GAN/M3GAN-trailer.mkv")};
  File("M3GAN/VIDEO_TS/VIDEO_TS-trailer.mkv");
  EXPECT_EQ(beside, Trailer(disc));
}

// Several ISOs can share a folder, their movies having no folders of their own: each has the
// trailer named after it, and movie-trailer is none of theirs
TEST_F(TestDiscTrailer, EachOfSeveralIsosInAFolderHasTheTrailerNamedAfterIt)
{
  const std::string discA{File("Movies/A.iso")};
  const std::string discB{File("Movies/B.iso")};
  const std::string trailerB{File("Movies/B-trailer.mkv")};
  File("Movies/movie-trailer.mkv");
  EXPECT_EQ("", Trailer(URIUtils::GetBlurayPlaylistPath(discA, 800), false));
  EXPECT_EQ(trailerB, Trailer(URIUtils::GetBlurayPlaylistPath(discB, 800), false));
}

// movie-trailer is no particular video's, so it is only the trailer of a movie with a folder of its
// own
TEST_F(TestDiscTrailer, MovieTrailerIsOnlyTheTrailerOfAMovieWithAFolderOfItsOwn)
{
  const std::string movie{File("Film/Film.mkv")};
  const std::string trailer{File("Film/movie-trailer.avi")};
  EXPECT_EQ(trailer, Trailer(movie, true));
  EXPECT_EQ("", Trailer(movie, false));
}

// ..even when a trailer expression matches it, while the expression still finds other trailers
TEST_F(TestDiscTrailer, MovieTrailerIsNoTrailerOfAMovieWithoutAFolderOfItsOwnByExpression)
{
  const auto advancedSettings{CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()};
  const std::vector<std::string> trailerExpressions{advancedSettings->m_trailerMatchRegExps};
  advancedSettings->m_trailerMatchRegExps = {"trailer$"};

  const std::string film{File("Film/Film.mkv")};
  const std::string movieTrailer{File("Film/movie-trailer.avi")};
  const std::string other{File("Other/Other.mkv")};
  const std::string otherTrailer{File("Other/Other.Trailer.avi")};
  const std::string filmWithFolder{Trailer(film, true)};
  const std::string filmWithoutFolder{Trailer(film, false)};
  const std::string otherWithoutFolder{Trailer(other, false)};

  advancedSettings->m_trailerMatchRegExps = trailerExpressions;
  EXPECT_EQ(movieTrailer, filmWithFolder);
  EXPECT_EQ("", filmWithoutFolder);
  EXPECT_EQ(otherTrailer, otherWithoutFolder);
}

// A disc folder rip's folder is its own, whatever its source's setting
TEST_F(TestDiscTrailer, ADiscFolderRipAlwaysHasItsMovieTrailer)
{
  const std::string disc{File("M3GAN/BDMV/index.bdmv")};
  const std::string trailer{File("M3GAN/movie-trailer.mkv")};
  EXPECT_EQ(trailer, Trailer(URIUtils::GetBlurayPlaylistPath(disc, 800), false));
}

TEST_F(TestDiscTrailer, ADiscFolderRipAlwaysHasItsMovieTrailerInsideBDMV)
{
  const std::string disc{File("M3GAN/BDMV/index.bdmv")};
  const std::string trailer{File("M3GAN/BDMV/movie-trailer.mkv")};
  EXPECT_EQ(trailer, Trailer(URIUtils::GetBlurayPlaylistPath(disc, 800), false));
}

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

TEST(TestVideoUtils, EpisodeCountsDeriveFromTotalAndWatched)
{
  CFileItem show;
  VIDEO::UTILS::SetEpisodeCounts(show, 8, 2);
  EXPECT_EQ(show.GetProperty("totalepisodes").asInteger(), 8);
  EXPECT_EQ(show.GetProperty("numepisodes").asInteger(), 8);
  EXPECT_EQ(show.GetProperty("watchedepisodes").asInteger(), 2);
  EXPECT_EQ(show.GetProperty("unwatchedepisodes").asInteger(), 6);
  EXPECT_EQ(show.GetProperty("watchedepisodepercent").asInteger(), 25);
  EXPECT_FALSE(show.HasProperty("inprogressepisodes"));
}

TEST(TestVideoUtils, EpisodeCountsOfAnEmptyShow)
{
  CFileItem show;
  VIDEO::UTILS::SetEpisodeCounts(show, 0, 0);
  EXPECT_EQ(show.GetProperty("watchedepisodepercent").asInteger(), 0);
}

namespace
{
//! The videos EnumerateVideoExtras() finds below root, as paths relative to it
std::set<std::string> GetVideoExtras(const std::string& root, const std::string& folder)
{
  std::set<std::string> videos;
  VIDEO::UTILS::EnumerateVideoExtras(URIUtils::AddFileToFolder(root, folder),
                                     [&root, &videos](const std::shared_ptr<CFileItem>& item)
                                     {
                                       std::string video{item->GetPath().substr(root.size())};
                                       StringUtils::Replace(video, '\\', '/');
                                       videos.emplace(video);
                                     });
  return videos;
}

void CreateFiles(const std::string& root, const std::vector<std::string>& files)
{
  for (const std::string& file : files)
  {
    const std::string path{URIUtils::AddFileToFolder(root, file)};
    ASSERT_TRUE(CUtil::CreateDirectoryEx(URIUtils::GetDirectory(path)));
    ASSERT_TRUE(WriteTextFile(path, ""));
  }
}
} // namespace

TEST(TestVideoUtils, EnumerateVideoExtrasTakesADiscAsOneVideo)
{
  std::error_code ec;
  std::string root{fs::create_temp_directory(ec)};
  ASSERT_FALSE(ec);
  URIUtils::AddSlashAtEnd(root);

  CreateFiles(root, {
                        "Extras/Trailer.mkv",
                        "Extras/Featurettes/Making Of.mkv",
                        "Extras/Hidden/.nomedia",
                        "Extras/Hidden/Hidden.mkv",
                        "Extras/DVD/VIDEO_TS/VIDEO_TS.IFO",
                        "Extras/DVD/VIDEO_TS/VTS_01_1.VOB",
#ifdef HAVE_LIBBLURAY
                        "Extras/Disc 2/BDMV/index.bdmv",
                        "Extras/Disc 2/BDMV/MovieObject.bdmv",
                        "Extras/Disc 2/BDMV/PLAYLIST/00001.mpls",
                        "Extras/Disc 2/BDMV/STREAM/00001.m2ts",
                        "Extras/Disc 2/BDMV/BACKUP/index.bdmv",
                        "Bonus Disc/BDMV/index.bdmv",
                        "Bonus Disc/BDMV/STREAM/00001.m2ts",
#endif
                    });

  const std::set<std::string> extras{
      "Extras/Trailer.mkv",
      "Extras/Featurettes/Making Of.mkv",
      "Extras/DVD/VIDEO_TS/VIDEO_TS.IFO",
#ifdef HAVE_LIBBLURAY
      "Extras/Disc 2/BDMV/index.bdmv",
#endif
  };
  EXPECT_EQ(GetVideoExtras(root, "Extras"), extras);
#ifdef HAVE_LIBBLURAY
  // The extras folder may itself be the disc
  EXPECT_EQ(GetVideoExtras(root, "Bonus Disc"),
            std::set<std::string>{"Bonus Disc/BDMV/index.bdmv"});
#endif

  XFILE::CDirectory::RemoveRecursive(root);
}

TEST(TestVideoUtils, VideoExtrasAreNamedAfterTheirFileOrDisc)
{
  const std::string extras{"/movies/Movie/Extras/"};
  EXPECT_EQ(CGUIDialogVideoManagerExtras::GenerateVideoExtra(extras,
                                                             extras + "Featurettes/Making Of.mkv"),
            "Featurettes/Making Of");
  EXPECT_EQ(CGUIDialogVideoManagerExtras::GenerateVideoExtra(extras, extras + "Bonus.iso"),
            "Bonus");
  EXPECT_EQ(
      CGUIDialogVideoManagerExtras::GenerateVideoExtra(extras, extras + "Disc 2/BDMV/index.bdmv"),
      "Disc 2");
  EXPECT_EQ(CGUIDialogVideoManagerExtras::GenerateVideoExtra(extras,
                                                             extras + "DVD/VIDEO_TS/VIDEO_TS.IFO"),
            "DVD");

  const std::string bonus{"/movies/Movie/Bonus Disc/"};
  EXPECT_EQ(CGUIDialogVideoManagerExtras::GenerateVideoExtra(bonus, bonus + "BDMV/index.bdmv"),
            "Bonus Disc");
}

// An ISO in a folder of its own has the movie-trailer there, and none in a folder it shares
TEST_F(TestDiscTrailer, AnIsoHasTheMovieTrailerOnlyInAFolderOfItsOwn)
{
  const std::string disc{File("M3GAN/M3GAN.iso")};
  const std::string trailer{File("M3GAN/movie-trailer.mkv")};
  EXPECT_EQ(trailer, Trailer(URIUtils::GetBlurayPlaylistPath(disc, 800), true));
  EXPECT_EQ("", Trailer(URIUtils::GetBlurayPlaylistPath(disc, 800), false));
}

// A dvd folder rip's folder is its own, whatever its source's setting
TEST_F(TestDiscTrailer, ADvdFolderRipAlwaysHasItsMovieTrailer)
{
  const std::string disc{File("M3GAN/VIDEO_TS/VIDEO_TS.IFO")};
  const std::string trailer{File("M3GAN/movie-trailer.mkv")};
  EXPECT_EQ(trailer, Trailer(disc, false));
}

TEST_F(TestDiscTrailer, ADvdFolderRipAlwaysHasItsMovieTrailerInsideVIDEO_TS)
{
  const std::string disc{File("M3GAN/VIDEO_TS/VIDEO_TS.IFO")};
  const std::string trailer{File("M3GAN/VIDEO_TS/movie-trailer.mkv")};
  EXPECT_EQ(trailer, Trailer(disc, false));
}

TEST(TestVideoUtils, NextPartBookmarkKeepsPlayerState)
{
  const std::string navState{R"(<navstate version="2"><title>1</title><pgn>3</pgn></navstate>)"};

  CBookmark bookmark;
  bookmark.playerState = VIDEO::UTILS::MakeNextPartPlayerState(2, navState);
  EXPECT_EQ(VIDEO::UTILS::GetNextPartFromBookmark(bookmark), 2);
  EXPECT_EQ(VIDEO::UTILS::GetNextPartPlayerStateFromBookmark(bookmark), navState);

  // Without a state the part resumes from its start, as before
  bookmark.playerState = VIDEO::UTILS::MakeNextPartPlayerState(2, "");
  EXPECT_EQ(bookmark.playerState, "<nextpart>2</nextpart>");
  EXPECT_EQ(VIDEO::UTILS::GetNextPartFromBookmark(bookmark), 2);
  EXPECT_FALSE(VIDEO::UTILS::GetNextPartPlayerStateFromBookmark(bookmark));

  // A player state that isn't a next part bookmark
  bookmark.playerState = navState;
  EXPECT_FALSE(VIDEO::UTILS::GetNextPartFromBookmark(bookmark));
  EXPECT_FALSE(VIDEO::UTILS::GetNextPartPlayerStateFromBookmark(bookmark));
}

TEST(TestVideoUtils, StackPartResumeOffsetOfNextPartBookmark)
{
  CFileItem item(
      R"(stack://D:\Movie\Movie cd1.iso , D:\Movie\Movie cd2.iso , D:\Movie\Movie cd3.iso)", false);

  // Resume point within part 3 (0-based part 2), its time relative to that part
  CBookmark bookmark;
  bookmark.timeInSeconds = 1234.5;
  bookmark.totalTimeInSeconds = 4106.0;
  bookmark.playerState = VIDEO::UTILS::MakeNextPartPlayerState(
      2, R"(<navstate version="2"><title>1</title><pgn>9</pgn></navstate>)");
  item.GetVideoInfoTag()->SetResumePoint(bookmark);

  EXPECT_EQ(VIDEO::UTILS::GetStackPartResumeOffset(item, 3), 1234500);
  // The relative time isn't a resume point for any other part
  EXPECT_EQ(VIDEO::UTILS::GetStackPartResumeOffset(item, 1), 0);
  EXPECT_EQ(VIDEO::UTILS::GetStackPartResumeOffset(item, 2), 0);

  // Without a player state, part 3 resumes from its start
  bookmark.timeInSeconds = 2.0;
  bookmark.totalTimeInSeconds = 3.0;
  bookmark.playerState = VIDEO::UTILS::MakeNextPartPlayerState(2, "");
  item.GetVideoInfoTag()->SetResumePoint(bookmark);

  EXPECT_EQ(VIDEO::UTILS::GetStackPartResumeOffset(item, 3), 0);
  EXPECT_EQ(VIDEO::UTILS::GetStackPartResumeOffset(item, 1), 0);
}

TEST(TestVideoUtils, ItemResumeInformationOfNextPartBookmark)
{
  CFileItem item(
      R"(stack://D:\Movie\Movie cd1.iso , D:\Movie\Movie cd2.iso , D:\Movie\Movie cd3.iso)", false);

  // Resume point within part 3 (0-based part 2), time relative to that part
  CBookmark bookmark;
  bookmark.timeInSeconds = 1234.5;
  bookmark.totalTimeInSeconds = 4106.0;
  bookmark.playerState = VIDEO::UTILS::MakeNextPartPlayerState(
      2, R"(<navstate version="2"><title>1</title><pgn>9</pgn></navstate>)");
  item.GetVideoInfoTag()->SetResumePoint(bookmark);

  VIDEO::UTILS::ResumeInformation info{VIDEO::UTILS::GetItemResumeInformation(item)};
  EXPECT_TRUE(info.isResumable);
  EXPECT_EQ(info.startOffset, 1234500);
  EXPECT_EQ(info.partNumber, 3);

  // Without a player state the part is offered from its start
  bookmark.timeInSeconds = 2.0;
  bookmark.totalTimeInSeconds = 3.0;
  bookmark.playerState = VIDEO::UTILS::MakeNextPartPlayerState(2, "");
  item.GetVideoInfoTag()->SetResumePoint(bookmark);

  info = VIDEO::UTILS::GetItemResumeInformation(item);
  EXPECT_TRUE(info.isResumable);
  EXPECT_EQ(info.startOffset, 0);
  EXPECT_EQ(info.partNumber, 3);
}

namespace
{
//! The item as CDiscDirectoryHelper::GetOrShowPlaylistSelection() returns the playlist it picked
//! (see its ApplyPlaylistDetails()): the paths in the tag point at the playlist too
void PickPlaylist(CFileItem& item, const std::string& playlist)
{
  item.SetProperty(ITEM::PROPERTY::ORIGINAL_LISTITEM_URL, item.GetDynPath());
  item.SetDynPath(playlist);
  item.GetVideoInfoTag()->SetFileNameAndPath(playlist);
}
} // namespace

TEST(TestVideoUtils, FlagMainTitleToSave)
{
  const std::string image{"/movies/Film (2015)/Film.iso"};
  const std::string bdmv{"/movies/Film (2015)/BDMV/index.bdmv"};
  const struct
  {
    std::string before;
    bool inLibrary;
    bool flagged;
  } cases[]{
      {image, true, true},
      {bdmv, true, true},
      // Already a playlist, maybe one the user chose, which the main title must not replace
      {URIUtils::GetBlurayPlaylistPath(image, 801), true, false},
      // Not in the library (Videos > Files), or a part of a stack, which
      // CApplicationStackHelper::InitializeStack() gives a blank tag. The stack's own file must not
      // be replaced by the playlist of one of its parts.
      {image, false, false},
  };

  for (const auto& c : cases)
  {
    SCOPED_TRACE(c.before + (c.inLibrary ? " in library" : " not in library"));
    CFileItem item{c.before, false};
    CVideoInfoTag* tag{item.GetVideoInfoTag()};
    if (c.inLibrary)
    {
      tag->SetMediaType(MEDIA::TYPE::MOVIE);
      tag->m_iDbId = 1;
      tag->m_iFileId = 1;
    }
    PickPlaylist(item, URIUtils::GetBlurayPlaylistPath(c.before, 800));

    EXPECT_EQ(c.flagged, VIDEO::UTILS::FlagMainTitleToSave(item, c.before));
    EXPECT_EQ(c.flagged, item.HasProperty(ITEM::PROPERTY::NEW_PLAYLIST_PATH));
  }
}

namespace
{
//! CSaveFileState updates the listing through the window manager, and the database's commit the
//! library info through the info manager
class DiscTestGUI : public CGUIComponent
{
public:
  DiscTestGUI() : CGUIComponent(false)
  {
    m_pWindowManager = std::make_unique<CGUIWindowManager>();
    m_guiInfoManager = std::make_unique<CGUIInfoManager>();
  }

  ~DiscTestGUI() override { m_pWindowManager.reset(); }
};
} // namespace

//! A disc in a library scanned before playlists were picked at scan time, stored as the disc
//! itself, played with its main title picked (disc playback setting "Play main movie") and stopped
//! part way through
class TestDiscMainTitleSave : public testing::Test
{
protected:
  void SetUp() override
  {
    m_previousAnnouncements = CServiceBroker::GetAnnouncementManager();
    CServiceBroker::RegisterAnnouncementManager(
        std::make_shared<ANNOUNCEMENT::CAnnouncementManager>());
    // CSaveFileState opens the profile's database itself
    if (!CServiceBroker::GetDatabaseManager().CanOpen("MyVideos"))
    {
      ASSERT_TRUE(CServiceBroker::GetDatabaseManager().Initialize());
    }
    ASSERT_TRUE(m_db.Open());
    CServiceBroker::RegisterGUI(&m_gui);

    // A disc of each test's own, as the tests share the database. It need not exist.
    m_image = URIUtils::AddFileToFolder(
        "/TestDiscMainTitleSave", testing::UnitTest::GetInstance()->current_test_info()->name(),
        "Disc.iso");
  }

  void TearDown() override
  {
    CServiceBroker::UnregisterGUI();
    if (m_idMovie > 0)
      m_db.DeleteMovie(m_idMovie);
    if (m_idShow > 0)
      m_db.DeleteTvShow(m_idShow);
    m_db.Close();
    CServiceBroker::UnregisterAnnouncementManager();
    if (m_previousAnnouncements)
      CServiceBroker::RegisterAnnouncementManager(m_previousAnnouncements);
  }

  int AddMovie()
  {
    CVideoInfoTag tag;
    tag.m_strTitle = "Film";
    tag.m_strFileNameAndPath = m_image;
    tag.m_strPath = URIUtils::GetDirectory(m_image);
    m_idMovie = m_db.SetDetailsForMovie(tag, ART::Artwork{});
    return m_idMovie;
  }

  //! Picks playlist 1 for the item as listed, and saves its state when playback of it stops at
  //! \p seconds, or finishes if that is negative, as CApplicationPlayerCallback passes them
  void PlayMainTitle(VideoDbContentType type, int id, double seconds = 664.0)
  {
    CFileItem item{Listed(type, id)};
    const std::string listed{item.GetDynPath()};
    PickPlaylist(item, Playlist());
    EXPECT_TRUE(VIDEO::UTILS::FlagMainTitleToSave(item, listed));

    CBookmark bookmark;
    bookmark.timeInSeconds = seconds;
    bookmark.totalTimeInSeconds = 7461.0;
    bookmark.player = "VideoPlayer";
    bookmark.playerState =
        R"(<libbluraystate version="1"><playlistId>1</playlistId></libbluraystate>)";
    CSaveFileState::DoWork(item, bookmark, seconds < 0.0);
  }

  //! The item as its listing holds it
  CFileItem Listed(VideoDbContentType type, int id)
  {
    return CFileItem{m_db.GetDetailsByTypeAndId(type, id)};
  }

  std::string Playlist() const { return URIUtils::GetBlurayPlaylistPath(m_image, 1); }

  CVideoDatabase m_db;
  std::string m_image;
  int m_idMovie{-1};
  int m_idShow{-1};

private:
  DiscTestGUI m_gui;
  std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> m_previousAnnouncements;
};

TEST_F(TestDiscMainTitleSave, AMovieKeepsTheResumePointOfItsMainTitle)
{
  ASSERT_GT(AddMovie(), 0);

  PlayMainTitle(VideoDbContentType::MOVIES, m_idMovie);

  const CFileItem relisted{Listed(VideoDbContentType::MOVIES, m_idMovie)};
  EXPECT_EQ(Playlist(), relisted.GetVideoInfoTag()->m_strFileNameAndPath);
  EXPECT_EQ(664.0, relisted.GetVideoInfoTag()->GetResumePoint().timeInSeconds);
}

TEST_F(TestDiscMainTitleSave, AMovieStoppedWhereItsImageWasStoppedBeforeKeepsThatResumePoint)
{
  // The image's file, and the resume point with it, is deleted when the movie moves to the
  // playlist, so the resume point has to be written to the playlist though it is the same
  ASSERT_GT(AddMovie(), 0);
  CBookmark before;
  before.timeInSeconds = 664.0;
  before.totalTimeInSeconds = 7461.0;
  ASSERT_TRUE(m_db.AddBookMarkToFile(m_image, before, CBookmark::RESUME));

  PlayMainTitle(VideoDbContentType::MOVIES, m_idMovie, 664.0);

  const CFileItem relisted{Listed(VideoDbContentType::MOVIES, m_idMovie)};
  EXPECT_EQ(Playlist(), relisted.GetVideoInfoTag()->m_strFileNameAndPath);
  EXPECT_EQ(664.0, relisted.GetVideoInfoTag()->GetResumePoint().timeInSeconds);
}

TEST_F(TestDiscMainTitleSave, AMovieStaysOnItsImageWhenItsResumePointCannotBeWrittenToThePlaylist)
{
  // Moving to the playlist could delete the image's file and its resume point, leaving none
  ASSERT_GT(AddMovie(), 0);
  CBookmark before;
  before.timeInSeconds = 100.0;
  before.totalTimeInSeconds = 7461.0;
  ASSERT_TRUE(m_db.AddBookMarkToFile(m_image, before, CBookmark::RESUME));

  // A new resume point fails to be written, as on a database error
  ASSERT_TRUE(m_db.ExecuteQuery("CREATE TRIGGER test_fail_bookmark BEFORE INSERT ON bookmark "
                                "BEGIN SELECT RAISE(ABORT, 'test'); END"));
  PlayMainTitle(VideoDbContentType::MOVIES, m_idMovie, 664.0);
  ASSERT_TRUE(m_db.ExecuteQuery("DROP TRIGGER test_fail_bookmark"));

  const CFileItem relisted{Listed(VideoDbContentType::MOVIES, m_idMovie)};
  EXPECT_EQ(m_image, relisted.GetVideoInfoTag()->m_strFileNameAndPath);
  EXPECT_EQ(100.0, relisted.GetVideoInfoTag()->GetResumePoint().timeInSeconds);
}

TEST_F(TestDiscMainTitleSave, AMovieKeepsItsWatchedStateWhenItsMainTitleIsFinished)
{
  ASSERT_GT(AddMovie(), 0);
  CDateTime lastPlayed;
  lastPlayed.SetFromDBDateTime("2026-01-01 20:00:00");
  m_db.SetPlayCount(CFileItem{m_image, false}, 2, lastPlayed);

  PlayMainTitle(VideoDbContentType::MOVIES, m_idMovie, -1.0);

  const CFileItem relisted{Listed(VideoDbContentType::MOVIES, m_idMovie)};
  const CVideoInfoTag& tag{*relisted.GetVideoInfoTag()};
  EXPECT_EQ(Playlist(), tag.m_strFileNameAndPath);
  EXPECT_EQ(3, tag.GetPlayCount());
  EXPECT_TRUE(tag.m_lastPlayed.IsValid());
  EXPECT_GT(tag.m_lastPlayed, lastPlayed);
}

TEST_F(TestDiscMainTitleSave, AnEpisodeMovesToItsPlaylistLeavingTheOtherOnTheImageTheyShare)
{
  CVideoInfoTag show;
  show.m_strTitle = "Show";
  show.m_strPath = URIUtils::GetDirectory(m_image);
  m_idShow =
      m_db.SetDetailsForTvShow({show.m_strPath}, show, ART::Artwork{}, ART::SeasonsArtwork{});
  ASSERT_GT(m_idShow, 0);

  int idEpisodes[2]{};
  for (int number : {1, 2})
  {
    CVideoInfoTag tag;
    tag.m_strTitle = "Episode";
    tag.m_strFileNameAndPath = m_image;
    tag.m_strPath = show.m_strPath;
    tag.m_iSeason = 1;
    tag.m_iEpisode = number;
    // Added first, as the scanner does, or the second would replace the first on the same file
    const int idEpisode{m_db.AddNewEpisode(m_idShow, tag)};
    idEpisodes[number - 1] = m_db.SetDetailsForEpisode(tag, ART::Artwork{}, m_idShow, idEpisode);
    ASSERT_GT(idEpisodes[number - 1], 0);
  }
  ASSERT_NE(idEpisodes[0], idEpisodes[1]);

  PlayMainTitle(VideoDbContentType::EPISODES, idEpisodes[0]);

  const CFileItem first{Listed(VideoDbContentType::EPISODES, idEpisodes[0])};
  EXPECT_EQ(Playlist(), first.GetVideoInfoTag()->m_strFileNameAndPath);
  EXPECT_EQ(664.0, first.GetVideoInfoTag()->GetResumePoint().timeInSeconds);
  const CFileItem second{Listed(VideoDbContentType::EPISODES, idEpisodes[1])};
  EXPECT_EQ(m_image, second.GetVideoInfoTag()->m_strFileNameAndPath);
}
