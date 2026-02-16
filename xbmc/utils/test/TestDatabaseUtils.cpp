/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "dbwrappers/qry_dat.h"
#include "music/MusicDatabase.h"
#include "utils/DatabaseUtils.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "video/VideoDatabase.h"
#include "video/VideoDatabaseColumns.h"

#include <gtest/gtest.h>

class TestDatabaseUtilsHelper
{
public:
  TestDatabaseUtilsHelper()
  {
    album_idAlbum = CMusicDatabase::album_idAlbum;
    album_strAlbum = CMusicDatabase::album_strAlbum;
    album_strArtists = CMusicDatabase::album_strArtists;
    album_strGenres = CMusicDatabase::album_strGenres;
    album_strMoods = CMusicDatabase::album_strMoods;
    album_strReleaseDate = CMusicDatabase::album_strReleaseDate;
    album_strOrigReleaseDate = CMusicDatabase::album_strOrigReleaseDate;
    album_strStyles = CMusicDatabase::album_strStyles;
    album_strThemes = CMusicDatabase::album_strThemes;
    album_strReview = CMusicDatabase::album_strReview;
    album_strLabel = CMusicDatabase::album_strLabel;
    album_strType = CMusicDatabase::album_strType;
    album_fRating = CMusicDatabase::album_fRating;
    album_iVotes = CMusicDatabase::album_iVotes;
    album_iUserrating = CMusicDatabase::album_iUserrating;
    album_dtDateAdded = CMusicDatabase::album_dateAdded;

    song_idSong = CMusicDatabase::song_idSong;
    song_strTitle = CMusicDatabase::song_strTitle;
    song_iTrack = CMusicDatabase::song_iTrack;
    song_iDuration = CMusicDatabase::song_iDuration;
    song_strReleaseDate = CMusicDatabase::song_strReleaseDate;
    song_strOrigReleaseDate = CMusicDatabase::song_strOrigReleaseDate;
    song_strFileName = CMusicDatabase::song_strFileName;
    song_iTimesPlayed = CMusicDatabase::song_iTimesPlayed;
    song_iStartOffset = CMusicDatabase::song_iStartOffset;
    song_iEndOffset = CMusicDatabase::song_iEndOffset;
    song_lastplayed = CMusicDatabase::song_lastplayed;
    song_rating = CMusicDatabase::song_rating;
    song_votes = CMusicDatabase::song_votes;
    song_userrating = CMusicDatabase::song_userrating;
    song_comment = CMusicDatabase::song_comment;
    song_strAlbum = CMusicDatabase::song_strAlbum;
    song_strPath = CMusicDatabase::song_strPath;
    song_strGenres = CMusicDatabase::song_strGenres;
    song_strArtists = CMusicDatabase::song_strArtists;
  }

  int album_idAlbum;
  int album_strAlbum;
  int album_strArtists;
  int album_strGenres;
  int album_strMoods;
  int album_strReleaseDate;
  int album_strOrigReleaseDate;
  int album_strStyles;
  int album_strThemes;
  int album_strReview;
  int album_strLabel;
  int album_strType;
  int album_fRating;
  int album_iVotes;
  int album_iUserrating;
  int album_dtDateAdded;

  int song_idSong;
  int song_strTitle;
  int song_iTrack;
  int song_iDuration;
  int song_strReleaseDate;
  int song_strOrigReleaseDate;
  int song_strFileName;
  int song_iTimesPlayed;
  int song_iStartOffset;
  int song_iEndOffset;
  int song_lastplayed;
  int song_rating;
  int song_votes;
  int song_userrating;
  int song_comment;
  int song_strAlbum;
  int song_strPath;
  int song_strGenres;
  int song_strArtists;
};

TEST(TestDatabaseUtils, GetField_None)
{
  std::string refstr, varstr;

  refstr = "";
  varstr = DatabaseUtils::GetField(Field::NONE, MediaTypeNone, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  varstr = DatabaseUtils::GetField(Field::NONE, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestDatabaseUtils, GetField_MediaTypeAlbum)
{
  std::string refstr, varstr;

  refstr = "albumview.idAlbum";
  varstr = DatabaseUtils::GetField(Field::ID, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strAlbum";
  varstr = DatabaseUtils::GetField(Field::ALBUM, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strArtists";
  varstr = DatabaseUtils::GetField(Field::ARTIST, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strArtists";
  varstr = DatabaseUtils::GetField(Field::ALBUM_ARTIST, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strGenres";
  varstr = DatabaseUtils::GetField(Field::GENRE, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strReleaseDate";
  varstr = DatabaseUtils::GetField(Field::YEAR, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strOrigReleaseDate";
  varstr = DatabaseUtils::GetField(Field::ORIG_YEAR, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strMoods";
  varstr = DatabaseUtils::GetField(Field::MOODS, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strStyles";
  varstr = DatabaseUtils::GetField(Field::STYLES, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strThemes";
  varstr = DatabaseUtils::GetField(Field::THEMES, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strReview";
  varstr = DatabaseUtils::GetField(Field::REVIEW, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strLabel";
  varstr = DatabaseUtils::GetField(Field::MUSIC_LABEL, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strType";
  varstr = DatabaseUtils::GetField(Field::ALBUM_TYPE, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.fRating";
  varstr = DatabaseUtils::GetField(Field::RATING, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.iVotes";
  varstr = DatabaseUtils::GetField(Field::VOTES, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.iUserrating";
  varstr = DatabaseUtils::GetField(Field::USER_RATING, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.dateAdded";
  varstr = DatabaseUtils::GetField(Field::DATE_ADDED, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = DatabaseUtils::GetField(Field::NONE, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strAlbum";
  varstr = DatabaseUtils::GetField(Field::ALBUM, MediaTypeAlbum, DatabaseQueryPart::WHERE);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  varstr = DatabaseUtils::GetField(Field::ALBUM, MediaTypeAlbum, DatabaseQueryPart::ORDER_BY);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestDatabaseUtils, GetField_MediaTypeSong)
{
  std::string refstr, varstr;

  refstr = "songview.idSong";
  varstr = DatabaseUtils::GetField(Field::ID, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.strTitle";
  varstr = DatabaseUtils::GetField(Field::TITLE, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.iTrack";
  varstr = DatabaseUtils::GetField(Field::TRACK_NUMBER, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.iDuration";
  varstr = DatabaseUtils::GetField(Field::TIME, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.strFilename";
  varstr = DatabaseUtils::GetField(Field::FILENAME, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.iTimesPlayed";
  varstr = DatabaseUtils::GetField(Field::PLAYCOUNT, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.iStartOffset";
  varstr = DatabaseUtils::GetField(Field::START_OFFSET, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.iEndOffset";
  varstr = DatabaseUtils::GetField(Field::END_OFFSET, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.lastPlayed";
  varstr = DatabaseUtils::GetField(Field::LAST_PLAYED, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.rating";
  varstr = DatabaseUtils::GetField(Field::RATING, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.votes";
  varstr = DatabaseUtils::GetField(Field::VOTES, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.userrating";
  varstr = DatabaseUtils::GetField(Field::USER_RATING, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.comment";
  varstr = DatabaseUtils::GetField(Field::COMMENT, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.strReleaseDate";
  varstr = DatabaseUtils::GetField(Field::YEAR, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.strOrigReleaseDate";
  varstr = DatabaseUtils::GetField(Field::ORIG_YEAR, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.strAlbum";
  varstr = DatabaseUtils::GetField(Field::ALBUM, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.strPath";
  varstr = DatabaseUtils::GetField(Field::PATH, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.strArtists";
  varstr = DatabaseUtils::GetField(Field::ARTIST, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.strArtists";
  varstr = DatabaseUtils::GetField(Field::ALBUM_ARTIST, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.strGenres";
  varstr = DatabaseUtils::GetField(Field::GENRE, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.dateAdded";
  varstr = DatabaseUtils::GetField(Field::DATE_ADDED, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.strPath";
  varstr = DatabaseUtils::GetField(Field::PATH, MediaTypeSong, DatabaseQueryPart::WHERE);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  varstr = DatabaseUtils::GetField(Field::PATH, MediaTypeSong, DatabaseQueryPart::ORDER_BY);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestDatabaseUtils, GetField_MediaTypeMusicVideo)
{
  std::string refstr, varstr;

  refstr = "musicvideo_view.idMVideo";
  varstr = DatabaseUtils::GetField(Field::ID, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_TITLE);
  varstr = DatabaseUtils::GetField(Field::TITLE, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_RUNTIME);
  varstr = DatabaseUtils::GetField(Field::TIME, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_DIRECTOR);
  varstr = DatabaseUtils::GetField(Field::DIRECTOR, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_STUDIOS);
  varstr = DatabaseUtils::GetField(Field::STUDIO, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_PLOT);
  varstr = DatabaseUtils::GetField(Field::PLOT, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_ALBUM);
  varstr = DatabaseUtils::GetField(Field::ALBUM, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_ARTIST);
  varstr = DatabaseUtils::GetField(Field::ARTIST, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_GENRE);
  varstr = DatabaseUtils::GetField(Field::GENRE, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_TRACK);
  varstr =
      DatabaseUtils::GetField(Field::TRACK_NUMBER, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "musicvideo_view.strFilename";
  varstr = DatabaseUtils::GetField(Field::FILENAME, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "musicvideo_view.strPath";
  varstr = DatabaseUtils::GetField(Field::PATH, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "musicvideo_view.playCount";
  varstr =
      DatabaseUtils::GetField(Field::PLAYCOUNT, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "musicvideo_view.lastPlayed";
  varstr =
      DatabaseUtils::GetField(Field::LAST_PLAYED, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "musicvideo_view.dateAdded";
  varstr =
      DatabaseUtils::GetField(Field::DATE_ADDED, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = DatabaseUtils::GetField(Field::VIDEO_RESOLUTION, MediaTypeMusicVideo,
                                   DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "musicvideo_view.strPath";
  varstr = DatabaseUtils::GetField(Field::PATH, MediaTypeMusicVideo, DatabaseQueryPart::WHERE);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "musicvideo_view.strPath";
  varstr = DatabaseUtils::GetField(Field::PATH, MediaTypeMusicVideo, DatabaseQueryPart::ORDER_BY);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "musicvideo_view.userrating";
  varstr =
      DatabaseUtils::GetField(Field::USER_RATING, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestDatabaseUtils, GetField_MediaTypeMovie)
{
  std::string refstr, varstr;

  refstr = "movie_view.idMovie";
  varstr = DatabaseUtils::GetField(Field::ID, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_TITLE);
  varstr = DatabaseUtils::GetField(Field::TITLE, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("CASE WHEN length(movie_view.c{:02}) > 0 THEN movie_view.c{:02} "
                               "ELSE movie_view.c{:02} END",
                               VIDEODB_ID_SORTTITLE, VIDEODB_ID_SORTTITLE, VIDEODB_ID_TITLE);
  varstr = DatabaseUtils::GetField(Field::TITLE, MediaTypeMovie, DatabaseQueryPart::ORDER_BY);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_PLOT);
  varstr = DatabaseUtils::GetField(Field::PLOT, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_PLOTOUTLINE);
  varstr = DatabaseUtils::GetField(Field::PLOT_OUTLINE, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_TAGLINE);
  varstr = DatabaseUtils::GetField(Field::TAGLINE, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "movie_view.votes";
  varstr = DatabaseUtils::GetField(Field::VOTES, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "movie_view.rating";
  varstr = DatabaseUtils::GetField(Field::RATING, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_CREDITS);
  varstr = DatabaseUtils::GetField(Field::WRITER, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_SORTTITLE);
  varstr = DatabaseUtils::GetField(Field::SORT_TITLE, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_RUNTIME);
  varstr = DatabaseUtils::GetField(Field::TIME, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_MPAA);
  varstr = DatabaseUtils::GetField(Field::MPAA, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_TOP250);
  varstr = DatabaseUtils::GetField(Field::TOP250, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_GENRE);
  varstr = DatabaseUtils::GetField(Field::GENRE, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_DIRECTOR);
  varstr = DatabaseUtils::GetField(Field::DIRECTOR, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_STUDIOS);
  varstr = DatabaseUtils::GetField(Field::STUDIO, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_TRAILER);
  varstr = DatabaseUtils::GetField(Field::TRAILER, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_COUNTRY);
  varstr = DatabaseUtils::GetField(Field::COUNTRY, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "movie_view.strFilename";
  varstr = DatabaseUtils::GetField(Field::FILENAME, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "movie_view.strPath";
  varstr = DatabaseUtils::GetField(Field::PATH, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "movie_view.playCount";
  varstr = DatabaseUtils::GetField(Field::PLAYCOUNT, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "movie_view.lastPlayed";
  varstr = DatabaseUtils::GetField(Field::LAST_PLAYED, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "movie_view.dateAdded";
  varstr = DatabaseUtils::GetField(Field::DATE_ADDED, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "movie_view.userrating";
  varstr = DatabaseUtils::GetField(Field::USER_RATING, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = DatabaseUtils::GetField(Field::RANDOM, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestDatabaseUtils, GetField_MediaTypeTvShow)
{
  std::string refstr, varstr;

  refstr = "tvshow_view.idShow";
  varstr = DatabaseUtils::GetField(Field::ID, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr =
      StringUtils::Format("CASE WHEN length(tvshow_view.c{:02}) > 0 THEN tvshow_view.c{:02} "
                          "ELSE tvshow_view.c{:02} END",
                          VIDEODB_ID_TV_SORTTITLE, VIDEODB_ID_TV_SORTTITLE, VIDEODB_ID_TV_TITLE);
  varstr = DatabaseUtils::GetField(Field::TITLE, MediaTypeTvShow, DatabaseQueryPart::ORDER_BY);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_TITLE);
  varstr = DatabaseUtils::GetField(Field::TITLE, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_PLOT);
  varstr = DatabaseUtils::GetField(Field::PLOT, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_STATUS);
  varstr =
      DatabaseUtils::GetField(Field::TVSHOW_STATUS, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "tvshow_view.votes";
  varstr = DatabaseUtils::GetField(Field::VOTES, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "tvshow_view.rating";
  varstr = DatabaseUtils::GetField(Field::RATING, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_PREMIERED);
  varstr = DatabaseUtils::GetField(Field::YEAR, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_GENRE);
  varstr = DatabaseUtils::GetField(Field::GENRE, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_MPAA);
  varstr = DatabaseUtils::GetField(Field::MPAA, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_STUDIOS);
  varstr = DatabaseUtils::GetField(Field::STUDIO, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_SORTTITLE);
  varstr = DatabaseUtils::GetField(Field::SORT_TITLE, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "tvshow_view.strPath";
  varstr = DatabaseUtils::GetField(Field::PATH, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "tvshow_view.dateAdded";
  varstr = DatabaseUtils::GetField(Field::DATE_ADDED, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "tvshow_view.totalSeasons";
  varstr = DatabaseUtils::GetField(Field::SEASON, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "tvshow_view.totalCount";
  varstr = DatabaseUtils::GetField(Field::NUMBER_OF_EPISODES, MediaTypeTvShow,
                                   DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "tvshow_view.watchedcount";
  varstr = DatabaseUtils::GetField(Field::NUMBER_OF_WATCHED_EPISODES, MediaTypeTvShow,
                                   DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "tvshow_view.userrating";
  varstr = DatabaseUtils::GetField(Field::USER_RATING, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = DatabaseUtils::GetField(Field::RANDOM, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestDatabaseUtils, GetField_MediaTypeEpisode)
{
  std::string refstr, varstr;

  refstr = "episode_view.idEpisode";
  varstr = DatabaseUtils::GetField(Field::ID, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_TITLE);
  varstr = DatabaseUtils::GetField(Field::TITLE, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_PLOT);
  varstr = DatabaseUtils::GetField(Field::PLOT, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "episode_view.votes";
  varstr = DatabaseUtils::GetField(Field::VOTES, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "episode_view.rating";
  varstr = DatabaseUtils::GetField(Field::RATING, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_CREDITS);
  varstr = DatabaseUtils::GetField(Field::WRITER, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_AIRED);
  varstr = DatabaseUtils::GetField(Field::AIR_DATE, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_RUNTIME);
  varstr = DatabaseUtils::GetField(Field::TIME, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_DIRECTOR);
  varstr = DatabaseUtils::GetField(Field::DIRECTOR, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_SEASON);
  varstr = DatabaseUtils::GetField(Field::SEASON, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_EPISODE);
  varstr =
      DatabaseUtils::GetField(Field::EPISODE_NUMBER, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "episode_view.strFilename";
  varstr = DatabaseUtils::GetField(Field::FILENAME, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "episode_view.strPath";
  varstr = DatabaseUtils::GetField(Field::PATH, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "episode_view.playCount";
  varstr = DatabaseUtils::GetField(Field::PLAYCOUNT, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "episode_view.lastPlayed";
  varstr = DatabaseUtils::GetField(Field::LAST_PLAYED, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "episode_view.dateAdded";
  varstr = DatabaseUtils::GetField(Field::DATE_ADDED, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "episode_view.strTitle";
  varstr =
      DatabaseUtils::GetField(Field::TVSHOW_TITLE, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "episode_view.premiered";
  varstr = DatabaseUtils::GetField(Field::YEAR, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "episode_view.mpaa";
  varstr = DatabaseUtils::GetField(Field::MPAA, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "episode_view.strStudio";
  varstr = DatabaseUtils::GetField(Field::STUDIO, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "episode_view.userrating";
  varstr = DatabaseUtils::GetField(Field::USER_RATING, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = DatabaseUtils::GetField(Field::RANDOM, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestDatabaseUtils, GetField_FieldRandom)
{
  std::string refstr, varstr;

  refstr = "";
  varstr = DatabaseUtils::GetField(Field::RANDOM, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = DatabaseUtils::GetField(Field::RANDOM, MediaTypeEpisode, DatabaseQueryPart::WHERE);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "RANDOM()";
  varstr = DatabaseUtils::GetField(Field::RANDOM, MediaTypeEpisode, DatabaseQueryPart::ORDER_BY);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestDatabaseUtils, GetFieldIndex_None)
{
  int refindex, varindex;

  refindex = -1;
  varindex = DatabaseUtils::GetFieldIndex(Field::RANDOM, MediaTypeNone);
  EXPECT_EQ(refindex, varindex);

  varindex = DatabaseUtils::GetFieldIndex(Field::NONE, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);
}

//! @todo Should enums in CMusicDatabase be made public instead?
TEST(TestDatabaseUtils, GetFieldIndex_MediaTypeAlbum)
{
  int refindex, varindex;
  TestDatabaseUtilsHelper a;

  refindex = a.album_idAlbum;
  varindex = DatabaseUtils::GetFieldIndex(Field::ID, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_strAlbum;
  varindex = DatabaseUtils::GetFieldIndex(Field::ALBUM, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_strArtists;
  varindex = DatabaseUtils::GetFieldIndex(Field::ARTIST, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_strArtists;
  varindex = DatabaseUtils::GetFieldIndex(Field::ALBUM_ARTIST, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_strGenres;
  varindex = DatabaseUtils::GetFieldIndex(Field::GENRE, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_strReleaseDate;
  varindex = DatabaseUtils::GetFieldIndex(Field::YEAR, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_strOrigReleaseDate;
  varindex = DatabaseUtils::GetFieldIndex(Field::ORIG_YEAR, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_strMoods;
  varindex = DatabaseUtils::GetFieldIndex(Field::MOODS, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_strStyles;
  varindex = DatabaseUtils::GetFieldIndex(Field::STYLES, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_strThemes;
  varindex = DatabaseUtils::GetFieldIndex(Field::THEMES, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_strReview;
  varindex = DatabaseUtils::GetFieldIndex(Field::REVIEW, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_strLabel;
  varindex = DatabaseUtils::GetFieldIndex(Field::MUSIC_LABEL, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_strType;
  varindex = DatabaseUtils::GetFieldIndex(Field::ALBUM_TYPE, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_fRating;
  varindex = DatabaseUtils::GetFieldIndex(Field::RATING, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_dtDateAdded;
  varindex = DatabaseUtils::GetFieldIndex(Field::DATE_ADDED, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = -1;
  varindex = DatabaseUtils::GetFieldIndex(Field::RANDOM, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);
}

TEST(TestDatabaseUtils, GetFieldIndex_MediaTypeSong)
{
  int refindex, varindex;
  TestDatabaseUtilsHelper a;

  refindex = a.song_idSong;
  varindex = DatabaseUtils::GetFieldIndex(Field::ID, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_strTitle;
  varindex = DatabaseUtils::GetFieldIndex(Field::TITLE, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_iTrack;
  varindex = DatabaseUtils::GetFieldIndex(Field::TRACK_NUMBER, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_iDuration;
  varindex = DatabaseUtils::GetFieldIndex(Field::TIME, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_strReleaseDate;
  varindex = DatabaseUtils::GetFieldIndex(Field::YEAR, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_strFileName;
  varindex = DatabaseUtils::GetFieldIndex(Field::FILENAME, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_iTimesPlayed;
  varindex = DatabaseUtils::GetFieldIndex(Field::PLAYCOUNT, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_iStartOffset;
  varindex = DatabaseUtils::GetFieldIndex(Field::START_OFFSET, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_iEndOffset;
  varindex = DatabaseUtils::GetFieldIndex(Field::END_OFFSET, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_lastplayed;
  varindex = DatabaseUtils::GetFieldIndex(Field::LAST_PLAYED, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_rating;
  varindex = DatabaseUtils::GetFieldIndex(Field::RATING, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_votes;
  varindex = DatabaseUtils::GetFieldIndex(Field::VOTES, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_userrating;
  varindex = DatabaseUtils::GetFieldIndex(Field::USER_RATING, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_comment;
  varindex = DatabaseUtils::GetFieldIndex(Field::COMMENT, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_strAlbum;
  varindex = DatabaseUtils::GetFieldIndex(Field::ALBUM, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_strPath;
  varindex = DatabaseUtils::GetFieldIndex(Field::PATH, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_strArtists;
  varindex = DatabaseUtils::GetFieldIndex(Field::ARTIST, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_strGenres;
  varindex = DatabaseUtils::GetFieldIndex(Field::GENRE, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = -1;
  varindex = DatabaseUtils::GetFieldIndex(Field::RANDOM, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);
}

TEST(TestDatabaseUtils, GetFieldIndex_MediaTypeMusicVideo)
{
  int refindex, varindex;

  refindex = 0;
  varindex = DatabaseUtils::GetFieldIndex(Field::ID, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_MUSICVIDEO_TITLE + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::TITLE, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_MUSICVIDEO_RUNTIME + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::TIME, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_MUSICVIDEO_DIRECTOR + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::DIRECTOR, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_MUSICVIDEO_STUDIOS + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::STUDIO, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_MUSICVIDEO_PLOT + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::PLOT, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_MUSICVIDEO_ALBUM + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::ALBUM, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_MUSICVIDEO_ARTIST + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::ARTIST, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_MUSICVIDEO_GENRE + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::GENRE, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_MUSICVIDEO_TRACK + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::TRACK_NUMBER, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MUSICVIDEO_FILE;
  varindex = DatabaseUtils::GetFieldIndex(Field::FILENAME, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MUSICVIDEO_PATH;
  varindex = DatabaseUtils::GetFieldIndex(Field::PATH, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MUSICVIDEO_PLAYCOUNT;
  varindex = DatabaseUtils::GetFieldIndex(Field::PLAYCOUNT, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MUSICVIDEO_LASTPLAYED;
  varindex = DatabaseUtils::GetFieldIndex(Field::LAST_PLAYED, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MUSICVIDEO_DATEADDED;
  varindex = DatabaseUtils::GetFieldIndex(Field::DATE_ADDED, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MUSICVIDEO_USER_RATING;
  varindex = DatabaseUtils::GetFieldIndex(Field::USER_RATING, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MUSICVIDEO_PREMIERED;
  varindex = DatabaseUtils::GetFieldIndex(Field::YEAR, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = -1;
  varindex = DatabaseUtils::GetFieldIndex(Field::RANDOM, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);
}

TEST(TestDatabaseUtils, GetFieldIndex_MediaTypeMovie)
{
  int refindex, varindex;

  refindex = 0;
  varindex = DatabaseUtils::GetFieldIndex(Field::ID, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_TITLE + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::TITLE, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_SORTTITLE + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::SORT_TITLE, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_PLOT + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::PLOT, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_PLOTOUTLINE + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::PLOT_OUTLINE, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_TAGLINE + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::TAGLINE, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_CREDITS + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::WRITER, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_RUNTIME + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::TIME, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_MPAA + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::MPAA, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_TOP250 + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::TOP250, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_GENRE + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::GENRE, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_DIRECTOR + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::DIRECTOR, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_STUDIOS + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::STUDIO, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_TRAILER + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::TRAILER, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_COUNTRY + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::COUNTRY, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MOVIE_FILE + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::FILENAME, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MOVIE_PATH;
  varindex = DatabaseUtils::GetFieldIndex(Field::PATH, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MOVIE_PLAYCOUNT;
  varindex = DatabaseUtils::GetFieldIndex(Field::PLAYCOUNT, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MOVIE_LASTPLAYED;
  varindex = DatabaseUtils::GetFieldIndex(Field::LAST_PLAYED, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MOVIE_DATEADDED;
  varindex = DatabaseUtils::GetFieldIndex(Field::DATE_ADDED, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MOVIE_USER_RATING;
  varindex = DatabaseUtils::GetFieldIndex(Field::USER_RATING, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MOVIE_VOTES;
  varindex = DatabaseUtils::GetFieldIndex(Field::VOTES, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MOVIE_RATING;
  varindex = DatabaseUtils::GetFieldIndex(Field::RATING, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MOVIE_PREMIERED;
  varindex = DatabaseUtils::GetFieldIndex(Field::YEAR, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = -1;
  varindex = DatabaseUtils::GetFieldIndex(Field::RANDOM, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);
}

TEST(TestDatabaseUtils, GetFieldIndex_MediaTypeTvShow)
{
  int refindex, varindex;

  refindex = 0;
  varindex = DatabaseUtils::GetFieldIndex(Field::ID, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_TV_TITLE + 1;
  varindex = DatabaseUtils::GetFieldIndex(Field::TITLE, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_TV_SORTTITLE + 1;
  varindex = DatabaseUtils::GetFieldIndex(Field::SORT_TITLE, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_TV_PLOT + 1;
  varindex = DatabaseUtils::GetFieldIndex(Field::PLOT, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_TV_STATUS + 1;
  varindex = DatabaseUtils::GetFieldIndex(Field::TVSHOW_STATUS, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_TV_PREMIERED + 1;
  varindex = DatabaseUtils::GetFieldIndex(Field::YEAR, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_TV_GENRE + 1;
  varindex = DatabaseUtils::GetFieldIndex(Field::GENRE, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_TV_MPAA + 1;
  varindex = DatabaseUtils::GetFieldIndex(Field::MPAA, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_TV_STUDIOS + 1;
  varindex = DatabaseUtils::GetFieldIndex(Field::STUDIO, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_TVSHOW_PATH;
  varindex = DatabaseUtils::GetFieldIndex(Field::PATH, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_TVSHOW_DATEADDED;
  varindex = DatabaseUtils::GetFieldIndex(Field::DATE_ADDED, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_TVSHOW_NUM_EPISODES;
  varindex = DatabaseUtils::GetFieldIndex(Field::NUMBER_OF_EPISODES, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_TVSHOW_NUM_WATCHED;
  varindex = DatabaseUtils::GetFieldIndex(Field::NUMBER_OF_WATCHED_EPISODES, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_TVSHOW_NUM_SEASONS;
  varindex = DatabaseUtils::GetFieldIndex(Field::SEASON, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_TVSHOW_USER_RATING;
  varindex = DatabaseUtils::GetFieldIndex(Field::USER_RATING, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_TVSHOW_VOTES;
  varindex = DatabaseUtils::GetFieldIndex(Field::VOTES, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_TVSHOW_RATING;
  varindex = DatabaseUtils::GetFieldIndex(Field::RATING, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = -1;
  varindex = DatabaseUtils::GetFieldIndex(Field::RANDOM, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);
}

TEST(TestDatabaseUtils, GetFieldIndex_MediaTypeEpisode)
{
  int refindex, varindex;

  refindex = 0;
  varindex = DatabaseUtils::GetFieldIndex(Field::ID, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_EPISODE_TITLE + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::TITLE, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_EPISODE_PLOT + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::PLOT, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_EPISODE_CREDITS + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::WRITER, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_EPISODE_AIRED + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::AIR_DATE, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_EPISODE_RUNTIME + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::TIME, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_EPISODE_DIRECTOR + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::DIRECTOR, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_EPISODE_SEASON + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::SEASON, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_EPISODE_EPISODE + 2;
  varindex = DatabaseUtils::GetFieldIndex(Field::EPISODE_NUMBER, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_EPISODE_FILE;
  varindex = DatabaseUtils::GetFieldIndex(Field::FILENAME, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_EPISODE_PATH;
  varindex = DatabaseUtils::GetFieldIndex(Field::PATH, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_EPISODE_PLAYCOUNT;
  varindex = DatabaseUtils::GetFieldIndex(Field::PLAYCOUNT, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_EPISODE_LASTPLAYED;
  varindex = DatabaseUtils::GetFieldIndex(Field::LAST_PLAYED, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_EPISODE_DATEADDED;
  varindex = DatabaseUtils::GetFieldIndex(Field::DATE_ADDED, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_EPISODE_TVSHOW_NAME;
  varindex = DatabaseUtils::GetFieldIndex(Field::TVSHOW_TITLE, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_EPISODE_TVSHOW_STUDIO;
  varindex = DatabaseUtils::GetFieldIndex(Field::STUDIO, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_EPISODE_TVSHOW_AIRED;
  varindex = DatabaseUtils::GetFieldIndex(Field::YEAR, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_EPISODE_TVSHOW_MPAA;
  varindex = DatabaseUtils::GetFieldIndex(Field::MPAA, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_EPISODE_USER_RATING;
  varindex = DatabaseUtils::GetFieldIndex(Field::USER_RATING, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_EPISODE_VOTES;
  varindex = DatabaseUtils::GetFieldIndex(Field::VOTES, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_EPISODE_RATING;
  varindex = DatabaseUtils::GetFieldIndex(Field::RATING, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = -1;
  varindex = DatabaseUtils::GetFieldIndex(Field::RANDOM, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);
}

TEST(TestDatabaseUtils, GetSelectFields)
{
  Fields fields;
  FieldList fieldlist;

  EXPECT_FALSE(DatabaseUtils::GetSelectFields(fields, MediaTypeAlbum,
                                              fieldlist));

  fields = {
      Field::ID, Field::GENRE, Field::ALBUM, Field::ARTIST, Field::TITLE,
  };
  EXPECT_FALSE(DatabaseUtils::GetSelectFields(fields, MediaTypeNone,
                                              fieldlist));
  EXPECT_TRUE(DatabaseUtils::GetSelectFields(fields, MediaTypeAlbum,
                                             fieldlist));
  EXPECT_FALSE(fieldlist.empty());
}

TEST(TestDatabaseUtils, GetFieldValue)
{
  CVariant v_null, v_string;
  dbiplus::field_value f_null, f_string("test");

  f_null.set_isNull();
  EXPECT_TRUE(DatabaseUtils::GetFieldValue(f_null, v_null));
  EXPECT_TRUE(v_null.isNull());

  EXPECT_TRUE(DatabaseUtils::GetFieldValue(f_string, v_string));
  EXPECT_FALSE(v_string.isNull());
  EXPECT_TRUE(v_string.isString());
}

//! @todo Need some way to test this function
// TEST(TestDatabaseUtils, GetDatabaseResults)
// {
//   static bool GetDatabaseResults(MediaType mediaType, const FieldList &fields,
//                                  const std::unique_ptr<dbiplus::Dataset> &dataset,
//                                  DatabaseResults &results);
// }

TEST(TestDatabaseUtils, BuildLimitClause)
{
  std::string a = DatabaseUtils::BuildLimitClause(100);
  EXPECT_STREQ(" LIMIT 100", a.c_str());
}

// class DatabaseUtils
// {
// public:
//
//
//   static std::string BuildLimitClause(int end, int start = 0);
// };
