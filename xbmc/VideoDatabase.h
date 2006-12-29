#pragma once
#include "Database.h"
#include "utils\IMDB.h"
#include "settings/VideoSettings.h"

typedef vector<CStdString> VECMOVIEYEARS;
typedef vector<CStdString> VECMOVIEACTORS;
typedef vector<CStdString> VECMOVIEGENRES;
typedef vector<CIMDBMovie> VECMOVIES;
typedef vector<CStdString> VECMOVIESFILES;

#define VIDEO_SHOW_ALL 0
#define VIDEO_SHOW_UNWATCHED 1
#define VIDEO_SHOW_WATCHED 2

typedef enum // this enum MUST match the offset struct further down!! and make sure to keep min and max at -1 and sizeof(offsets)
{
  VIDEODB_ID_MIN = -1,
  VIDEODB_ID_TITLE = 0,
  VIDEODB_ID_PLOT = 1,
  VIDEODB_ID_DIRECTOR = 2,
  VIDEODB_ID_PLOTOUTLINE = 3,
  VIDEODB_ID_TAGLINE = 4,
  VIDEODB_ID_VOTES = 5,
  VIDEODB_ID_RATING = 6,
  VIDEODB_ID_CREDITS = 7,
  VIDEODB_ID_YEAR = 8,
  VIDEODB_ID_GENRE = 9,
  VIDEODB_ID_THUMBURL = 10,
  VIDEODB_ID_IDENT = 11,
  VIDEODB_ID_WATCHED = 12,
  VIDEODB_ID_RUNTIME = 13,
  VIDEODB_ID_MPAA = 14,
  VIDEODB_ID_TOP250 = 15,
  VIDEDB_ID_MAX
} VIDEODB_IDS;

#define VIDEODB_TYPE_STRING 1
#define VIDEODB_TYPE_INT 2
#define VIDEODB_TYPE_FLOAT 3
#define VIDEODB_TYPE_BOOL 4

const struct SDbMovieOffsets
{
  int type;
  size_t offset;
} DbMovieOffsets[] = 
{
  { VIDEODB_TYPE_STRING, offsetof(CIMDBMovie,m_strTitle) },
  { VIDEODB_TYPE_STRING, offsetof(CIMDBMovie,m_strPlot) },
  { VIDEODB_TYPE_STRING, offsetof(CIMDBMovie,m_strDirector) },
  { VIDEODB_TYPE_STRING, offsetof(CIMDBMovie,m_strPlotOutline) },
  { VIDEODB_TYPE_STRING, offsetof(CIMDBMovie,m_strTagLine) },
  { VIDEODB_TYPE_STRING, offsetof(CIMDBMovie,m_strVotes) },
  { VIDEODB_TYPE_FLOAT, offsetof(CIMDBMovie,m_fRating) },
  { VIDEODB_TYPE_STRING, offsetof(CIMDBMovie,m_strWritingCredits) },
  { VIDEODB_TYPE_INT, offsetof(CIMDBMovie,m_iYear) },
  { VIDEODB_TYPE_STRING, offsetof(CIMDBMovie,m_strGenre) },
  { VIDEODB_TYPE_STRING, offsetof(CIMDBMovie,m_strPictureURL) },
  { VIDEODB_TYPE_STRING, offsetof(CIMDBMovie,m_strIMDBNumber) },
  { VIDEODB_TYPE_BOOL, offsetof(CIMDBMovie,m_bWatched) },
  { VIDEODB_TYPE_STRING, offsetof(CIMDBMovie,m_strRuntime) },
  { VIDEODB_TYPE_STRING, offsetof(CIMDBMovie,m_strMPAARating) },
  { VIDEODB_TYPE_INT, offsetof(CIMDBMovie,m_iTop250) }
};

class CBookmark
{
public:
  CBookmark();
  double timeInSeconds;
  CStdString thumbNailImage;
  CStdString playerState;
  CStdString player;

  enum EType
  {
    STANDARD = 0,
    RESUME = 1,
  } type;
};

typedef vector<CBookmark> VECBOOKMARKS;

#define COMPARE_PERCENTAGE     0.90f // 90%
#define COMPARE_PERCENTAGE_MIN 0.50f // 50%


class CVideoDatabase : public CDatabase
{
public:
  CVideoDatabase(void);
  virtual ~CVideoDatabase(void);

  long AddMovie(const CStdString& strFilenameAndPath);

  void MarkAsWatched(const CFileItem &item);
  void MarkAsWatched(long lMovieId);

  void MarkAsUnWatched(long lMovieId);
  void UpdateMovieTitle(long lMovieId, const CStdString& strNewMovieTitle);

  void GetMoviesByActor(CStdString& strActor, VECMOVIES& movies);

  bool HasMovieInfo(const CStdString& strFilenameAndPath);
  bool HasSubtitle(const CStdString& strFilenameAndPath);
  void DeleteMovieInfo(const CStdString& strFileNameAndPath);

  void GetFilePath(long lMovieId, CStdString &filePath);
  void GetMovieInfo(const CStdString& strFilenameAndPath, CIMDBMovie& details, long lMovieId = -1);
  long GetMovieInfo(const CStdString& strFilenameAndPath);
  void SetMovieInfo(const CStdString& strFilenameAndPath, CIMDBMovie& details);
  void GetMoviesByPath(CStdString& strPath1, VECMOVIES& movies);

  void GetBookMarksForFile(const CStdString& strFilenameAndPath, VECBOOKMARKS& bookmarks, CBookmark::EType type = CBookmark::STANDARD);
  void AddBookMarkToFile(const CStdString& strFilenameAndPath, const CBookmark &bookmark, CBookmark::EType type = CBookmark::STANDARD);
  bool GetResumeBookMark(const CStdString& strFilenameAndPath, CBookmark &bookmark);
  void ClearBookMarkOfFile(const CStdString& strFilenameAndPath, CBookmark& bookmark, CBookmark::EType type = CBookmark::STANDARD);
  void ClearBookMarksOfFile(const CStdString& strFilenameAndPath, CBookmark::EType type = CBookmark::STANDARD);

  void DeleteMovie(const CStdString& strFilenameAndPath);
  int GetRecentMovies(long* pMovieIdArray, int nSize);

  bool GetVideoSettings(const CStdString &strFilenameAndPath, CVideoSettings &settings);
  void SetVideoSettings(const CStdString &strFilenameAndPath, const CVideoSettings &settings);
  void EraseVideoSettings();

  bool GetStackTimes(const CStdString &filePath, vector<long> &times);
  void SetStackTimes(const CStdString &filePath, vector<long> &times);

  bool GetGenresNav(const CStdString& strBaseDir, CFileItemList& items);
  bool GetActorsNav(const CStdString& strBaseDir, CFileItemList& items);
  bool GetTitlesNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre=-1, long idYear=-1, long idActor=-1);
  bool GetYearsNav(const CStdString& strBaseDir, CFileItemList& items);
  bool GetGenreById(long lIdGenre, CStdString& strGenre);
  int GetMovieCount();

  void CleanDatabase();
  
  long AddFile(const CStdString& strFileName);
protected:
  long GetPath(const CStdString& strPath);
  long GetFile(const CStdString& strFilenameAndPath, long& lMovieId, long& lEpisodeId, bool bExact = false);

  long GetMovie(const CStdString& strFilenameAndPath);

  long AddPath(const CStdString& strPath);
  long AddGenre(const CStdString& strGenre1);
  long AddActor(const CStdString& strActor);

  void AddActorToMovie(long lMovieId, long lActorId);
  void AddGenreToMovie(long lMovieId, long lGenreId);

  CIMDBMovie GetDetailsForMovie(long lMovieId);

private:
  virtual bool CreateTables();
  virtual bool UpdateOldVersion(float fVersion);

};
