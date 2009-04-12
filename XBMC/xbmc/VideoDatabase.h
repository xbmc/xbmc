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
#include "Database.h"
#include "VideoInfoTag.h"

#include <set>

struct SScraperInfo;
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
#define VIDEODB_MAX_COLUMNS 21 // leave room for the fileid
#define VIDEODB_DETAILS_FILE                 VIDEODB_MAX_COLUMNS + 2
#define VIDEODB_DETAILS_PATH                 VIDEODB_MAX_COLUMNS + 3
#define VIDEODB_DETAILS_PLAYCOUNT            VIDEODB_MAX_COLUMNS + 4
#define VIDEODB_DETAILS_LASTPLAYED           VIDEODB_MAX_COLUMNS + 5
#define VIDEODB_DETAILS_EPISODE_TVSHOW_NAME  VIDEODB_MAX_COLUMNS + 6

#define VIDEODB_DETAILS_TVSHOW_PATH          VIDEODB_MAX_COLUMNS + 1
#define VIDEODB_DETAILS_TVSHOW_NUM_EPISODES  VIDEODB_MAX_COLUMNS + 2
#define VIDEODB_DETAILS_TVSHOW_NUM_WATCHED   VIDEODB_MAX_COLUMNS + 3


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
  VIDEODB_CONTENT_EPISODES = 4
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
  VIDEODB_ID_PLAYCOUNT = 10, // unused - feel free to repurpose
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
  { VIDEODB_TYPE_COUNT, my_offsetof(CVideoInfoTag,m_playCount) }, // unused
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strRuntime) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strMPAARating) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iTop250) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strGenre) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strDirector) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strOriginalTitle) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPictureURL.m_spoof) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strStudio) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strTrailer) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_fanart.m_xml) }
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
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strMPAARating)}
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
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iTrack) }
};

class CBookmark
{
public:
  CBookmark();
  double timeInSeconds;
  CStdString thumbNailImage;
  CStdString playerState;
  CStdString player;
  long seasonNumber;
  long episodeNumber;

  enum EType
  {
    STANDARD = 0,
    RESUME = 1,
    EPISODE = 2
  } type;
};

typedef std::vector<CBookmark> VECBOOKMARKS;

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
    int numEpisodes;
    int numWatched;
  };

  CVideoDatabase(void);
  virtual ~CVideoDatabase(void);

  virtual bool CommitTransaction();

  long AddMovie(const CStdString& strFilenameAndPath);
  long AddEpisode(long idShow, const CStdString& strFilenameAndPath);

  // editing functions
  void MarkAsWatched(const CFileItem &item);
  void MarkAsUnWatched(const CFileItem &item);
  void UpdateMovieTitle(long lMovieId, const CStdString& strNewMovieTitle, VIDEODB_CONTENT_TYPE iType=VIDEODB_CONTENT_MOVIES);

  bool HasMovieInfo(const CStdString& strFilenameAndPath);
  bool HasTvShowInfo(const CStdString& strFilenameAndPath);
  bool HasEpisodeInfo(const CStdString& strFilenameAndPath);
  bool HasMusicVideoInfo(const CStdString& strFilenameAndPath);

  void GetFilePathById(long id, CStdString &filePath, VIDEODB_CONTENT_TYPE iType);
  bool GetGenreById(long id, CStdString& strGenre);

  void GetMovieInfo(const CStdString& strFilenameAndPath, CVideoInfoTag& details, long lMovieId = -1);
  void GetTvShowInfo(const CStdString& strPath, CVideoInfoTag& details, long lTvShowId = -1);
  bool GetEpisodeInfo(const CStdString& strFilenameAndPath, CVideoInfoTag& details, long lEpisodeId = -1);
  void GetMusicVideoInfo(const CStdString& strFilenameAndPath, CVideoInfoTag& details, long idMVideo=-1);

  long GetPathId(const CStdString& strPath);
  long GetTvShowId(const CStdString& strPath);
  long GetEpisodeId(const CStdString& strFilenameAndPath, long lEpisodeId=-1, long lSeasonId=-1); // lEpisodeId, lSeasonId are used for multipart episodes as hints

  void GetEpisodesByFile(const CStdString& strFilenameAndPath, std::vector<CVideoInfoTag>& episodes);

  void SetDetailsForMovie(const CStdString& strFilenameAndPath, const CVideoInfoTag& details);
  long SetDetailsForTvShow(const CStdString& strPath, const CVideoInfoTag& details);
  long SetDetailsForEpisode(const CStdString& strFilenameAndPath, const CVideoInfoTag& details, long idShow, long lEpisodeId=-1);
  void SetDetailsForMusicVideo(const CStdString& strFilenameAndPath, const CVideoInfoTag& details);

  void DeleteMovie(const CStdString& strFilenameAndPath, bool bKeepId = false, bool bKeepThumb = false);
  void DeleteTvShow(const CStdString& strPath, bool bKeepId = false, bool bKeepThumb = false);
  void DeleteEpisode(const CStdString& strFilenameAndPath, long lEpisodeId = -1, bool bKeepId = false, bool bKeepThumb = false);
  void DeleteMusicVideo(const CStdString& strFilenameAndPath, bool bKeepId = false, bool bKeepThumb = false);
  void DeleteDetailsForTvShow(const CStdString& strPath);
  void RemoveContentForPath(const CStdString& strPath,CGUIDialogProgress *progress = NULL);
  void UpdateFanart(const CFileItem &item, VIDEODB_CONTENT_TYPE type);

  // per-file video settings
  bool GetVideoSettings(const CStdString &strFilenameAndPath, CVideoSettings &settings);
  void SetVideoSettings(const CStdString &strFilenameAndPath, const CVideoSettings &settings);
  void EraseVideoSettings();

  bool GetStackTimes(const CStdString &filePath, std::vector<long> &times);
  void SetStackTimes(const CStdString &filePath, std::vector<long> &times);

  void GetBookMarksForFile(const CStdString& strFilenameAndPath, VECBOOKMARKS& bookmarks, CBookmark::EType type = CBookmark::STANDARD, bool bAppend=false);
  void AddBookMarkToFile(const CStdString& strFilenameAndPath, const CBookmark &bookmark, CBookmark::EType type = CBookmark::STANDARD);
  bool GetResumeBookMark(const CStdString& strFilenameAndPath, CBookmark &bookmark);
  void DeleteResumeBookMark(const CStdString &strFilenameAndPath);
  void ClearBookMarkOfFile(const CStdString& strFilenameAndPath, CBookmark& bookmark, CBookmark::EType type = CBookmark::STANDARD);
  void ClearBookMarksOfFile(const CStdString& strFilenameAndPath, CBookmark::EType type = CBookmark::STANDARD);
  bool GetBookMarkForEpisode(const CVideoInfoTag& tag, CBookmark& bookmark);
  void AddBookMarkForEpisode(const CVideoInfoTag& tag, const CBookmark& bookmark);
  void DeleteBookMarkForEpisode(const CVideoInfoTag& tag);

  // scraper settings
  void SetScraperForPath(const CStdString& filePath, const SScraperInfo& info, const VIDEO::SScanSettings& settings);
  bool GetScraperForPath(const CStdString& strPath, SScraperInfo& info);
  bool GetScraperForPath(const CStdString& strPath, SScraperInfo& info, int& iFound);
  bool GetScraperForPath(const CStdString& strPath, SScraperInfo& info, VIDEO::SScanSettings& settings);
  bool GetScraperForPath(const CStdString& strPath, SScraperInfo& info, VIDEO::SScanSettings& settings, int& iFound);

  // scanning hashes and paths scanned
  bool SetPathHash(const CStdString &path, const CStdString &hash);
  bool GetPathHash(const CStdString &path, CStdString &hash);
  bool GetPaths(std::map<CStdString,VIDEO::SScanSettings> &paths);

  // for music + musicvideo linkups - if no album and title given it will return the artist id, else the id of the matching video
  long GetMatchingMusicVideo(const CStdString& strArtist, const CStdString& strAlbum = "", const CStdString& strTitle = "");

  // searching functions
  void GetMoviesByActor(const CStdString& strActor, VECMOVIES& movies);
  void GetTvShowsByActor(const CStdString& strActor, VECMOVIES& movies);
  void GetEpisodesByActor(const CStdString& strActor, VECMOVIES& movies);

  void GetMusicVideosByArtist(const CStdString& strArtist, CFileItemList& items);
  void GetMusicVideosByAlbum(const CStdString& strAlbum, CFileItemList& items);

  void GetMovieGenresByName(const CStdString& strSearch, CFileItemList& items);
  void GetTvShowGenresByName(const CStdString& strSearch, CFileItemList& items);
  void GetMusicVideoGenresByName(const CStdString& strSearch, CFileItemList& items);

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

  bool LinkMovieToTvshow(long idMovie, long idShow, bool bRemove);
  bool IsLinkedToTvshow(long idMovie);
  bool GetLinksToTvShow(long idMovie, std::vector<long>& ids);

  bool GetArbitraryQuery(const CStdString& strQuery, const CStdString& strOpenRecordSet, const CStdString& strCloseRecordSet,
                         const CStdString& strOpenRecord, const CStdString& strCloseRecord, const CStdString& strOpenField, const CStdString& strCloseField, CStdString& strResult);
  bool ArbitraryExec(const CStdString& strExec);

  // general browsing
  bool GetGenresNav(const CStdString& strBaseDir, CFileItemList& items, long idContent=-1);
  bool GetStudiosNav(const CStdString& strBaseDir, CFileItemList& items, long idContent=-1);
  bool GetActorsNav(const CStdString& strBaseDir, CFileItemList& items, long idContent=-1);
  bool GetDirectorsNav(const CStdString& strBaseDir, CFileItemList& items, long idContent=-1);
  bool GetWritersNav(const CStdString& strBaseDir, CFileItemList& items, long idContent=-1);
  bool GetYearsNav(const CStdString& strBaseDir, CFileItemList& items, long idContent=-1);
  bool GetMusicVideoAlbumsNav(const CStdString& strBaseDir, CFileItemList& items, long idArtist);

  bool GetMoviesNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre=-1, long idYear=-1, long idActor=-1, long idDirector=-1, long idStudio=-1);
  bool GetTvShowsNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre=-1, long idYear=-1, long idActor=-1, long idDirector=-1);
  bool GetSeasonsNav(const CStdString& strBaseDir, CFileItemList& items, long idActor=-1, long idDirector=-1, long idGenre=-1, long idYear=-1, long idShow=-1);
  bool GetEpisodesNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre=-1, long idYear=-1, long idActor=-1, long idDirector=-1, long idShow=-1, long idSeason=-1);
  bool GetMusicVideosNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre=-1, long idYear=-1, long idArtist=-1, long idDirector=-1, long idStudio=-1, long idAlbum=-1);

  bool GetRecentlyAddedMoviesNav(const CStdString& strBaseDir, CFileItemList& items);
  bool GetRecentlyAddedEpisodesNav(const CStdString& strBaseDir, CFileItemList& items);
  bool GetRecentlyAddedMusicVideosNav(const CStdString& strBaseDir, CFileItemList& items);

  bool HasContent();
  bool HasContent(VIDEODB_CONTENT_TYPE type);

  void CleanDatabase(VIDEO::IVideoInfoScannerObserver* pObserver=NULL, const std::vector<long>* paths=NULL);

  long AddFile(const CStdString& strFileName);
  void ExportToXML(const CStdString &xmlFile, bool singleFiles = false, bool images=false, bool overwrite=false);
  void ImportFromXML(const CStdString &xmlFile);
  void DumpToDummyFiles(const CStdString &path);

  // smart playlists and main retrieval work in these functions
  bool GetMoviesByWhere(const CStdString& strBaseDir, const CStdString &where, CFileItemList& items);
  bool GetTvShowsByWhere(const CStdString& strBaseDir, const CStdString &where, CFileItemList& items);
  bool GetEpisodesByWhere(const CStdString& strBaseDir, const CStdString &where, CFileItemList& items, bool appendFullShowPath = true);
  bool GetMusicVideosByWhere(const CStdString &baseDir, const CStdString &whereClause, CFileItemList& items, bool checkLocks = true);

  // partymode
  int GetMusicVideoCount(const CStdString& strWhere);
  unsigned int GetMusicVideoIDs(const CStdString& strWhere, std::vector<std::pair<int,long> > &songIDs);
  bool GetRandomMusicVideo(CFileItem* item, long& lSongId, const CStdString& strWhere);

protected:
  long GetFileId(const CStdString& strFilenameAndPath);
  long GetMovieId(const CStdString& strFilenameAndPath);
  long GetMusicVideoId(const CStdString& strFilenameAndPath);

  long AddPath(const CStdString& strPath);
  long AddGenre(const CStdString& strGenre1);
  long AddActor(const CStdString& strActor, const CStdString& strThumb);
  long AddStudio(const CStdString& strStudio1);
  long AddTvShow(const CStdString& strPath);
  long AddMusicVideo(const CStdString& strFilenameAndPath);

  // link functions - these two do all the work
  void AddLinkToActor(const char *table, long actorID, const char *secondField, long secondID, const CStdString &role);
  void AddToLinkTable(const char *table, const char *firstField, long firstID, const char *secondField, long secondID);

  void AddActorToMovie(long lMovieId, long lActorId, const CStdString& strRole);
  void AddActorToTvShow(long lTvShowId, long lActorId, const CStdString& strRole);
  void AddActorToEpisode(long lEpisode, long lActorId, const CStdString& strRole);
  void AddArtistToMusicVideo(long lMVideo, long lArtistId);

  void AddDirectorToMovie(long lMovieId, long lDirectorId);
  void AddDirectorToTvShow(long lTvShowId, long lDirectorId);
  void AddDirectorToEpisode(long lEpisodeId, long lDirectorId);
  void AddDirectorToMusicVideo(long lMVideo, long lDirectorId);
  void AddWriterToEpisode(long lEpisodeId, long lWriterId);
  void AddWriterToMovie(long lMovieId, long lWriterId);

  void AddGenreToMovie(long lMovieId, long lGenreId);
  void AddGenreToTvShow(long lTvShowId, long lGenreId);
  void AddGenreToMusicVideo(long lMVideoId, long lGenreId);

  void AddStudioToMovie(long lMovieId, long lStudioId);
  void AddStudioToMusicVideo(long lMVideoId, long lStudioId);

  void AddGenreAndDirectorsAndStudios(const CVideoInfoTag& details, std::vector<long>& vecDirectors, std::vector<long>& vecGenres, std::vector<long>& vecStudios);

  int GetPlayCount(long id);
  CVideoInfoTag GetDetailsByTypeAndId(VIDEODB_CONTENT_TYPE type, long id);
  CVideoInfoTag GetDetailsForMovie(std::auto_ptr<dbiplus::Dataset> &pDS, bool needsCast = false);
  CVideoInfoTag GetDetailsForTvShow(std::auto_ptr<dbiplus::Dataset> &pDS, bool needsCast = false);
  CVideoInfoTag GetDetailsForEpisode(std::auto_ptr<dbiplus::Dataset> &pDS, bool needsCast = false);
  CVideoInfoTag GetDetailsForMusicVideo(std::auto_ptr<dbiplus::Dataset> &pDS);
  void GetCommonDetails(std::auto_ptr<dbiplus::Dataset> &pDS, CVideoInfoTag &details);
  bool GetPeopleNav(const CStdString& strBaseDir, CFileItemList& items, const CStdString &type, long idContent=-1);

  void GetDetailsFromDB(std::auto_ptr<dbiplus::Dataset> &pDS, int min, int max, const SDbTableOffsets *offsets, CVideoInfoTag &details);
  CStdString GetValueString(const CVideoInfoTag &details, int min, int max, const SDbTableOffsets *offsets) const;

private:
  virtual bool CreateTables();
  virtual bool UpdateOldVersion(int version);

  void ConstructPath(CStdString& strDest, const CStdString& strPath, const CStdString& strFileName);
  void SplitPath(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName);
  void InvalidatePathHash(const CStdString& strPath);
  void DeleteThumbForItem(const CStdString& strPath, bool bFolder);

  bool GetStackedTvShowList(long idShow, CStdString& strIn);
  void Stack(CFileItemList& items, VIDEODB_CONTENT_TYPE type, bool maintainSortOrder = false);
};
