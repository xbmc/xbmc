/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DatabaseManager.h"
#include "FileItem.h"
#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "XBDateTime.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/json-rpc/VideoLibrary.h"
#include "utils/Artwork.h"
#include "utils/Variant.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoTag.h"

#include <memory>
#include <string>

#include <gtest/gtest.h>

using namespace JSONRPC;

namespace
{
class CTestVideoLibrary : public CVideoLibrary
{
public:
  using CVideoLibrary::EpisodePlaybackUpdate;
};

CVideoInfoTag Show(int playCount, const char* lastPlayed)
{
  CVideoInfoTag show;
  show.SetPlayCount(playCount);
  show.m_lastPlayed.SetFromDBDateTime(lastPlayed);
  return show;
}

CVideoInfoTag Episode(int playCount, const char* lastPlayed)
{
  CVideoInfoTag episode;
  episode.SetPlayCount(playCount);
  if (lastPlayed)
    episode.m_lastPlayed.SetFromDBDateTime(lastPlayed);
  return episode;
}
} // unnamed namespace

TEST(TestVideoLibraryEpisodePlaybackUpdate, APlaycountOnlyUpdateKeepsTheEpisodesLastPlayed)
{
  const auto update = CTestVideoLibrary::EpisodePlaybackUpdate(
      Show(1, "2026-09-02 20:00:00"), true, false, Episode(0, "2026-08-10 21:49:28"));

  ASSERT_TRUE(update);
  EXPECT_EQ(1, update->playCount);
  EXPECT_EQ("2026-08-10 21:49:28", update->lastPlayed.GetAsDBDateTime());
}

TEST(TestVideoLibraryEpisodePlaybackUpdate, ANeverPlayedEpisodeHasNoTimeToKeep)
{
  const auto update = CTestVideoLibrary::EpisodePlaybackUpdate(Show(1, "2026-09-02 20:00:00"), true,
                                                               false, Episode(0, nullptr));

  ASSERT_TRUE(update);
  EXPECT_FALSE(update->lastPlayed.IsValid());
}

TEST(TestVideoLibraryEpisodePlaybackUpdate, AnEpisodeAlreadyAtThePlaycountIsLeftAlone)
{
  EXPECT_FALSE(CTestVideoLibrary::EpisodePlaybackUpdate(Show(2, "2026-09-02 20:00:00"), true, false,
                                                        Episode(2, "2026-08-10 21:49:28")));
}

TEST(TestVideoLibraryEpisodePlaybackUpdate, ALastPlayedUpdateAppliesTheShowsTime)
{
  const auto update = CTestVideoLibrary::EpisodePlaybackUpdate(
      Show(0, "2026-09-02 20:00:00"), false, true, Episode(2, "2026-08-10 21:49:28"));

  ASSERT_TRUE(update);
  EXPECT_EQ(2, update->playCount);
  EXPECT_EQ("2026-09-02 20:00:00", update->lastPlayed.GetAsDBDateTime());
}

namespace
{
class TestGUI : public CGUIComponent
{
public:
  TestGUI() : CGUIComponent(false)
  {
    m_pWindowManager = std::make_unique<CGUIWindowManager>();
    m_guiInfoManager = std::make_unique<CGUIInfoManager>();
    CServiceBroker::RegisterGUI(this);
  }

  ~TestGUI() override { m_pWindowManager.reset(); }
};

constexpr const char* LAST_PLAYED = "2026-09-02 20:00:00";

class TestVideoLibrarySetDetails : public testing::Test
{
protected:
  void SetUp() override
  {
    m_previousAnnouncements = CServiceBroker::GetAnnouncementManager();
    CServiceBroker::RegisterAnnouncementManager(
        std::make_shared<ANNOUNCEMENT::CAnnouncementManager>());
    if (!CServiceBroker::GetDatabaseManager().CanOpen("MyVideos"))
    {
      ASSERT_TRUE(CServiceBroker::GetDatabaseManager().Initialize());
    }
    ASSERT_TRUE(m_db.Open());
  }

  void TearDown() override
  {
    if (m_movieId > 0)
      m_db.DeleteMovie(m_movieId);
    if (m_showId > 0)
      m_db.DeleteTvShow(m_showId);
    if (m_musicVideoId > 0)
      m_db.DeleteMusicVideo(m_musicVideoId);
    m_db.Close();
    CServiceBroker::RegisterAnnouncementManager(m_previousAnnouncements);
  }

  static CVideoInfoTag Tag(const std::string& file)
  {
    CVideoInfoTag tag;
    tag.m_strTitle = "Original";
    tag.m_strPath = "special://temp/setdetails/";
    tag.m_strFileNameAndPath = tag.m_strPath + file;
    return tag;
  }

  void Play(const std::string& file, int playCount)
  {
    CDateTime lastPlayed;
    lastPlayed.SetFromDBDateTime(LAST_PLAYED);
    m_db.SetPlayCount(CFileItem(Tag(file).m_strFileNameAndPath, false), playCount, lastPlayed);
  }

  void ExpectPlayed(const std::string& file, int playCount, const char* lastPlayed)
  {
    const std::string path = Tag(file).m_strFileNameAndPath;
    EXPECT_EQ(playCount, m_db.GetPlayCount(path)) << file;
    const CDateTime stored = m_db.GetLastPlayed(path);
    if (lastPlayed)
      EXPECT_EQ(lastPlayed, stored.GetAsDBDateTime()) << file;
    else
      EXPECT_FALSE(stored.IsValid()) << file;
  }

  int AddMovie(int playCount)
  {
    CVideoInfoTag tag = Tag("movie.mkv");
    m_movieId = m_db.SetDetailsForMovie(tag, KODI::ART::Artwork{});
    Play("movie.mkv", playCount);
    return m_movieId;
  }

  int AddShow(int firstPlayCount, int secondPlayCount)
  {
    CVideoInfoTag show = Tag("");
    m_showId = m_db.SetDetailsForTvShow({show.m_strPath}, show, KODI::ART::Artwork{},
                                        KODI::ART::SeasonsArtwork{});
    for (int episode : {1, 2})
    {
      const std::string file = "episode" + std::to_string(episode) + ".mkv";
      CVideoInfoTag tag = Tag(file);
      tag.m_iSeason = 1;
      tag.m_iEpisode = episode;
      const int id = m_db.SetDetailsForEpisode(tag, KODI::ART::Artwork{}, m_showId);
      if (episode == 1)
        m_episodeId = id;
      Play(file, episode == 1 ? firstPlayCount : secondPlayCount);
    }
    return m_showId;
  }

  int AddMusicVideo(int playCount)
  {
    CVideoInfoTag tag = Tag("musicvideo.mkv");
    m_musicVideoId = m_db.SetDetailsForMusicVideo(tag, KODI::ART::Artwork{});
    Play("musicvideo.mkv", playCount);
    return m_musicVideoId;
  }

  static CVariant Params(const char* idName, int id, const CVariant& playcount)
  {
    CVariant params(CVariant::VariantTypeObject);
    params[idName] = id;
    params["title"] = "Changed";
    params["playcount"] = playcount;
    params["lastplayed"] = "";
    return params;
  }

  TestGUI m_gui;
  CVideoDatabase m_db;
  int m_movieId{-1};
  int m_showId{-1};
  int m_episodeId{-1};
  int m_musicVideoId{-1};
  std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> m_previousAnnouncements;
};

const CVariant ABSENT(CVariant::VariantTypeNull);
} // unnamed namespace

TEST_F(TestVideoLibrarySetDetails, ClearingLastPlayedOfAWatchedMovieIsRefused)
{
  CVariant result;
  EXPECT_EQ(InvalidParams,
            CVideoLibrary::SetMovieDetails("VideoLibrary.SetMovieDetails", nullptr, nullptr,
                                           Params("movieid", AddMovie(2), ABSENT), result));

  ExpectPlayed("movie.mkv", 2, LAST_PLAYED);
  CVideoInfoTag movie;
  ASSERT_TRUE(m_db.GetMovieInfo("", movie, m_movieId));
  EXPECT_EQ("Original", movie.m_strTitle);
}

TEST_F(TestVideoLibrarySetDetails, MarkingAMovieWatchedWithNoLastPlayedIsRefused)
{
  CVariant result;
  EXPECT_EQ(InvalidParams,
            CVideoLibrary::SetMovieDetails("VideoLibrary.SetMovieDetails", nullptr, nullptr,
                                           Params("movieid", AddMovie(0), 1), result));

  ExpectPlayed("movie.mkv", 0, LAST_PLAYED);
}

TEST_F(TestVideoLibrarySetDetails, ClearingAMovieWithPlaycountZeroClearsBoth)
{
  CVariant result;
  EXPECT_EQ(ACK, CVideoLibrary::SetMovieDetails("VideoLibrary.SetMovieDetails", nullptr, nullptr,
                                                Params("movieid", AddMovie(2), 0), result));

  ExpectPlayed("movie.mkv", 0, nullptr);
}

TEST_F(TestVideoLibrarySetDetails, ClearingLastPlayedOfAWatchedEpisodeIsRefused)
{
  AddShow(2, 0);
  CVariant result;
  EXPECT_EQ(InvalidParams,
            CVideoLibrary::SetEpisodeDetails("VideoLibrary.SetEpisodeDetails", nullptr, nullptr,
                                             Params("episodeid", m_episodeId, ABSENT), result));

  ExpectPlayed("episode1.mkv", 2, LAST_PLAYED);
  CVideoInfoTag episode;
  ASSERT_TRUE(m_db.GetEpisodeInfo("", episode, m_episodeId));
  EXPECT_EQ("Original", episode.m_strTitle);
}

TEST_F(TestVideoLibrarySetDetails, ClearingLastPlayedOfAWatchedMusicVideoIsRefused)
{
  CVariant result;
  EXPECT_EQ(InvalidParams, CVideoLibrary::SetMusicVideoDetails(
                               "VideoLibrary.SetMusicVideoDetails", nullptr, nullptr,
                               Params("musicvideoid", AddMusicVideo(2), ABSENT), result));

  ExpectPlayed("musicvideo.mkv", 2, LAST_PLAYED);
  CVideoInfoTag musicVideo;
  ASSERT_TRUE(m_db.GetMusicVideoInfo("", musicVideo, m_musicVideoId));
  EXPECT_EQ("Original", musicVideo.m_strTitle);
}

TEST_F(TestVideoLibrarySetDetails, ClearingLastPlayedOfAShowWithAWatchedEpisodeIsRefused)
{
  CVariant result;
  EXPECT_EQ(InvalidParams,
            CVideoLibrary::SetTVShowDetails("VideoLibrary.SetTVShowDetails", nullptr, nullptr,
                                            Params("tvshowid", AddShow(2, 0), ABSENT), result));

  ExpectPlayed("episode1.mkv", 2, LAST_PLAYED);
  ExpectPlayed("episode2.mkv", 0, LAST_PLAYED);
  CVideoInfoTag show;
  ASSERT_TRUE(m_db.GetTvShowInfo("", show, m_showId));
  EXPECT_EQ("Original", show.m_strTitle);
}

TEST_F(TestVideoLibrarySetDetails, ClearingAShowWithPlaycountZeroClearsEveryEpisode)
{
  CVariant result;
  EXPECT_EQ(ACK, CVideoLibrary::SetTVShowDetails("VideoLibrary.SetTVShowDetails", nullptr, nullptr,
                                                 Params("tvshowid", AddShow(2, 1), 0), result));

  ExpectPlayed("episode1.mkv", 0, nullptr);
  ExpectPlayed("episode2.mkv", 0, nullptr);
}
