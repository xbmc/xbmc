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

#include <span>
#include <string>

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

namespace
{
struct FieldCase
{
  Field field;
  DatabaseQueryPart part;
  std::string expected;
};

void ExpectFields(KODI::MEDIA::TYPE type, std::span<const FieldCase> cases)
{
  for (const FieldCase& c : cases)
    EXPECT_EQ(DatabaseUtils::GetField(c.field, type, c.part), c.expected)
        << "field " << static_cast<int>(c.field) << ", part " << static_cast<int>(c.part);
}

struct IndexCase
{
  Field field;
  int expected;
};

void ExpectIndexes(KODI::MEDIA::TYPE type, std::span<const IndexCase> cases)
{
  for (const IndexCase& c : cases)
    EXPECT_EQ(DatabaseUtils::GetFieldIndex(c.field, type), c.expected)
        << "field " << static_cast<int>(c.field);
}
} // namespace

TEST(TestDatabaseUtils, GetField_None)
{
  EXPECT_EQ(
      DatabaseUtils::GetField(Field::NONE, KODI::MEDIA::TYPE::NONE, DatabaseQueryPart::SELECT), "");
  EXPECT_EQ(
      DatabaseUtils::GetField(Field::NONE, KODI::MEDIA::TYPE::MOVIE, DatabaseQueryPart::SELECT),
      "");
}

TEST(TestDatabaseUtils, GetField_MediaTypeAlbum)
{
  const FieldCase cases[] = {
      {Field::ID, DatabaseQueryPart::SELECT, "albumview.idAlbum"},
      {Field::ALBUM, DatabaseQueryPart::SELECT, "albumview.strAlbum"},
      {Field::ARTIST, DatabaseQueryPart::SELECT, "albumview.strArtists"},
      {Field::ALBUM_ARTIST, DatabaseQueryPart::SELECT, "albumview.strArtists"},
      {Field::GENRE, DatabaseQueryPart::SELECT, "albumview.strGenres"},
      {Field::YEAR, DatabaseQueryPart::SELECT, "albumview.strReleaseDate"},
      {Field::ORIG_YEAR, DatabaseQueryPart::SELECT, "albumview.strOrigReleaseDate"},
      {Field::MOODS, DatabaseQueryPart::SELECT, "albumview.strMoods"},
      {Field::STYLES, DatabaseQueryPart::SELECT, "albumview.strStyles"},
      {Field::THEMES, DatabaseQueryPart::SELECT, "albumview.strThemes"},
      {Field::REVIEW, DatabaseQueryPart::SELECT, "albumview.strReview"},
      {Field::MUSIC_LABEL, DatabaseQueryPart::SELECT, "albumview.strLabel"},
      {Field::ALBUM_TYPE, DatabaseQueryPart::SELECT, "albumview.strType"},
      {Field::RATING, DatabaseQueryPart::SELECT, "albumview.fRating"},
      {Field::VOTES, DatabaseQueryPart::SELECT, "albumview.iVotes"},
      {Field::USER_RATING, DatabaseQueryPart::SELECT, "albumview.iUserrating"},
      {Field::DATE_ADDED, DatabaseQueryPart::SELECT, "albumview.dateAdded"},
      {Field::NONE, DatabaseQueryPart::SELECT, ""},
      {Field::ALBUM, DatabaseQueryPart::WHERE, "albumview.strAlbum"},
      {Field::ALBUM, DatabaseQueryPart::ORDER_BY, "albumview.strAlbum"},
  };
  ExpectFields(KODI::MEDIA::TYPE::ALBUM, cases);
}

TEST(TestDatabaseUtils, GetField_MediaTypeSong)
{
  const FieldCase cases[] = {
      {Field::ID, DatabaseQueryPart::SELECT, "songview.idSong"},
      {Field::TITLE, DatabaseQueryPart::SELECT, "songview.strTitle"},
      {Field::TRACK_NUMBER, DatabaseQueryPart::SELECT, "songview.iTrack"},
      {Field::TIME, DatabaseQueryPart::SELECT, "songview.iDuration"},
      {Field::FILENAME, DatabaseQueryPart::SELECT, "songview.strFilename"},
      {Field::PLAYCOUNT, DatabaseQueryPart::SELECT, "songview.iTimesPlayed"},
      {Field::START_OFFSET, DatabaseQueryPart::SELECT, "songview.iStartOffset"},
      {Field::END_OFFSET, DatabaseQueryPart::SELECT, "songview.iEndOffset"},
      {Field::LAST_PLAYED, DatabaseQueryPart::SELECT, "songview.lastPlayed"},
      {Field::RATING, DatabaseQueryPart::SELECT, "songview.rating"},
      {Field::VOTES, DatabaseQueryPart::SELECT, "songview.votes"},
      {Field::USER_RATING, DatabaseQueryPart::SELECT, "songview.userrating"},
      {Field::COMMENT, DatabaseQueryPart::SELECT, "songview.comment"},
      {Field::YEAR, DatabaseQueryPart::SELECT, "songview.strReleaseDate"},
      {Field::ORIG_YEAR, DatabaseQueryPart::SELECT, "songview.strOrigReleaseDate"},
      {Field::ALBUM, DatabaseQueryPart::SELECT, "songview.strAlbum"},
      {Field::PATH, DatabaseQueryPart::SELECT, "songview.strPath"},
      {Field::ARTIST, DatabaseQueryPart::SELECT, "songview.strArtists"},
      {Field::ALBUM_ARTIST, DatabaseQueryPart::SELECT, "songview.strArtists"},
      {Field::GENRE, DatabaseQueryPart::SELECT, "songview.strGenres"},
      {Field::DATE_ADDED, DatabaseQueryPart::SELECT, "songview.dateAdded"},
      {Field::PATH, DatabaseQueryPart::WHERE, "songview.strPath"},
      {Field::PATH, DatabaseQueryPart::ORDER_BY, "songview.strPath"},
  };
  ExpectFields(KODI::MEDIA::TYPE::SONG, cases);
}

TEST(TestDatabaseUtils, GetField_MediaTypeMusicVideo)
{
  const FieldCase cases[] = {
      {Field::ID, DatabaseQueryPart::SELECT, "musicvideo_view.idMVideo"},
      {Field::TITLE, DatabaseQueryPart::SELECT,
       StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_TITLE)},
      {Field::TIME, DatabaseQueryPart::SELECT,
       StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_RUNTIME)},
      {Field::DIRECTOR, DatabaseQueryPart::SELECT,
       StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_DIRECTOR)},
      {Field::STUDIO, DatabaseQueryPart::SELECT,
       StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_STUDIOS)},
      {Field::PLOT, DatabaseQueryPart::SELECT,
       StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_PLOT)},
      {Field::ALBUM, DatabaseQueryPart::SELECT,
       StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_ALBUM)},
      {Field::ARTIST, DatabaseQueryPart::SELECT,
       StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_ARTIST)},
      {Field::GENRE, DatabaseQueryPart::SELECT,
       StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_GENRE)},
      {Field::TRACK_NUMBER, DatabaseQueryPart::SELECT,
       StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_TRACK)},
      {Field::FILENAME, DatabaseQueryPart::SELECT, "musicvideo_view.strFilename"},
      {Field::PATH, DatabaseQueryPart::SELECT, "musicvideo_view.strPath"},
      {Field::PLAYCOUNT, DatabaseQueryPart::SELECT, "musicvideo_view.playCount"},
      {Field::LAST_PLAYED, DatabaseQueryPart::SELECT, "musicvideo_view.lastPlayed"},
      {Field::DATE_ADDED, DatabaseQueryPart::SELECT, "musicvideo_view.dateAdded"},
      {Field::VIDEO_RESOLUTION, DatabaseQueryPart::SELECT, ""},
      {Field::PATH, DatabaseQueryPart::WHERE, "musicvideo_view.strPath"},
      {Field::PATH, DatabaseQueryPart::ORDER_BY, "musicvideo_view.strPath"},
      {Field::USER_RATING, DatabaseQueryPart::SELECT, "musicvideo_view.userrating"},
  };
  ExpectFields(KODI::MEDIA::TYPE::MUSIC_VIDEO, cases);
}

TEST(TestDatabaseUtils, GetField_MediaTypeMovie)
{
  const FieldCase cases[] = {
      {Field::ID, DatabaseQueryPart::SELECT, "movie_view.idMovie"},
      {Field::TITLE, DatabaseQueryPart::SELECT,
       StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_TITLE)},
      {Field::TITLE, DatabaseQueryPart::ORDER_BY,
       StringUtils::Format("CASE WHEN length(movie_view.c{:02}) > 0 THEN movie_view.c{:02} "
                           "ELSE movie_view.c{:02} END",
                           VIDEODB_ID_SORTTITLE, VIDEODB_ID_SORTTITLE, VIDEODB_ID_TITLE)},
      {Field::PLOT, DatabaseQueryPart::SELECT,
       StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_PLOT)},
      {Field::PLOT_OUTLINE, DatabaseQueryPart::SELECT,
       StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_PLOTOUTLINE)},
      {Field::TAGLINE, DatabaseQueryPart::SELECT,
       StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_TAGLINE)},
      {Field::VOTES, DatabaseQueryPart::SELECT, "movie_view.votes"},
      {Field::RATING, DatabaseQueryPart::SELECT, "movie_view.rating"},
      {Field::WRITER, DatabaseQueryPart::SELECT,
       StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_CREDITS)},
      {Field::SORT_TITLE, DatabaseQueryPart::SELECT,
       StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_SORTTITLE)},
      {Field::TIME, DatabaseQueryPart::SELECT,
       StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_RUNTIME)},
      {Field::MPAA, DatabaseQueryPart::SELECT,
       StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_MPAA)},
      {Field::TOP250, DatabaseQueryPart::SELECT,
       StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_TOP250)},
      {Field::GENRE, DatabaseQueryPart::SELECT,
       StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_GENRE)},
      {Field::DIRECTOR, DatabaseQueryPart::SELECT,
       StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_DIRECTOR)},
      {Field::STUDIO, DatabaseQueryPart::SELECT,
       StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_STUDIOS)},
      {Field::TRAILER, DatabaseQueryPart::SELECT,
       StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_TRAILER)},
      {Field::COUNTRY, DatabaseQueryPart::SELECT,
       StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_COUNTRY)},
      {Field::FILENAME, DatabaseQueryPart::SELECT, "movie_view.strFilename"},
      {Field::PATH, DatabaseQueryPart::SELECT, "movie_view.strPath"},
      {Field::PLAYCOUNT, DatabaseQueryPart::SELECT, "movie_view.playCount"},
      {Field::LAST_PLAYED, DatabaseQueryPart::SELECT, "movie_view.lastPlayed"},
      {Field::DATE_ADDED, DatabaseQueryPart::SELECT, "movie_view.dateAdded"},
      {Field::USER_RATING, DatabaseQueryPart::SELECT, "movie_view.userrating"},
      {Field::RANDOM, DatabaseQueryPart::SELECT, ""},
  };
  ExpectFields(KODI::MEDIA::TYPE::MOVIE, cases);
}

TEST(TestDatabaseUtils, GetField_MediaTypeTvShow)
{
  const FieldCase cases[] = {
      {Field::ID, DatabaseQueryPart::SELECT, "tvshow_view.idShow"},
      {Field::TITLE, DatabaseQueryPart::ORDER_BY,
       StringUtils::Format("CASE WHEN length(tvshow_view.c{:02}) > 0 THEN tvshow_view.c{:02} "
                           "ELSE tvshow_view.c{:02} END",
                           VIDEODB_ID_TV_SORTTITLE, VIDEODB_ID_TV_SORTTITLE, VIDEODB_ID_TV_TITLE)},
      {Field::TITLE, DatabaseQueryPart::SELECT,
       StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_TITLE)},
      {Field::PLOT, DatabaseQueryPart::SELECT,
       StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_PLOT)},
      {Field::TVSHOW_STATUS, DatabaseQueryPart::SELECT,
       StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_STATUS)},
      {Field::VOTES, DatabaseQueryPart::SELECT, "tvshow_view.votes"},
      {Field::RATING, DatabaseQueryPart::SELECT, "tvshow_view.rating"},
      {Field::YEAR, DatabaseQueryPart::SELECT,
       StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_PREMIERED)},
      {Field::GENRE, DatabaseQueryPart::SELECT,
       StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_GENRE)},
      {Field::MPAA, DatabaseQueryPart::SELECT,
       StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_MPAA)},
      {Field::STUDIO, DatabaseQueryPart::SELECT,
       StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_STUDIOS)},
      {Field::SORT_TITLE, DatabaseQueryPart::SELECT,
       StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_SORTTITLE)},
      {Field::PATH, DatabaseQueryPart::SELECT, "tvshow_view.strPath"},
      {Field::DATE_ADDED, DatabaseQueryPart::SELECT, "tvshow_view.dateAdded"},
      {Field::SEASON, DatabaseQueryPart::SELECT, "tvshow_view.totalSeasons"},
      {Field::NUMBER_OF_EPISODES, DatabaseQueryPart::SELECT, "tvshow_view.totalCount"},
      {Field::NUMBER_OF_WATCHED_EPISODES, DatabaseQueryPart::SELECT, "tvshow_view.watchedcount"},
      {Field::USER_RATING, DatabaseQueryPart::SELECT, "tvshow_view.userrating"},
      {Field::RANDOM, DatabaseQueryPart::SELECT, ""},
  };
  ExpectFields(KODI::MEDIA::TYPE::TV_SHOW, cases);
}

TEST(TestDatabaseUtils, GetField_MediaTypeEpisode)
{
  const FieldCase cases[] = {
      {Field::ID, DatabaseQueryPart::SELECT, "episode_view.idEpisode"},
      {Field::TITLE, DatabaseQueryPart::SELECT,
       StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_TITLE)},
      {Field::PLOT, DatabaseQueryPart::SELECT,
       StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_PLOT)},
      {Field::VOTES, DatabaseQueryPart::SELECT, "episode_view.votes"},
      {Field::RATING, DatabaseQueryPart::SELECT, "episode_view.rating"},
      {Field::WRITER, DatabaseQueryPart::SELECT,
       StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_CREDITS)},
      {Field::AIR_DATE, DatabaseQueryPart::SELECT,
       StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_AIRED)},
      {Field::TIME, DatabaseQueryPart::SELECT,
       StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_RUNTIME)},
      {Field::DIRECTOR, DatabaseQueryPart::SELECT,
       StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_DIRECTOR)},
      {Field::SEASON, DatabaseQueryPart::SELECT,
       StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_SEASON)},
      {Field::EPISODE_NUMBER, DatabaseQueryPart::SELECT,
       StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_EPISODE)},
      {Field::FILENAME, DatabaseQueryPart::SELECT, "episode_view.strFilename"},
      {Field::PATH, DatabaseQueryPart::SELECT, "episode_view.strPath"},
      {Field::PLAYCOUNT, DatabaseQueryPart::SELECT, "episode_view.playCount"},
      {Field::LAST_PLAYED, DatabaseQueryPart::SELECT, "episode_view.lastPlayed"},
      {Field::DATE_ADDED, DatabaseQueryPart::SELECT, "episode_view.dateAdded"},
      {Field::TVSHOW_TITLE, DatabaseQueryPart::SELECT, "episode_view.strTitle"},
      {Field::YEAR, DatabaseQueryPart::SELECT, "episode_view.premiered"},
      {Field::MPAA, DatabaseQueryPart::SELECT, "episode_view.mpaa"},
      {Field::STUDIO, DatabaseQueryPart::SELECT, "episode_view.strStudio"},
      {Field::USER_RATING, DatabaseQueryPart::SELECT, "episode_view.userrating"},
      {Field::RANDOM, DatabaseQueryPart::SELECT, ""},
  };
  ExpectFields(KODI::MEDIA::TYPE::EPISODE, cases);
}

TEST(TestDatabaseUtils, GetField_FieldRandom)
{
  const FieldCase cases[] = {
      {Field::RANDOM, DatabaseQueryPart::SELECT, ""},
      {Field::RANDOM, DatabaseQueryPart::WHERE, ""},
      {Field::RANDOM, DatabaseQueryPart::ORDER_BY, "RANDOM()"},
  };
  ExpectFields(KODI::MEDIA::TYPE::EPISODE, cases);
}

TEST(TestDatabaseUtils, GetFieldIndex_None)
{
  EXPECT_EQ(DatabaseUtils::GetFieldIndex(Field::RANDOM, KODI::MEDIA::TYPE::NONE), -1);
  EXPECT_EQ(DatabaseUtils::GetFieldIndex(Field::NONE, KODI::MEDIA::TYPE::ALBUM), -1);
}

//! @todo Should enums in CMusicDatabase be made public instead?
TEST(TestDatabaseUtils, GetFieldIndex_MediaTypeAlbum)
{
  TestDatabaseUtilsHelper a;
  const IndexCase cases[] = {
      {Field::ID, a.album_idAlbum},
      {Field::ALBUM, a.album_strAlbum},
      {Field::ARTIST, a.album_strArtists},
      {Field::ALBUM_ARTIST, a.album_strArtists},
      {Field::GENRE, a.album_strGenres},
      {Field::YEAR, a.album_strReleaseDate},
      {Field::ORIG_YEAR, a.album_strOrigReleaseDate},
      {Field::MOODS, a.album_strMoods},
      {Field::STYLES, a.album_strStyles},
      {Field::THEMES, a.album_strThemes},
      {Field::REVIEW, a.album_strReview},
      {Field::MUSIC_LABEL, a.album_strLabel},
      {Field::ALBUM_TYPE, a.album_strType},
      {Field::RATING, a.album_fRating},
      {Field::DATE_ADDED, a.album_dtDateAdded},
      {Field::RANDOM, -1},
  };
  ExpectIndexes(KODI::MEDIA::TYPE::ALBUM, cases);
}

TEST(TestDatabaseUtils, GetFieldIndex_MediaTypeSong)
{
  TestDatabaseUtilsHelper a;
  const IndexCase cases[] = {
      {Field::ID, a.song_idSong},
      {Field::TITLE, a.song_strTitle},
      {Field::TRACK_NUMBER, a.song_iTrack},
      {Field::TIME, a.song_iDuration},
      {Field::YEAR, a.song_strReleaseDate},
      {Field::FILENAME, a.song_strFileName},
      {Field::PLAYCOUNT, a.song_iTimesPlayed},
      {Field::START_OFFSET, a.song_iStartOffset},
      {Field::END_OFFSET, a.song_iEndOffset},
      {Field::LAST_PLAYED, a.song_lastplayed},
      {Field::RATING, a.song_rating},
      {Field::VOTES, a.song_votes},
      {Field::USER_RATING, a.song_userrating},
      {Field::COMMENT, a.song_comment},
      {Field::ALBUM, a.song_strAlbum},
      {Field::PATH, a.song_strPath},
      {Field::ARTIST, a.song_strArtists},
      {Field::GENRE, a.song_strGenres},
      {Field::RANDOM, -1},
  };
  ExpectIndexes(KODI::MEDIA::TYPE::SONG, cases);
}

TEST(TestDatabaseUtils, GetFieldIndex_MediaTypeMusicVideo)
{
  const IndexCase cases[] = {
      {Field::ID, 0},
      {Field::TITLE, VIDEODB_ID_MUSICVIDEO_TITLE + 2},
      {Field::TIME, VIDEODB_ID_MUSICVIDEO_RUNTIME + 2},
      {Field::DIRECTOR, VIDEODB_ID_MUSICVIDEO_DIRECTOR + 2},
      {Field::STUDIO, VIDEODB_ID_MUSICVIDEO_STUDIOS + 2},
      {Field::PLOT, VIDEODB_ID_MUSICVIDEO_PLOT + 2},
      {Field::ALBUM, VIDEODB_ID_MUSICVIDEO_ALBUM + 2},
      {Field::ARTIST, VIDEODB_ID_MUSICVIDEO_ARTIST + 2},
      {Field::GENRE, VIDEODB_ID_MUSICVIDEO_GENRE + 2},
      {Field::TRACK_NUMBER, VIDEODB_ID_MUSICVIDEO_TRACK + 2},
      {Field::FILENAME, VIDEODB_DETAILS_MUSICVIDEO_FILE},
      {Field::PATH, VIDEODB_DETAILS_MUSICVIDEO_PATH},
      {Field::PLAYCOUNT, VIDEODB_DETAILS_MUSICVIDEO_PLAYCOUNT},
      {Field::LAST_PLAYED, VIDEODB_DETAILS_MUSICVIDEO_LASTPLAYED},
      {Field::DATE_ADDED, VIDEODB_DETAILS_MUSICVIDEO_DATEADDED},
      {Field::USER_RATING, VIDEODB_DETAILS_MUSICVIDEO_USER_RATING},
      {Field::YEAR, VIDEODB_DETAILS_MUSICVIDEO_PREMIERED},
      {Field::RANDOM, -1},
  };
  ExpectIndexes(KODI::MEDIA::TYPE::MUSIC_VIDEO, cases);
}

TEST(TestDatabaseUtils, GetFieldIndex_MediaTypeMovie)
{
  const IndexCase cases[] = {
      {Field::ID, 0},
      {Field::TITLE, VIDEODB_ID_TITLE + 2},
      {Field::SORT_TITLE, VIDEODB_ID_SORTTITLE + 2},
      {Field::PLOT, VIDEODB_ID_PLOT + 2},
      {Field::PLOT_OUTLINE, VIDEODB_ID_PLOTOUTLINE + 2},
      {Field::TAGLINE, VIDEODB_ID_TAGLINE + 2},
      {Field::WRITER, VIDEODB_ID_CREDITS + 2},
      {Field::TIME, VIDEODB_ID_RUNTIME + 2},
      {Field::MPAA, VIDEODB_ID_MPAA + 2},
      {Field::TOP250, VIDEODB_ID_TOP250 + 2},
      {Field::GENRE, VIDEODB_ID_GENRE + 2},
      {Field::DIRECTOR, VIDEODB_ID_DIRECTOR + 2},
      {Field::STUDIO, VIDEODB_ID_STUDIOS + 2},
      {Field::TRAILER, VIDEODB_ID_TRAILER + 2},
      {Field::COUNTRY, VIDEODB_ID_COUNTRY + 2},
      {Field::FILENAME, VIDEODB_DETAILS_MOVIE_FILE},
      {Field::PATH, VIDEODB_DETAILS_MOVIE_PATH},
      {Field::PLAYCOUNT, VIDEODB_DETAILS_MOVIE_PLAYCOUNT},
      {Field::LAST_PLAYED, VIDEODB_DETAILS_MOVIE_LASTPLAYED},
      {Field::DATE_ADDED, VIDEODB_DETAILS_MOVIE_DATEADDED},
      {Field::USER_RATING, VIDEODB_DETAILS_MOVIE_USER_RATING},
      {Field::VOTES, VIDEODB_DETAILS_MOVIE_VOTES},
      {Field::RATING, VIDEODB_DETAILS_MOVIE_RATING},
      {Field::YEAR, VIDEODB_DETAILS_MOVIE_PREMIERED},
      {Field::RANDOM, -1},
  };
  ExpectIndexes(KODI::MEDIA::TYPE::MOVIE, cases);
}

TEST(TestDatabaseUtils, GetFieldIndex_MediaTypeTvShow)
{
  const IndexCase cases[] = {
      {Field::ID, 0},
      {Field::TITLE, VIDEODB_ID_TV_TITLE + 1},
      {Field::SORT_TITLE, VIDEODB_ID_TV_SORTTITLE + 1},
      {Field::PLOT, VIDEODB_ID_TV_PLOT + 1},
      {Field::TVSHOW_STATUS, VIDEODB_ID_TV_STATUS + 1},
      {Field::YEAR, VIDEODB_ID_TV_PREMIERED + 1},
      {Field::GENRE, VIDEODB_ID_TV_GENRE + 1},
      {Field::MPAA, VIDEODB_ID_TV_MPAA + 1},
      {Field::STUDIO, VIDEODB_ID_TV_STUDIOS + 1},
      {Field::PATH, VIDEODB_DETAILS_TVSHOW_PATH},
      {Field::DATE_ADDED, VIDEODB_DETAILS_TVSHOW_DATEADDED},
      {Field::NUMBER_OF_EPISODES, VIDEODB_DETAILS_TVSHOW_NUM_EPISODES},
      {Field::NUMBER_OF_WATCHED_EPISODES, VIDEODB_DETAILS_TVSHOW_NUM_WATCHED},
      {Field::SEASON, VIDEODB_DETAILS_TVSHOW_NUM_SEASONS},
      {Field::USER_RATING, VIDEODB_DETAILS_TVSHOW_USER_RATING},
      {Field::VOTES, VIDEODB_DETAILS_TVSHOW_VOTES},
      {Field::RATING, VIDEODB_DETAILS_TVSHOW_RATING},
      {Field::RANDOM, -1},
  };
  ExpectIndexes(KODI::MEDIA::TYPE::TV_SHOW, cases);
}

TEST(TestDatabaseUtils, GetFieldIndex_MediaTypeEpisode)
{
  const IndexCase cases[] = {
      {Field::ID, 0},
      {Field::TITLE, VIDEODB_ID_EPISODE_TITLE + 2},
      {Field::PLOT, VIDEODB_ID_EPISODE_PLOT + 2},
      {Field::WRITER, VIDEODB_ID_EPISODE_CREDITS + 2},
      {Field::AIR_DATE, VIDEODB_ID_EPISODE_AIRED + 2},
      {Field::TIME, VIDEODB_ID_EPISODE_RUNTIME + 2},
      {Field::DIRECTOR, VIDEODB_ID_EPISODE_DIRECTOR + 2},
      {Field::SEASON, VIDEODB_ID_EPISODE_SEASON + 2},
      {Field::EPISODE_NUMBER, VIDEODB_ID_EPISODE_EPISODE + 2},
      {Field::FILENAME, VIDEODB_DETAILS_EPISODE_FILE},
      {Field::PATH, VIDEODB_DETAILS_EPISODE_PATH},
      {Field::PLAYCOUNT, VIDEODB_DETAILS_EPISODE_PLAYCOUNT},
      {Field::LAST_PLAYED, VIDEODB_DETAILS_EPISODE_LASTPLAYED},
      {Field::DATE_ADDED, VIDEODB_DETAILS_EPISODE_DATEADDED},
      {Field::TVSHOW_TITLE, VIDEODB_DETAILS_EPISODE_TVSHOW_NAME},
      {Field::STUDIO, VIDEODB_DETAILS_EPISODE_TVSHOW_STUDIO},
      {Field::YEAR, VIDEODB_DETAILS_EPISODE_TVSHOW_AIRED},
      {Field::MPAA, VIDEODB_DETAILS_EPISODE_TVSHOW_MPAA},
      {Field::USER_RATING, VIDEODB_DETAILS_EPISODE_USER_RATING},
      {Field::VOTES, VIDEODB_DETAILS_EPISODE_VOTES},
      {Field::RATING, VIDEODB_DETAILS_EPISODE_RATING},
      {Field::RANDOM, -1},
  };
  ExpectIndexes(KODI::MEDIA::TYPE::EPISODE, cases);
}

TEST(TestDatabaseUtils, GetSelectFields)
{
  Fields fields;
  FieldList fieldlist;

  EXPECT_FALSE(DatabaseUtils::GetSelectFields(fields, KODI::MEDIA::TYPE::ALBUM, fieldlist));

  fields = {
      Field::ID, Field::GENRE, Field::ALBUM, Field::ARTIST, Field::TITLE,
  };
  EXPECT_FALSE(DatabaseUtils::GetSelectFields(fields, KODI::MEDIA::TYPE::NONE, fieldlist));
  EXPECT_TRUE(DatabaseUtils::GetSelectFields(fields, KODI::MEDIA::TYPE::ALBUM, fieldlist));
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
//   static bool GetDatabaseResults(KODI::MEDIA::TYPE mediaType, const FieldList &fields,
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
