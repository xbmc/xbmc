#pragma once
#include "lib/sqlLite/sqlitedataset.h"
#include <string>
#include <vector>
using namespace std;

class CSong
{
public:
	string strFileName;
	string strTitle;
	string strArtist;
	string strAlbum;
	string strGenre;
	int iTrack;
	int iDuration;
	int iYear;
};

class CAlbum
{
public:
	string strAlbum;
	string strArtist;
	string strGenre;
	string strTones ;
	string strStyles ;
	string strReview ;
	string strImage ;
	int    iRating ;
	int		 iYear;
};

typedef vector<CSong>  VECSONGS;
typedef vector<string> VECARTISTS;
typedef vector<string> VECGENRES;
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
	bool		GetSong(const string& strTitle, CSong& song);
	bool		GetSongByFileName(const string& strFileName, CSong& song);
	bool		GetSongsByArtist(const string strArtist, VECSONGS& songs);
	bool		GetSongsByAlbum(const string& strAlbum, VECSONGS& songs);
	bool		GetSongsByGenre(const string& strGenre, VECSONGS& songs);
	bool    GetArtists(VECARTISTS& artists);
	bool    GetAlbums(VECALBUMS& albums);
	bool    GetGenres(VECGENRES& genres);

protected:
  SqliteDatabase* m_pDB;
	Dataset*				m_pDS;
	bool						CreateTables();
	long						AddAlbum(const string& strAlbum);
	long						AddGenre(const string& strGenre);
	long						AddArtist(const string& strArtist);
	void						RemoveInvalidChars(string& strTxt);
};
