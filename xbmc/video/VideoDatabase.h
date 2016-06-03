#pragma once
/*
 *      Copyright (C) 2016 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "addons/Scraper.h"
#include "Bookmark.h"
#include "dbwrappers/Database.h"
#include "utils/SortUtils.h"
#include "video/VideoDbUrl.h"
#include "VideoInfoTag.h"

class CFileItem;
class CFileItemList;
class CVideoSettings;
class CGUIDialogProgress;
class CGUIDialogProgressBarHandle;

namespace dbiplus
{
  class field_value;
  typedef std::vector<field_value> sql_record;
}

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
#define my_offsetof(TYPE, MEMBER) \
               ((size_t)((char *)&(((TYPE *)0x10)->MEMBER) - (char*)0x10))
#endif
#endif

typedef std::vector<CVideoInfoTag> VECMOVIES;

namespace VIDEO
{
  class IVideoInfoScannerObserver;
  struct SScanSettings;
}

enum VideoDbDetails
{
  VideoDbDetailsNone     = 0x00,
  VideoDbDetailsRating   = 0x01,
  VideoDbDetailsTag      = 0x02,
  VideoDbDetailsShowLink = 0x04,
  VideoDbDetailsStream   = 0x08,
  VideoDbDetailsCast     = 0x10,
  VideoDbDetailsBookmark = 0x20,
  VideoDbDetailsUniqueID = 0x40,
  VideoDbDetailsAll      = 0xFF
} ;

// these defines are based on how many columns we have and which column certain data is going to be in
// when we do GetDetailsForMovie()
#define VIDEODB_MAX_COLUMNS 24
#define VIDEODB_DETAILS_FILEID      1

#define VIDEODB_DETAILS_MOVIE_SET_ID            VIDEODB_MAX_COLUMNS + 2
#define VIDEODB_DETAILS_MOVIE_USER_RATING       VIDEODB_MAX_COLUMNS + 3
#define VIDEODB_DETAILS_MOVIE_PREMIERED         VIDEODB_MAX_COLUMNS + 4
#define VIDEODB_DETAILS_MOVIE_SET_NAME          VIDEODB_MAX_COLUMNS + 5
#define VIDEODB_DETAILS_MOVIE_SET_OVERVIEW      VIDEODB_MAX_COLUMNS + 6
#define VIDEODB_DETAILS_MOVIE_FILE              VIDEODB_MAX_COLUMNS + 7
#define VIDEODB_DETAILS_MOVIE_PATH              VIDEODB_MAX_COLUMNS + 8
#define VIDEODB_DETAILS_MOVIE_PLAYCOUNT         VIDEODB_MAX_COLUMNS + 9
#define VIDEODB_DETAILS_MOVIE_LASTPLAYED        VIDEODB_MAX_COLUMNS + 10
#define VIDEODB_DETAILS_MOVIE_DATEADDED         VIDEODB_MAX_COLUMNS + 11
#define VIDEODB_DETAILS_MOVIE_RESUME_TIME       VIDEODB_MAX_COLUMNS + 12
#define VIDEODB_DETAILS_MOVIE_TOTAL_TIME        VIDEODB_MAX_COLUMNS + 13
#define VIDEODB_DETAILS_MOVIE_RATING            VIDEODB_MAX_COLUMNS + 14
#define VIDEODB_DETAILS_MOVIE_VOTES             VIDEODB_MAX_COLUMNS + 15
#define VIDEODB_DETAILS_MOVIE_RATING_TYPE       VIDEODB_MAX_COLUMNS + 16
#define VIDEODB_DETAILS_MOVIE_UNIQUEID_VALUE    VIDEODB_MAX_COLUMNS + 17
#define VIDEODB_DETAILS_MOVIE_UNIQUEID_TYPE     VIDEODB_MAX_COLUMNS + 18

#define VIDEODB_DETAILS_EPISODE_TVSHOW_ID       VIDEODB_MAX_COLUMNS + 2
#define VIDEODB_DETAILS_EPISODE_USER_RATING     VIDEODB_MAX_COLUMNS + 3
#define VIDEODB_DETAILS_EPISODE_SEASON_ID       VIDEODB_MAX_COLUMNS + 4
#define VIDEODB_DETAILS_EPISODE_FILE            VIDEODB_MAX_COLUMNS + 5
#define VIDEODB_DETAILS_EPISODE_PATH            VIDEODB_MAX_COLUMNS + 6
#define VIDEODB_DETAILS_EPISODE_PLAYCOUNT       VIDEODB_MAX_COLUMNS + 7
#define VIDEODB_DETAILS_EPISODE_LASTPLAYED      VIDEODB_MAX_COLUMNS + 8
#define VIDEODB_DETAILS_EPISODE_DATEADDED       VIDEODB_MAX_COLUMNS + 9
#define VIDEODB_DETAILS_EPISODE_TVSHOW_NAME     VIDEODB_MAX_COLUMNS + 10
#define VIDEODB_DETAILS_EPISODE_TVSHOW_GENRE    VIDEODB_MAX_COLUMNS + 11
#define VIDEODB_DETAILS_EPISODE_TVSHOW_STUDIO   VIDEODB_MAX_COLUMNS + 12
#define VIDEODB_DETAILS_EPISODE_TVSHOW_AIRED    VIDEODB_MAX_COLUMNS + 13
#define VIDEODB_DETAILS_EPISODE_TVSHOW_MPAA     VIDEODB_MAX_COLUMNS + 14
#define VIDEODB_DETAILS_EPISODE_RESUME_TIME     VIDEODB_MAX_COLUMNS + 15
#define VIDEODB_DETAILS_EPISODE_TOTAL_TIME      VIDEODB_MAX_COLUMNS + 16
#define VIDEODB_DETAILS_EPISODE_RATING          VIDEODB_MAX_COLUMNS + 17
#define VIDEODB_DETAILS_EPISODE_VOTES           VIDEODB_MAX_COLUMNS + 18
#define VIDEODB_DETAILS_EPISODE_RATING_TYPE     VIDEODB_MAX_COLUMNS + 19
#define VIDEODB_DETAILS_EPISODE_UNIQUEID_VALUE  VIDEODB_MAX_COLUMNS + 20
#define VIDEODB_DETAILS_EPISODE_UNIQUEID_TYPE   VIDEODB_MAX_COLUMNS + 21

#define VIDEODB_DETAILS_TVSHOW_USER_RATING      VIDEODB_MAX_COLUMNS + 1
#define VIDEODB_DETAILS_TVSHOW_DURATION         VIDEODB_MAX_COLUMNS + 2
#define VIDEODB_DETAILS_TVSHOW_PARENTPATHID     VIDEODB_MAX_COLUMNS + 3
#define VIDEODB_DETAILS_TVSHOW_PATH             VIDEODB_MAX_COLUMNS + 4
#define VIDEODB_DETAILS_TVSHOW_DATEADDED        VIDEODB_MAX_COLUMNS + 5
#define VIDEODB_DETAILS_TVSHOW_LASTPLAYED       VIDEODB_MAX_COLUMNS + 6
#define VIDEODB_DETAILS_TVSHOW_NUM_EPISODES     VIDEODB_MAX_COLUMNS + 7
#define VIDEODB_DETAILS_TVSHOW_NUM_WATCHED      VIDEODB_MAX_COLUMNS + 8
#define VIDEODB_DETAILS_TVSHOW_NUM_SEASONS      VIDEODB_MAX_COLUMNS + 9
#define VIDEODB_DETAILS_TVSHOW_RATING           VIDEODB_MAX_COLUMNS + 10
#define VIDEODB_DETAILS_TVSHOW_VOTES            VIDEODB_MAX_COLUMNS + 11
#define VIDEODB_DETAILS_TVSHOW_RATING_TYPE      VIDEODB_MAX_COLUMNS + 12
#define VIDEODB_DETAILS_TVSHOW_UNIQUEID_VALUE   VIDEODB_MAX_COLUMNS + 13
#define VIDEODB_DETAILS_TVSHOW_UNIQUEID_TYPE    VIDEODB_MAX_COLUMNS + 14

#define VIDEODB_DETAILS_MUSICVIDEO_USER_RATING  VIDEODB_MAX_COLUMNS + 2
#define VIDEODB_DETAILS_MUSICVIDEO_PREMIERED    VIDEODB_MAX_COLUMNS + 3
#define VIDEODB_DETAILS_MUSICVIDEO_FILE         VIDEODB_MAX_COLUMNS + 4
#define VIDEODB_DETAILS_MUSICVIDEO_PATH         VIDEODB_MAX_COLUMNS + 5
#define VIDEODB_DETAILS_MUSICVIDEO_PLAYCOUNT    VIDEODB_MAX_COLUMNS + 6
#define VIDEODB_DETAILS_MUSICVIDEO_LASTPLAYED   VIDEODB_MAX_COLUMNS + 7
#define VIDEODB_DETAILS_MUSICVIDEO_DATEADDED    VIDEODB_MAX_COLUMNS + 8
#define VIDEODB_DETAILS_MUSICVIDEO_RESUME_TIME  VIDEODB_MAX_COLUMNS + 9
#define VIDEODB_DETAILS_MUSICVIDEO_TOTAL_TIME   VIDEODB_MAX_COLUMNS + 10

#define VIDEODB_TYPE_UNUSED 0
#define VIDEODB_TYPE_STRING 1
#define VIDEODB_TYPE_INT 2
#define VIDEODB_TYPE_FLOAT 3
#define VIDEODB_TYPE_BOOL 4
#define VIDEODB_TYPE_COUNT 5
#define VIDEODB_TYPE_STRINGARRAY 6
#define VIDEODB_TYPE_DATE 7
#define VIDEODB_TYPE_DATETIME 8

typedef enum
{
  VIDEODB_CONTENT_MOVIES = 1,
  VIDEODB_CONTENT_TVSHOWS = 2,
  VIDEODB_CONTENT_MUSICVIDEOS = 3,
  VIDEODB_CONTENT_EPISODES = 4,
  VIDEODB_CONTENT_MOVIE_SETS = 5
} VIDEODB_CONTENT_TYPE;

typedef enum // this enum MUST match the offset struct further down!! and make sure to keep min and max at -1 and sizeof(offsets)
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
} VIDEODB_IDS;

const struct SDbTableOffsets
{
  int type;
  size_t offset;
} DbMovieOffsets[] =
{
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strTitle) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPlot) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPlotOutline) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strTagLine) },
  { VIDEODB_TYPE_UNUSED, 0 }, // unused
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iIdRating) },
  { VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag,m_writingCredits) },
  { VIDEODB_TYPE_UNUSED, 0 }, // unused
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPictureURL.m_xml) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iIdUniqueID) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strSortTitle) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_duration) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strMPAARating) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iTop250) },
  { VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag,m_genre) },
  { VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag,m_director) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strOriginalTitle) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPictureURL.m_spoof) },
  { VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag,m_studio) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strTrailer) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_fanart.m_xml) },
  { VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag,m_country) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_basePath) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_parentPathID) }
};

typedef enum // this enum MUST match the offset struct further down!! and make sure to keep min and max at -1 and sizeof(offsets)
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
  VIDEODB_ID_TV_MAX
} VIDEODB_TV_IDS;

const struct SDbTableOffsets DbTvShowOffsets[] =
{
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strTitle) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPlot) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strStatus) },
  { VIDEODB_TYPE_UNUSED, 0 }, //unused
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iIdRating) },
  { VIDEODB_TYPE_DATE, my_offsetof(CVideoInfoTag,m_premiered) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPictureURL.m_xml) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPictureURL.m_spoof) },
  { VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag,m_genre) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strOriginalTitle)},
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strEpisodeGuide)},
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_fanart.m_xml)},
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iIdUniqueID)},
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strMPAARating)},
  { VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag,m_studio)},
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strSortTitle)},
};

//! @todo is this comment valid for seasons? There is no offset structure or am I wrong?
typedef enum // this enum MUST match the offset struct further down!! and make sure to keep min and max at -1 and sizeof(offsets)
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
  VIDEODB_ID_SEASON_MAX
} VIDEODB_SEASON_IDS;

typedef enum // this enum MUST match the offset struct further down!! and make sure to keep min and max at -1 and sizeof(offsets)
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
} VIDEODB_EPISODE_IDS;

const struct SDbTableOffsets DbEpisodeOffsets[] =
{
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strTitle) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPlot) },
  { VIDEODB_TYPE_UNUSED, 0 }, // unused
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iIdRating) },
  { VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag,m_writingCredits) },
  { VIDEODB_TYPE_DATE, my_offsetof(CVideoInfoTag,m_firstAired) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPictureURL.m_xml) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPictureURL.m_spoof) },
  { VIDEODB_TYPE_UNUSED, 0 }, // unused
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_duration) },
  { VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag,m_director) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strProductionCode) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iSeason) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iEpisode) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strOriginalTitle)},
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iSpecialSortSeason) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iSpecialSortEpisode) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iBookmarkId) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_basePath) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_parentPathID) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iIdUniqueID) }
};

typedef enum // this enum MUST match the offset struct further down!! and make sure to keep min and max at -1 and sizeof(offsets)
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
  VIDEODB_ID_MUSICVIDEO_MAX
} VIDEODB_MUSICVIDEO_IDS;

const struct SDbTableOffsets DbMusicVideoOffsets[] =
{
  { VIDEODB_TYPE_STRING, my_offsetof(class CVideoInfoTag,m_strTitle) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPictureURL.m_xml) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPictureURL.m_spoof) },
  { VIDEODB_TYPE_UNUSED, 0 }, // unused
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_duration) },
  { VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag,m_director) },
  { VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag,m_studio) },
  { VIDEODB_TYPE_UNUSED, 0 }, // unused
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPlot) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strAlbum) },
  { VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag,m_artist) },
  { VIDEODB_TYPE_STRINGARRAY, my_offsetof(CVideoInfoTag,m_genre) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iTrack) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_basePath) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_parentPathID) }
};

#define COMPARE_PERCENTAGE     0.90f // 90%
#define COMPARE_PERCENTAGE_MIN 0.50f // 50%

class CVideoDatabase : public CDatabase
{
public:

  class CActor    // used for actor retrieval for non-master users
  {
  public:
    std::string name;
    std::string thumb;
    int playcount;
    int appearances;
  };

  class CSeason   // used for season retrieval for non-master users
  {
  public:
    std::string path;
    std::vector<std::string> genre;
    int numEpisodes;
    int numWatched;
    int id;
  };

  class CSetInfo
  {
  public:
    std::string name;
    VECMOVIES movies;
    DatabaseResults results;
  };

  CVideoDatabase(void);
  virtual ~CVideoDatabase(void);

  virtual bool Open();
  virtual bool CommitTransaction();

  int AddMovie(const std::string& strFilenameAndPath);
  int AddEpisode(int idShow, const std::string& strFilenameAndPath);

  // editing functions
  /*! \brief Set the playcount of an item
   Sets the playcount and last played date to a given value
   \param item CFileItem to set the playcount for
   \param count The playcount to set.
   \param date The date the file was last viewed (does not denote the video was watched to completion).  If empty we current datetime (if count > 0) or never viewed (if count = 0).
   \sa GetPlayCount, IncrementPlayCount, UpdateLastPlayed
   */
  void SetPlayCount(const CFileItem &item, int count, const CDateTime &date = CDateTime());

  /*! \brief Increment the playcount of an item
   Increments the playcount and updates the last played date
   \param item CFileItem to increment the playcount for
   \sa GetPlayCount, SetPlayCount, GetPlayCounts
   */
  void IncrementPlayCount(const CFileItem &item);

  /*! \brief Get the playcount of an item
   \param item CFileItem to get the playcount for
   \return the playcount of the item, or -1 on error
   \sa SetPlayCount, IncrementPlayCount, GetPlayCounts
   */
  int GetPlayCount(const CFileItem &item);

  /*! \brief Get the playcount of a filename and path
   \param strFilenameAndPath filename and path to get the playcount for
   \return the playcount of the item, or -1 on error
   \sa SetPlayCount, IncrementPlayCount, GetPlayCounts
   */
  int GetPlayCount(const std::string& strFilenameAndPath);

  /*! \brief Update the last played time of an item
   Updates the last played date
   \param item CFileItem to update the last played time for
   \sa GetPlayCount, SetPlayCount, IncrementPlayCount, GetPlayCounts
   */
  void UpdateLastPlayed(const CFileItem &item);

  /*! \brief Get the playcount and resume point of a list of items
   Note that if the resume point is already set on an item, it won't be overridden.
   \param path the path to fetch videos from
   \param items CFileItemList to fetch the playcounts for
   \sa GetPlayCount, SetPlayCount, IncrementPlayCount
   */
  bool GetPlayCounts(const std::string &path, CFileItemList &items);

  void UpdateMovieTitle(int idMovie, const std::string& strNewMovieTitle, VIDEODB_CONTENT_TYPE iType=VIDEODB_CONTENT_MOVIES);
  bool UpdateVideoSortTitle(int idDb, const std::string& strNewSortTitle, VIDEODB_CONTENT_TYPE iType = VIDEODB_CONTENT_MOVIES);

  bool HasMovieInfo(const std::string& strFilenameAndPath);
  bool HasTvShowInfo(const std::string& strFilenameAndPath);
  bool HasEpisodeInfo(const std::string& strFilenameAndPath);
  bool HasMusicVideoInfo(const std::string& strFilenameAndPath);

  void GetFilePathById(int idMovie, std::string &filePath, VIDEODB_CONTENT_TYPE iType);
  std::string GetGenreById(int id);
  std::string GetCountryById(int id);
  std::string GetSetById(int id);
  std::string GetTagById(int id);
  std::string GetPersonById(int id);
  std::string GetStudioById(int id);
  std::string GetTvShowTitleById(int id);
  std::string GetMusicVideoAlbumById(int id);
  int GetTvShowForEpisode(int idEpisode);
  int GetSeasonForEpisode(int idEpisode);

  bool LoadVideoInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details, int getDetails = VideoDbDetailsAll);
  bool GetMovieInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details, int idMovie = -1, int getDetails = VideoDbDetailsAll);
  bool GetTvShowInfo(const std::string& strPath, CVideoInfoTag& details, int idTvShow = -1, CFileItem* item = NULL, int getDetails = VideoDbDetailsAll);
  bool GetSeasonInfo(int idSeason, CVideoInfoTag& details);
  bool GetEpisodeInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details, int idEpisode = -1, int getDetails = VideoDbDetailsAll);
  bool GetMusicVideoInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details, int idMVideo = -1, int getDetails = VideoDbDetailsAll);
  bool GetSetInfo(int idSet, CVideoInfoTag& details);
  bool GetFileInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details, int idFile = -1);

  int GetPathId(const std::string& strPath);
  int GetTvShowId(const std::string& strPath);
  int GetEpisodeId(const std::string& strFilenameAndPath, int idEpisode=-1, int idSeason=-1); // idEpisode, idSeason are used for multipart episodes as hints
  int GetSeasonId(int idShow, int season);

  void GetEpisodesByFile(const std::string& strFilenameAndPath, std::vector<CVideoInfoTag>& episodes);

  int SetDetailsForItem(CVideoInfoTag& details, const std::map<std::string, std::string> &artwork);
  int SetDetailsForItem(int id, const MediaType& mediaType, CVideoInfoTag& details, const std::map<std::string, std::string> &artwork);

  int SetDetailsForMovie(const std::string& strFilenameAndPath, CVideoInfoTag& details, const std::map<std::string, std::string> &artwork, int idMovie = -1);
  int SetDetailsForMovieSet(const CVideoInfoTag& details, const std::map<std::string, std::string> &artwork, int idSet = -1);

  /*! \brief add a tvshow to the library, setting metdata detail
   First checks for whether this TV Show is already in the database (based on idTvShow, or via GetMatchingTvShow)
   and if present adds the paths to the show.  If not present, we add a new show and set the show metadata.
   \param paths a vector<string,string> list of the path(s) and parent path(s) for the show.
   \param details a CVideoInfoTag filled with the metadata for the show.
   \param artwork the artwork map for the show.
   \param seasonArt the artwork map for seasons.
   \param idTvShow the database id of the tvshow if known (defaults to -1)
   \return the id of the tvshow.
   */
  int SetDetailsForTvShow(const std::vector< std::pair<std::string, std::string> > &paths, CVideoInfoTag& details, const std::map<std::string, std::string> &artwork, const std::map<int, std::map<std::string, std::string> > &seasonArt, int idTvShow = -1);
  bool UpdateDetailsForTvShow(int idTvShow, CVideoInfoTag &details, const std::map<std::string, std::string> &artwork, const std::map<int, std::map<std::string, std::string> > &seasonArt);
  int SetDetailsForSeason(const CVideoInfoTag& details, const std::map<std::string, std::string> &artwork, int idShow, int idSeason = -1);
  int SetDetailsForEpisode(const std::string& strFilenameAndPath, CVideoInfoTag& details, const std::map<std::string, std::string> &artwork, int idShow, int idEpisode=-1);
  int SetDetailsForMusicVideo(const std::string& strFilenameAndPath, const CVideoInfoTag& details, const std::map<std::string, std::string> &artwork, int idMVideo = -1);
  void SetStreamDetailsForFile(const CStreamDetails& details, const std::string &strFileNameAndPath);
  void SetStreamDetailsForFileId(const CStreamDetails& details, int idFile);

  bool SetSingleValue(VIDEODB_CONTENT_TYPE type, int dbId, int dbField, const std::string &strValue);
  bool SetSingleValue(VIDEODB_CONTENT_TYPE type, int dbId, Field dbField, const std::string &strValue);
  bool SetSingleValue(const std::string &table, const std::string &fieldName, const std::string &strValue,
                      const std::string &conditionName = "", int conditionValue = -1);

  int UpdateDetailsForMovie(int idMovie, CVideoInfoTag& details, const std::map<std::string, std::string> &artwork, const std::set<std::string> &updatedDetails);

  void DeleteMovie(int idMovie, bool bKeepId = false);
  void DeleteMovie(const std::string& strFilenameAndPath, bool bKeepId = false);
  void DeleteTvShow(int idTvShow, bool bKeepId = false);
  void DeleteTvShow(const std::string& strPath);
  void DeleteSeason(int idSeason, bool bKeepId = false);
  void DeleteEpisode(int idEpisode, bool bKeepId = false);
  void DeleteEpisode(const std::string& strFilenameAndPath, bool bKeepId = false);
  void DeleteMusicVideo(int idMusicVideo, bool bKeepId = false);
  void DeleteMusicVideo(const std::string& strFilenameAndPath, bool bKeepId = false);
  void DeleteDetailsForTvShow(int idTvShow);
  void DeleteDetailsForTvShow(const std::string& strPath);
  void RemoveContentForPath(const std::string& strPath,CGUIDialogProgress *progress = NULL);
  void UpdateFanart(const CFileItem &item, VIDEODB_CONTENT_TYPE type);
  void DeleteSet(int idSet);
  void DeleteTag(int idTag, VIDEODB_CONTENT_TYPE mediaType);

  /*! \brief Get video settings for the specified file id
   \param idFile file id to get the settings for
   \return true if video settings found, false otherwise
   \sa SetVideoSettings
   */
  bool GetVideoSettings(int idFile, CVideoSettings &settings);

  /*! \brief Get video settings for the specified file item
   \param item item to get the settings for
   \return true if video settings found, false otherwise
   \sa SetVideoSettings
   */
  bool GetVideoSettings(const CFileItem &item, CVideoSettings &settings);

  /*! \brief Get video settings for the specified file path
   \param filePath filepath to get the settings for
   \return true if video settings found, false otherwise
   \sa SetVideoSettings
   */
  bool GetVideoSettings(const std::string &filePath, CVideoSettings &settings);

  /*! \brief Set video settings for the specified file path
   \param filePath filepath to set the settings for
   \sa GetVideoSettings
   */
  void SetVideoSettings(const std::string &filePath, const CVideoSettings &settings);

  /**
   * Erases settings for all files beginning with the specified path. Defaults 
   * to an empty path, meaning all settings will be erased.
   * @param path partial path, e.g. pvr://channels
   */
  void EraseVideoSettings(const std::string &path = "");

  bool GetStackTimes(const std::string &filePath, std::vector<int> &times);
  void SetStackTimes(const std::string &filePath, const std::vector<int> &times);

  void GetBookMarksForFile(const std::string& strFilenameAndPath, VECBOOKMARKS& bookmarks, CBookmark::EType type = CBookmark::STANDARD, bool bAppend=false, long partNumber=0);
  void AddBookMarkToFile(const std::string& strFilenameAndPath, const CBookmark &bookmark, CBookmark::EType type = CBookmark::STANDARD);
  bool GetResumeBookMark(const std::string& strFilenameAndPath, CBookmark &bookmark);
  void DeleteResumeBookMark(const std::string &strFilenameAndPath);
  void ClearBookMarkOfFile(const std::string& strFilenameAndPath, CBookmark& bookmark, CBookmark::EType type = CBookmark::STANDARD);
  void ClearBookMarksOfFile(const std::string& strFilenameAndPath, CBookmark::EType type = CBookmark::STANDARD);
  void ClearBookMarksOfFile(int idFile, CBookmark::EType type = CBookmark::STANDARD);
  bool GetBookMarkForEpisode(const CVideoInfoTag& tag, CBookmark& bookmark);
  void AddBookMarkForEpisode(const CVideoInfoTag& tag, const CBookmark& bookmark);
  void DeleteBookMarkForEpisode(const CVideoInfoTag& tag);
  bool GetResumePoint(CVideoInfoTag& tag);
  bool GetStreamDetails(CFileItem& item);
  bool GetStreamDetails(CVideoInfoTag& tag) const;

  // scraper settings
  void SetScraperForPath(const std::string& filePath, const ADDON::ScraperPtr& info, const VIDEO::SScanSettings& settings);
  ADDON::ScraperPtr GetScraperForPath(const std::string& strPath);
  ADDON::ScraperPtr GetScraperForPath(const std::string& strPath, VIDEO::SScanSettings& settings);

  /*! \brief Retrieve the scraper and settings we should use for the specified path
   If the scraper is not set on this particular path, we'll recursively check parent folders.
   \param strPath path to start searching in.
   \param settings [out] scan settings for this folder.
   \param foundDirectly [out] true if a scraper was found directly for strPath, false if it was in a parent path.
   \return A ScraperPtr containing the scraper information. Returns NULL if a trivial (Content == CONTENT_NONE)
           scraper or no scraper is found.
   */
  ADDON::ScraperPtr GetScraperForPath(const std::string& strPath, VIDEO::SScanSettings& settings, bool& foundDirectly);

  /*! \brief Retrieve the content type of videos in the given path
   If content is set on the folder, we return the given content type, except in the case of tvshows,
   where we first check for whether we have episodes directly in the path (thus return episodes) or whether
   we've found a scraper directly (shows).  Any folders inbetween are treated as seasons (regardless of whether
   they actually are seasons). Note that any subfolders in movies will be treated as movies.
   \param strPath path to start searching in.
   \return A content type string for the current path.
   */
  std::string GetContentForPath(const std::string& strPath);

  /*! \brief Get videos of the given content type from the given path
   \param content the content type to fetch.
   \param path the path to fetch videos from.
   \param items the returned items
   \return true if items are found, false otherwise.
   */
  bool GetItemsForPath(const std::string &content, const std::string &path, CFileItemList &items);

  /*! \brief Check whether a given scraper is in use.
   \param scraperID the scraper to check for.
   \return true if the scraper is in use, false otherwise.
   */
  bool ScraperInUse(const std::string &scraperID) const;
  
  // scanning hashes and paths scanned
  bool SetPathHash(const std::string &path, const std::string &hash);
  bool GetPathHash(const std::string &path, std::string &hash);
  bool GetPaths(std::set<std::string> &paths);
  bool GetPathsForTvShow(int idShow, std::set<int>& paths);

  /*! \brief return the paths linked to a tvshow.
   \param idShow the id of the tvshow.
   \param paths [out] the list of paths associated with the show.
   \return true on success, false on failure.
   */
  bool GetPathsLinkedToTvShow(int idShow, std::vector<std::string> &paths);

  /*! \brief retrieve subpaths of a given path.  Assumes a heirarchical folder structure
   \param basepath the root path to retrieve subpaths for
   \param subpaths the returned subpaths
   \return true if we successfully retrieve subpaths (may be zero), false on error
   */
  bool GetSubPaths(const std::string& basepath, std::vector< std::pair<int, std::string> >& subpaths);

  bool GetSourcePath(const std::string &path, std::string &sourcePath);
  bool GetSourcePath(const std::string &path, std::string &sourcePath, VIDEO::SScanSettings& settings);

  // for music + musicvideo linkups - if no album and title given it will return the artist id, else the id of the matching video
  int GetMatchingMusicVideo(const std::string& strArtist, const std::string& strAlbum = "", const std::string& strTitle = "");

  // searching functions
  void GetMoviesByActor(const std::string& strActor, CFileItemList& items);
  void GetTvShowsByActor(const std::string& strActor, CFileItemList& items);
  void GetEpisodesByActor(const std::string& strActor, CFileItemList& items);

  void GetMusicVideosByArtist(const std::string& strArtist, CFileItemList& items);
  void GetMusicVideosByAlbum(const std::string& strAlbum, CFileItemList& items);

  void GetMovieGenresByName(const std::string& strSearch, CFileItemList& items);
  void GetTvShowGenresByName(const std::string& strSearch, CFileItemList& items);
  void GetMusicVideoGenresByName(const std::string& strSearch, CFileItemList& items);

  void GetMovieCountriesByName(const std::string& strSearch, CFileItemList& items);

  void GetMusicVideoAlbumsByName(const std::string& strSearch, CFileItemList& items);

  void GetMovieActorsByName(const std::string& strSearch, CFileItemList& items);
  void GetTvShowsActorsByName(const std::string& strSearch, CFileItemList& items);
  void GetMusicVideoArtistsByName(const std::string& strSearch, CFileItemList& items);

  void GetMovieDirectorsByName(const std::string& strSearch, CFileItemList& items);
  void GetTvShowsDirectorsByName(const std::string& strSearch, CFileItemList& items);
  void GetMusicVideoDirectorsByName(const std::string& strSearch, CFileItemList& items);

  void GetMoviesByName(const std::string& strSearch, CFileItemList& items);
  void GetTvShowsByName(const std::string& strSearch, CFileItemList& items);
  void GetEpisodesByName(const std::string& strSearch, CFileItemList& items);
  void GetMusicVideosByName(const std::string& strSearch, CFileItemList& items);

  void GetEpisodesByPlot(const std::string& strSearch, CFileItemList& items);
  void GetMoviesByPlot(const std::string& strSearch, CFileItemList& items);

  bool LinkMovieToTvshow(int idMovie, int idShow, bool bRemove);
  bool IsLinkedToTvshow(int idMovie);
  bool GetLinksToTvShow(int idMovie, std::vector<int>& ids);

  // general browsing
  bool GetGenresNav(const std::string& strBaseDir, CFileItemList& items, int idContent=-1, const Filter &filter = Filter(), bool countOnly = false);
  bool GetCountriesNav(const std::string& strBaseDir, CFileItemList& items, int idContent=-1, const Filter &filter = Filter(), bool countOnly = false);
  bool GetStudiosNav(const std::string& strBaseDir, CFileItemList& items, int idContent=-1, const Filter &filter = Filter(), bool countOnly = false);
  bool GetYearsNav(const std::string& strBaseDir, CFileItemList& items, int idContent=-1, const Filter &filter = Filter());
  bool GetActorsNav(const std::string& strBaseDir, CFileItemList& items, int idContent=-1, const Filter &filter = Filter(), bool countOnly = false);
  bool GetDirectorsNav(const std::string& strBaseDir, CFileItemList& items, int idContent=-1, const Filter &filter = Filter(), bool countOnly = false);
  bool GetWritersNav(const std::string& strBaseDir, CFileItemList& items, int idContent=-1, const Filter &filter = Filter(), bool countOnly = false);
  bool GetSetsNav(const std::string& strBaseDir, CFileItemList& items, int idContent=-1, const Filter &filter = Filter(), bool ignoreSingleMovieSets = false);
  bool GetTagsNav(const std::string& strBaseDir, CFileItemList& items, int idContent=-1, const Filter &filter = Filter(), bool countOnly = false);
  bool GetMusicVideoAlbumsNav(const std::string& strBaseDir, CFileItemList& items, int idArtist, const Filter &filter = Filter(), bool countOnly = false);

  bool GetMoviesNav(const std::string& strBaseDir, CFileItemList& items, int idGenre=-1, int idYear=-1, int idActor=-1, int idDirector=-1, int idStudio=-1, int idCountry=-1, int idSet=-1, int idTag=-1, const SortDescription &sortDescription = SortDescription(), int getDetails = VideoDbDetailsNone);
  bool GetTvShowsNav(const std::string& strBaseDir, CFileItemList& items, int idGenre=-1, int idYear=-1, int idActor=-1, int idDirector=-1, int idStudio=-1, int idTag=-1, const SortDescription &sortDescription = SortDescription(), int getDetails = VideoDbDetailsNone);
  bool GetSeasonsNav(const std::string& strBaseDir, CFileItemList& items, int idActor=-1, int idDirector=-1, int idGenre=-1, int idYear=-1, int idShow=-1, bool getLinkedMovies = true);
  bool GetEpisodesNav(const std::string& strBaseDir, CFileItemList& items, int idGenre=-1, int idYear=-1, int idActor=-1, int idDirector=-1, int idShow=-1, int idSeason=-1, const SortDescription &sortDescription = SortDescription(), int getDetails = VideoDbDetailsNone);
  bool GetMusicVideosNav(const std::string& strBaseDir, CFileItemList& items, int idGenre=-1, int idYear=-1, int idArtist=-1, int idDirector=-1, int idStudio=-1, int idAlbum=-1, int idTag=-1, const SortDescription &sortDescription = SortDescription(), int getDetails = VideoDbDetailsNone);
  
  bool GetRecentlyAddedMoviesNav(const std::string& strBaseDir, CFileItemList& items, unsigned int limit=0, int getDetails = VideoDbDetailsNone);
  bool GetRecentlyAddedEpisodesNav(const std::string& strBaseDir, CFileItemList& items, unsigned int limit=0, int getDetails = VideoDbDetailsNone);
  bool GetRecentlyAddedMusicVideosNav(const std::string& strBaseDir, CFileItemList& items, unsigned int limit=0, int getDetails = VideoDbDetailsNone);
  bool GetInProgressTvShowsNav(const std::string& strBaseDir, CFileItemList& items, unsigned int limit=0, int getDetails = VideoDbDetailsNone);

  bool HasContent();
  bool HasContent(VIDEODB_CONTENT_TYPE type);
  bool HasSets() const;

  void CleanDatabase(CGUIDialogProgressBarHandle* handle = NULL, const std::set<int>& paths = std::set<int>(), bool showProgress = true);

  /*! \brief Add a file to the database, if necessary
   If the file is already in the database, we simply return its id.
   \param url - full path of the file to add.
   \return id of the file, -1 if it could not be added.
   */
  int AddFile(const std::string& url);

  /*! \brief Add a file to the database, if necessary
   Works for both videodb:// items and normal fileitems
   \param item CFileItem to add.
   \return id of the file, -1 if it could not be added.
   */
  int AddFile(const CFileItem& item);

  /*! \brief Add a path to the database, if necessary
   If the path is already in the database, we simply return its id.
   \param strPath the path to add
   \param parentPath the parent path of the path to add. If empty, URIUtils::GetParentPath() will determine the path.
   \param dateAdded datetime when the path was added to the filesystem/database
   \return id of the file, -1 if it could not be added.
   */
  int AddPath(const std::string& strPath, const std::string &parentPath = "", const CDateTime& dateAdded = CDateTime());

  /*! \brief Updates the dateAdded field in the files table for the file
   with the given idFile and the given path based on the files modification date
   \param idFile id of the file in the files table
   \param strFileNameAndPath path to the file
   \param dateAdded datetime when the file was added to the filesystem/database
   */
  void UpdateFileDateAdded(int idFile, const std::string& strFileNameAndPathh, const CDateTime& dateAdded = CDateTime());

  void ExportToXML(const std::string &path, bool singleFile = true, bool images=false, bool actorThumbs=false, bool overwrite=false);
  void ExportActorThumbs(const std::string &path, const CVideoInfoTag& tag, bool singleFiles, bool overwrite=false);
  void ImportFromXML(const std::string &path);
  void DumpToDummyFiles(const std::string &path);
  bool ImportArtFromXML(const TiXmlNode *node, std::map<std::string, std::string> &artwork);

  // smart playlists and main retrieval work in these functions
  bool GetMoviesByWhere(const std::string& strBaseDir, const Filter &filter, CFileItemList& items, const SortDescription &sortDescription = SortDescription(), int getDetails = VideoDbDetailsNone);
  bool GetSetsByWhere(const std::string& strBaseDir, const Filter &filter, CFileItemList& items, bool ignoreSingleMovieSets = false);
  bool GetTvShowsByWhere(const std::string& strBaseDir, const Filter &filter, CFileItemList& items, const SortDescription &sortDescription = SortDescription(), int getDetails = VideoDbDetailsNone);
  bool GetSeasonsByWhere(const std::string& strBaseDir, const Filter &filter, CFileItemList& items, bool appendFullShowPath = true, const SortDescription &sortDescription = SortDescription());
  bool GetEpisodesByWhere(const std::string& strBaseDir, const Filter &filter, CFileItemList& items, bool appendFullShowPath = true, const SortDescription &sortDescription = SortDescription(), int getDetails = VideoDbDetailsNone);
  bool GetMusicVideosByWhere(const std::string &baseDir, const Filter &filter, CFileItemList& items, bool checkLocks = true, const SortDescription &sortDescription = SortDescription(), int getDetails = VideoDbDetailsNone);
  
  // retrieve sorted and limited items
  bool GetSortedVideos(const MediaType &mediaType, const std::string& strBaseDir, const SortDescription &sortDescription, CFileItemList& items, const Filter &filter = Filter());

  // retrieve a list of items
  bool GetItems(const std::string &strBaseDir, CFileItemList &items, const Filter &filter = Filter(), const SortDescription &sortDescription = SortDescription());
  bool GetItems(const std::string &strBaseDir, const std::string &mediaType, const std::string &itemType, CFileItemList &items, const Filter &filter = Filter(), const SortDescription &sortDescription = SortDescription());
  bool GetItems(const std::string &strBaseDir, VIDEODB_CONTENT_TYPE mediaType, const std::string &itemType, CFileItemList &items, const Filter &filter = Filter(), const SortDescription &sortDescription = SortDescription());
  std::string GetItemById(const std::string &itemType, int id);

  // partymode
  unsigned int GetMusicVideoIDs(const std::string& strWhere, std::vector<std::pair<int, int> > &songIDs);
  bool GetRandomMusicVideo(CFileItem* item, int& idSong, const std::string& strWhere);

  static void VideoContentTypeToString(VIDEODB_CONTENT_TYPE type, std::string& out)
  {
    switch (type)
    {
    case VIDEODB_CONTENT_MOVIES:
      out = MediaTypeMovie;
      break;
    case VIDEODB_CONTENT_TVSHOWS:
      out = MediaTypeTvShow;
      break;
    case VIDEODB_CONTENT_EPISODES:
      out = MediaTypeEpisode;
      break;
    case VIDEODB_CONTENT_MUSICVIDEOS:
      out = MediaTypeMusicVideo;
      break;
    default:
      break;
    }
  }

  void SetArtForItem(int mediaId, const MediaType &mediaType, const std::string &artType, const std::string &url);
  void SetArtForItem(int mediaId, const MediaType &mediaType, const std::map<std::string, std::string> &art);
  bool GetArtForItem(int mediaId, const MediaType &mediaType, std::map<std::string, std::string> &art);
  std::string GetArtForItem(int mediaId, const MediaType &mediaType, const std::string &artType);
  bool RemoveArtForItem(int mediaId, const MediaType &mediaType, const std::string &artType);
  bool RemoveArtForItem(int mediaId, const MediaType &mediaType, const std::set<std::string> &artTypes);
  bool GetTvShowSeasons(int showId, std::map<int, int> &seasons);
  bool GetTvShowSeasonArt(int mediaId, std::map<int, std::map<std::string, std::string> > &seasonArt);
  bool GetArtTypes(const MediaType &mediaType, std::vector<std::string> &artTypes);

  int AddTag(const std::string &tag);
  void AddTagToItem(int idItem, int idTag, const std::string &type);
  void RemoveTagFromItem(int idItem, int idTag, const std::string &type);
  void RemoveTagsFromItem(int idItem, const std::string &type);

  virtual bool GetFilter(CDbUrl &videoUrl, Filter &filter, SortDescription &sorting);

  /*! \brief Will check if the season exists and if that is not the case add it to the database.
  \param showID The id of the show in question.
  \param season The season number we want to add.
  \return The dbId of the season.
  */
  int AddSeason(int showID, int season);
  int AddSet(const std::string& strSet, const std::string& strOverview = "");
  void ClearMovieSet(int idMovie);
  void SetMovieSet(int idMovie, int idSet);
  bool SetVideoUserRating(int dbId, int rating, const MediaType& mediaType);

protected:
  int GetMovieId(const std::string& strFilenameAndPath);
  int GetMusicVideoId(const std::string& strFilenameAndPath);

  /*! \brief Get the id of this fileitem
   Works for both videodb:// items and normal fileitems
   \param item CFileItem to grab the fileid of
   \return id of the file, -1 if it is not in the db.
   */
  int GetFileId(const CFileItem &item);

  /*! \brief Get the id of a file from path
   \param url full path to the file
   \return id of the file, -1 if it is not in the db.
   */
  int GetFileId(const std::string& url);

  int AddToTable(const std::string& table, const std::string& firstField, const std::string& secondField, const std::string& value);
  int UpdateRatings(int mediaId, const char *mediaType, const RatingMap& values, const std::string& defaultRating);
  int AddRatings(int mediaId, const char *mediaType, const RatingMap& values, const std::string& defaultRating);
  int UpdateUniqueIDs(int mediaId, const char *mediaType, const CVideoInfoTag& details);
  int AddUniqueIDs(int mediaId, const char *mediaType, const CVideoInfoTag& details);
  int AddActor(const std::string& strActor, const std::string& thumbURL, const std::string &thumb = "");

  int AddTvShow();
  int AddMusicVideo(const std::string& strFilenameAndPath);

  /*! \brief Adds a path to the tvshow link table.
   \param idShow the id of the show.
   \param path the path to add.
   \param parentPath the parent path of the path to add.
   \param dateAdded date/time when the path was added
   \return true if successfully added, false otherwise.
   */
  bool AddPathToTvShow(int idShow, const std::string &path, const std::string &parentPath, const CDateTime& dateAdded = CDateTime());

  /*! \brief Check whether a show is already in the library.
   Matches on unique identifier or matching title and premiered date.
   \param show the details of the show to check for.
   \return the show id if found, else -1.
   */
  int GetMatchingTvShow(const CVideoInfoTag &show);

  // link functions - these two do all the work
  void AddLinkToActor(int mediaId, const char *mediaType, int actorId, const std::string &role, int order);
  void AddToLinkTable(int mediaId, const std::string& mediaType, const std::string& table, int valueId, const char *foreignKey = NULL);
  void RemoveFromLinkTable(int mediaId, const std::string& mediaType, const std::string& table, int valueId, const char *foreignKey = NULL);

  void AddLinksToItem(int mediaId, const std::string& mediaType, const std::string& field, const std::vector<std::string>& values);
  void UpdateLinksToItem(int mediaId, const std::string& mediaType, const std::string& field, const std::vector<std::string>& values);
  void AddActorLinksToItem(int mediaId, const std::string& mediaType, const std::string& field, const std::vector<std::string>& values);
  void UpdateActorLinksToItem(int mediaId, const std::string& mediaType, const std::string& field, const std::vector<std::string>& values);

  void AddCast(int mediaId, const char *mediaType, const std::vector<SActorInfo> &cast);

  void DeleteStreamDetails(int idFile);
  CVideoInfoTag GetDetailsForMovie(std::unique_ptr<dbiplus::Dataset> &pDS, int getDetails = VideoDbDetailsNone);
  CVideoInfoTag GetDetailsForMovie(const dbiplus::sql_record* const record, int getDetails = VideoDbDetailsNone);
  CVideoInfoTag GetDetailsForTvShow(std::unique_ptr<dbiplus::Dataset> &pDS, int getDetails = VideoDbDetailsNone, CFileItem* item = NULL);
  CVideoInfoTag GetDetailsForTvShow(const dbiplus::sql_record* const record, int getDetails = VideoDbDetailsNone, CFileItem* item = NULL);
  CVideoInfoTag GetDetailsForEpisode(std::unique_ptr<dbiplus::Dataset> &pDS, int getDetails = VideoDbDetailsNone);
  CVideoInfoTag GetDetailsForEpisode(const dbiplus::sql_record* const record, int getDetails = VideoDbDetailsNone);
  CVideoInfoTag GetDetailsForMusicVideo(std::unique_ptr<dbiplus::Dataset> &pDS, int getDetails = VideoDbDetailsNone);
  CVideoInfoTag GetDetailsForMusicVideo(const dbiplus::sql_record* const record, int getDetails = VideoDbDetailsNone);
  bool GetPeopleNav(const std::string& strBaseDir, CFileItemList& items, const char *type, int idContent = -1, const Filter &filter = Filter(), bool countOnly = false);
  bool GetNavCommon(const std::string& strBaseDir, CFileItemList& items, const char *type, int idContent=-1, const Filter &filter = Filter(), bool countOnly = false);
  void GetCast(int media_id, const std::string &media_type, std::vector<SActorInfo> &cast);
  void GetTags(int media_id, const std::string &media_type, std::vector<std::string> &tags);
  void GetRatings(int media_id, const std::string &media_type, RatingMap &ratings);
  void GetUniqueIDs(int media_id, const std::string &media_type, CVideoInfoTag& details);

  void GetDetailsFromDB(std::unique_ptr<dbiplus::Dataset> &pDS, int min, int max, const SDbTableOffsets *offsets, CVideoInfoTag &details, int idxOffset = 2);
  void GetDetailsFromDB(const dbiplus::sql_record* const record, int min, int max, const SDbTableOffsets *offsets, CVideoInfoTag &details, int idxOffset = 2);
  std::string GetValueString(const CVideoInfoTag &details, int min, int max, const SDbTableOffsets *offsets) const;

private:
  virtual void CreateTables();
  virtual void CreateAnalytics();
  virtual void UpdateTables(int version);
  void CreateLinkIndex(const char *table);
  void CreateForeignLinkIndex(const char *table, const char *foreignkey);

  /*! \brief (Re)Create the generic database views for movies, tvshows,
     episodes and music videos
   */
  virtual void CreateViews();

  /*! \brief Helper to get a database id given a query.
   Returns an integer, -1 if not found, and greater than 0 if found.
   \param query the SQL that will retrieve a database id.
   \return -1 if not found, else a valid database id (i.e. > 0)
   */
  int GetDbId(const std::string &query);

  /*! \brief Run a query on the main dataset and return the number of rows
   If no rows are found we close the dataset and return 0.
   \param sql the sql query to run
   \return the number of rows, -1 for an error.
   */
  int RunQuery(const std::string &sql);

  void AppendIdLinkFilter(const char* field, const char *table, const MediaType& mediaType, const char *view, const char *viewKey, const CUrlOptions::UrlOptions& options, Filter &filter);
  void AppendLinkFilter(const char* field, const char *table, const MediaType& mediaType, const char *view, const char *viewKey, const CUrlOptions::UrlOptions& options, Filter &filter);

  /*! \brief Determine whether the path is using lookup using folders
   \param path the path to check
   \param shows whether this path is from a tvshow (defaults to false)
   */
  bool LookupByFolders(const std::string &path, bool shows = false);

  /*! \brief Get the playcount for a file id
   \param iFileId file id to get the playcount for
   \return the playcount of the item, or -1 on error
   \sa SetPlayCount, IncrementPlayCount, GetPlayCounts
   */
  int GetPlayCount(int iFileId);

  virtual int GetMinSchemaVersion() const { return 75; };
  virtual int GetSchemaVersion() const;
  virtual int GetExportVersion() const { return 1; };
  const char *GetBaseDBName() const { return "MyVideos"; };

  void ConstructPath(std::string& strDest, const std::string& strPath, const std::string& strFileName);
  void SplitPath(const std::string& strFileNameAndPath, std::string& strPath, std::string& strFileName);
  void InvalidatePathHash(const std::string& strPath);

  /*! \brief Get a safe filename from a given string
   \param dir directory to use for the file
   \param name movie, show name, or actor to get a safe filename for
   \return safe filename based on this title
   */
  std::string GetSafeFile(const std::string &dir, const std::string &name) const;

  std::vector<int> CleanMediaType(const std::string &mediaType, const std::string &cleanableFileIDs,
                                  std::map<int, bool> &pathsDeleteDecisions, std::string &deletedFileIDs, bool silent);

  static void AnnounceRemove(std::string content, int id, bool scanning = false);
  static void AnnounceUpdate(std::string content, int id);
};
