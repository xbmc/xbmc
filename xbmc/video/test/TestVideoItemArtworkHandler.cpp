/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "ServiceBroker.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "media/MediaType.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/ArtTypes.h"
#include "utils/URIUtils.h"
#include "video/VideoInfoTag.h"
#include "video/VideoItemArtworkHandler.h"

#include <memory>
#include <string>

#include <gtest/gtest.h>

using namespace KODI;

namespace
{
//! The local art Choose art offers, which is that a scan finds
class TestVideoItemArtworkHandler : public ::testing::Test
{
protected:
  void SetUp() override
  {
    XFILE::CDirectory::RemoveRecursive(m_root);
    ASSERT_TRUE(XFILE::CDirectory::Create(m_root));
  }

  void TearDown() override { XFILE::CDirectory::RemoveRecursive(m_root); }

  std::string Folder(const std::string& name) const
  {
    std::string folder{URIUtils::AddFileToFolder(m_root, name)};
    URIUtils::AddSlashAtEnd(folder);
    EXPECT_TRUE(XFILE::CDirectory::Create(folder));
    return folder;
  }

  static std::string File(const std::string& folder, const std::string& name)
  {
    const std::string path{URIUtils::AddFileToFolder(folder, name)};
    XFILE::CFile output;
    EXPECT_TRUE(output.OpenForWrite(path, true));
    output.Close();
    return path;
  }

  //! A bluray folder rip in a folder of its own
  //! \return the disc's index.bdmv
  std::string BlurayRip(const std::string& name) const
  {
    Folder(name);
    return File(Folder(URIUtils::AddFileToFolder(name, "BDMV")), "index.bdmv");
  }

  //! A library item, whose path is the database's and whose file is its dynamic path
  static std::string LocalArt(const std::string& file,
                              MEDIA::TYPE mediaType,
                              const std::string& artType,
                              bool hasVersions = false,
                              int season = -1,
                              int episode = -1)
  {
    CVideoInfoTag tag;
    tag.m_strFileNameAndPath = file;
    tag.SetMediaType(mediaType);
    tag.SetHasVideoVersions(hasVersions);
    tag.m_iSeason = season;
    tag.m_iEpisode = episode;
    const auto item{std::make_shared<CFileItem>(tag)};
    item->SetPath("videodb://library/1");
    item->SetDynPath(file);
    return VIDEO::IVideoItemArtworkHandlerFactory::Create(item, MEDIA::NameOf(mediaType), artType)
        ->GetLocalArt();
  }

  const std::string m_root{URIUtils::AddFileToFolder(
      CSpecialProtocol::TranslatePath("special://temp/"), "TestVideoItemArtworkHandler")};
};
} // namespace

TEST_F(TestVideoItemArtworkHandler, AVersionOfABlurayRipHasItsOwnArt)
{
  const std::string disc{BlurayRip("M3GAN")};
  const std::string folder{URIUtils::GetParentPath(disc)};
  const std::string poster{File(folder, "poster.jpg")};
  const std::string poster800{File(folder, "index-00800-poster.jpg")};
  File(folder, "00800-poster.jpg");
  const std::string poster801{File(folder, "00801-poster.png")};
  const std::string fanart800{File(folder, "index-00800-fanart.jpg")};

  using enum MEDIA::TYPE;
  EXPECT_EQ(poster800,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 800), VIDEO_VERSION, ART::TYPE::POSTER));
  EXPECT_EQ(poster801,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 801), VIDEO_VERSION, ART::TYPE::POSTER));
  EXPECT_EQ(fanart800,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 800), VIDEO_VERSION, ART::TYPE::FANART));
  // A version without art of its own has the disc's
  EXPECT_EQ(poster,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 802), VIDEO_VERSION, ART::TYPE::POSTER));
  // The movie is the version it plays, when it has versions
  EXPECT_EQ(poster800,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 800), MOVIE, ART::TYPE::POSTER, true));
  EXPECT_EQ(poster, LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 800), MOVIE, ART::TYPE::POSTER));
}

TEST_F(TestVideoItemArtworkHandler, AnEpisodeOfSeveralHasItsOwnArt)
{
  const std::string disc{BlurayRip("Show.S01E01E02")};
  const std::string folder{URIUtils::GetParentPath(disc)};
  const std::string thumb1{File(folder, "index-S01E01-thumb.jpg")};
  const std::string thumb2{File(folder, "S01E02-thumb.jpg")};

  using enum MEDIA::TYPE;
  EXPECT_EQ(thumb1, LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 800), EPISODE, ART::TYPE::THUMB,
                             false, 1, 1));
  EXPECT_EQ(thumb2, LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 801), EPISODE, ART::TYPE::THUMB,
                             false, 1, 2));

  // As one of several episodes in a video file is
  const std::string file{File(m_root, "Show.S01E03E04.mkv")};
  const std::string thumb4{File(m_root, "Show.S01E03E04-S01E04-thumb.jpg")};
  EXPECT_EQ(thumb4, LocalArt(file, EPISODE, ART::TYPE::THUMB, false, 1, 4));
}

TEST_F(TestVideoItemArtworkHandler, AVersionOfABlurayRipInADiscFolderHasItsOwnArt)
{
  Folder("M3GAN");
  const std::string disc{BlurayRip(URIUtils::AddFileToFolder("M3GAN", "disc 1"))};
  const std::string folder{URIUtils::GetParentPath(disc)};
  const std::string poster800{File(folder, "index-00800-poster.jpg")};
  const std::string poster801{File(folder, "00801-poster.jpg")};

  using enum MEDIA::TYPE;
  EXPECT_EQ(poster800,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 800), VIDEO_VERSION, ART::TYPE::POSTER));
  EXPECT_EQ(poster801,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 801), VIDEO_VERSION, ART::TYPE::POSTER));
}

// Several ISOs can share a folder, so the playlist alone can't say which one the art is for
TEST_F(TestVideoItemArtworkHandler, AVersionOfAnIsoHasItsOwnArt)
{
  const std::string folder{Folder("M3GAN")};
  const std::string disc{File(folder, "M3GAN.iso")};
  const std::string poster{File(folder, "M3GAN-poster.jpg")};
  const std::string poster800{File(folder, "M3GAN-00800-poster.jpg")};
  File(folder, "00801-poster.jpg");

  using enum MEDIA::TYPE;
  EXPECT_EQ(poster800,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 800), VIDEO_VERSION, ART::TYPE::POSTER));
  EXPECT_EQ(poster,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 801), VIDEO_VERSION, ART::TYPE::POSTER));
}

TEST_F(TestVideoItemArtworkHandler, AnEpisodeOfSeveralOnADvdOrIsoHasItsOwnArt)
{
  const std::string dvdFolder{Folder("Show.S01E03E04")};
  const std::string dvd{
      File(Folder(URIUtils::AddFileToFolder("Show.S01E03E04", "VIDEO_TS")), "VIDEO_TS.IFO")};
  const std::string thumb3{File(dvdFolder, "VIDEO_TS-S01E03-thumb.jpg")};
  const std::string thumb4{File(dvdFolder, "S01E04-thumb.jpg")};

  using enum MEDIA::TYPE;
  EXPECT_EQ(thumb3, LocalArt(dvd, EPISODE, ART::TYPE::THUMB, false, 1, 3));
  EXPECT_EQ(thumb4, LocalArt(dvd, EPISODE, ART::TYPE::THUMB, false, 1, 4));

  // Several ISOs can share a folder, so the episode alone can't say which one the art is for
  const std::string iso{File(m_root, "Show.S01E05E06.iso")};
  const std::string thumb5{File(m_root, "Show.S01E05E06-S01E05-thumb.jpg")};
  File(m_root, "S01E06-thumb.jpg");
  EXPECT_EQ(thumb5, LocalArt(URIUtils::GetBlurayPlaylistPath(iso, 800), EPISODE, ART::TYPE::THUMB,
                             false, 1, 5));
  EXPECT_EQ("", LocalArt(URIUtils::GetBlurayPlaylistPath(iso, 801), EPISODE, ART::TYPE::THUMB,
                         false, 1, 6));
}

// Several ISOs can share a folder: Choose art offers only what is named after a version's own ISO
TEST_F(TestVideoItemArtworkHandler, EachOfSeveralIsosInAFolderHasOnlyItsOwnArt)
{
  const std::string discA{File(m_root, "A.iso")};
  File(m_root, "B.iso");
  const std::string posterA800{File(m_root, "A-00800-poster.jpg")};
  File(m_root, "B-00801-poster.jpg");
  File(m_root, "00801-poster.jpg");
  File(m_root, "poster.jpg");

  using enum MEDIA::TYPE;
  EXPECT_EQ(posterA800, LocalArt(URIUtils::GetBlurayPlaylistPath(discA, 800), VIDEO_VERSION,
                                 ART::TYPE::POSTER));
  EXPECT_EQ(
      "", LocalArt(URIUtils::GetBlurayPlaylistPath(discA, 801), VIDEO_VERSION, ART::TYPE::POSTER));
}

// A version's own art is matched as a scan matches it: any picture's extension, in any case, and
// its name in the case local art is matched in, winning over the plain art
TEST_F(TestVideoItemArtworkHandler, AVersionsOwnArtIsMatchedAsAScanMatchesIt)
{
  const std::string disc{BlurayRip("M3GAN")};
  const std::string folder{URIUtils::GetParentPath(disc)};
  const std::string poster{File(folder, "poster.jpg")};
  const std::string poster800{File(folder, "index-00800-poster.webp")};
  const std::string poster801{File(folder, "00801-poster.JPG")};
  const std::string poster802{File(folder, "INDEX-00802-POSTER.png")};

  using enum MEDIA::TYPE;
  EXPECT_EQ(poster800,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 800), VIDEO_VERSION, ART::TYPE::POSTER));
  EXPECT_EQ(poster801,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 801), VIDEO_VERSION, ART::TYPE::POSTER));
  // By default local art is matched in its name's case
  EXPECT_EQ(poster,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 802), VIDEO_VERSION, ART::TYPE::POSTER));

  const auto advancedSettings{CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()};
  const bool caseSensitive{advancedSettings->m_caseSensitiveLocalArtMatch};
  advancedSettings->m_caseSensitiveLocalArtMatch = false;
  const std::string art802{
      LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 802), VIDEO_VERSION, ART::TYPE::POSTER)};
  advancedSettings->m_caseSensitiveLocalArtMatch = caseSensitive;
  EXPECT_EQ(poster802, art802);
}

TEST_F(TestVideoItemArtworkHandler, AVersionOfAnIsoInADiscFolderHasItsOwnArt)
{
  Folder("M3GAN");
  const std::string folder{Folder(URIUtils::AddFileToFolder("M3GAN", "disc 1"))};
  const std::string disc{File(folder, "M3GAN.iso")};
  const std::string poster800{File(folder, "M3GAN-00800-poster.jpg")};

  using enum MEDIA::TYPE;
  EXPECT_EQ(poster800,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 800), VIDEO_VERSION, ART::TYPE::POSTER));
}

// A movie without versions, on an ISO or in a video file, has the art named after its file
TEST_F(TestVideoItemArtworkHandler, AMovieWithoutVersionsHasTheArtNamedAfterItsFile)
{
  const std::string isoFolder{Folder("M3GAN")};
  const std::string iso{File(isoFolder, "M3GAN.iso")};
  const std::string isoPoster{File(isoFolder, "M3GAN-poster.jpg")};
  File(isoFolder, "M3GAN-00800-poster.jpg");
  const std::string fileFolder{Folder("Film")};
  const std::string file{File(fileFolder, "Film.mkv")};
  const std::string filePoster{File(fileFolder, "Film-poster.jpg")};

  using enum MEDIA::TYPE;
  EXPECT_EQ(isoPoster,
            LocalArt(URIUtils::GetBlurayPlaylistPath(iso, 800), MOVIE, ART::TYPE::POSTER));
  EXPECT_EQ(isoPoster, LocalArt(iso, MOVIE, ART::TYPE::POSTER));
  EXPECT_EQ(filePoster, LocalArt(file, MOVIE, ART::TYPE::POSTER));
}

TEST_F(TestVideoItemArtworkHandler, AnEpisodeOfSeveralOnADiscInADiscFolderHasItsOwnArt)
{
  Folder("Show.S01E10E11");
  const std::string disc{BlurayRip(URIUtils::AddFileToFolder("Show.S01E10E11", "disc 1"))};
  const std::string folder{URIUtils::GetParentPath(disc)};
  const std::string thumb10{File(folder, "index-S01E10-thumb.jpg")};
  const std::string thumb11{File(folder, "S01E11-thumb.jpg")};

  using enum MEDIA::TYPE;
  EXPECT_EQ(thumb10, LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 800), EPISODE, ART::TYPE::THUMB,
                              false, 1, 10));
  EXPECT_EQ(thumb11, LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 801), EPISODE, ART::TYPE::THUMB,
                              false, 1, 11));
}
