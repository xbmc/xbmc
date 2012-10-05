/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <sstream>

#include "DatabaseUtils.h"
#include "dbwrappers/dataset.h"
#include "music/MusicDatabase.h"
#include "utils/log.h"
#include "utils/Variant.h"
#include "video/VideoDatabase.h"

std::string DatabaseUtils::MediaTypeToString(MediaType mediaType)
{
  switch (mediaType)
  {
  case MediaTypeMusic:
    return "music";
  case MediaTypeArtist:
    return "artist";
  case MediaTypeAlbum:
    return "album";
  case MediaTypeSong:
    return "song";
  case MediaTypeVideo:
    return "video";
  case MediaTypeVideoCollection:
    return "set";
  case MediaTypeMusicVideo:
    return "musicvideo";
  case MediaTypeMovie:
    return "movie";
  case MediaTypeTvShow:
    return "tvshow";
  case MediaTypeEpisode:
    return "episode";
  default:
    break;
  }

  return "";
}

MediaType DatabaseUtils::MediaTypeFromString(const std::string &strMediaType)
{
  if (strMediaType.compare("music") == 0)
    return MediaTypeMusic;
  else if (strMediaType.compare("artist") == 0 || strMediaType.compare("artists") == 0)
    return MediaTypeArtist;
  else if (strMediaType.compare("album") == 0 || strMediaType.compare("albums") == 0)
    return MediaTypeAlbum;
  else if (strMediaType.compare("song") == 0 || strMediaType.compare("songs") == 0)
    return MediaTypeSong;
  else if (strMediaType.compare("video") == 0 || strMediaType.compare("videos") == 0)
    return MediaTypeVideo;
  else if (strMediaType.compare("set") == 0 || strMediaType.compare("sets") == 0)
    return MediaTypeVideoCollection;
  else if (strMediaType.compare("musicvideo") == 0 || strMediaType.compare("musicvideos") == 0)
    return MediaTypeMusicVideo;
  else if (strMediaType.compare("movie") == 0 || strMediaType.compare("movies") == 0)
    return MediaTypeMovie;
  else if (strMediaType.compare("tvshow") == 0 || strMediaType.compare("tvshows") == 0)
    return MediaTypeTvShow;
  else if (strMediaType.compare("episode") == 0 || strMediaType.compare("episodes") == 0)
    return MediaTypeEpisode;

  return MediaTypeNone;
}

std::string DatabaseUtils::GetField(Field field, MediaType mediaType, DatabaseQueryPart queryPart)
{
  if (field == FieldNone || mediaType == MediaTypeNone)
    return "";

  if (mediaType == MediaTypeAlbum)
  {
    if (field == FieldId) return "albumview.idAlbum";
    else if (field == FieldAlbum) return "albumview.strAlbum";
    else if (field == FieldArtist || field == FieldAlbumArtist) return "albumview.strArtists";
    else if (field == FieldGenre) return "albumview.strGenre";
    else if (field == FieldYear) return "albumview.iYear";
    else if (field == FieldMoods) return "albumview.strMoods";
    else if (field == FieldStyles) return "albumview.strStyles";
    else if (field == FieldThemes) return "albumview.strThemes";
    else if (field == FieldReview) return "albumview.strReview";
    else if (field == FieldMusicLabel) return "albumview.strLabel";
    else if (field == FieldAlbumType) return "albumview.strType";
    else if (field == FieldRating) return "albumview.iRating";
    else if (field == FieldDateAdded && queryPart == DatabaseQueryPartOrderBy) return "albumview.idalbum";    // only used for order clauses
    else if (field == FieldPlaycount) return "albumview.iTimesPlayed";
  }
  else if (mediaType == MediaTypeSong)
  {
    if (field == FieldId) return "songview.idSong";
    else if (field == FieldTitle) return "songview.strTitle";
    else if (field == FieldTrackNumber) return "songview.iTrack";
    else if (field == FieldTime) return "songview.iDuration";
    else if (field == FieldYear) return "songview.iYear";
    else if (field == FieldFilename) return "songview.strFilename";
    else if (field == FieldPlaycount) return "songview.iTimesPlayed";
    else if (field == FieldStartOffset) return "songview.iStartOffset";
    else if (field == FieldEndOffset) return "songview.iEndOffset";
    else if (field == FieldLastPlayed) return "songview.lastPlayed";
    else if (field == FieldRating) return "songview.rating";
    else if (field == FieldComment) return "songview.comment";
    else if (field == FieldAlbum) return "songview.strAlbum";
    else if (field == FieldPath) return "songview.strPath";
    else if (field == FieldArtist || field == FieldAlbumArtist) return "songview.strArtists";
    else if (field == FieldGenre) return "songview.strGenre";
    else if (field == FieldDateAdded && queryPart == DatabaseQueryPartOrderBy) return "songview.idSong";     // only used for order clauses
  }
  else if (mediaType == MediaTypeArtist)
  {
    if (field == FieldId) return "artistview.idArtist";
    else if (field == FieldArtist) return "artistview.strArtist";
    else if (field == FieldGenre) return "artistview.strGenres";
    else if (field == FieldMoods) return "artistview.strMoods";
    else if (field == FieldStyles) return "artistview.strStyles";
    else if (field == FieldInstruments) return "artistview.strInstruments";
    else if (field == FieldBiography) return "artistview.strBiography";
    else if (field == FieldBorn) return "artistview.strBorn";
    else if (field == FieldBandFormed) return "artistview.strFormed";
    else if (field == FieldDisbanded) return "artistview.strDisbanded";
    else if (field == FieldDied) return "artistview.strDied";
  }
  else if (mediaType == MediaTypeMusicVideo)
  {
    CStdString result;
    if (field == FieldId) return "musicvideoview.idMVideo";
    else if (field == FieldTitle) result.Format("musicvideoview.c%02d",VIDEODB_ID_MUSICVIDEO_TITLE);
    else if (field == FieldTime) result.Format("musicvideoview.c%02d", VIDEODB_ID_MUSICVIDEO_RUNTIME);
    else if (field == FieldDirector) result.Format("musicvideoview.c%02d", VIDEODB_ID_MUSICVIDEO_DIRECTOR);
    else if (field == FieldStudio) result.Format("musicvideoview.c%02d", VIDEODB_ID_MUSICVIDEO_STUDIOS);
    else if (field == FieldYear) result.Format("musicvideoview.c%02d",VIDEODB_ID_MUSICVIDEO_YEAR);
    else if (field == FieldPlot) result.Format("musicvideoview.c%02d", VIDEODB_ID_MUSICVIDEO_PLOT);
    else if (field == FieldAlbum) result.Format("musicvideoview.c%02d",VIDEODB_ID_MUSICVIDEO_ALBUM);
    else if (field == FieldArtist) result.Format("musicvideoview.c%02d", VIDEODB_ID_MUSICVIDEO_ARTIST);
    else if (field == FieldGenre) result.Format("musicvideoview.c%02d", VIDEODB_ID_MUSICVIDEO_GENRE);
    else if (field == FieldTrackNumber) result.Format("musicvideoview.c%02d", VIDEODB_ID_MUSICVIDEO_TRACK);
    else if (field == FieldFilename) return "musicvideoview.strFilename";
    else if (field == FieldPath) return "musicvideoview.strPath";
    else if (field == FieldPlaycount) return "musicvideoview.playCount";
    else if (field == FieldLastPlayed) return "musicvideoview.lastPlayed";
    else if (field == FieldDateAdded) return "musicvideoview.dateAdded";

    if (!result.empty())
      return result;
  }
  else if (mediaType == MediaTypeMovie)
  {
    CStdString result;
    if (field == FieldId) return "movieview.idMovie";
    else if (field == FieldTitle)
    {
      // We need some extra logic to get the title value if sorttitle isn't set
      if (queryPart == DatabaseQueryPartOrderBy)
        result.Format("CASE WHEN length(movieview.c%02d) > 0 THEN movieview.c%02d ELSE movieview.c%02d END", VIDEODB_ID_SORTTITLE, VIDEODB_ID_SORTTITLE, VIDEODB_ID_TITLE);
      else
        result.Format("movieview.c%02d", VIDEODB_ID_TITLE);
    }
    else if (field == FieldPlot) result.Format("movieview.c%02d", VIDEODB_ID_PLOT);
    else if (field == FieldPlotOutline) result.Format("movieview.c%02d", VIDEODB_ID_PLOTOUTLINE);
    else if (field == FieldTagline) result.Format("movieview.c%02d", VIDEODB_ID_TAGLINE);
    else if (field == FieldVotes) result.Format("movieview.c%02d", VIDEODB_ID_VOTES);
    else if (field == FieldRating)
    {
      if (queryPart == DatabaseQueryPartOrderBy)
        result.Format("CAST(movieview.c%02d as DECIMAL(5,3))", VIDEODB_ID_RATING);
      else
        result.Format("movieview.c%02d", VIDEODB_ID_RATING);
    }
    else if (field == FieldWriter) result.Format("movieview.c%02d", VIDEODB_ID_CREDITS);
    else if (field == FieldYear) result.Format("movieview.c%02d", VIDEODB_ID_YEAR);
    else if (field == FieldSortTitle) result.Format("movieview.c%02d", VIDEODB_ID_SORTTITLE);
    else if (field == FieldTime) result.Format("movieview.c%02d", VIDEODB_ID_RUNTIME);
    else if (field == FieldMPAA) result.Format("movieview.c%02d", VIDEODB_ID_MPAA);
    else if (field == FieldTop250) result.Format("movieview.c%02d", VIDEODB_ID_TOP250);
    else if (field == FieldSet) return "movieview.strSet";
    else if (field == FieldGenre) result.Format("movieview.c%02d", VIDEODB_ID_GENRE);
    else if (field == FieldDirector) result.Format("movieview.c%02d", VIDEODB_ID_DIRECTOR);
    else if (field == FieldStudio) result.Format("movieview.c%02d", VIDEODB_ID_STUDIOS);
    else if (field == FieldTrailer) result.Format("movieview.c%02d", VIDEODB_ID_TRAILER);
    else if (field == FieldCountry) result.Format("movieview.c%02d", VIDEODB_ID_COUNTRY);
    else if (field == FieldFilename) return "movieview.strFilename";
    else if (field == FieldPath) return "movieview.strPath";
    else if (field == FieldPlaycount) return "movieview.playCount";
    else if (field == FieldLastPlayed) return "movieview.lastPlayed";
    else if (field == FieldDateAdded) return "movieview.dateAdded";

    if (!result.empty())
      return result;
  }
  else if (mediaType == MediaTypeTvShow)
  {
    CStdString result;
    if (field == FieldId) return "tvshowview.idShow";
    else if (field == FieldTitle)
    {
      // We need some extra logic to get the title value if sorttitle isn't set
      if (queryPart == DatabaseQueryPartOrderBy)
        result.Format("CASE WHEN length(tvshowview.c%02d) > 0 THEN tvshowview.c%02d ELSE tvshowview.c%02d END", VIDEODB_ID_TV_SORTTITLE, VIDEODB_ID_TV_SORTTITLE, VIDEODB_ID_TV_TITLE);
      else
        result.Format("tvshowview.c%02d", VIDEODB_ID_TV_TITLE);
    }
    else if (field == FieldPlot) result.Format("tvshowview.c%02d", VIDEODB_ID_TV_PLOT);
    else if (field == FieldTvShowStatus) result.Format("tvshowview.c%02d", VIDEODB_ID_TV_STATUS);
    else if (field == FieldVotes) result.Format("tvshowview.c%02d", VIDEODB_ID_TV_VOTES);
    else if (field == FieldRating) result.Format("tvshowview.c%02d", VIDEODB_ID_TV_RATING);
    else if (field == FieldYear) result.Format("tvshowview.c%02d", VIDEODB_ID_TV_PREMIERED);
    else if (field == FieldGenre) result.Format("tvshowview.c%02d", VIDEODB_ID_TV_GENRE);
    else if (field == FieldMPAA) result.Format("tvshowview.c%02d", VIDEODB_ID_TV_MPAA);
    else if (field == FieldStudio) result.Format("tvshowview.c%02d", VIDEODB_ID_TV_STUDIOS);
    else if (field == FieldSortTitle) result.Format("tvshowview.c%02d", VIDEODB_ID_TV_SORTTITLE);
    else if (field == FieldPath) return "tvshowview.strPath";
    else if (field == FieldDateAdded) return "tvshowview.dateAdded";
    else if (field == FieldLastPlayed) return "tvshowview.lastPlayed";
    else if (field == FieldSeason) return "tvshowview.totalSeasons";
    else if (field == FieldNumberOfEpisodes) return "tvshowview.totalCount";
    else if (field == FieldNumberOfWatchedEpisodes) return "tvshowview.watchedcount";

    if (!result.empty())
      return result;
  }
  else if (mediaType == MediaTypeEpisode)
  {
    CStdString result;
    if (field == FieldId) return "episodeview.idEpisode";
    else if (field == FieldTitle) result.Format("episodeview.c%02d", VIDEODB_ID_EPISODE_TITLE);
    else if (field == FieldPlot) result.Format("episodeview.c%02d", VIDEODB_ID_EPISODE_PLOT);
    else if (field == FieldVotes) result.Format("episodeview.c%02d", VIDEODB_ID_EPISODE_VOTES);
    else if (field == FieldRating) result.Format("episodeview.c%02d", VIDEODB_ID_EPISODE_RATING);
    else if (field == FieldWriter) result.Format("episodeview.c%02d", VIDEODB_ID_EPISODE_CREDITS);
    else if (field == FieldAirDate) result.Format("episodeview.c%02d", VIDEODB_ID_EPISODE_AIRED);
    else if (field == FieldTime) result.Format("episodeview.c%02d", VIDEODB_ID_EPISODE_RUNTIME);
    else if (field == FieldDirector) result.Format("episodeview.c%02d", VIDEODB_ID_EPISODE_DIRECTOR);
    else if (field == FieldSeason) result.Format("episodeview.c%02d", VIDEODB_ID_EPISODE_SEASON);
    else if (field == FieldEpisodeNumber) result.Format("episodeview.c%02d", VIDEODB_ID_EPISODE_EPISODE);
    else if (field == FieldUniqueId) result.Format("episodeview.c%02d", VIDEODB_ID_EPISODE_UNIQUEID);
    else if (field == FieldEpisodeNumberSpecialSort) result.Format("episodeview.c%02d", VIDEODB_ID_EPISODE_SORTEPISODE);
    else if (field == FieldSeasonSpecialSort) result.Format("episodeview.c%02d", VIDEODB_ID_EPISODE_SORTSEASON);
    else if (field == FieldFilename) return "episodeview.strFilename";
    else if (field == FieldPath) return "episodeview.strPath";
    else if (field == FieldPlaycount) return "episodeview.playCount";
    else if (field == FieldLastPlayed) return "episodeview.lastPlayed";
    else if (field == FieldDateAdded) return "episodeview.dateAdded";
    else if (field == FieldTvShowTitle) return "episodeview.strTitle";
    else if (field == FieldYear) return "episodeview.premiered";
    else if (field == FieldMPAA) return "episodeview.mpaa";
    else if (field == FieldStudio) return "episodeview.strStudio";

    if (!result.empty())
      return result;
  }

  if (field == FieldRandom && queryPart == DatabaseQueryPartOrderBy)
    return "RANDOM()";

  return "";
}

int DatabaseUtils::GetFieldIndex(Field field, MediaType mediaType)
{
  if (field == FieldNone || mediaType == MediaTypeNone)
    return -1;

  int index = -1;

  if (mediaType == MediaTypeAlbum)
  {
    if (field == FieldId) return CMusicDatabase::album_idAlbum;
    else if (field == FieldAlbum) return CMusicDatabase::album_strAlbum;
    else if (field == FieldArtist || field == FieldAlbumArtist) return CMusicDatabase::album_strArtists;
    else if (field == FieldGenre) return CMusicDatabase::album_strGenres;
    else if (field == FieldYear) return CMusicDatabase::album_iYear;
    else if (field == FieldMoods) return CMusicDatabase::album_strMoods;
    else if (field == FieldStyles) return CMusicDatabase::album_strStyles;
    else if (field == FieldThemes) return CMusicDatabase::album_strThemes;
    else if (field == FieldReview) return CMusicDatabase::album_strReview;
    else if (field == FieldMusicLabel) return CMusicDatabase::album_strLabel;
    else if (field == FieldAlbumType) return CMusicDatabase::album_strType;
    else if (field == FieldRating) return CMusicDatabase::album_iRating;
    else if (field == FieldPlaycount) return CMusicDatabase::album_iTimesPlayed;
  }
  else if (mediaType == MediaTypeSong)
  {
    if (field == FieldId) return CMusicDatabase::song_idSong;
    else if (field == FieldTitle) return CMusicDatabase::song_strTitle;
    else if (field == FieldTrackNumber) return CMusicDatabase::song_iTrack;
    else if (field == FieldTime) return CMusicDatabase::song_iDuration;
    else if (field == FieldYear) return CMusicDatabase::song_iYear;
    else if (field == FieldFilename) return CMusicDatabase::song_strFileName;
    else if (field == FieldPlaycount) return CMusicDatabase::song_iTimesPlayed;
    else if (field == FieldStartOffset) return CMusicDatabase::song_iStartOffset;
    else if (field == FieldEndOffset) return CMusicDatabase::song_iEndOffset;
    else if (field == FieldLastPlayed) return CMusicDatabase::song_lastplayed;
    else if (field == FieldRating) return CMusicDatabase::song_rating;
    else if (field == FieldComment) return CMusicDatabase::song_comment;
    else if (field == FieldAlbum) return CMusicDatabase::song_strAlbum;
    else if (field == FieldPath) return CMusicDatabase::song_strPath;
    else if (field == FieldGenre) return CMusicDatabase::song_strGenres;
    else if (field == FieldArtist || field == FieldAlbumArtist) return CMusicDatabase::song_strArtists;
  }
  else if (mediaType == MediaTypeArtist)
  {
    if (field == FieldId) return CMusicDatabase::artist_idArtist;
    else if (field == FieldArtist) return CMusicDatabase::artist_strArtist;
    else if (field == FieldGenre) return CMusicDatabase::artist_strGenres;
    else if (field == FieldMoods) return CMusicDatabase::artist_strMoods;
    else if (field == FieldStyles) return CMusicDatabase::artist_strStyles;
    else if (field == FieldInstruments) return CMusicDatabase::artist_strInstruments;
    else if (field == FieldBiography) return CMusicDatabase::artist_strBiography;
    else if (field == FieldBorn) return CMusicDatabase::artist_strBorn;
    else if (field == FieldBandFormed) return CMusicDatabase::artist_strFormed;
    else if (field == FieldDisbanded) return CMusicDatabase::artist_strDisbanded;
    else if (field == FieldDied) return CMusicDatabase::artist_strDied;
  }
  else if (mediaType == MediaTypeMusicVideo)
  {
    if (field == FieldId) return 0;
    else if (field == FieldTitle) index = VIDEODB_ID_MUSICVIDEO_TITLE;
    else if (field == FieldTime) index =  VIDEODB_ID_MUSICVIDEO_RUNTIME;
    else if (field == FieldDirector) index =  VIDEODB_ID_MUSICVIDEO_DIRECTOR;
    else if (field == FieldStudio) index =  VIDEODB_ID_MUSICVIDEO_STUDIOS;
    else if (field == FieldYear) index = VIDEODB_ID_MUSICVIDEO_YEAR;
    else if (field == FieldPlot) index =  VIDEODB_ID_MUSICVIDEO_PLOT;
    else if (field == FieldAlbum) index = VIDEODB_ID_MUSICVIDEO_ALBUM;
    else if (field == FieldArtist) index =  VIDEODB_ID_MUSICVIDEO_ARTIST;
    else if (field == FieldGenre) index =  VIDEODB_ID_MUSICVIDEO_GENRE;
    else if (field == FieldTrackNumber) index =  VIDEODB_ID_MUSICVIDEO_TRACK;
    else if (field == FieldFilename) return VIDEODB_DETAILS_MUSICVIDEO_FILE;
    else if (field == FieldPath) return VIDEODB_DETAILS_MUSICVIDEO_PATH;
    else if (field == FieldPlaycount) return VIDEODB_DETAILS_MUSICVIDEO_PLAYCOUNT;
    else if (field == FieldLastPlayed) return VIDEODB_DETAILS_MUSICVIDEO_LASTPLAYED;
    else if (field == FieldDateAdded) return VIDEODB_DETAILS_MUSICVIDEO_DATEADDED;

    if (index < 0)
      return index;

    // see VideoDatabase.h
    // the first field is the item's ID and the second is the item's file ID
    index += 2;
  }
  else if (mediaType == MediaTypeMovie)
  {
    if (field == FieldId) return 0;
    else if (field == FieldTitle) index = VIDEODB_ID_TITLE;
    else if (field == FieldSortTitle) index = VIDEODB_ID_SORTTITLE;
    else if (field == FieldPlot) index = VIDEODB_ID_PLOT;
    else if (field == FieldPlotOutline) index = VIDEODB_ID_PLOTOUTLINE;
    else if (field == FieldTagline) index = VIDEODB_ID_TAGLINE;
    else if (field == FieldVotes) index = VIDEODB_ID_VOTES;
    else if (field == FieldRating) index = VIDEODB_ID_RATING;
    else if (field == FieldWriter) index = VIDEODB_ID_CREDITS;
    else if (field == FieldYear) index = VIDEODB_ID_YEAR;
    else if (field == FieldTime) index = VIDEODB_ID_RUNTIME;
    else if (field == FieldMPAA) index = VIDEODB_ID_MPAA;
    else if (field == FieldTop250) index = VIDEODB_ID_TOP250;
    else if (field == FieldSet) return VIDEODB_DETAILS_MOVIE_SET_NAME;
    else if (field == FieldGenre) index = VIDEODB_ID_GENRE;
    else if (field == FieldDirector) index = VIDEODB_ID_DIRECTOR;
    else if (field == FieldStudio) index = VIDEODB_ID_STUDIOS;
    else if (field == FieldTrailer) index = VIDEODB_ID_TRAILER;
    else if (field == FieldCountry) index = VIDEODB_ID_COUNTRY;
    else if (field == FieldFilename) index = VIDEODB_DETAILS_MOVIE_FILE;
    else if (field == FieldPath) return VIDEODB_DETAILS_MOVIE_PATH;
    else if (field == FieldPlaycount) return VIDEODB_DETAILS_MOVIE_PLAYCOUNT;
    else if (field == FieldLastPlayed) return VIDEODB_DETAILS_MOVIE_LASTPLAYED;
    else if (field == FieldDateAdded) return VIDEODB_DETAILS_MOVIE_DATEADDED;

    if (index < 0)
      return index;

    // see VideoDatabase.h
    // the first field is the item's ID and the second is the item's file ID
    index += 2;
  }
  else if (mediaType == MediaTypeTvShow)
  {
    if (field == FieldId) return 0;
    else if (field == FieldTitle) index = VIDEODB_ID_TV_TITLE;
    else if (field == FieldSortTitle) index = VIDEODB_ID_TV_SORTTITLE;
    else if (field == FieldPlot) index = VIDEODB_ID_TV_PLOT;
    else if (field == FieldTvShowStatus) index = VIDEODB_ID_TV_STATUS;
    else if (field == FieldVotes) index = VIDEODB_ID_TV_VOTES;
    else if (field == FieldRating) index = VIDEODB_ID_TV_RATING;
    else if (field == FieldYear) index = VIDEODB_ID_TV_PREMIERED;
    else if (field == FieldGenre) index = VIDEODB_ID_TV_GENRE;
    else if (field == FieldMPAA) index = VIDEODB_ID_TV_MPAA;
    else if (field == FieldStudio) index = VIDEODB_ID_TV_STUDIOS;
    else if (field == FieldPath) return VIDEODB_DETAILS_TVSHOW_PATH;
    else if (field == FieldDateAdded) return VIDEODB_DETAILS_TVSHOW_DATEADDED;
    else if (field == FieldLastPlayed) return VIDEODB_DETAILS_TVSHOW_LASTPLAYED;
    else if (field == FieldNumberOfEpisodes) return VIDEODB_DETAILS_TVSHOW_NUM_EPISODES;
    else if (field == FieldNumberOfWatchedEpisodes) return VIDEODB_DETAILS_TVSHOW_NUM_WATCHED;
    else if (field == FieldSeason) return VIDEODB_DETAILS_TVSHOW_NUM_SEASONS;

    if (index < 0)
      return index;

    // see VideoDatabase.h
    // the first field is the item's ID
    index += 1;
  }
  else if (mediaType == MediaTypeEpisode)
  {
    if (field == FieldId) return 0;
    else if (field == FieldTitle) index = VIDEODB_ID_EPISODE_TITLE;
    else if (field == FieldPlot) index = VIDEODB_ID_EPISODE_PLOT;
    else if (field == FieldVotes) index = VIDEODB_ID_EPISODE_VOTES;
    else if (field == FieldRating) index = VIDEODB_ID_EPISODE_RATING;
    else if (field == FieldWriter) index = VIDEODB_ID_EPISODE_CREDITS;
    else if (field == FieldAirDate) index = VIDEODB_ID_EPISODE_AIRED;
    else if (field == FieldTime) index = VIDEODB_ID_EPISODE_RUNTIME;
    else if (field == FieldDirector) index = VIDEODB_ID_EPISODE_DIRECTOR;
    else if (field == FieldSeason) index = VIDEODB_ID_EPISODE_SEASON;
    else if (field == FieldEpisodeNumber) index = VIDEODB_ID_EPISODE_EPISODE;
    else if (field == FieldUniqueId) index = VIDEODB_ID_EPISODE_UNIQUEID;
    else if (field == FieldEpisodeNumberSpecialSort) index = VIDEODB_ID_EPISODE_SORTEPISODE;
    else if (field == FieldSeasonSpecialSort) index = VIDEODB_ID_EPISODE_SORTSEASON;
    else if (field == FieldFilename) return VIDEODB_DETAILS_EPISODE_FILE;
    else if (field == FieldPath) return VIDEODB_DETAILS_EPISODE_PATH;
    else if (field == FieldPlaycount) return VIDEODB_DETAILS_EPISODE_PLAYCOUNT;
    else if (field == FieldLastPlayed) return VIDEODB_DETAILS_EPISODE_LASTPLAYED;
    else if (field == FieldDateAdded) return VIDEODB_DETAILS_EPISODE_DATEADDED;
    else if (field == FieldTvShowTitle) return VIDEODB_DETAILS_EPISODE_TVSHOW_NAME;
    else if (field == FieldStudio) return VIDEODB_DETAILS_EPISODE_TVSHOW_STUDIO;
    else if (field == FieldYear) return VIDEODB_DETAILS_EPISODE_TVSHOW_AIRED;
    else if (field == FieldMPAA) return VIDEODB_DETAILS_EPISODE_TVSHOW_MPAA;

    if (index < 0)
      return index;

    // see VideoDatabase.h
    // the first field is the item's ID and the second is the item's file ID
    index += 2;
  }

  return index;
}

bool DatabaseUtils::GetSelectFields(const Fields &fields, MediaType mediaType, FieldList &selectFields)
{
  if (mediaType == MediaTypeNone || fields.empty())
    return false;

  Fields sortFields = fields;

  // add necessary fields to create the label
  if (mediaType == MediaTypeSong || mediaType == MediaTypeVideo || mediaType == MediaTypeVideoCollection ||
      mediaType == MediaTypeMusicVideo || mediaType == MediaTypeMovie || mediaType == MediaTypeTvShow || mediaType == MediaTypeEpisode)
    sortFields.insert(FieldTitle);
  if (mediaType == MediaTypeEpisode)
  {
    sortFields.insert(FieldSeason);
    sortFields.insert(FieldEpisodeNumber);
  }
  else if (mediaType == MediaTypeAlbum)
    sortFields.insert(FieldAlbum);
  else if (mediaType == MediaTypeSong)
    sortFields.insert(FieldTrackNumber);
  else if (mediaType == MediaTypeArtist)
    sortFields.insert(FieldArtist);

  selectFields.clear();
  for (Fields::const_iterator it = sortFields.begin(); it != sortFields.end(); it++)
  {
    // ignore FieldLabel because it needs special handling (see further up)
    if (*it == FieldLabel)
      continue;

    if (GetField(*it, mediaType, DatabaseQueryPartSelect).empty())
    {
      CLog::Log(LOGWARNING, "DatabaseUtils::GetSortFieldList: unknown field %d", *it);
      continue;
    }
    selectFields.push_back(*it);
  }

  return !selectFields.empty();
}

bool DatabaseUtils::GetFieldValue(const dbiplus::field_value &fieldValue, CVariant &variantValue)
{
  if (fieldValue.get_isNull())
  {
    variantValue = CVariant::ConstNullVariant;
    return true;
  }

  switch (fieldValue.get_fType())
  {
  case dbiplus::ft_String:
  case dbiplus::ft_WideString:
  case dbiplus::ft_Object:
    variantValue = fieldValue.get_asString();
    return true;
  case dbiplus::ft_Char:
  case dbiplus::ft_WChar:
    variantValue = fieldValue.get_asChar();
    return true;
  case dbiplus::ft_Boolean:
    variantValue = fieldValue.get_asBool();
    return true;
  case dbiplus::ft_Short:
    variantValue = fieldValue.get_asShort();
    return true;
  case dbiplus::ft_UShort:
    variantValue = fieldValue.get_asShort();
    return true;
  case dbiplus::ft_Int:
    variantValue = fieldValue.get_asInt();
    return true;
  case dbiplus::ft_UInt:
    variantValue = fieldValue.get_asUInt();
    return true;
  case dbiplus::ft_Float:
    variantValue = fieldValue.get_asFloat();
    return true;
  case dbiplus::ft_Double:
  case dbiplus::ft_LongDouble:
    variantValue = fieldValue.get_asDouble();
    return true;
  case dbiplus::ft_Int64:
    variantValue = fieldValue.get_asInt64();
    return true;
  }

  return false;
}

bool DatabaseUtils::GetDatabaseResults(MediaType mediaType, const FieldList &fields, const std::auto_ptr<dbiplus::Dataset> &dataset, DatabaseResults &results)
{
  if (dataset->num_rows() == 0)
    return true;

  const dbiplus::result_set &resultSet = dataset->get_result_set();
  unsigned int offset = results.size();

  if (fields.empty())
  {
    DatabaseResult result;
    for (unsigned int index = 0; index < resultSet.records.size(); index++)
    {
      result[FieldRow] = index + offset;
      results.push_back(result);
    }

    return true;
  }

  if (resultSet.record_header.size() < fields.size())
    return false;

  std::vector<int> fieldIndexLookup;
  fieldIndexLookup.reserve(fields.size());
  for (FieldList::const_iterator it = fields.begin(); it != fields.end(); it++)
    fieldIndexLookup.push_back(GetFieldIndex(*it, mediaType));

  results.reserve(resultSet.records.size() + offset);
  for (unsigned int index = 0; index < resultSet.records.size(); index++)
  {
    DatabaseResult result;
    result[FieldRow] = index + offset;

    unsigned int lookupIndex = 0;
    for (FieldList::const_iterator it = fields.begin(); it != fields.end(); it++)
    {
      int fieldIndex = fieldIndexLookup[lookupIndex++];
      if (fieldIndex < 0)
        return false;

      std::pair<Field, CVariant> value;
      value.first = *it;
      if (!GetFieldValue(resultSet.records[index]->at(fieldIndex), value.second))
        CLog::Log(LOGWARNING, "GetDatabaseResults: unable to retrieve value of field %s", resultSet.record_header[fieldIndex].name.c_str());

      if (value.first == FieldYear &&
         (mediaType == MediaTypeTvShow || mediaType == MediaTypeEpisode))
      {
        CDateTime dateTime;
        dateTime.SetFromDBDate(value.second.asString());
        if (dateTime.IsValid())
        {
          value.second.clear();
          value.second = dateTime.GetYear();
        }
      }

      result.insert(value);
    }

    result[FieldMediaType] = mediaType;
    switch (mediaType)
    {
    case MediaTypeMovie:
    case MediaTypeVideoCollection:
    case MediaTypeTvShow:
    case MediaTypeMusicVideo:
      result[FieldLabel] = result.at(FieldTitle).asString();
      break;
      
    case MediaTypeEpisode:
    {
      std::ostringstream label;
      label << (int)(result.at(FieldSeason).asInteger() * 100 + result.at(FieldEpisodeNumber).asInteger());
      label << ". ";
      label << result.at(FieldTitle).asString();
      result[FieldLabel] = label.str();
      break;
    }

    case MediaTypeAlbum:
      result[FieldLabel] = result.at(FieldAlbum).asString();
      break;

    case MediaTypeSong:
    {
      std::ostringstream label;
      label << (int)result.at(FieldTrackNumber).asInteger();
      label << ". ";
      label << result.at(FieldTitle).asString();
      result[FieldLabel] = label.str();
      break;
    }

    case MediaTypeArtist:
      result[FieldLabel] = result.at(FieldArtist).asString();
      break;

    default:
      break;
    }

    results.push_back(result);
  }

  return true;
}

std::string DatabaseUtils::BuildLimitClause(int end, int start /* = 0 */)
{
  std::ostringstream sql;
  sql << " LIMIT ";
  if (start > 0)
  {
    if (end > 0)
    {
      end = end - start;
      if (end < 0)
        end = 0;
    }

    sql << start << "," << end;
  }
  else
    sql << end;

  return sql.str();
}
