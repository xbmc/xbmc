#pragma once
#include "lib/sqlLite/sqlitedataset.h"
#include "StdString.h"
#include <vector>
#include <memory>
#include "MusicDatabaseReorg.h"
using namespace std;

class CSong
{
public:
	CSong() ;
	virtual ~CSong(){};
	void Clear() ;

	CStdString strFileName;
	CStdString strTitle;
	CStdString strArtist;
	CStdString strAlbum;
	CStdString strGenre;
	int iTrack;
	int iDuration;
	int iYear;
	int iTimedPlayed;
};

class CAlbum
{
public:
	CStdString strAlbum;
	CStdString strPath;
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
	friend class CMusicDatabaseReorg;

	class CArtistCache
	{
	public:
		long idArtist;
		CStdString strArtist;
	};

	class CPathCache
	{
	public:
		long idPath;
		CStdString strPath;
	};

	class CGenreCache
	{
	public:
		long idGenre;
		CStdString strGenre;
	};

	class CAlbumCache : public CAlbum
	{
	public:
		long idAlbum;
		long idArtist;
		long idPath;
	};

public:
	CMusicDatabase(void);
	virtual ~CMusicDatabase(void);
	bool		Open() ;
	void		Close() ;
	void		AddSong(const CSong& song,bool bCheck=true);
	long		AddAlbumInfo(const CAlbum& album);
	bool		GetAlbumInfo(const CStdString& strAlbum, CAlbum& album);
	bool		GetSong(const CStdString& strTitle, CSong& song);
	bool		GetSongByFileName(const CStdString& strFileName, CSong& song);
	bool		GetSongsByPath(const CStdString& strPath, VECSONGS& songs);
	bool		GetSongsByArtist(const CStdString strArtist, VECSONGS& songs);
	bool		GetSongsByAlbum(const CStdString& strAlbum1, const CStdString& strPath1, VECSONGS& songs);
	bool		GetSongsByGenre(const CStdString& strGenre, VECSONGS& songs);
	bool		GetArtists(VECARTISTS& artists);
	bool		GetAlbums(VECALBUMS& albums);
	bool		GetGenres(VECGENRES& genres);
	bool		GetTop100(VECSONGS& songs);
	bool		IncrTop100CounterByFileName(const CStdString& strFileName1);
	void		BeginTransaction();
	void		CommitTransaction();
	void		RollbackTransaction();
	bool		InTransaction();
	void		EmptyCache();
	void		CheckVariousArtistsAndCoverArt();
	bool		GetRecentlyPlayedAlbums(VECALBUMS& albums);
protected:
	auto_ptr<SqliteDatabase> m_pDB;
	auto_ptr<Dataset>				 m_pDS;
	map<CStdString, CArtistCache> m_artistCache;
	map<CStdString, CGenreCache> m_genreCache;
	map<CStdString, CPathCache> m_pathCache;
	map<CStdString, CAlbumCache> m_albumCache;
	bool		CreateTables();
	long		AddAlbum(const CStdString& strAlbum, long lArtistId, long lPathId, const CStdString& strPath);
	long		AddGenre(const CStdString& strGenre);
	long		AddArtist(const CStdString& strArtist);
	long		AddPath(const CStdString& strPath);
	void		RemoveInvalidChars(CStdString& strTxt);
};
