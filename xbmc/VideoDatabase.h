#pragma once
#include "Database.h"
#include "utils\IMDB.h"
#include "settings/VideoSettings.h"

typedef vector<CStdString> VECMOVIEYEARS;
typedef vector<CStdString> VECMOVIEACTORS;
typedef vector<CStdString> VECMOVIEGENRES;
typedef vector<CIMDBMovie> VECMOVIES;
typedef vector<CStdString> VECMOVIESFILES;

struct CBookmark
{
  int timeInSeconds;
  CStdString thumbNailImage;
};

typedef vector<CBookmark> VECBOOKMARKS;

#define COMPARE_PERCENTAGE     0.90f // 90%
#define COMPARE_PERCENTAGE_MIN 0.50f // 50%


class CVideoDatabase : public CDatabase
{
public:
  CVideoDatabase(void);
  virtual ~CVideoDatabase(void);
  long AddMovie(const CStdString& strFilenameAndPath, const CStdString& strcdLabel, bool bHassubtitles);
  void GetGenres(VECMOVIEGENRES& genres);
  void GetActors(VECMOVIEACTORS& actors);
  void GetYears(VECMOVIEYEARS& years);

  bool HasMovieInfo(const CStdString& strFilenameAndPath);
  bool HasSubtitle(const CStdString& strFilenameAndPath);
  void DeleteMovieInfo(const CStdString& strFileNameAndPath);

  void GetFiles(long lMovieId, VECMOVIESFILES& movies);
  void GetMovieInfo(const CStdString& strFilenameAndPath, CIMDBMovie& details);
  void SetMovieInfo(const CStdString& strFilenameAndPath, CIMDBMovie& details);
  void GetMoviesByGenre(CStdString& strGenre, VECMOVIES& movies);
  void GetMoviesByActor(CStdString& strActor, VECMOVIES& movies);
  void GetMoviesByYear(CStdString& strYear, VECMOVIES& movies);
  void GetMoviesByPath(CStdString& strPath1, VECMOVIES& movies);
  void GetMovies(VECMOVIES& movies);
  void GetBookMarksForMovie(const CStdString& strFilenameAndPath, VECBOOKMARKS& bookmarks);
  void AddBookMarkToMovie(const CStdString& strFilenameAndPath, const CBookmark &bookmark);
  void ClearBookMarksOfMovie(const CStdString& strFilenameAndPath);
  void DeleteMovie(const CStdString& strFilenameAndPath);
  void GetDVDLabel(long lMovieId, CStdString& strDVDLabel);
  void SetDVDLabel(long lMovieId, const CStdString& strDVDLabel1);
  int GetRecentMovies(long* pMovieIdArray, int nSize);

  bool GetVideoSettings(const CStdString &strFilenameAndPath, CVideoSettings &settings);
  void SetVideoSettings(const CStdString &strFilenameAndPath, const CVideoSettings &settings);

protected:
  long GetPath(const CStdString& strPath);
  long AddPath(const CStdString& strPath);

  long GetFile(const CStdString& strFilenameAndPath, long &lPathId, long& lMovieId, bool bExact = false);
  long AddFile(long lMovieId, long lPathId, const CStdString& strFileName);

  long GetMovie(const CStdString& strFilenameAndPath);

  long AddGenre(const CStdString& strGenre1);
  long AddActor(const CStdString& strActor);

  void AddActorToMovie(long lMovieId, long lActorId);
  void AddGenreToMovie(long lMovieId, long lGenreId);

  CIMDBMovie GetDetailsFromDataset(auto_ptr<Dataset> &pDS);

private:
  virtual bool CreateTables();
  virtual bool UpdateOldVersion(float fVersion);

};
