#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "dbwrappers/Database.h"
#include "VideoInfoTag.h"
#include "addons/Scraper.h"
#include "Bookmark.h"

#include <memory>
#include <set>

class CFileItem;
class CFileItemList;
class CVideoSettings;
class CGUIDialogProgress;

#ifndef my_offsetof
#ifndef _LINUX
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

// these defines are based on how many columns we have and which column certain data is going to be in
// when we do GetDetailsForMovie()
#define VIDEODB_MAX_COLUMNS 24
#define VIDEODB_DETAILS_FILEID			1
#define VIDEODB_DETAILS_FILE			VIDEODB_MAX_COLUMNS + 2
#define VIDEODB_DETAILS_PATH			VIDEODB_MAX_COLUMNS + 3
#define VIDEODB_DETAILS_PLAYCOUNT		VIDEODB_MAX_COLUMNS + 4
#define VIDEODB_DETAILS_LASTPLAYED		VIDEODB_MAX_COLUMNS + 5
#define VIDEODB_DETAILS_EPISODE_TVSHOW_NAME	VIDEODB_MAX_COLUMNS + 6
#define VIDEODB_DETAILS_EPISODE_TVSHOW_STUDIO	VIDEODB_MAX_COLUMNS + 7
#define VIDEODB_DETAILS_EPISODE_TVSHOW_ID	VIDEODB_MAX_COLUMNS + 8
#define VIDEODB_DETAILS_EPISODE_TVSHOW_AIRED	VIDEODB_MAX_COLUMNS + 9
#define VIDEODB_DETAILS_EPISODE_TVSHOW_MPAA	VIDEODB_MAX_COLUMNS + 10
#define VIDEODB_DETAILS_EPISODE_TVSHOW_PATH	VIDEODB_MAX_COLUMNS + 11
						
#define VIDEODB_DETAILS_TVSHOW_PATH		VIDEODB_MAX_COLUMNS + 1
#define VIDEODB_DETAILS_TVSHOW_NUM_EPISODES	VIDEODB_MAX_COLUMNS + 2
#define VIDEODB_DETAILS_TVSHOW_NUM_WATCHED	VIDEODB_MAX_COLUMNS + 3
#define VIDEODB_DETAILS_TVSHOW_NUM_SEASONS	VIDEODB_MAX_COLUMNS + 4


#define VIDEODB_TYPE_STRING 1
#define VIDEODB_TYPE_INT 2
#define VIDEODB_TYPE_FLOAT 3
#define VIDEODB_TYPE_BOOL 4
#define VIDEODB_TYPE_COUNT 5

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
  VIDEODB_ID_VOTES = 4,
  VIDEODB_ID_RATING = 5,
  VIDEODB_ID_CREDITS = 6,
  VIDEODB_ID_YEAR = 7,
  VIDEODB_ID_THUMBURL = 8,
  VIDEODB_ID_IDENT = 9,
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
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strVotes) },
  { VIDEODB_TYPE_FLOAT, my_offsetof(CVideoInfoTag,m_fRating) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strWritingCredits) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iYear) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPictureURL.m_xml) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strIMDBNumber) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strSortTitle) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strRuntime) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strMPAARating) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iTop250) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strGenre) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strDirector) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strOriginalTitle) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPictureURL.m_spoof) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strStudio) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strTrailer) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_fanart.m_xml) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strCountry) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_basePath) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_parentPathID) }
};

typedef enum // this enum MUST match the offset struct further down!! and make sure to keep min and max at -1 and sizeof(offsets)
{
  VIDEODB_ID_TV_MIN = -1,
  VIDEODB_ID_TV_TITLE = 0,
  VIDEODB_ID_TV_PLOT = 1,
  VIDEODB_ID_TV_STATUS = 2,
  VIDEODB_ID_TV_VOTES = 3,
  VIDEODB_ID_TV_RATING = 4,
  VIDEODB_ID_TV_PREMIERED = 5,
  VIDEODB_ID_TV_THUMBURL = 6,
  VIDEODB_ID_TV_THUMBURL_SPOOF = 7,
  VIDEODB_ID_TV_GENRE = 8,
  VIDEODB_ID_TV_ORIGINALTITLE = 9,
  VIDEODB_ID_TV_EPISODEGUIDE = 10,
  VIDEODB_ID_TV_FANART = 11,
  VIDEODB_ID_TV_IDENT = 12,
  VIDEODB_ID_TV_MPAA = 13,
  VIDEODB_ID_TV_STUDIOS = 14,
  VIDEODB_ID_TV_SORTTITLE = 15,
  VIDEODB_ID_TV_BASEPATH = 16,
  VIDEODB_ID_TV_PARENTPATHID = 17,
  VIDEODB_ID_TV_MAX
} VIDEODB_TV_IDS;

const struct SDbTableOffsets DbTvShowOffsets[] =
{
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strTitle) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPlot) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strStatus) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strVotes) },
  { VIDEODB_TYPE_FLOAT, my_offsetof(CVideoInfoTag,m_fRating) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPremiered) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPictureURL.m_xml) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPictureURL.m_spoof) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strGenre) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strOriginalTitle)},
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strEpisodeGuide)},
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_fanart.m_xml)},
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strIMDBNumber)},
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strMPAARating)},
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strStudio)},
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strSortTitle)},
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_basePath) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_parentPathID) }
};

typedef enum // this enum MUST match the offset struct further down!! and make sure to keep min and max at -1 and sizeof(offsets)
{
  VIDEODB_ID_EPISODE_MIN = -1,
  VIDEODB_ID_EPISODE_TITLE = 0,
  VIDEODB_ID_EPISODE_PLOT = 1,
  VIDEODB_ID_EPISODE_VOTES = 2,
  VIDEODB_ID_EPISODE_RATING = 3,
  VIDEODB_ID_EPISODE_CREDITS = 4,
  VIDEODB_ID_EPISODE_AIRED = 5,
  VIDEODB_ID_EPISODE_THUMBURL = 6,
  VIDEODB_ID_EPISODE_THUMBURL_SPOOF = 7,
  VIDEODB_ID_EPISODE_PLAYCOUNT = 8, // unused - feel free to repurpose
  VIDEODB_ID_EPISODE_RUNTIME = 9,
  VIDEODB_ID_EPISODE_DIRECTOR = 10,
  VIDEODB_ID_EPISODE_IDENT = 11,
  VIDEODB_ID_EPISODE_SEASON = 12,
  VIDEODB_ID_EPISODE_EPISODE = 13,
  VIDEODB_ID_EPISODE_ORIGINALTITLE = 14,
  VIDEODB_ID_EPISODE_SORTSEASON = 15,
  VIDEODB_ID_EPISODE_SORTEPISODE = 16,
  VIDEODB_ID_EPISODE_BOOKMARK = 17,
  VIDEODB_ID_EPISODE_BASEPATH = 18,
  VIDEODB_ID_EPISODE_PARENTPATHID = 19,
  VIDEODB_ID_EPISODE_MAX
} VIDEODB_EPISODE_IDS;

const struct SDbTableOffsets DbEpisodeOffsets[] =
{
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strTitle) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPlot) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strVotes) },
  { VIDEODB_TYPE_FLOAT, my_offsetof(CVideoInfoTag,m_fRating) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strWritingCredits) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strFirstAired) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPictureURL.m_xml) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPictureURL.m_spoof) },
  { VIDEODB_TYPE_COUNT, my_offsetof(CVideoInfoTag,m_playCount) }, // unused
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strRuntime) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strDirector) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strProductionCode) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iSeason) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iEpisode) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strOriginalTitle)},
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iSpecialSortSeason) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iSpecialSortEpisode) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iBookmarkId) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_basePath) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_parentPathID) }
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
  VIDEODB_ID_MUSICVIDEO_YEAR = 7,
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
  { VIDEODB_TYPE_COUNT, my_offsetof(CVideoInfoTag,m_playCount) }, // unused
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strRuntime) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strDirector) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strStudio) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iYear) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPlot) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strAlbum) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strArtist) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strGenre) },
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
    CStdString name;
    CStdString thumb;
    int playcount;
  };

  class CSeason   // used for season retrieval for non-master users
  {
  public:
    CStdString path;
    CStdString genre;
    int numEpisodes;
    int numWatched;
  };

  CVideoDatabase(void);
  virtual ~CVideoDatabase(void);

  virtual bool Open();
  virtual bool CommitTransaction();

  int AddMovie(const CStdString& strFilenameAndPath);
  int AddEpisode(int idShow, const CStdString& strFilenameAndPath);

  // editing functions
  /*! \brief Set the playcount of an item
   Sets the playcount and last played date to a given value
   \param item CFileItem to set the playcount for
   \param count The playcount to set.
   \param date The date the file was last viewed (does not denote the video was watched to completion).  If empty we current datetime (if count > 0) or never viewed (if count = 0).
   \sa GetPlayCount, IncrementPlayCount, UpdateLastPlayed
   */
  void SetPlayCount(const CFileItem &item, int count, const CStdString &date = "");

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

  /*! \brief Update the last played time of an item
   Updates the last played date
   \param item CFileItem to update the last played time for
   \sa GetPlayCount, SetPlayCount, IncrementPlayCount, GetPlayCounts
   */
  void UpdateLastPlayed(const CFileItem &item);

  /*! \brief Get the playcount of a list of items
   \param path the path to fetch videos from
   \param items CFileItemList to fetch the playcounts for
   \sa GetPlayCount, SetPlayCount, IncrementPlayCount
   */
  bool GetPlayCounts(const CStdString &path, CFileItemList &items);

  void UpdateMovieTitle(int idMovie, const CStdString& strNewMovieTitle, VIDEODB_CONTENT_TYPE iType=VIDEODB_CONTENT_MOVIES);

  bool HasMovieInfo(const CStdString& strFilenameAndPath);
  bool HasTvShowInfo(const CStdString& strFilenameAndPath);
  bool HasEpisodeInfo(const CStdString& strFilenameAndPath);
  bool HasMusicVideoInfo(const CStdString& strFilenameAndPath);

  void GetFilePathById(int idMovie, CStdString &filePath, VIDEODB_CONTENT_TYPE iType);
  CStdString GetGenreById(int id);
  CStdString GetCountryById(int id);
  CStdString GetSetById(int id);
  CStdString GetPersonById(int id);
  CStdString GetStudioById(int id);
  CStdString GetTvShowTitleById(int id);
  CStdString GetMusicVideoAlbumById(int id);
  int GetTvShowForEpisode(int idEpisode);

  bool LoadVideoInfo(const CStdString& strFilenameAndPath, CVideoInfoTag& details);
  bool GetMovieInfo(const CStdString& strFilenameAndPath, CVideoInfoTag& details, int idMovie = -1);
  bool GetTvShowInfo(const CStdString& strPath, CVideoInfoTag& details, int idTvShow = -1);
  bool GetEpisodeInfo(const CStdString& strFilenameAndPath, CVideoInfoTag& details, int idEpisode = -1);
  bool GetMusicVideoInfo(const CStdString& strFilenameAndPath, CVideoInfoTag& details, int idMVideo=-1);
  bool GetSetInfo(int idSet, CVideoInfoTag& details);

  int GetPathId(const CStdString& strPath);
  int GetTvShowId(const CStdString& strPath);
  int GetEpisodeId(const CStdString& strFilenameAndPath, int idEpisode=-1, int idSeason=-1); // idEpisode, idSeason are used for multipart episodes as hints

  void GetEpisodesByFile(const CStdString& strFilenameAndPath, std::vector<CVideoInfoTag>& episodes);

  int SetDetailsForMovie(const CStdString& strFilenameAndPath, const CVideoInfoTag& details);
  int SetDetailsForTvShow(const CStdString& strPath, const CVideoInfoTag& details);
  int SetDetailsForEpisode(const CStdString& strFilenameAndPath, const CVideoInfoTag& details, int idShow, int idEpisode=-1);
  int SetDetailsForMusicVideo(const CStdString& strFilenameAndPath, const CVideoInfoTag& details);
  void SetStreamDetailsForFile(const CStreamDetails& details, const CStdString &strFileNameAndPath);
  void SetStreamDetailsForFileId(const CStreamDetails& details, int idFile);
  void SetDetail(const CStdString& strDetail, int id, int field, VIDEODB_CONTENT_TYPE type);

  void DeleteMovie(const CStdString& strFilenameAndPath, bool bKeepId = false, bool bKeepThumb = false);
  void DeleteTvShow(const CStdString& strPath, bool bKeepId = false, bool bKeepThumb = false);
  void DeleteEpisode(const CStdString& strFilenameAndPath, int idEpisode = -1, bool bKeepId = false, bool bKeepThumb = false);
  void DeleteMusicVideo(const CStdString& strFilenameAndPath, bool bKeepId = false, bool bKeepThumb = false);
  void DeleteDetailsForTvShow(const CStdString& strPath);
  void RemoveContentForPath(const CStdString& strPath,CGUIDialogProgress *progress = NULL);
  void UpdateFanart(const CFileItem &item, VIDEODB_CONTENT_TYPE type);
  void DeleteSet(int idSet);

  // per-file video settings
  bool GetVideoSettings(const CStdString &strFilenameAndPath, CVideoSettings &settings);
  void SetVideoSettings(const CStdString &strFilenameAndPath, const CVideoSettings &settings);
  void EraseVideoSettings();

  bool GetStackTimes(const CStdString &filePath, std::vector<int> &times);
  void SetStackTimes(const CStdString &filePath, std::vector<int> &times);

  void GetBookMarksForFile(const CStdString& strFilenameAndPath, VECBOOKMARKS& bookmarks, CBookmark::EType type = CBookmark::STANDARD, bool bAppend=false);
  void AddBookMarkToFile(const CStdString& strFilenameAndPath, const CBookmark &bookmark, CBookmark::EType type = CBookmark::STANDARD);
  bool GetResumeBookMark(const CStdString& strFilenameAndPath, CBookmark &bookmark);
  void DeleteResumeBookMark(const CStdString &strFilenameAndPath);
  void ClearBookMarkOfFile(const CStdString& strFilenameAndPath, CBookmark& bookmark, CBookmark::EType type = CBookmark::STANDARD);
  void ClearBookMarksOfFile(const CStdString& strFilenameAndPath, CBookmark::EType type = CBookmark::STANDARD);
  bool GetBookMarkForEpisode(const CVideoInfoTag& tag, CBookmark& bookmark);
  void AddBookMarkForEpisode(const CVideoInfoTag& tag, const CBookmark& bookmark);
  void DeleteBookMarkForEpisode(const CVideoInfoTag& tag);
  bool GetResumePoint(CVideoInfoTag& tag) const;

  // scraper settings
  void SetScraperForPath(const CStdString& filePath, const ADDON::ScraperPtr& info, const VIDEO::SScanSettings& settings);
  ADDON::ScraperPtr GetScraperForPath(const CStdString& strPath);
  ADDON::ScraperPtr GetScraperForPath(const CStdString& strPath, VIDEO::SScanSettings& settings);

  /*! \brief Retrieve the scraper and settings we should use for the specified path
   If the scraper is not set on this particular path, we'll recursively check parent folders.
   \param strPath path to start searching in.
   \param settings [out] scan settings for this folder.
   \param foundDirectly [out] true if a scraper was found directly for strPath, false if it was in a parent path.
   \return A ScraperPtr containing the scraper information. Returns NULL if a trivial (Content == CONTENT_NONE)
           scraper or no scraper is found.
   */
  ADDON::ScraperPtr GetScraperForPath(const CStdString& strPath, VIDEO::SScanSettings& settings, bool& foundDirectly);

  /*! \brief Retrieve the content type of videos in the given path
   If content is set on the folder, we return the given content type, except in the case of tvshows,
   where we first check for whether we have episodes directly in the path (thus return episodes) or whether
   we've found a scraper directly (shows).  Any folders inbetween are treated as seasons (regardless of whether
   they actually are seasons). Note that any subfolders in movies will be treated as movies.
   \param strPath path to start searching in.
   \return A content type string for the current path.
   */
  CStdString GetContentForPath(const CStdString& strPath);

  /*! \brief Get videos of the given content type from the given path
   \param content the content type to fetch.
   \param path the path to fetch videos from.
   \param items the returned items
   \return true if items are found, false otherwise.
   */
  bool GetItemsForPath(const CStdString &content, const CStdString &path, CFileItemList &items);

  /*! \brief Check whether a given scraper is in use.
   \param scraperID the scraper to check for.
   \return true if the scraper is in use, false otherwise.
   */
  bool ScraperInUse(const CStdString &scraperID) const;
  
  // scanning hashes and paths scanned
  bool SetPathHash(const CStdString &path, const CStdString &hash);
  bool GetPathHash(const CStdString &path, CStdString &hash);
  bool GetPaths(std::set<CStdString> &paths);
  bool GetPathsForTvShow(int idShow, std::vector<int>& paths);

  /*! \brief retrieve subpaths of a given path.  Assumes a heirarchical folder structure
   \param basepath the root path to retrieve subpaths for
   \param subpaths the returned subpaths
   \return true if we successfully retrieve subpaths (may be zero), false on error
   */
  bool GetSubPaths(const CStdString& basepath, std::vector<int>& subpaths);

  // for music + musicvideo linkups - if no album and title given it will return the artist id, else the id of the matching video
  int GetMatchingMusicVideo(const CStdString& strArtist, const CStdString& strAlbum = "", const CStdString& strTitle = "");

  // searching functions
  void GetMoviesByActor(const CStdString& strActor, CFileItemList& items);
  void GetTvShowsByActor(const CStdString& strActor, CFileItemList& items);
  void GetEpisodesByActor(const CStdString& strActor, CFileItemList& items);

  void GetMusicVideosByArtist(const CStdString& strArtist, CFileItemList& items);
  void GetMusicVideosByAlbum(const CStdString& strAlbum, CFileItemList& items);

  void GetMovieGenresByName(const CStdString& strSearch, CFileItemList& items);
  void GetTvShowGenresByName(const CStdString& strSearch, CFileItemList& items);
  void GetMusicVideoGenresByName(const CStdString& strSearch, CFileItemList& items);

  void GetMovieCountriesByName(const CStdString& strSearch, CFileItemList& items);

  void GetMusicVideoAlbumsByName(const CStdString& strSearch, CFileItemList& items);

  void GetMovieActorsByName(const CStdString& strSearch, CFileItemList& items);
  void GetTvShowsActorsByName(const CStdString& strSearch, CFileItemList& items);
  void GetMusicVideoArtistsByName(const CStdString& strSearch, CFileItemList& items);

  void GetMovieDirectorsByName(const CStdString& strSearch, CFileItemList& items);
  void GetTvShowsDirectorsByName(const CStdString& strSearch, CFileItemList& items);
  void GetMusicVideoDirectorsByName(const CStdString& strSearch, CFileItemList& items);

  void GetMoviesByName(const CStdString& strSearch, CFileItemList& items);
  void GetTvShowsByName(const CStdString& strSearch, CFileItemList& items);
  void GetEpisodesByName(const CStdString& strSearch, CFileItemList& items);
  void GetMusicVideosByName(const CStdString& strSearch, CFileItemList& items);

  void GetEpisodesByPlot(const CStdString& strSearch, CFileItemList& items);
  void GetMoviesByPlot(const CStdString& strSearch, CFileItemList& items);

  bool LinkMovieToTvshow(int idMovie, int idShow, bool bRemove);
  bool IsLinkedToTvshow(int idMovie);
  bool GetLinksToTvShow(int idMovie, std::vector<int>& ids);

  bool GetArbitraryQuery(const CStdString& strQuery, const CStdString& strOpenRecordSet, const CStdString& strCloseRecordSet,
                         const CStdString& strOpenRecord, const CStdString& strCloseRecord, const CStdString& strOpenField, const CStdString& strCloseField, CStdString& strResult);
  bool ArbitraryExec(const CStdString& strExec);

  // general browsing
  bool GetGenresNav(const CStdString& strBaseDir, CFileItemList& items, int idContent=-1);
  bool GetCountriesNav(const CStdString& strBaseDir, CFileItemList& items, int idContent=-1);
  bool GetStudiosNav(const CStdString& strBaseDir, CFileItemList& items, int idContent=-1);
  bool GetYearsNav(const CStdString& strBaseDir, CFileItemList& items, int idContent=-1);
  bool GetActorsNav(const CStdString& strBaseDir, CFileItemList& items, int idContent=-1);
  bool GetDirectorsNav(const CStdString& strBaseDir, CFileItemList& items, int idContent=-1);
  bool GetWritersNav(const CStdString& strBaseDir, CFileItemList& items, int idContent=-1);
  bool GetSetsNav(const CStdString& strBaseDir, CFileItemList& items, int idContent=-1, const CStdString &where = "");
  bool GetMusicVideoAlbumsNav(const CStdString& strBaseDir, CFileItemList& items, int idArtist);

  bool GetMoviesNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre=-1, int idYear=-1, int idActor=-1, int idDirector=-1, int idStudio=-1, int idCountry=-1, int idSet=-1);
  bool GetTvShowsNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre=-1, int idYear=-1, int idActor=-1, int idDirector=-1, int idStudio=-1);
  bool GetSeasonsNav(const CStdString& strBaseDir, CFileItemList& items, int idActor=-1, int idDirector=-1, int idGenre=-1, int idYear=-1, int idShow=-1);
  bool GetEpisodesNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre=-1, int idYear=-1, int idActor=-1, int idDirector=-1, int idShow=-1, int idSeason=-1);
  bool GetMusicVideosNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre=-1, int idYear=-1, int idArtist=-1, int idDirector=-1, int idStudio=-1, int idAlbum=-1);
  
  bool GetRecentlyAddedMoviesNav(const CStdString& strBaseDir, CFileItemList& items, unsigned int limit=0);
  bool GetRecentlyAddedEpisodesNav(const CStdString& strBaseDir, CFileItemList& items, unsigned int limit=0);
  bool GetRecentlyAddedMusicVideosNav(const CStdString& strBaseDir, CFileItemList& items, unsigned int limit=0);

  bool HasContent();
  bool HasContent(VIDEODB_CONTENT_TYPE type);
  bool HasSets() const;

  void CleanDatabase(VIDEO::IVideoInfoScannerObserver* pObserver=NULL, const std::vector<int>* paths=NULL);

  /*! \brief Add a file to the database, if necessary
   If the file is already in the database, we simply return its id.
   \param url - full path of the file to add.
   \return id of the file, -1 if it could not be added.
   */
  int AddFile(const CStdString& url);

  /*! \brief Add a file to the database, if necessary
   Works for both videodb:// items and normal fileitems
   \param item CFileItem to add.
   \return id of the file, -1 if it could not be added.
   */
  int AddFile(const CFileItem& item);

  /*! \brief Add a path to the database, if necessary
   If the path is already in the database, we simply return its id.
   \param strPath the path to add
   \return id of the file, -1 if it could not be added.
   */
  int AddPath(const CStdString& strPath);

  void ExportToXML(const CStdString &path, bool singleFiles = false, bool images=false, bool actorThumbs=false, bool overwrite=false);
  bool ExportSkipEntry(const CStdString &nfoFile);
  void ExportActorThumbs(const CStdString &path, const CVideoInfoTag& tag, bool singleFiles, bool overwrite=false);
  void ImportFromXML(const CStdString &path);
  void DumpToDummyFiles(const CStdString &path);
  CStdString GetCachedThumb(const CFileItem& item) const;

  // smart playlists and main retrieval work in these functions
  bool GetMoviesByWhere(const CStdString& strBaseDir, const CStdString &where, const CStdString &order, CFileItemList& items, bool fetchSets = false);
  bool GetTvShowsByWhere(const CStdString& strBaseDir, const CStdString &where, CFileItemList& items);
  bool GetEpisodesByWhere(const CStdString& strBaseDir, const CStdString &where, CFileItemList& items, bool appendFullShowPath = true);
  bool GetMusicVideosByWhere(const CStdString &baseDir, const CStdString &whereClause, CFileItemList& items, bool checkLocks = true);

  // partymode
  int GetMusicVideoCount(const CStdString& strWhere);
  unsigned int GetMusicVideoIDs(const CStdString& strWhere, std::vector<std::pair<int,int> > &songIDs);
  bool GetRandomMusicVideo(CFileItem* item, int& idSong, const CStdString& strWhere);

  static void VideoContentTypeToString(VIDEODB_CONTENT_TYPE type, CStdString& out)
  {
    switch (type)
    {
    case VIDEODB_CONTENT_MOVIES:
      out = "movie";
      break;
    case VIDEODB_CONTENT_TVSHOWS:
      out = "tvshow";
      break;
    case VIDEODB_CONTENT_EPISODES:
      out = "episode";
      break;
    case VIDEODB_CONTENT_MUSICVIDEOS:
      out = "musicvideo";
      break;
    default:
      break;
    }
  }

protected:
  int GetMovieId(const CStdString& strFilenameAndPath);
  int GetMusicVideoId(const CStdString& strFilenameAndPath);

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
  int GetFileId(const CStdString& url);

  int AddToTable(const CStdString& table, const CStdString& firstField, const CStdString& secondField, const CStdString& value);
  int AddGenre(const CStdString& strGenre1);
  int AddActor(const CStdString& strActor, const CStdString& strThumb);
  int AddCountry(const CStdString& strCountry);
  int AddSet(const CStdString& strSet);
  int AddStudio(const CStdString& strStudio1);

  int AddTvShow(const CStdString& strPath);
  int AddMusicVideo(const CStdString& strFilenameAndPath);

  // link functions - these two do all the work
  void AddLinkToActor(const char *table, int actorID, const char *secondField, int secondID, const CStdString &role, int order);
  void AddToLinkTable(const char *table, const char *firstField, int firstID, const char *secondField, int secondID);

  void AddSetToMovie(int idMovie, int idSet);

  void AddActorToMovie(int idMovie, int idActor, const CStdString& strRole, int order);
  void AddActorToTvShow(int idTvShow, int idActor, const CStdString& strRole, int order);
  void AddActorToEpisode(int idEpisode, int idActor, const CStdString& strRole, int order);
  void AddArtistToMusicVideo(int lMVideo, int idArtist);

  void AddDirectorToMovie(int idMovie, int idDirector);
  void AddDirectorToTvShow(int idTvShow, int idDirector);
  void AddDirectorToEpisode(int idEpisode, int idDirector);
  void AddDirectorToMusicVideo(int lMVideo, int idDirector);
  void AddWriterToEpisode(int idEpisode, int idWriter);
  void AddWriterToMovie(int idMovie, int idWriter);

  void AddGenreToMovie(int idMovie, int idGenre);
  void AddGenreToTvShow(int idTvShow, int idGenre);
  void AddGenreToMusicVideo(int idMVideo, int idGenre);

  void AddStudioToMovie(int idMovie, int idStudio);
  void AddStudioToTvShow(int idTvShow, int idStudio);
  void AddStudioToMusicVideo(int idMVideo, int idStudio);

  void AddCountryToMovie(int idMovie, int idCountry);

  void AddGenreAndDirectorsAndStudios(const CVideoInfoTag& details, std::vector<int>& vecDirectors, std::vector<int>& vecGenres, std::vector<int>& vecStudios);

  void DeleteStreamDetails(int idFile);
  CVideoInfoTag GetDetailsByTypeAndId(VIDEODB_CONTENT_TYPE type, int id);
  CVideoInfoTag GetDetailsForMovie(std::auto_ptr<dbiplus::Dataset> &pDS, bool needsCast = false);
  CVideoInfoTag GetDetailsForTvShow(std::auto_ptr<dbiplus::Dataset> &pDS, bool needsCast = false);
  CVideoInfoTag GetDetailsForEpisode(std::auto_ptr<dbiplus::Dataset> &pDS, bool needsCast = false);
  CVideoInfoTag GetDetailsForMusicVideo(std::auto_ptr<dbiplus::Dataset> &pDS);
  void GetCommonDetails(std::auto_ptr<dbiplus::Dataset> &pDS, CVideoInfoTag &details);
  bool GetPeopleNav(const CStdString& strBaseDir, CFileItemList& items, const CStdString& type, int idContent=-1);
  bool GetNavCommon(const CStdString& strBaseDir, CFileItemList& items, const CStdString& type, int idContent=-1);

  void GetDetailsFromDB(std::auto_ptr<dbiplus::Dataset> &pDS, int min, int max, const SDbTableOffsets *offsets, CVideoInfoTag &details, int idxOffset = 2);
  CStdString GetValueString(const CVideoInfoTag &details, int min, int max, const SDbTableOffsets *offsets) const;
  bool GetStreamDetails(CVideoInfoTag& tag) const;

private:
  virtual bool CreateTables();
  virtual bool UpdateOldVersion(int version);

  /*! \brief (Re)Create the generic database views for movies, tvshows,
     episodes and music videos
   */
  virtual void CreateViews();

  /*! \brief Run a query on the main dataset and return the number of rows
   If no rows are found we close the dataset and return 0.
   \param sql the sql query to run
   \return the number of rows, -1 for an error.
   */
  int RunQuery(const CStdString &sql);

  /*! \brief Update routine for base path of videos
   Only required for videodb version < 44
   \param table the table to update
   \param id the primary id in the given table
   \param column the basepath column to update
   \param shows whether we're fetching shows (defaults to false)
   */
  void UpdateBasePath(const char *table, const char *id, int column, bool shows = false);

  /*! \brief Update routine for base path id of videos
   Only required for videodb version < 52
   \param table the table to update
   \param id the primary id in the given table
   \param column the column of the basepath
   \param idColumn the column of the parent path id to update
   */
  void UpdateBasePathID(const char *table, const char *id, int column, int idColumn);

  /*! \brief Determine whether the path is using lookup using folders
   \param path the path to check
   \param shows whether this path is from a tvshow (defaults to false)
   */
  bool LookupByFolders(const CStdString &path, bool shows = false);

  virtual int GetMinVersion() const { return 58; };
  virtual int GetExportVersion() const { return 1; };
  const char *GetBaseDBName() const { return "MyVideos"; };

  void ConstructPath(CStdString& strDest, const CStdString& strPath, const CStdString& strFileName);
  void SplitPath(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName);
  void InvalidatePathHash(const CStdString& strPath);
  void DeleteThumbForItem(const CStdString& strPath, bool bFolder, int idEpisode = -1);

  bool GetStackedTvShowList(int idShow, CStdString& strIn);
  void Stack(CFileItemList& items, VIDEODB_CONTENT_TYPE type, bool maintainSortOrder = false);

  /*! \brief Get a safe filename from a given string
   \param dir directory to use for the file
   \param name movie, show name, or actor to get a safe filename for
   \return safe filename based on this title
   */
  CStdString GetSafeFile(const CStdString &dir, const CStdString &name) const;

  void AnnounceRemove(std::string content, int id);
  void AnnounceUpdate(std::string content, int id);
};
