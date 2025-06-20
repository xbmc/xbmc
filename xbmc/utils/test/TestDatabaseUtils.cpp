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
  varstr = DatabaseUtils::GetField(FieldNone, MediaTypeNone, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  varstr = DatabaseUtils::GetField(FieldNone, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestDatabaseUtils, GetField_MediaTypeAlbum)
{
  std::string refstr, varstr;

  refstr = "albumview.idAlbum";
  varstr = DatabaseUtils::GetField(FieldId, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strAlbum";
  varstr = DatabaseUtils::GetField(FieldAlbum, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strArtists";
  varstr = DatabaseUtils::GetField(FieldArtist, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strArtists";
  varstr = DatabaseUtils::GetField(FieldAlbumArtist, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strGenres";
  varstr = DatabaseUtils::GetField(FieldGenre, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strReleaseDate";
  varstr = DatabaseUtils::GetField(FieldYear, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strOrigReleaseDate";
  varstr = DatabaseUtils::GetField(FieldOrigYear, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strMoods";
  varstr = DatabaseUtils::GetField(FieldMoods, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strStyles";
  varstr = DatabaseUtils::GetField(FieldStyles, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strThemes";
  varstr = DatabaseUtils::GetField(FieldThemes, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strReview";
  varstr = DatabaseUtils::GetField(FieldReview, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strLabel";
  varstr = DatabaseUtils::GetField(FieldMusicLabel, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strType";
  varstr = DatabaseUtils::GetField(FieldAlbumType, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.fRating";
  varstr = DatabaseUtils::GetField(FieldRating, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.iVotes";
  varstr = DatabaseUtils::GetField(FieldVotes, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.iUserrating";
  varstr = DatabaseUtils::GetField(FieldUserRating, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.dateAdded";
  varstr = DatabaseUtils::GetField(FieldDateAdded, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = DatabaseUtils::GetField(FieldNone, MediaTypeAlbum, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "albumview.strAlbum";
  varstr = DatabaseUtils::GetField(FieldAlbum, MediaTypeAlbum, DatabaseQueryPart::WHERE);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  varstr = DatabaseUtils::GetField(FieldAlbum, MediaTypeAlbum, DatabaseQueryPart::ORDER_BY);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestDatabaseUtils, GetField_MediaTypeSong)
{
  std::string refstr, varstr;

  refstr = "songview.idSong";
  varstr = DatabaseUtils::GetField(FieldId, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.strTitle";
  varstr = DatabaseUtils::GetField(FieldTitle, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.iTrack";
  varstr = DatabaseUtils::GetField(FieldTrackNumber, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.iDuration";
  varstr = DatabaseUtils::GetField(FieldTime, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.strFilename";
  varstr = DatabaseUtils::GetField(FieldFilename, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.iTimesPlayed";
  varstr = DatabaseUtils::GetField(FieldPlaycount, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.iStartOffset";
  varstr = DatabaseUtils::GetField(FieldStartOffset, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.iEndOffset";
  varstr = DatabaseUtils::GetField(FieldEndOffset, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.lastPlayed";
  varstr = DatabaseUtils::GetField(FieldLastPlayed, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.rating";
  varstr = DatabaseUtils::GetField(FieldRating, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.votes";
  varstr = DatabaseUtils::GetField(FieldVotes, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.userrating";
  varstr = DatabaseUtils::GetField(FieldUserRating, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.comment";
  varstr = DatabaseUtils::GetField(FieldComment, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.strReleaseDate";
  varstr = DatabaseUtils::GetField(FieldYear, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.strOrigReleaseDate";
  varstr = DatabaseUtils::GetField(FieldOrigYear, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.strAlbum";
  varstr = DatabaseUtils::GetField(FieldAlbum, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.strPath";
  varstr = DatabaseUtils::GetField(FieldPath, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.strArtists";
  varstr = DatabaseUtils::GetField(FieldArtist, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.strArtists";
  varstr = DatabaseUtils::GetField(FieldAlbumArtist, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.strGenres";
  varstr = DatabaseUtils::GetField(FieldGenre, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.dateAdded";
  varstr = DatabaseUtils::GetField(FieldDateAdded, MediaTypeSong, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "songview.strPath";
  varstr = DatabaseUtils::GetField(FieldPath, MediaTypeSong, DatabaseQueryPart::WHERE);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  varstr = DatabaseUtils::GetField(FieldPath, MediaTypeSong, DatabaseQueryPart::ORDER_BY);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestDatabaseUtils, GetField_MediaTypeMusicVideo)
{
  std::string refstr, varstr;

  refstr = "musicvideo_view.idMVideo";
  varstr = DatabaseUtils::GetField(FieldId, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_TITLE);
  varstr = DatabaseUtils::GetField(FieldTitle, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_RUNTIME);
  varstr = DatabaseUtils::GetField(FieldTime, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_DIRECTOR);
  varstr = DatabaseUtils::GetField(FieldDirector, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_STUDIOS);
  varstr = DatabaseUtils::GetField(FieldStudio, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_PLOT);
  varstr = DatabaseUtils::GetField(FieldPlot, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_ALBUM);
  varstr = DatabaseUtils::GetField(FieldAlbum, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_ARTIST);
  varstr = DatabaseUtils::GetField(FieldArtist, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_GENRE);
  varstr = DatabaseUtils::GetField(FieldGenre, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_TRACK);
  varstr =
      DatabaseUtils::GetField(FieldTrackNumber, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "musicvideo_view.strFilename";
  varstr = DatabaseUtils::GetField(FieldFilename, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "musicvideo_view.strPath";
  varstr = DatabaseUtils::GetField(FieldPath, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "musicvideo_view.playCount";
  varstr = DatabaseUtils::GetField(FieldPlaycount, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "musicvideo_view.lastPlayed";
  varstr = DatabaseUtils::GetField(FieldLastPlayed, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "musicvideo_view.dateAdded";
  varstr = DatabaseUtils::GetField(FieldDateAdded, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr =
      DatabaseUtils::GetField(FieldVideoResolution, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "musicvideo_view.strPath";
  varstr = DatabaseUtils::GetField(FieldPath, MediaTypeMusicVideo, DatabaseQueryPart::WHERE);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "musicvideo_view.strPath";
  varstr = DatabaseUtils::GetField(FieldPath, MediaTypeMusicVideo, DatabaseQueryPart::ORDER_BY);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "musicvideo_view.userrating";
  varstr = DatabaseUtils::GetField(FieldUserRating, MediaTypeMusicVideo, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestDatabaseUtils, GetField_MediaTypeMovie)
{
  std::string refstr, varstr;

  refstr = "movie_view.idMovie";
  varstr = DatabaseUtils::GetField(FieldId, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_TITLE);
  varstr = DatabaseUtils::GetField(FieldTitle, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("CASE WHEN length(movie_view.c{:02}) > 0 THEN movie_view.c{:02} "
                               "ELSE movie_view.c{:02} END",
                               VIDEODB_ID_SORTTITLE, VIDEODB_ID_SORTTITLE, VIDEODB_ID_TITLE);
  varstr = DatabaseUtils::GetField(FieldTitle, MediaTypeMovie, DatabaseQueryPart::ORDER_BY);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_PLOT);
  varstr = DatabaseUtils::GetField(FieldPlot, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_PLOTOUTLINE);
  varstr = DatabaseUtils::GetField(FieldPlotOutline, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_TAGLINE);
  varstr = DatabaseUtils::GetField(FieldTagline, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "movie_view.votes";
  varstr = DatabaseUtils::GetField(FieldVotes, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "movie_view.rating";
  varstr = DatabaseUtils::GetField(FieldRating, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_CREDITS);
  varstr = DatabaseUtils::GetField(FieldWriter, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_SORTTITLE);
  varstr = DatabaseUtils::GetField(FieldSortTitle, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_RUNTIME);
  varstr = DatabaseUtils::GetField(FieldTime, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_MPAA);
  varstr = DatabaseUtils::GetField(FieldMPAA, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_TOP250);
  varstr = DatabaseUtils::GetField(FieldTop250, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_GENRE);
  varstr = DatabaseUtils::GetField(FieldGenre, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_DIRECTOR);
  varstr = DatabaseUtils::GetField(FieldDirector, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_STUDIOS);
  varstr = DatabaseUtils::GetField(FieldStudio, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_TRAILER);
  varstr = DatabaseUtils::GetField(FieldTrailer, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_COUNTRY);
  varstr = DatabaseUtils::GetField(FieldCountry, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "movie_view.strFilename";
  varstr = DatabaseUtils::GetField(FieldFilename, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "movie_view.strPath";
  varstr = DatabaseUtils::GetField(FieldPath, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "movie_view.playCount";
  varstr = DatabaseUtils::GetField(FieldPlaycount, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "movie_view.lastPlayed";
  varstr = DatabaseUtils::GetField(FieldLastPlayed, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "movie_view.dateAdded";
  varstr = DatabaseUtils::GetField(FieldDateAdded, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "movie_view.userrating";
  varstr = DatabaseUtils::GetField(FieldUserRating, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = DatabaseUtils::GetField(FieldRandom, MediaTypeMovie, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestDatabaseUtils, GetField_MediaTypeTvShow)
{
  std::string refstr, varstr;

  refstr = "tvshow_view.idShow";
  varstr = DatabaseUtils::GetField(FieldId, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr =
      StringUtils::Format("CASE WHEN length(tvshow_view.c{:02}) > 0 THEN tvshow_view.c{:02} "
                          "ELSE tvshow_view.c{:02} END",
                          VIDEODB_ID_TV_SORTTITLE, VIDEODB_ID_TV_SORTTITLE, VIDEODB_ID_TV_TITLE);
  varstr = DatabaseUtils::GetField(FieldTitle, MediaTypeTvShow, DatabaseQueryPart::ORDER_BY);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_TITLE);
  varstr = DatabaseUtils::GetField(FieldTitle, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_PLOT);
  varstr = DatabaseUtils::GetField(FieldPlot, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_STATUS);
  varstr = DatabaseUtils::GetField(FieldTvShowStatus, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "tvshow_view.votes";
  varstr = DatabaseUtils::GetField(FieldVotes, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "tvshow_view.rating";
  varstr = DatabaseUtils::GetField(FieldRating, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_PREMIERED);
  varstr = DatabaseUtils::GetField(FieldYear, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_GENRE);
  varstr = DatabaseUtils::GetField(FieldGenre, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_MPAA);
  varstr = DatabaseUtils::GetField(FieldMPAA, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_STUDIOS);
  varstr = DatabaseUtils::GetField(FieldStudio, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_SORTTITLE);
  varstr = DatabaseUtils::GetField(FieldSortTitle, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "tvshow_view.strPath";
  varstr = DatabaseUtils::GetField(FieldPath, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "tvshow_view.dateAdded";
  varstr = DatabaseUtils::GetField(FieldDateAdded, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "tvshow_view.totalSeasons";
  varstr = DatabaseUtils::GetField(FieldSeason, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "tvshow_view.totalCount";
  varstr =
      DatabaseUtils::GetField(FieldNumberOfEpisodes, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "tvshow_view.watchedcount";
  varstr = DatabaseUtils::GetField(FieldNumberOfWatchedEpisodes, MediaTypeTvShow,
                                   DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "tvshow_view.userrating";
  varstr = DatabaseUtils::GetField(FieldUserRating, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = DatabaseUtils::GetField(FieldRandom, MediaTypeTvShow, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestDatabaseUtils, GetField_MediaTypeEpisode)
{
  std::string refstr, varstr;

  refstr = "episode_view.idEpisode";
  varstr = DatabaseUtils::GetField(FieldId, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_TITLE);
  varstr = DatabaseUtils::GetField(FieldTitle, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_PLOT);
  varstr = DatabaseUtils::GetField(FieldPlot, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "episode_view.votes";
  varstr = DatabaseUtils::GetField(FieldVotes, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "episode_view.rating";
  varstr = DatabaseUtils::GetField(FieldRating, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_CREDITS);
  varstr = DatabaseUtils::GetField(FieldWriter, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_AIRED);
  varstr = DatabaseUtils::GetField(FieldAirDate, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_RUNTIME);
  varstr = DatabaseUtils::GetField(FieldTime, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_DIRECTOR);
  varstr = DatabaseUtils::GetField(FieldDirector, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_SEASON);
  varstr = DatabaseUtils::GetField(FieldSeason, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_EPISODE);
  varstr = DatabaseUtils::GetField(FieldEpisodeNumber, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "episode_view.strFilename";
  varstr = DatabaseUtils::GetField(FieldFilename, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "episode_view.strPath";
  varstr = DatabaseUtils::GetField(FieldPath, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "episode_view.playCount";
  varstr = DatabaseUtils::GetField(FieldPlaycount, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "episode_view.lastPlayed";
  varstr = DatabaseUtils::GetField(FieldLastPlayed, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "episode_view.dateAdded";
  varstr = DatabaseUtils::GetField(FieldDateAdded, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "episode_view.strTitle";
  varstr = DatabaseUtils::GetField(FieldTvShowTitle, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "episode_view.premiered";
  varstr = DatabaseUtils::GetField(FieldYear, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "episode_view.mpaa";
  varstr = DatabaseUtils::GetField(FieldMPAA, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "episode_view.strStudio";
  varstr = DatabaseUtils::GetField(FieldStudio, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "episode_view.userrating";
  varstr = DatabaseUtils::GetField(FieldUserRating, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = DatabaseUtils::GetField(FieldRandom, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestDatabaseUtils, GetField_FieldRandom)
{
  std::string refstr, varstr;

  refstr = "";
  varstr = DatabaseUtils::GetField(FieldRandom, MediaTypeEpisode, DatabaseQueryPart::SELECT);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = DatabaseUtils::GetField(FieldRandom, MediaTypeEpisode, DatabaseQueryPart::WHERE);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "RANDOM()";
  varstr = DatabaseUtils::GetField(FieldRandom, MediaTypeEpisode, DatabaseQueryPart::ORDER_BY);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestDatabaseUtils, GetFieldIndex_None)
{
  int refindex, varindex;

  refindex = -1;
  varindex = DatabaseUtils::GetFieldIndex(FieldRandom, MediaTypeNone);
  EXPECT_EQ(refindex, varindex);

  varindex = DatabaseUtils::GetFieldIndex(FieldNone, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);
}

//! @todo Should enums in CMusicDatabase be made public instead?
TEST(TestDatabaseUtils, GetFieldIndex_MediaTypeAlbum)
{
  int refindex, varindex;
  TestDatabaseUtilsHelper a;

  refindex = a.album_idAlbum;
  varindex = DatabaseUtils::GetFieldIndex(FieldId, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_strAlbum;
  varindex = DatabaseUtils::GetFieldIndex(FieldAlbum, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_strArtists;
  varindex = DatabaseUtils::GetFieldIndex(FieldArtist, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_strArtists;
  varindex = DatabaseUtils::GetFieldIndex(FieldAlbumArtist, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_strGenres;
  varindex = DatabaseUtils::GetFieldIndex(FieldGenre, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_strReleaseDate;
  varindex = DatabaseUtils::GetFieldIndex(FieldYear, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_strOrigReleaseDate;
  varindex = DatabaseUtils::GetFieldIndex(FieldOrigYear, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_strMoods;
  varindex = DatabaseUtils::GetFieldIndex(FieldMoods, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_strStyles;
  varindex = DatabaseUtils::GetFieldIndex(FieldStyles, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_strThemes;
  varindex = DatabaseUtils::GetFieldIndex(FieldThemes, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_strReview;
  varindex = DatabaseUtils::GetFieldIndex(FieldReview, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_strLabel;
  varindex = DatabaseUtils::GetFieldIndex(FieldMusicLabel, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_strType;
  varindex = DatabaseUtils::GetFieldIndex(FieldAlbumType, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_fRating;
  varindex = DatabaseUtils::GetFieldIndex(FieldRating, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = a.album_dtDateAdded;
  varindex = DatabaseUtils::GetFieldIndex(FieldDateAdded, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);

  refindex = -1;
  varindex = DatabaseUtils::GetFieldIndex(FieldRandom, MediaTypeAlbum);
  EXPECT_EQ(refindex, varindex);
}

TEST(TestDatabaseUtils, GetFieldIndex_MediaTypeSong)
{
  int refindex, varindex;
  TestDatabaseUtilsHelper a;

  refindex = a.song_idSong;
  varindex = DatabaseUtils::GetFieldIndex(FieldId, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_strTitle;
  varindex = DatabaseUtils::GetFieldIndex(FieldTitle, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_iTrack;
  varindex = DatabaseUtils::GetFieldIndex(FieldTrackNumber, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_iDuration;
  varindex = DatabaseUtils::GetFieldIndex(FieldTime, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_strReleaseDate;
  varindex = DatabaseUtils::GetFieldIndex(FieldYear, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_strFileName;
  varindex = DatabaseUtils::GetFieldIndex(FieldFilename, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_iTimesPlayed;
  varindex = DatabaseUtils::GetFieldIndex(FieldPlaycount, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_iStartOffset;
  varindex = DatabaseUtils::GetFieldIndex(FieldStartOffset, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_iEndOffset;
  varindex = DatabaseUtils::GetFieldIndex(FieldEndOffset, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_lastplayed;
  varindex = DatabaseUtils::GetFieldIndex(FieldLastPlayed, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_rating;
  varindex = DatabaseUtils::GetFieldIndex(FieldRating, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_votes;
  varindex = DatabaseUtils::GetFieldIndex(FieldVotes, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_userrating;
  varindex = DatabaseUtils::GetFieldIndex(FieldUserRating, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_comment;
  varindex = DatabaseUtils::GetFieldIndex(FieldComment, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_strAlbum;
  varindex = DatabaseUtils::GetFieldIndex(FieldAlbum, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_strPath;
  varindex = DatabaseUtils::GetFieldIndex(FieldPath, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_strArtists;
  varindex = DatabaseUtils::GetFieldIndex(FieldArtist, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = a.song_strGenres;
  varindex = DatabaseUtils::GetFieldIndex(FieldGenre, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);

  refindex = -1;
  varindex = DatabaseUtils::GetFieldIndex(FieldRandom, MediaTypeSong);
  EXPECT_EQ(refindex, varindex);
}

TEST(TestDatabaseUtils, GetFieldIndex_MediaTypeMusicVideo)
{
  int refindex, varindex;

  refindex = 0;
  varindex = DatabaseUtils::GetFieldIndex(FieldId, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_MUSICVIDEO_TITLE + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldTitle, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_MUSICVIDEO_RUNTIME + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldTime, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_MUSICVIDEO_DIRECTOR + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldDirector, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_MUSICVIDEO_STUDIOS + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldStudio, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_MUSICVIDEO_PLOT + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldPlot, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_MUSICVIDEO_ALBUM + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldAlbum, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_MUSICVIDEO_ARTIST + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldArtist, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_MUSICVIDEO_GENRE + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldGenre, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_MUSICVIDEO_TRACK + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldTrackNumber, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MUSICVIDEO_FILE;
  varindex = DatabaseUtils::GetFieldIndex(FieldFilename, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MUSICVIDEO_PATH;
  varindex = DatabaseUtils::GetFieldIndex(FieldPath, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MUSICVIDEO_PLAYCOUNT;
  varindex = DatabaseUtils::GetFieldIndex(FieldPlaycount, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MUSICVIDEO_LASTPLAYED;
  varindex = DatabaseUtils::GetFieldIndex(FieldLastPlayed, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MUSICVIDEO_DATEADDED;
  varindex = DatabaseUtils::GetFieldIndex(FieldDateAdded, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MUSICVIDEO_USER_RATING;
  varindex = DatabaseUtils::GetFieldIndex(FieldUserRating, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MUSICVIDEO_PREMIERED;
  varindex = DatabaseUtils::GetFieldIndex(FieldYear, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);

  refindex = -1;
  varindex = DatabaseUtils::GetFieldIndex(FieldRandom, MediaTypeMusicVideo);
  EXPECT_EQ(refindex, varindex);
}

TEST(TestDatabaseUtils, GetFieldIndex_MediaTypeMovie)
{
  int refindex, varindex;

  refindex = 0;
  varindex = DatabaseUtils::GetFieldIndex(FieldId, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_TITLE + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldTitle, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_SORTTITLE + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldSortTitle, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_PLOT + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldPlot, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_PLOTOUTLINE + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldPlotOutline, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_TAGLINE + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldTagline, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_CREDITS + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldWriter, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_RUNTIME + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldTime, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_MPAA + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldMPAA, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_TOP250 + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldTop250, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_GENRE + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldGenre, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_DIRECTOR + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldDirector, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_STUDIOS + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldStudio, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_TRAILER + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldTrailer, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_COUNTRY + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldCountry, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MOVIE_FILE + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldFilename, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MOVIE_PATH;
  varindex = DatabaseUtils::GetFieldIndex(FieldPath, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MOVIE_PLAYCOUNT;
  varindex = DatabaseUtils::GetFieldIndex(FieldPlaycount, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MOVIE_LASTPLAYED;
  varindex = DatabaseUtils::GetFieldIndex(FieldLastPlayed, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MOVIE_DATEADDED;
  varindex = DatabaseUtils::GetFieldIndex(FieldDateAdded, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MOVIE_USER_RATING;
  varindex = DatabaseUtils::GetFieldIndex(FieldUserRating, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MOVIE_VOTES;
  varindex = DatabaseUtils::GetFieldIndex(FieldVotes, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MOVIE_RATING;
  varindex = DatabaseUtils::GetFieldIndex(FieldRating, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_MOVIE_PREMIERED;
  varindex = DatabaseUtils::GetFieldIndex(FieldYear, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);

  refindex = -1;
  varindex = DatabaseUtils::GetFieldIndex(FieldRandom, MediaTypeMovie);
  EXPECT_EQ(refindex, varindex);
}

TEST(TestDatabaseUtils, GetFieldIndex_MediaTypeTvShow)
{
  int refindex, varindex;

  refindex = 0;
  varindex = DatabaseUtils::GetFieldIndex(FieldId, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_TV_TITLE + 1;
  varindex = DatabaseUtils::GetFieldIndex(FieldTitle, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_TV_SORTTITLE + 1;
  varindex = DatabaseUtils::GetFieldIndex(FieldSortTitle, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_TV_PLOT + 1;
  varindex = DatabaseUtils::GetFieldIndex(FieldPlot, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_TV_STATUS + 1;
  varindex = DatabaseUtils::GetFieldIndex(FieldTvShowStatus, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_TV_PREMIERED + 1;
  varindex = DatabaseUtils::GetFieldIndex(FieldYear, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_TV_GENRE + 1;
  varindex = DatabaseUtils::GetFieldIndex(FieldGenre, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_TV_MPAA + 1;
  varindex = DatabaseUtils::GetFieldIndex(FieldMPAA, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_TV_STUDIOS + 1;
  varindex = DatabaseUtils::GetFieldIndex(FieldStudio, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_TVSHOW_PATH;
  varindex = DatabaseUtils::GetFieldIndex(FieldPath, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_TVSHOW_DATEADDED;
  varindex = DatabaseUtils::GetFieldIndex(FieldDateAdded, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_TVSHOW_NUM_EPISODES;
  varindex = DatabaseUtils::GetFieldIndex(FieldNumberOfEpisodes, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_TVSHOW_NUM_WATCHED;
  varindex = DatabaseUtils::GetFieldIndex(FieldNumberOfWatchedEpisodes, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_TVSHOW_NUM_SEASONS;
  varindex = DatabaseUtils::GetFieldIndex(FieldSeason, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_TVSHOW_USER_RATING;
  varindex = DatabaseUtils::GetFieldIndex(FieldUserRating, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_TVSHOW_VOTES;
  varindex = DatabaseUtils::GetFieldIndex(FieldVotes, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_TVSHOW_RATING;
  varindex = DatabaseUtils::GetFieldIndex(FieldRating, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);

  refindex = -1;
  varindex = DatabaseUtils::GetFieldIndex(FieldRandom, MediaTypeTvShow);
  EXPECT_EQ(refindex, varindex);
}

TEST(TestDatabaseUtils, GetFieldIndex_MediaTypeEpisode)
{
  int refindex, varindex;

  refindex = 0;
  varindex = DatabaseUtils::GetFieldIndex(FieldId, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_EPISODE_TITLE + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldTitle, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_EPISODE_PLOT + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldPlot, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_EPISODE_CREDITS + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldWriter, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_EPISODE_AIRED + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldAirDate, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_EPISODE_RUNTIME + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldTime, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_EPISODE_DIRECTOR + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldDirector, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_EPISODE_SEASON + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldSeason, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_ID_EPISODE_EPISODE + 2;
  varindex = DatabaseUtils::GetFieldIndex(FieldEpisodeNumber, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_EPISODE_FILE;
  varindex = DatabaseUtils::GetFieldIndex(FieldFilename, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_EPISODE_PATH;
  varindex = DatabaseUtils::GetFieldIndex(FieldPath, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_EPISODE_PLAYCOUNT;
  varindex = DatabaseUtils::GetFieldIndex(FieldPlaycount, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_EPISODE_LASTPLAYED;
  varindex = DatabaseUtils::GetFieldIndex(FieldLastPlayed, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_EPISODE_DATEADDED;
  varindex = DatabaseUtils::GetFieldIndex(FieldDateAdded, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_EPISODE_TVSHOW_NAME;
  varindex = DatabaseUtils::GetFieldIndex(FieldTvShowTitle, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_EPISODE_TVSHOW_STUDIO;
  varindex = DatabaseUtils::GetFieldIndex(FieldStudio, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_EPISODE_TVSHOW_AIRED;
  varindex = DatabaseUtils::GetFieldIndex(FieldYear, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_EPISODE_TVSHOW_MPAA;
  varindex = DatabaseUtils::GetFieldIndex(FieldMPAA, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_EPISODE_USER_RATING;
  varindex = DatabaseUtils::GetFieldIndex(FieldUserRating, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_EPISODE_VOTES;
  varindex = DatabaseUtils::GetFieldIndex(FieldVotes, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = VIDEODB_DETAILS_EPISODE_RATING;
  varindex = DatabaseUtils::GetFieldIndex(FieldRating, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);

  refindex = -1;
  varindex = DatabaseUtils::GetFieldIndex(FieldRandom, MediaTypeEpisode);
  EXPECT_EQ(refindex, varindex);
}

TEST(TestDatabaseUtils, GetSelectFields)
{
  Fields fields;
  FieldList fieldlist;

  EXPECT_FALSE(DatabaseUtils::GetSelectFields(fields, MediaTypeAlbum,
                                              fieldlist));

  fields.insert(FieldId);
  fields.insert(FieldGenre);
  fields.insert(FieldAlbum);
  fields.insert(FieldArtist);
  fields.insert(FieldTitle);
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
