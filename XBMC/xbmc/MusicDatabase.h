#pragma once
#include "lib/sqlLite/sqlitedataset.h"
#include "StdString.h"
#include <vector>
#include <memory>
using namespace std;

class CSong
{
public:
	CStdString strFileName;
	CStdString strTitle;
	CStdString strArtist;
	CStdString strAlbum;
	CStdString strGenre;
	int iTrack;
	int iDuration;
	int iYear;
};

class CAlbum
{
public:
	CStdString strAlbum;
	CStdString strArtist;
	CStdString strGenre;
	CStdString strTones ;
	CStdString strStyles ;
	CStdString strReview ;
	CStdString strImage ;
	int    iRating ;
	int		 iYear;
};

typedef vector<CSong>  VECSONGS;
typedef vector<CStdString> VECARTISTS;
typedef vector<CStdString> VECGENRES;
typedef vector<CAlbum> VECALBUMS;

class CMusicDatabase
{
public:
	CMusicDatabase(void);
	virtual ~CMusicDatabase(void);
	bool		Open() ;
	void		Close() ;
	void		AddSong(const CSong& song);
	long		AddAlbumInfo(const CAlbum& album);
	bool		GetAlbumInfo(const CStdString& strAlbum, CAlbum& album);
	bool		GetSong(const CStdString& strTitle, CSong& song);
	bool		GetSongByFileName(const CStdString& strFileName, CSong& song);
	bool		GetSongsByArtist(const CStdString strArtist, VECSONGS& songs);
	bool		GetSongsByAlbum(const CStdString& strAlbum, VECSONGS& songs);
	bool		GetSongsByGenre(const CStdString& strGenre, VECSONGS& songs);
	bool    GetArtists(VECARTISTS& artists);
	bool    GetAlbums(VECALBUMS& albums);
	bool    GetGenres(VECGENRES& genres);
	void    Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName);
protected:
  auto_ptr<SqliteDatabase> m_pDB;
	auto_ptr<Dataset>				 m_pDS;
	bool						CreateTables();
	long						AddAlbum(const CStdString& strAlbum, long lArtistId);
	long						AddGenre(const CStdString& strGenre);
	long						AddArtist(const CStdString& strArtist);
	long						AddPath(const CStdString& strPath);
	void						RemoveInvalidChars(CStdString& strTxt);
};
