/*!
  \file MusicDatabase.h
	\brief
	*/
#pragma once
#include "lib/sqlLite/sqlitedataset.h"
#include "StdString.h"
#include <vector>
#include <set>
#include <memory>
#include "MusicDatabaseReorg.h"
using namespace std;

/*!
	\ingroup music
	\brief Class to store and read song information from CMusicDatabase
	\sa CAlbum, CMusicDatabase
	*/
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

/*!
	\ingroup music
	\brief Class to store and read album information from CMusicDatabase
	\sa CSong, CMusicDatabase
	*/
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

/*!
	\ingroup music
	\brief A vector of CSong objects, used for CMusicDatabase
	\sa CMusicDatabase
	*/
typedef vector<CSong>  VECSONGS;

/*!
	\ingroup music
	\brief A map of CSong objects, used for CMusicDatabase
	\sa IMAPSONGS, CMusicDatabase
	*/
typedef map<CStdString,CSong>  MAPSONGS;

/*!
	\ingroup music
	\brief The MAPSONGS iterator
	\sa MAPSONGS, CMusicDatabase
	*/
typedef map<CStdString,CSong>::iterator  IMAPSONGS;

/*!
	\ingroup music
	\brief A vector of CStdString objects, used for CMusicDatabase
	*/
typedef vector<CStdString> VECARTISTS;

/*!
	\ingroup music
	\brief A vector of CStdString objects, used for CMusicDatabase
	\sa CMusicDatabase
	*/
typedef vector<CStdString> VECGENRES;

/*!
	\ingroup music
	\brief A vector of CAlbum objects, used for CMusicDatabase
	\sa CMusicDatabase
	*/
typedef vector<CAlbum> VECALBUMS;

/*!
	\ingroup music
	\brief A set of CStdString objects, used for CMusicDatabase
	\sa ISETPATHES, CMusicDatabase
	*/
typedef set<CStdString> SETPATHES;

/*!
	\ingroup music
	\brief The SETPATHES iterator
	\sa SETPATHES, CMusicDatabase
	*/
typedef set<CStdString>::iterator ISETPATHES;

/*!
	\ingroup music
	\brief Class to store and read tag information

	CMusicDatabase can be used to read and store
	tag information for faster access. It is based on
	sqlite (http://www.sqlite.org).

	Here is the database layout:
  \image html musicdatabase.png

	\sa CAlbum, CSong, VECSONGS, MAPSONGS, VECARTISTS, VECALBUMS, VECGENRES
	*/
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
	bool		IsOpen();
	void		Close() ;
	void		AddSong(const CSong& song,bool bCheck=true);
	long		AddAlbumInfo(const CAlbum& album);
	bool		GetAlbumInfo(const CStdString& strAlbum, const CStdString& strPath, CAlbum& album);
	bool		GetSong(const CStdString& strTitle, CSong& song);
	bool		GetSongByFileName(const CStdString& strFileName, CSong& song);
	bool		GetSongsByPath(const CStdString& strPath, VECSONGS& songs);
	bool		GetSongsByPath(const CStdString& strPath, MAPSONGS& songs);
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
	bool		GetSongsByPathes(SETPATHES& pathes, MAPSONGS& songs);
	bool		GetAlbumByPath(const CStdString& strPath, CAlbum& album);
protected:
	auto_ptr<SqliteDatabase> m_pDB;
	auto_ptr<Dataset>				 m_pDS;
	map<CStdString, CArtistCache> m_artistCache;
	map<CStdString, CGenreCache> m_genreCache;
	map<CStdString, CPathCache> m_pathCache;
	map<CStdString, CAlbumCache> m_albumCache;
	bool		m_bOpen;
	int			m_iRefCount;
	bool		CreateTables();
	long		AddAlbum(const CStdString& strAlbum, long lArtistId, long lPathId, const CStdString& strPath);
	long		AddGenre(const CStdString& strGenre);
	long		AddArtist(const CStdString& strArtist);
	long		AddPath(const CStdString& strPath);
	void		RemoveInvalidChars(CStdString& strTxt);
};

extern CMusicDatabase	g_musicDatabase;
