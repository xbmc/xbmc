#pragma once
#include "lib/sqlLite/sqlitedataset.h"
#include "StdString.h"
#include "utils\IMDB.h"
#include <vector>
#include <memory>
using namespace std;

typedef vector<CStdString> VECMOVIEACTORS;
typedef vector<CStdString> VECMOVIEGENRES;
typedef vector<CIMDBMovie> VECMOVIES;

class CVideoDatabase
{
public:
  CVideoDatabase(void);
  virtual ~CVideoDatabase(void);
	bool		Open() ;
	void		Close() ;
  long    AddMovie(const CStdString& strFilenameAndPath, const CStdString& strcdLabel, bool bHassubtitles);
  bool    MovieExists(const CStdString& strFilenameAndPath);
  bool    HasSubtitle(const CStdString& strFilenameAndPath);
  bool    HasMovieInfo(const CStdString& strFilenameAndPath);
  void    GetMovieInfo(const CStdString& strFilenameAndPath,CIMDBMovie& details);
  void    SetMovieInfo(const CStdString& strFilenameAndPath,const CIMDBMovie& details);
  void    GetGenres(VECMOVIEGENRES& genres);
  void    GetActors(VECMOVIEACTORS& actors);
  void    GetMoviesByGenre(CStdString& strGenre, VECMOVIES& movies);
  void    GetMoviesByActor(CStdString& strActor, VECMOVIES& movies);
  void    DeleteMovieInfo(const CStdString& strFileNameAndPath);
protected:
  auto_ptr<SqliteDatabase>  m_pDB;
	auto_ptr<Dataset>				  m_pDS;
  long                      AddGenre(const CStdString& strGenre1);
  long                      AddActor(const CStdString& strActor);
  void                      RemoveInvalidChars(CStdString& strTxt);
  void                      Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName);
	bool						          CreateTables();
  long                      AddPath(const CStdString& strPath, const CStdString& strFilename, const CStdString& strCdLabel);
  void                      AddActorToMovie(long lMovieId, long lActorId);
  void                      AddGenreToMovie(long lMovieId, long lGenreId);
};
