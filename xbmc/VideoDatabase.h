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

#define VIDEODB_ID_TITLE         1
#define VIDEODB_ID_PLOT          2
#define VIDEODB_ID_DIRECTOR      3
#define VIDEODB_ID_PLOTOUTLINE   4
#define VIDEODB_ID_TAGLINE       5
#define VIDEODB_ID_VOTES         6
#define VIDEODB_ID_RATING        7
#define VIDEODB_ID_CREDITS       8
#define VIDEODB_ID_YEAR          9
#define VIDEODB_ID_GENRE         10
#define VIDEODB_ID_THUMBURL      11
#define VIDEODB_ID_IDENT         12
#define VIDEODB_ID_WATCHED       13
#define VIDEODB_ID_RUNTIME       14
#define VIDEODB_ID_MPAA          15
#define VIDEODB_ID_TOP250        16

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

  CIMDBMovie GetDetailsFromMovie(long lMovieId);

private:
  virtual bool CreateTables();
  virtual bool UpdateOldVersion(float fVersion);

};
