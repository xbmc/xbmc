/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DatabaseManager.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "interfaces/AnnouncementManager.h"
#include "media/MediaType.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoTag.h"
#include "video/VideoItemArtworkHandler.h"
#include "video/VideoManagerTypes.h"

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

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

  void TearDown() override
  {
    if (m_library)
    {
      for (const int idShow : m_shows)
        m_library->DeleteTvShow(idShow);
      for (const int idFile : m_files)
        m_library->DeleteFile(idFile);
      m_library->Close();
      m_library.reset();
      // Left as found: the scanner's tests rely on the library being unavailable to the code they
      // reach, which otherwise goes on to read their discs
      if (m_initialisedDatabases)
        CServiceBroker::GetDatabaseManager().Deinitialize();
      CServiceBroker::UnregisterAnnouncementManager();
      if (m_previousAnnouncementManager)
        CServiceBroker::RegisterAnnouncementManager(m_previousAnnouncementManager);
    }
    XFILE::CDirectory::RemoveRecursive(m_root);
  }

  //! The library, which tells whose the plain art beside a video is
  CVideoDatabase& Library()
  {
    if (!m_library)
    {
      // One of the test's own, as removing a show from the library announces it
      m_previousAnnouncementManager = CServiceBroker::GetAnnouncementManager();
      CServiceBroker::RegisterAnnouncementManager(
          std::make_shared<ANNOUNCEMENT::CAnnouncementManager>());
      m_initialisedDatabases = !CServiceBroker::GetDatabaseManager().CanOpen("MyVideos");
      if (m_initialisedDatabases)
      {
        EXPECT_TRUE(CServiceBroker::GetDatabaseManager().Initialize());
      }
      m_library = std::make_unique<CVideoDatabase>();
      EXPECT_TRUE(m_library->Open());
    }
    return *m_library;
  }

  //! The info of an extra of a movie, such as a featurette, the library stores as the given file:
  //! one of a disc's playlists, or a whole disc
  CVideoInfoTag Extra(const std::string& stored)
  {
    CVideoInfoTag tag;
    tag.m_strFileNameAndPath = stored;
    tag.m_type = MediaTypeVideoVersion;
    tag.GetAssetInfo().SetType(VideoAssetType::EXTRA);
    tag.m_iFileId = Library().AddFile(stored);
    EXPECT_GT(tag.m_iFileId, 0);
    m_files.insert(tag.m_iFileId);
    return tag;
  }

  //! A show in the library, of the given episodes of season 1, each in its file
  //! \return the info of each episode, as the library has it
  std::vector<CVideoInfoTag> Show(const std::vector<std::pair<int, std::string>>& episodes)
  {
    CVideoInfoTag show;
    show.m_strTitle = "Show";
    show.m_strPath = m_root;
    const int idShow{
        Library().SetDetailsForTvShow({m_root}, show, ART::Artwork{}, ART::SeasonsArtwork{})};
    EXPECT_GT(idShow, 0);
    m_shows.insert(idShow);

    std::vector<CVideoInfoTag> tags;
    for (const auto& [episode, file] : episodes)
    {
      CVideoInfoTag tag;
      tag.m_strTitle = StringUtils::Format("Episode {}", episode);
      tag.m_strFileNameAndPath = file;
      tag.m_strPath = URIUtils::GetDirectory(file);
      tag.m_iSeason = 1;
      tag.m_iEpisode = episode;
      tag.m_type = MediaTypeEpisode;
      // By its own id, as one of several in a file would otherwise be taken for the one before
      EXPECT_GT(Library().SetDetailsForEpisode(tag, ART::Artwork{}, idShow,
                                               Library().AddNewEpisode(idShow, tag)),
                0);
      tag.m_iIdShow = idShow;
      tags.emplace_back(tag);
    }
    return tags;
  }

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
                              const MediaType& mediaType,
                              const std::string& artType,
                              bool hasVersions = false,
                              int season = -1,
                              int episode = -1)
  {
    CVideoInfoTag tag;
    tag.m_strFileNameAndPath = file;
    tag.m_type = mediaType;
    tag.SetHasVideoVersions(hasVersions);
    tag.m_iSeason = season;
    tag.m_iEpisode = episode;
    return LocalArt(tag, artType);
  }

  //! ..with the given info. One listed among a movie's versions and extras has its file as its path.
  static std::string LocalArt(const CVideoInfoTag& tag,
                              const std::string& artType,
                              bool asset = false)
  {
    const auto item{std::make_shared<CFileItem>(tag)};
    if (!asset)
      item->SetPath("videodb://library/1");
    item->SetDynPath(tag.m_strFileNameAndPath);
    return VIDEO::IVideoItemArtworkHandlerFactory::Create(item, tag.m_type, artType)->GetLocalArt();
  }

  const std::string m_root{URIUtils::AddFileToFolder(
      CSpecialProtocol::TranslatePath("special://temp/"), "TestVideoItemArtworkHandler")};
  std::unique_ptr<CVideoDatabase> m_library;
  std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> m_previousAnnouncementManager;
  bool m_initialisedDatabases{false};
  //! What was added to the library, removed again afterwards
  std::set<int> m_shows;
  std::set<int> m_files;
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

  EXPECT_EQ(poster800,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 800), MediaTypeVideoVersion, "poster"));
  EXPECT_EQ(poster801,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 801), MediaTypeVideoVersion, "poster"));
  EXPECT_EQ(fanart800,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 800), MediaTypeVideoVersion, "fanart"));
  // A version without art of its own has the disc's
  EXPECT_EQ(poster,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 802), MediaTypeVideoVersion, "poster"));
  // The movie is the version it plays, when it has versions
  EXPECT_EQ(poster800,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 800), MediaTypeMovie, "poster", true));
  EXPECT_EQ(poster, LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 800), MediaTypeMovie, "poster"));
}

TEST_F(TestVideoItemArtworkHandler, AnEpisodeOfSeveralHasItsOwnArt)
{
  const std::string disc{BlurayRip("Show.S01E01E02")};
  const std::string folder{URIUtils::GetParentPath(disc)};
  const std::string thumb1{File(folder, "index-S01E01-thumb.jpg")};
  const std::string thumb2{File(folder, "S01E02-thumb.jpg")};

  EXPECT_EQ(thumb1, LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 800), MediaTypeEpisode, "thumb",
                             false, 1, 1));
  EXPECT_EQ(thumb2, LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 801), MediaTypeEpisode, "thumb",
                             false, 1, 2));

  // As one of several episodes in a video file is
  const std::string file{File(m_root, "Show.S01E03E04.mkv")};
  const std::string thumb4{File(m_root, "Show.S01E03E04-S01E04-thumb.jpg")};
  EXPECT_EQ(thumb4, LocalArt(file, MediaTypeEpisode, "thumb", false, 1, 4));
}

TEST_F(TestVideoItemArtworkHandler, AVersionOfABlurayRipInADiscFolderHasItsOwnArt)
{
  Folder("M3GAN");
  const std::string disc{BlurayRip(URIUtils::AddFileToFolder("M3GAN", "disc 1"))};
  const std::string folder{URIUtils::GetParentPath(disc)};
  const std::string poster800{File(folder, "index-00800-poster.jpg")};
  const std::string poster801{File(folder, "00801-poster.jpg")};

  EXPECT_EQ(poster800,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 800), MediaTypeVideoVersion, "poster"));
  EXPECT_EQ(poster801,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 801), MediaTypeVideoVersion, "poster"));
}

// Several ISOs can share a folder, so the playlist alone can't say which one the art is for
TEST_F(TestVideoItemArtworkHandler, AVersionOfAnIsoHasItsOwnArt)
{
  const std::string folder{Folder("M3GAN")};
  const std::string disc{File(folder, "M3GAN.iso")};
  const std::string poster{File(folder, "M3GAN-poster.jpg")};
  const std::string poster800{File(folder, "M3GAN-00800-poster.jpg")};
  File(folder, "00801-poster.jpg");

  EXPECT_EQ(poster800,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 800), MediaTypeVideoVersion, "poster"));
  EXPECT_EQ(poster,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 801), MediaTypeVideoVersion, "poster"));
}

TEST_F(TestVideoItemArtworkHandler, AnEpisodeOfSeveralOnADvdOrIsoHasItsOwnArt)
{
  const std::string dvdFolder{Folder("Show.S01E03E04")};
  const std::string dvd{
      File(Folder(URIUtils::AddFileToFolder("Show.S01E03E04", "VIDEO_TS")), "VIDEO_TS.IFO")};
  const std::string thumb3{File(dvdFolder, "VIDEO_TS-S01E03-thumb.jpg")};
  const std::string thumb4{File(dvdFolder, "S01E04-thumb.jpg")};

  EXPECT_EQ(thumb3, LocalArt(dvd, MediaTypeEpisode, "thumb", false, 1, 3));
  EXPECT_EQ(thumb4, LocalArt(dvd, MediaTypeEpisode, "thumb", false, 1, 4));

  // Several ISOs can share a folder, so the episode alone can't say which one the art is for
  const std::string iso{File(m_root, "Show.S01E05E06.iso")};
  const std::string thumb5{File(m_root, "Show.S01E05E06-S01E05-thumb.jpg")};
  File(m_root, "S01E06-thumb.jpg");
  EXPECT_EQ(thumb5, LocalArt(URIUtils::GetBlurayPlaylistPath(iso, 800), MediaTypeEpisode, "thumb",
                             false, 1, 5));
  EXPECT_EQ("", LocalArt(URIUtils::GetBlurayPlaylistPath(iso, 801), MediaTypeEpisode, "thumb",
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

  EXPECT_EQ(posterA800,
            LocalArt(URIUtils::GetBlurayPlaylistPath(discA, 800), MediaTypeVideoVersion, "poster"));
  EXPECT_EQ("",
            LocalArt(URIUtils::GetBlurayPlaylistPath(discA, 801), MediaTypeVideoVersion, "poster"));
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

  EXPECT_EQ(poster800,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 800), MediaTypeVideoVersion, "poster"));
  EXPECT_EQ(poster801,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 801), MediaTypeVideoVersion, "poster"));
  // By default local art is matched in its name's case
  EXPECT_EQ(poster,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 802), MediaTypeVideoVersion, "poster"));

  const auto advancedSettings{CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()};
  const bool caseSensitive{advancedSettings->m_caseSensitiveLocalArtMatch};
  advancedSettings->m_caseSensitiveLocalArtMatch = false;
  const std::string art802{
      LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 802), MediaTypeVideoVersion, "poster")};
  advancedSettings->m_caseSensitiveLocalArtMatch = caseSensitive;
  EXPECT_EQ(poster802, art802);
}

TEST_F(TestVideoItemArtworkHandler, AVersionOfAnIsoInADiscFolderHasItsOwnArt)
{
  Folder("M3GAN");
  const std::string folder{Folder(URIUtils::AddFileToFolder("M3GAN", "disc 1"))};
  const std::string disc{File(folder, "M3GAN.iso")};
  const std::string poster800{File(folder, "M3GAN-00800-poster.jpg")};

  EXPECT_EQ(poster800,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 800), MediaTypeVideoVersion, "poster"));
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

  EXPECT_EQ(isoPoster,
            LocalArt(URIUtils::GetBlurayPlaylistPath(iso, 800), MediaTypeMovie, "poster"));
  EXPECT_EQ(isoPoster, LocalArt(iso, MediaTypeMovie, "poster"));
  EXPECT_EQ(filePoster, LocalArt(file, MediaTypeMovie, "poster"));
}

TEST_F(TestVideoItemArtworkHandler, AnEpisodeOfSeveralOnADiscInADiscFolderHasItsOwnArt)
{
  Folder("Show.S01E10E11");
  const std::string disc{BlurayRip(URIUtils::AddFileToFolder("Show.S01E10E11", "disc 1"))};
  const std::string folder{URIUtils::GetParentPath(disc)};
  const std::string thumb10{File(folder, "index-S01E10-thumb.jpg")};
  const std::string thumb11{File(folder, "S01E11-thumb.jpg")};

  EXPECT_EQ(thumb10, LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 800), MediaTypeEpisode, "thumb",
                              false, 1, 10));
  EXPECT_EQ(thumb11, LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 801), MediaTypeEpisode, "thumb",
                              false, 1, 11));
}

// An extra on a disc it shares, such as a featurette on the movie's disc, is offered only its own
// art, as a scan finds it: the plain art beside the disc is the movie's. A version of the movie on
// the disc has the plain art.
TEST_F(TestVideoItemArtworkHandler, AnExtraOnADiscItSharesHasNoneOfThePlainArt)
{
  // A folder of the test's own, as the directory cache may still hold another test's listing
  const std::string disc{BlurayRip("Featured")};
  const std::string folder{URIUtils::GetParentPath(disc)};
  const std::string poster{File(folder, "poster.jpg")};
  File(folder, "fanart.jpg");
  const std::string poster805{File(folder, "index-00805-poster.jpg")};

  EXPECT_EQ(poster805, LocalArt(Extra(URIUtils::GetBlurayPlaylistPath(disc, 805)), "poster", true));
  const CVideoInfoTag extra{Extra(URIUtils::GetBlurayPlaylistPath(disc, 806))};
  EXPECT_EQ("", LocalArt(extra, "poster", true));
  EXPECT_EQ("", LocalArt(extra, "fanart", true));
  EXPECT_EQ(poster,
            LocalArt(URIUtils::GetBlurayPlaylistPath(disc, 801), MediaTypeVideoVersion, "poster"));
}

// An extra that is a whole disc of its own, such as one in an extras folder, has the plain art
// beside its disc, as a scan gives it
TEST_F(TestVideoItemArtworkHandler, AnExtraThatIsAWholeDiscHasThePlainArtBesideIt)
{
  const std::string disc{BlurayRip("Bonus")};
  const std::string poster{File(URIUtils::GetParentPath(disc), "poster.jpg")};

  EXPECT_EQ(poster, LocalArt(Extra(disc), "poster", true));
}

// An episode of several on a disc or in a file is offered only its own art, as a scan finds it and
// the export names it: the plain art beside the disc can't say which episode it is for. A disc of
// one episode has it.
TEST_F(TestVideoItemArtworkHandler, AnEpisodeOfSeveralHasNoneOfThePlainArt)
{
  // Folders of the test's own, as the directory cache may still hold another test's listing
  const std::string disc{BlurayRip("Series.S01E01E02")};
  const std::string folder{URIUtils::GetParentPath(disc)};
  File(folder, "thumb.jpg");
  const std::string thumb2{File(folder, "S01E02-thumb.jpg")};
  const std::string single{BlurayRip("Series.S01E09")};
  const std::string thumb9{File(URIUtils::GetParentPath(single), "thumb.jpg")};
  const std::string fileFolder{Folder("Series")};
  const std::string file{File(fileFolder, "Series.S01E03E04.mkv")};
  File(fileFolder, "Series.S01E03E04-thumb.jpg");

  const std::vector<CVideoInfoTag> episodes{
      Show({{1, URIUtils::GetBlurayPlaylistPath(disc, 800)},
            {2, URIUtils::GetBlurayPlaylistPath(disc, 801)},
            {3, file},
            {4, file},
            {9, URIUtils::GetBlurayPlaylistPath(single, 800)}})};
  ASSERT_EQ(5u, episodes.size());

  EXPECT_EQ("", LocalArt(episodes[0], "thumb"));
  EXPECT_EQ(thumb2, LocalArt(episodes[1], "thumb"));
  EXPECT_EQ("", LocalArt(episodes[2], "thumb"));
  EXPECT_EQ(thumb9, LocalArt(episodes[4], "thumb"));
}
