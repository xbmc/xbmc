/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

/*
 * This file contains video database internal details.
 * Users of the video database should access the data through the data access layer.
 */

#pragma once

#include "VideoInfoTag.h"

#include <array>
#include <cstddef>

#ifndef my_offsetof
#ifndef TARGET_POSIX
#define my_offsetof(TYPE, MEMBER) offsetof(TYPE, MEMBER)
#else
/*
   Custom version of standard offsetof() macro which can be used to get
   offsets of members in class for non-POD types (according to the current
   version of C++ standard offsetof() macro can't be used in such cases and
   attempt to do so causes warnings to be emitted, OTOH in many cases it is
   still OK to assume that all instances of the class has the same offsets
   for the same members).
 */
#define my_offsetof(TYPE, MEMBER) ((size_t)((char*)&(((TYPE*)0x10)->MEMBER) - (char*)0x10))
#endif
#endif

// these defines are based on how many columns we have and which column certain data is going to be in
// when we do GetDetailsForMovie() and similar for other media types
constexpr int VIDEODB_MAX_COLUMNS = 24;
constexpr int VIDEODB_DETAILS_FILEID = 1;

// clang-format off
// movie_view columns past idMovie idFile c00-cxx
constexpr int VIDEODB_DETAILS_MOVIE_SET_ID              = VIDEODB_MAX_COLUMNS + 2;
constexpr int VIDEODB_DETAILS_MOVIE_USER_RATING         = VIDEODB_MAX_COLUMNS + 3;
constexpr int VIDEODB_DETAILS_MOVIE_PREMIERED           = VIDEODB_MAX_COLUMNS + 4;
constexpr int VIDEODB_DETAILS_MOVIE_ORIGINAL_LANGUAGE   = VIDEODB_MAX_COLUMNS + 5;
// *** IMPORTANT *** update the last attribute index below after adding columns to the movie table
constexpr int VIDEODB_DETAILS_MOVIE_TABLE_LAST_ATTR     = VIDEODB_DETAILS_MOVIE_ORIGINAL_LANGUAGE;

constexpr int VIDEODB_DETAILS_MOVIE_SET_NAME            = VIDEODB_DETAILS_MOVIE_TABLE_LAST_ATTR + 1;
constexpr int VIDEODB_DETAILS_MOVIE_SET_OVERVIEW        = VIDEODB_DETAILS_MOVIE_TABLE_LAST_ATTR + 2;
constexpr int VIDEODB_DETAILS_MOVIE_SET_ORIGINALNAME    = VIDEODB_DETAILS_MOVIE_TABLE_LAST_ATTR + 3;
constexpr int VIDEODB_DETAILS_MOVIE_FILE                = VIDEODB_DETAILS_MOVIE_TABLE_LAST_ATTR + 4;
constexpr int VIDEODB_DETAILS_MOVIE_PATH                = VIDEODB_DETAILS_MOVIE_TABLE_LAST_ATTR + 5;
constexpr int VIDEODB_DETAILS_MOVIE_PLAYCOUNT           = VIDEODB_DETAILS_MOVIE_TABLE_LAST_ATTR + 6;
constexpr int VIDEODB_DETAILS_MOVIE_LASTPLAYED          = VIDEODB_DETAILS_MOVIE_TABLE_LAST_ATTR + 7;
constexpr int VIDEODB_DETAILS_MOVIE_DATEADDED           = VIDEODB_DETAILS_MOVIE_TABLE_LAST_ATTR + 8;
constexpr int VIDEODB_DETAILS_MOVIE_RESUME_TIME         = VIDEODB_DETAILS_MOVIE_TABLE_LAST_ATTR + 9;
constexpr int VIDEODB_DETAILS_MOVIE_TOTAL_TIME          = VIDEODB_DETAILS_MOVIE_TABLE_LAST_ATTR + 10;
constexpr int VIDEODB_DETAILS_MOVIE_PLAYER_STATE        = VIDEODB_DETAILS_MOVIE_TABLE_LAST_ATTR + 11;
constexpr int VIDEODB_DETAILS_MOVIE_RATING              = VIDEODB_DETAILS_MOVIE_TABLE_LAST_ATTR + 12;
constexpr int VIDEODB_DETAILS_MOVIE_VOTES               = VIDEODB_DETAILS_MOVIE_TABLE_LAST_ATTR + 13;
constexpr int VIDEODB_DETAILS_MOVIE_RATING_TYPE         = VIDEODB_DETAILS_MOVIE_TABLE_LAST_ATTR + 14;
constexpr int VIDEODB_DETAILS_MOVIE_UNIQUEID_VALUE      = VIDEODB_DETAILS_MOVIE_TABLE_LAST_ATTR + 15;
constexpr int VIDEODB_DETAILS_MOVIE_UNIQUEID_TYPE       = VIDEODB_DETAILS_MOVIE_TABLE_LAST_ATTR + 16;
constexpr int VIDEODB_DETAILS_MOVIE_HASVERSIONS         = VIDEODB_DETAILS_MOVIE_TABLE_LAST_ATTR + 17;
constexpr int VIDEODB_DETAILS_MOVIE_HASEXTRAS           = VIDEODB_DETAILS_MOVIE_TABLE_LAST_ATTR + 18;
constexpr int VIDEODB_DETAILS_MOVIE_ISDEFAULTVERSION    = VIDEODB_DETAILS_MOVIE_TABLE_LAST_ATTR + 19;
constexpr int VIDEODB_DETAILS_MOVIE_VERSION_FILEID      = VIDEODB_DETAILS_MOVIE_TABLE_LAST_ATTR + 20;
constexpr int VIDEODB_DETAILS_MOVIE_VERSION_TYPEID      = VIDEODB_DETAILS_MOVIE_TABLE_LAST_ATTR + 21;
constexpr int VIDEODB_DETAILS_MOVIE_VERSION_TYPENAME    = VIDEODB_DETAILS_MOVIE_TABLE_LAST_ATTR + 22;
constexpr int VIDEODB_DETAILS_MOVIE_VERSION_ITEMTYPE    = VIDEODB_DETAILS_MOVIE_TABLE_LAST_ATTR + 23;

// episode_view columns past idEpisode idFile c00-cxx
constexpr int VIDEODB_DETAILS_EPISODE_TVSHOW_ID         = VIDEODB_MAX_COLUMNS + 2;
constexpr int VIDEODB_DETAILS_EPISODE_USER_RATING       = VIDEODB_MAX_COLUMNS + 3;
constexpr int VIDEODB_DETAILS_EPISODE_SEASON_ID         = VIDEODB_MAX_COLUMNS + 4;
// *** IMPORTANT *** update the last attribute index below after adding columns to the episode table
constexpr int VIDEODB_DETAILS_EPISODE_TABLE_LAST_ATTR   = VIDEODB_DETAILS_EPISODE_SEASON_ID;

constexpr int VIDEODB_DETAILS_EPISODE_FILE              = VIDEODB_DETAILS_EPISODE_TABLE_LAST_ATTR + 1;
constexpr int VIDEODB_DETAILS_EPISODE_PATH              = VIDEODB_DETAILS_EPISODE_TABLE_LAST_ATTR + 2;
constexpr int VIDEODB_DETAILS_EPISODE_PLAYCOUNT         = VIDEODB_DETAILS_EPISODE_TABLE_LAST_ATTR + 3;
constexpr int VIDEODB_DETAILS_EPISODE_LASTPLAYED        = VIDEODB_DETAILS_EPISODE_TABLE_LAST_ATTR + 4;
constexpr int VIDEODB_DETAILS_EPISODE_DATEADDED         = VIDEODB_DETAILS_EPISODE_TABLE_LAST_ATTR + 5;
constexpr int VIDEODB_DETAILS_EPISODE_TVSHOW_NAME       = VIDEODB_DETAILS_EPISODE_TABLE_LAST_ATTR + 6;
constexpr int VIDEODB_DETAILS_EPISODE_TVSHOW_GENRE      = VIDEODB_DETAILS_EPISODE_TABLE_LAST_ATTR + 7;
constexpr int VIDEODB_DETAILS_EPISODE_TVSHOW_STUDIO     = VIDEODB_DETAILS_EPISODE_TABLE_LAST_ATTR + 8;
constexpr int VIDEODB_DETAILS_EPISODE_TVSHOW_AIRED      = VIDEODB_DETAILS_EPISODE_TABLE_LAST_ATTR + 9;
constexpr int VIDEODB_DETAILS_EPISODE_TVSHOW_MPAA       = VIDEODB_DETAILS_EPISODE_TABLE_LAST_ATTR + 10;
constexpr int VIDEODB_DETAILS_EPISODE_TVSHOW_ORIGINAL_LANGUAGE = VIDEODB_DETAILS_EPISODE_TABLE_LAST_ATTR + 11;
constexpr int VIDEODB_DETAILS_EPISODE_RESUME_TIME       = VIDEODB_DETAILS_EPISODE_TABLE_LAST_ATTR + 12;
constexpr int VIDEODB_DETAILS_EPISODE_TOTAL_TIME        = VIDEODB_DETAILS_EPISODE_TABLE_LAST_ATTR + 13;
constexpr int VIDEODB_DETAILS_EPISODE_PLAYER_STATE      = VIDEODB_DETAILS_EPISODE_TABLE_LAST_ATTR + 14;
constexpr int VIDEODB_DETAILS_EPISODE_RATING            = VIDEODB_DETAILS_EPISODE_TABLE_LAST_ATTR + 15;
constexpr int VIDEODB_DETAILS_EPISODE_VOTES             = VIDEODB_DETAILS_EPISODE_TABLE_LAST_ATTR + 16;
constexpr int VIDEODB_DETAILS_EPISODE_RATING_TYPE       = VIDEODB_DETAILS_EPISODE_TABLE_LAST_ATTR + 17;
constexpr int VIDEODB_DETAILS_EPISODE_UNIQUEID_VALUE    = VIDEODB_DETAILS_EPISODE_TABLE_LAST_ATTR + 18;
constexpr int VIDEODB_DETAILS_EPISODE_UNIQUEID_TYPE     = VIDEODB_DETAILS_EPISODE_TABLE_LAST_ATTR + 19;

// tvshow_view columns past idShow c00-cxx (tvshow table)
constexpr int VIDEODB_DETAILS_TVSHOW_USER_RATING        = VIDEODB_MAX_COLUMNS + 1;
constexpr int VIDEODB_DETAILS_TVSHOW_DURATION           = VIDEODB_MAX_COLUMNS + 2;
constexpr int VIDEODB_DETAILS_TVSHOW_ORIGINAL_LANGUAGE  = VIDEODB_MAX_COLUMNS + 3;
constexpr int VIDEODB_DETAILS_TVSHOW_TAGLINE            = VIDEODB_MAX_COLUMNS + 4;
// *** IMPORTANT *** update the last attribute index below after adding columns to the tvshow table
constexpr int VIDEODB_DETAILS_TVSHOW_TABLE_LAST_ATTR    = VIDEODB_DETAILS_TVSHOW_TAGLINE;

constexpr int VIDEODB_DETAILS_TVSHOW_PARENTPATHID       = VIDEODB_DETAILS_TVSHOW_TABLE_LAST_ATTR + 1;
constexpr int VIDEODB_DETAILS_TVSHOW_PATH               = VIDEODB_DETAILS_TVSHOW_TABLE_LAST_ATTR + 2;
constexpr int VIDEODB_DETAILS_TVSHOW_DATEADDED          = VIDEODB_DETAILS_TVSHOW_TABLE_LAST_ATTR + 3;
constexpr int VIDEODB_DETAILS_TVSHOW_LASTPLAYED         = VIDEODB_DETAILS_TVSHOW_TABLE_LAST_ATTR + 4;
constexpr int VIDEODB_DETAILS_TVSHOW_NUM_EPISODES       = VIDEODB_DETAILS_TVSHOW_TABLE_LAST_ATTR + 5;
constexpr int VIDEODB_DETAILS_TVSHOW_NUM_WATCHED        = VIDEODB_DETAILS_TVSHOW_TABLE_LAST_ATTR + 6;
constexpr int VIDEODB_DETAILS_TVSHOW_NUM_SEASONS        = VIDEODB_DETAILS_TVSHOW_TABLE_LAST_ATTR + 7;
constexpr int VIDEODB_DETAILS_TVSHOW_RATING             = VIDEODB_DETAILS_TVSHOW_TABLE_LAST_ATTR + 8;
constexpr int VIDEODB_DETAILS_TVSHOW_VOTES              = VIDEODB_DETAILS_TVSHOW_TABLE_LAST_ATTR + 9;
constexpr int VIDEODB_DETAILS_TVSHOW_RATING_TYPE        = VIDEODB_DETAILS_TVSHOW_TABLE_LAST_ATTR + 10;
constexpr int VIDEODB_DETAILS_TVSHOW_UNIQUEID_VALUE     = VIDEODB_DETAILS_TVSHOW_TABLE_LAST_ATTR + 11;
constexpr int VIDEODB_DETAILS_TVSHOW_UNIQUEID_TYPE      = VIDEODB_DETAILS_TVSHOW_TABLE_LAST_ATTR + 12;
constexpr int VIDEODB_DETAILS_TVSHOW_NUM_INPROGRESS     = VIDEODB_DETAILS_TVSHOW_TABLE_LAST_ATTR + 13;

// musicvideo_view columns past idMVideo idFile c00-cxx (musicvideo table)
constexpr int VIDEODB_DETAILS_MUSICVIDEO_USER_RATING    = VIDEODB_MAX_COLUMNS + 2;
constexpr int VIDEODB_DETAILS_MUSICVIDEO_PREMIERED      = VIDEODB_MAX_COLUMNS + 3;
// *** IMPORTANT *** update the last attribute index below after adding columns to the musicvideo table
constexpr int VIDEODB_DETAILS_MUSICVIDEO_TABLE_LAST_ATTR = VIDEODB_DETAILS_MUSICVIDEO_PREMIERED;

constexpr int VIDEODB_DETAILS_MUSICVIDEO_FILE           = VIDEODB_DETAILS_MUSICVIDEO_TABLE_LAST_ATTR + 1;
constexpr int VIDEODB_DETAILS_MUSICVIDEO_PATH           = VIDEODB_DETAILS_MUSICVIDEO_TABLE_LAST_ATTR + 2;
constexpr int VIDEODB_DETAILS_MUSICVIDEO_PLAYCOUNT      = VIDEODB_DETAILS_MUSICVIDEO_TABLE_LAST_ATTR + 3;
constexpr int VIDEODB_DETAILS_MUSICVIDEO_LASTPLAYED     = VIDEODB_DETAILS_MUSICVIDEO_TABLE_LAST_ATTR + 4;
constexpr int VIDEODB_DETAILS_MUSICVIDEO_DATEADDED      = VIDEODB_DETAILS_MUSICVIDEO_TABLE_LAST_ATTR + 5;
constexpr int VIDEODB_DETAILS_MUSICVIDEO_RESUME_TIME    = VIDEODB_DETAILS_MUSICVIDEO_TABLE_LAST_ATTR + 6;
constexpr int VIDEODB_DETAILS_MUSICVIDEO_TOTAL_TIME     = VIDEODB_DETAILS_MUSICVIDEO_TABLE_LAST_ATTR + 7;
constexpr int VIDEODB_DETAILS_MUSICVIDEO_PLAYER_STATE   = VIDEODB_DETAILS_MUSICVIDEO_TABLE_LAST_ATTR + 8;
constexpr int VIDEODB_DETAILS_MUSICVIDEO_UNIQUEID_VALUE = VIDEODB_DETAILS_MUSICVIDEO_TABLE_LAST_ATTR + 9;
constexpr int VIDEODB_DETAILS_MUSICVIDEO_UNIQUEID_TYPE  = VIDEODB_DETAILS_MUSICVIDEO_TABLE_LAST_ATTR + 10;

constexpr int VIDEODB_TYPE_UNUSED       = 0;
constexpr int VIDEODB_TYPE_STRING       = 1;
constexpr int VIDEODB_TYPE_INT          = 2;
constexpr int VIDEODB_TYPE_FLOAT        = 3;
constexpr int VIDEODB_TYPE_BOOL         = 4;
constexpr int VIDEODB_TYPE_COUNT        = 5;
constexpr int VIDEODB_TYPE_STRINGARRAY  = 6;
constexpr int VIDEODB_TYPE_DATE         = 7;
constexpr int VIDEODB_TYPE_DATETIME     = 8;
// clang-format on

enum VIDEODB_IDS // this enum MUST match the offset struct further down!! and make sure to keep min and max at -1 and sizeof(offsets)
{
  VIDEODB_ID_MIN = -1,
  VIDEODB_ID_TITLE = 0,
  VIDEODB_ID_PLOT = 1,
  VIDEODB_ID_PLOTOUTLINE = 2,
  VIDEODB_ID_TAGLINE = 3,
  VIDEODB_ID_VOTES = 4, // unused
  VIDEODB_ID_RATING_ID = 5,
  VIDEODB_ID_CREDITS = 6,
  VIDEODB_ID_YEAR = 7, // unused
  VIDEODB_ID_THUMBURL = 8,
  VIDEODB_ID_IDENT_ID = 9,
  VIDEODB_ID_SORTTITLE = 10,
  VIDEODB_ID_RUNTIME = 11,
  VIDEODB_ID_MPAA = 12,
  VIDEODB_ID_TOP250 = 13,
  VIDEODB_ID_GENRE = 14,
  VIDEODB_ID_DIRECTOR = 15,
  VIDEODB_ID_ORIGINALTITLE = 16,
  VIDEODB_ID_THUMBURL_SPOOF = 17,
  VIDEODB_ID_STUDIOS = 18,
  VIDEODB_ID_TRAILER = 19,
  VIDEODB_ID_FANART = 20,
  VIDEODB_ID_COUNTRY = 21,
  VIDEODB_ID_BASEPATH = 22,
  VIDEODB_ID_PARENTPATHID = 23,
  VIDEODB_ID_MAX
};

struct SDbTableOffsets
{
  int type{VIDEODB_TYPE_UNUSED};
  std::size_t offset{0};
};

constexpr int DB_MOVIE_OFFSETS_SIZE = 24;

const std::array<SDbTableOffsets, DB_MOVIE_OFFSETS_SIZE> DbMovieOffsets = {{
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strTitle)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strPlot)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strPlotOutline)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strTagLine)},
    {VIDEODB_TYPE_UNUSED, 0}, // unused
    {VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag, m_iIdRating)},
    {VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag, m_writingCredits)},
    {VIDEODB_TYPE_UNUSED, 0}, // unused
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strPictureURL.m_data)},
    {VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag, m_iIdUniqueID)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strSortTitle)},
    {VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag, m_duration)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strMPAARating)},
    {VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag, m_iTop250)},
    {VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag, m_genre)},
    {VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag, m_director)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strOriginalTitle)},
    {VIDEODB_TYPE_UNUSED, 0}, // unused
    {VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag, m_studio)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strTrailer)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_fanart.m_xml)},
    {VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag, m_country)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_basePath)},
    {VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag, m_parentPathID)},
}};

enum VIDEODB_SET_IDS // this enum MUST match the offset struct further down!! and make sure to keep min and max at -1 and sizeof(offsets)
{
  VIDEODB_ID_SET_MIN = -1,
  VIDEODB_ID_SET_TITLE = 0,
  VIDEODB_ID_SET_OVERVIEW = 1,
  VIDEODB_ID_SET_ORIGINALTITLE = 2,
  VIDEODB_ID_SET_MAX
};

constexpr int DB_SET_OFFSETS_SIZE = 3;

const std::array<SDbTableOffsets, DB_SET_OFFSETS_SIZE> DbSetOffsets = {{
    {VIDEODB_TYPE_STRING, my_offsetof(CSetInfoTag, m_title)},
    {VIDEODB_TYPE_STRING, my_offsetof(CSetInfoTag, m_overview)},
    {VIDEODB_TYPE_STRING, my_offsetof(CSetInfoTag, m_originalTitle)},
}};

enum VIDEODB_TV_IDS // this enum MUST match the offset struct further down!! and make sure to keep min and max at -1 and sizeof(offsets)
{
  VIDEODB_ID_TV_MIN = -1,
  VIDEODB_ID_TV_TITLE = 0,
  VIDEODB_ID_TV_PLOT = 1,
  VIDEODB_ID_TV_STATUS = 2,
  VIDEODB_ID_TV_VOTES = 3, // unused
  VIDEODB_ID_TV_RATING_ID = 4,
  VIDEODB_ID_TV_PREMIERED = 5,
  VIDEODB_ID_TV_THUMBURL = 6,
  VIDEODB_ID_TV_THUMBURL_SPOOF = 7,
  VIDEODB_ID_TV_GENRE = 8,
  VIDEODB_ID_TV_ORIGINALTITLE = 9,
  VIDEODB_ID_TV_EPISODEGUIDE = 10,
  VIDEODB_ID_TV_FANART = 11,
  VIDEODB_ID_TV_IDENT_ID = 12,
  VIDEODB_ID_TV_MPAA = 13,
  VIDEODB_ID_TV_STUDIOS = 14,
  VIDEODB_ID_TV_SORTTITLE = 15,
  VIDEODB_ID_TV_TRAILER = 16,
  VIDEODB_ID_TV_MAX
};

constexpr int DB_TVSHOW_OFFSETS_SIZE = 17;

const std::array<SDbTableOffsets, DB_TVSHOW_OFFSETS_SIZE> DbTvShowOffsets = {{
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strTitle)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strPlot)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strStatus)},
    {VIDEODB_TYPE_UNUSED, 0}, //unused
    {VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag, m_iIdRating)},
    {VIDEODB_TYPE_DATE, my_offsetof(CVideoInfoTag, m_premiered)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strPictureURL.m_data)},
    {VIDEODB_TYPE_UNUSED, 0}, // unused
    {VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag, m_genre)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strOriginalTitle)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strEpisodeGuide)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_fanart.m_xml)},
    {VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag, m_iIdUniqueID)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strMPAARating)},
    {VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag, m_studio)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strSortTitle)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strTrailer)},
}};

//! @todo is this comment valid for seasons? There is no offset structure or am I wrong?
enum VIDEODB_SEASON_IDS // this enum MUST match the offset struct further down!! and make sure to keep min and max at -1 and sizeof(offsets)
{
  VIDEODB_ID_SEASON_MIN = -1,
  VIDEODB_ID_SEASON_ID = 0,
  VIDEODB_ID_SEASON_TVSHOW_ID = 1,
  VIDEODB_ID_SEASON_NUMBER = 2,
  VIDEODB_ID_SEASON_NAME = 3,
  VIDEODB_ID_SEASON_USER_RATING = 4,
  VIDEODB_ID_SEASON_TVSHOW_PATH = 5,
  VIDEODB_ID_SEASON_TVSHOW_TITLE = 6,
  VIDEODB_ID_SEASON_TVSHOW_PLOT = 7,
  VIDEODB_ID_SEASON_TVSHOW_PREMIERED = 8,
  VIDEODB_ID_SEASON_TVSHOW_GENRE = 9,
  VIDEODB_ID_SEASON_TVSHOW_STUDIO = 10,
  VIDEODB_ID_SEASON_TVSHOW_MPAA = 11,
  VIDEODB_ID_SEASON_EPISODES_TOTAL = 12,
  VIDEODB_ID_SEASON_EPISODES_WATCHED = 13,
  VIDEODB_ID_SEASON_PREMIERED = 14,
  VIDEODB_ID_SEASON_EPISODES_INPROGRESS = 15,
  VIDEODB_ID_SEASON_PLOT = 16,
  VIDEODB_ID_SEASON_MAX
};

enum VIDEODB_EPISODE_IDS // this enum MUST match the offset struct further down!! and make sure to keep min and max at -1 and sizeof(offsets)
{
  VIDEODB_ID_EPISODE_MIN = -1,
  VIDEODB_ID_EPISODE_TITLE = 0,
  VIDEODB_ID_EPISODE_PLOT = 1,
  VIDEODB_ID_EPISODE_VOTES = 2, // unused
  VIDEODB_ID_EPISODE_RATING_ID = 3,
  VIDEODB_ID_EPISODE_CREDITS = 4,
  VIDEODB_ID_EPISODE_AIRED = 5,
  VIDEODB_ID_EPISODE_THUMBURL = 6,
  VIDEODB_ID_EPISODE_THUMBURL_SPOOF = 7,
  VIDEODB_ID_EPISODE_PLAYCOUNT = 8, // unused - feel free to repurpose
  VIDEODB_ID_EPISODE_RUNTIME = 9,
  VIDEODB_ID_EPISODE_DIRECTOR = 10,
  VIDEODB_ID_EPISODE_PRODUCTIONCODE = 11,
  VIDEODB_ID_EPISODE_SEASON = 12,
  VIDEODB_ID_EPISODE_EPISODE = 13,
  VIDEODB_ID_EPISODE_ORIGINALTITLE = 14,
  VIDEODB_ID_EPISODE_SORTSEASON = 15,
  VIDEODB_ID_EPISODE_SORTEPISODE = 16,
  VIDEODB_ID_EPISODE_BOOKMARK = 17,
  VIDEODB_ID_EPISODE_BASEPATH = 18,
  VIDEODB_ID_EPISODE_PARENTPATHID = 19,
  VIDEODB_ID_EPISODE_IDENT_ID = 20,
  VIDEODB_ID_EPISODE_MAX
};

constexpr int DB_EPISODE_OFFSETS_SIZE = 21;

const std::array<SDbTableOffsets, DB_EPISODE_OFFSETS_SIZE> DbEpisodeOffsets = {{
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strTitle)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strPlot)},
    {VIDEODB_TYPE_UNUSED, 0}, // unused
    {VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag, m_iIdRating)},
    {VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag, m_writingCredits)},
    {VIDEODB_TYPE_DATE, my_offsetof(CVideoInfoTag, m_firstAired)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strPictureURL.m_data)},
    {VIDEODB_TYPE_UNUSED, 0}, // unused
    {VIDEODB_TYPE_UNUSED, 0}, // unused
    {VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag, m_duration)},
    {VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag, m_director)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strProductionCode)},
    {VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag, m_iSeason)},
    {VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag, m_iEpisode)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strOriginalTitle)},
    {VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag, m_iSpecialSortSeason)},
    {VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag, m_iSpecialSortEpisode)},
    {VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag, m_iBookmarkId)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_basePath)},
    {VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag, m_parentPathID)},
    {VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag, m_iIdUniqueID)},
}};

enum VIDEODB_MUSICVIDEO_IDS // this enum MUST match the offset struct further down!! and make sure to keep min and max at -1 and sizeof(offsets)
{
  VIDEODB_ID_MUSICVIDEO_MIN = -1,
  VIDEODB_ID_MUSICVIDEO_TITLE = 0,
  VIDEODB_ID_MUSICVIDEO_THUMBURL = 1,
  VIDEODB_ID_MUSICVIDEO_THUMBURL_SPOOF = 2,
  VIDEODB_ID_MUSICVIDEO_PLAYCOUNT = 3, // unused - feel free to repurpose
  VIDEODB_ID_MUSICVIDEO_RUNTIME = 4,
  VIDEODB_ID_MUSICVIDEO_DIRECTOR = 5,
  VIDEODB_ID_MUSICVIDEO_STUDIOS = 6,
  VIDEODB_ID_MUSICVIDEO_YEAR = 7, // unused
  VIDEODB_ID_MUSICVIDEO_PLOT = 8,
  VIDEODB_ID_MUSICVIDEO_ALBUM = 9,
  VIDEODB_ID_MUSICVIDEO_ARTIST = 10,
  VIDEODB_ID_MUSICVIDEO_GENRE = 11,
  VIDEODB_ID_MUSICVIDEO_TRACK = 12,
  VIDEODB_ID_MUSICVIDEO_BASEPATH = 13,
  VIDEODB_ID_MUSICVIDEO_PARENTPATHID = 14,
  VIDEODB_ID_MUSICVIDEO_IDENT_ID = 15,
  VIDEODB_ID_MUSICVIDEO_MAX
};

constexpr int DB_MUSIC_VIDEO_OFFSETS_SIZE = 16;

const std::array<SDbTableOffsets, DB_MUSIC_VIDEO_OFFSETS_SIZE> DbMusicVideoOffsets = {{
    {VIDEODB_TYPE_STRING, my_offsetof(class CVideoInfoTag, m_strTitle)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strPictureURL.m_data)},
    {VIDEODB_TYPE_UNUSED, 0}, // unused
    {VIDEODB_TYPE_UNUSED, 0}, // unused
    {VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag, m_duration)},
    {VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag, m_director)},
    {VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag, m_studio)},
    {VIDEODB_TYPE_UNUSED, 0}, // unused
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strPlot)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_strAlbum)},
    {VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag, m_artist)},
    {VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag, m_genre)},
    {VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag, m_iTrack)},
    {VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag, m_basePath)},
    {VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag, m_parentPathID)},
    {VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag, m_iIdUniqueID)},
}};
