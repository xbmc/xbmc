#pragma once
#include "lib/sqlLite/sqlitedataset.h"
#include "StdString.h"
#include "utils\IMDB.h"
#include <vector>
#include <memory>
using namespace std;

typedef vector<CStdString> VECMOVIEYEARS;
typedef vector<CStdString> VECMOVIEACTORS;
typedef vector<CStdString> VECMOVIEGENRES;
typedef vector<CIMDBMovie> VECMOVIES;
typedef vector<CStdString> VECMOVIESFILES;

class CVideoDatabase
{
public:
  CVideoDatabase(void);
  virtual ~CVideoDatabase(void);
	bool		Open() ;
	void		Close() ;
  long    AddMovie(const CStdString& strFilenameAndPath, const CStdString& strcdLabel, bool bHassubtitles);
  void    GetGenres(VECMOVIEGENRES& genres);
  void    GetActors(VECMOVIEACTORS& actors);
	void    GetYears(VECMOVIEYEARS& years);

  bool    HasMovieInfo(const CStdString& strFilenameAndPath);
  bool    HasSubtitle(const CStdString& strFilenameAndPath);
  void    DeleteMovieInfo(const CStdString& strFileNameAndPath);

  void		GetFiles(long lMovieId, VECMOVIESFILES& movies);
  void    GetMovieInfo(const CStdString& strFilenameAndPath,CIMDBMovie& details);
  void    SetMovieInfo(const CStdString& strFilenameAndPath,CIMDBMovie& details);
  void    GetMoviesByGenre(CStdString& strGenre, VECMOVIES& movies);
  void    GetMoviesByActor(CStdString& strActor, VECMOVIES& movies);
	void    GetMoviesByYear(CStdString& strYear, VECMOVIES& movies);
  void    GetMoviesByPath(CStdString& strPath1, VECMOVIES& movies);
protected:
  auto_ptr<SqliteDatabase>  m_pDB;
	auto_ptr<Dataset>				  m_pDS;
  long    GetPath(const CStdString& strPath);
  long    AddPath(const CStdString& strPath, const CStdString& strCdLabel);
  
  long    GetFile(const CStdString& strFilenameAndPath, long &lPathId, long& lMovieId);
  long    AddFile(long lMovieId, long lPathId, const CStdString& strFileName);

  long    GetMovie(const CStdString& strFilenameAndPath);

  long    AddGenre(const CStdString& strGenre1);
  long    AddActor(const CStdString& strActor);
  
  void    AddActorToMovie(long lMovieId, long lActorId);
  void    AddGenreToMovie(long lMovieId, long lGenreId);

  void    Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName);
  void    RemoveInvalidChars(CStdString& strTxt);
	bool		CreateTables();
};
