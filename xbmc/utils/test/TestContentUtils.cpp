/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "XBDateTime.h"
#include "utils/ContentUtils.h"
#include "utils/StreamDetails.h"
#include "video/Bookmark.h"
#include "video/VideoInfoTag.h"

#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace
{
constexpr const char* MOVIE_PATH{"/movies/Movie (2007)/Movie (2007).bluray.iso"};
constexpr const char* TRAILER_PATH{"/movies/Movie (2007)/Movie (2007)-trailer.mkv"};
constexpr const char* PLAYLIST_PATH{
    "bluray://udf%3a%2f%2f%252fmovies%252fMovie%2520(2007).bluray.iso%2f/BDMV/PLAYLIST/00601.mpls"};

/*!
 * \brief Build a movie item as it looks after having been played, ie. with playback state and
 *        with the paths of a disc image resolved to the played playlist.
 */
CFileItem GetPlayedMovie()
{
  CFileItem movie;
  movie.SetPath(MOVIE_PATH);
  movie.SetArt("poster", "image://poster.jpg/");

  CVideoInfoTag* tag{movie.GetVideoInfoTag()};
  tag->m_type = MediaTypeMovie;
  tag->m_strTitle = "Movie";
  tag->m_strTrailer = TRAILER_PATH;
  tag->m_iDbId = 42;
  tag->m_iFileId = 43;
  tag->m_strFile = "Movie (2007).bluray.iso";
  tag->m_strPath = "/movies/Movie (2007)/";

  // Playing a disc image resolves the tag path to the playlist that was played
  tag->SetFileNameAndPath(PLAYLIST_PATH);

  auto* video{new CStreamDetailVideo()};
  video->m_iWidth = 1920;
  video->m_iHeight = 1080;
  video->m_strCodec = "h264";
  tag->m_streamDetails.AddStream(video);

  CBookmark resumePoint;
  resumePoint.type = CBookmark::RESUME;
  resumePoint.timeInSeconds = 1234.0;
  resumePoint.totalTimeInSeconds = 5678.0;
  resumePoint.playerState = "<player></player>";
  tag->SetResumePoint(resumePoint);

  tag->m_iBookmarkId = 7;
  tag->m_EpBookmark.type = CBookmark::EPISODE;
  tag->m_EpBookmark.timeInSeconds = 12.0;
  tag->m_EpBookmark.totalTimeInSeconds = 34.0;
  tag->m_EpBookmark.playerState = "<player></player>";

  tag->SetPlayCount(3);
  tag->m_lastPlayed = CDateTime(2026, 8, 10, 1, 4, 18);

  return movie;
}
} // namespace

TEST(TestContentUtils, GeneratePlayableTrailerItemUsesTrailerPath)
{
  const CFileItem movie{GetPlayedMovie()};
  const std::unique_ptr<CFileItem> trailer{
      ContentUtils::GeneratePlayableTrailerItem(movie, "Trailer")};

  EXPECT_EQ(trailer->GetPath(), TRAILER_PATH);
  EXPECT_EQ(trailer->GetDynPath(), TRAILER_PATH);
  EXPECT_FALSE(trailer->HasDynPath());
}

TEST(TestContentUtils, GeneratePlayableTrailerItemDoesNotInheritTheMoviePath)
{
  const CFileItem movie{GetPlayedMovie()};
  const std::unique_ptr<CFileItem> trailer{
      ContentUtils::GeneratePlayableTrailerItem(movie, "Trailer")};
  const CVideoInfoTag* tag{trailer->GetVideoInfoTag()};

  // The tag must describe the trailer. If the movie's path is left behind then the resolved
  // bluray:// playlist path is played instead of the trailer.
  EXPECT_EQ(tag->GetPath(), TRAILER_PATH);
  EXPECT_EQ(tag->m_strFileNameAndPath, TRAILER_PATH);
  EXPECT_TRUE(tag->m_strFile.empty());
  EXPECT_TRUE(tag->m_strPath.empty());
}

TEST(TestContentUtils, GeneratePlayableTrailerItemResetsPlaybackState)
{
  const CFileItem movie{GetPlayedMovie()};
  const std::unique_ptr<CFileItem> trailer{
      ContentUtils::GeneratePlayableTrailerItem(movie, "Trailer")};
  const CVideoInfoTag* tag{trailer->GetVideoInfoTag()};

  // The trailer must not resume where the movie was left, nor show as watched
  const CBookmark resumePoint{tag->GetResumePoint()};
  EXPECT_FALSE(resumePoint.IsSet());
  EXPECT_EQ(resumePoint.type, CBookmark::RESUME);
  EXPECT_FALSE(resumePoint.HasSavedPlayerState());

  EXPECT_EQ(tag->m_iBookmarkId, -1);
  EXPECT_FALSE(tag->m_EpBookmark.IsSet());
  EXPECT_EQ(tag->m_EpBookmark.type, CBookmark::EPISODE);
  EXPECT_FALSE(tag->m_EpBookmark.HasSavedPlayerState());

  EXPECT_FALSE(tag->IsPlayCountSet());
  EXPECT_FALSE(tag->m_lastPlayed.IsValid());

  EXPECT_FALSE(tag->HasStreamDetails());

  EXPECT_EQ(tag->m_iDbId, -1);
  EXPECT_EQ(tag->m_iFileId, -1);
}

TEST(TestContentUtils, GeneratePlayableTrailerItemKeepsMovieDetails)
{
  const CFileItem movie{GetPlayedMovie()};
  const std::unique_ptr<CFileItem> trailer{
      ContentUtils::GeneratePlayableTrailerItem(movie, "Trailer")};
  const CVideoInfoTag* tag{trailer->GetVideoInfoTag()};

  EXPECT_EQ(tag->m_strTitle, "Movie (Trailer)");
  EXPECT_EQ(tag->m_type, MediaTypeMovie);
  EXPECT_EQ(trailer->GetArt("poster"), "image://poster.jpg/");
}

TEST(TestContentUtils, GeneratePlayableTrailerItemLeavesTheMovieItemUntouched)
{
  const CFileItem movie{GetPlayedMovie()};
  const std::unique_ptr<CFileItem> trailer{
      ContentUtils::GeneratePlayableTrailerItem(movie, "Trailer")};
  const CVideoInfoTag* tag{movie.GetVideoInfoTag()};

  EXPECT_EQ(movie.GetPath(), MOVIE_PATH);
  EXPECT_EQ(tag->GetPath(), PLAYLIST_PATH);
  EXPECT_EQ(tag->m_strTitle, "Movie");
  EXPECT_EQ(tag->m_iDbId, 42);
  EXPECT_TRUE(tag->GetResumePoint().IsSet());
  EXPECT_EQ(tag->GetPlayCount(), 3);
}
