/*!
  \file MusicDatabase.h
	\brief
	*/
#pragma once
#include "lib/sqlLite/sqlitedataset.h"
#include "StdString.h"
#include "StringUtils.h"
#include <vector>
#include <set>
#include <memory>
#include "MusicInfotag.h"

using namespace std;
using namespace MUSIC_INFO;
using namespace dbiplus;

//	return codes of Cleaning up the Database
//	numbers are strings from strings.xml
#define ERROR_OK					317
#define ERROR_CANCEL				0
#define ERROR_DATABASE				315
#define ERROR_REORG_SONGS			319			
#define ERROR_REORG_ARTIST			321
#define ERROR_REORG_GENRE			323
#define ERROR_REORG_PATH			325
#define ERROR_REORG_ALBUM			327
#define ERROR_WRITING_CHANGES		329	
#define ERROR_COMPRESSING			332

#define NUM_SONGS_BEFORE_COMMIT	500

/*!
	\ingroup music
	\brief Class to store and read song information from CMusicDatabase
	\sa CAlbum, CMusicDatabase
	*/
class CSong
{
public:
	CSong() ;
	CSong(CMusicInfoTag& tag);
	virtual ~CSong(){};
	void Clear() ;

	bool operator<(const CSong &song) const
	{
		if (strFileName < song.strFileName) return true;
		if (strFileName > song.strFileName) return false;
		if (iTrack < song.iTrack) return true;
		return false;
	}
	CStdString strFileName;
	CStdString strTitle;
	CStdString strArtist;
	CStdString strAlbum;
	CStdString strGenre;
  CStdString strThumb;
	int iTrack;
	int iDuration;
	int iYear;
	int iTimedPlayed;
	int iStartOffset;
	int iEndOffset;
};

/*!
	\ingroup music
	\brief Class to store and read album information from CMusicDatabase
	\sa CSong, CMusicDatabase
	*/
class CAlbum
{
public:
	bool operator<(const CAlbum &a) const
	{
		return strAlbum+strPath<a.strAlbum+a.strPath;
	}
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
	\brief A vector of longs for iDs, used for CMusicDatabase's multiple artist/genre capability
	*/
typedef vector<long> VECLONGS;

class CGUIDialogProgress;

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
		long idPath;
	};

public:
	CMusicDatabase(void);
	virtual ~CMusicDatabase(void);
	bool		Open() ;
	bool		IsOpen();
	void		Close() ;
	void		AddSong(const CSong& song,bool bCheck=true);
	long		AddAlbumInfo(const CAlbum& album, const VECSONGS& songs);
	long		UpdateAlbumInfo(const CAlbum& album, const VECSONGS& songs);
	bool		GetAlbumInfo(const CStdString& strAlbum, const CStdString& strPath, CAlbum& album, VECSONGS& songs);
	bool		GetSong(const CStdString& strTitle, CSong& song);
	bool		GetSongByFileName(const CStdString& strFileName, CSong& song);
	bool		GetSongsByPath(const CStdString& strPath, VECSONGS& songs);
	bool		GetSongsByPath(const CStdString& strPath, MAPSONGS& songs, bool bAppendToMap=false);
	bool		GetSongsByArtist(const CStdString strArtist, VECSONGS& songs);
	bool		GetSongsByAlbum(const CStdString& strAlbum, const CStdString& strPath, VECSONGS& songs);
	bool		GetSongsByGenre(const CStdString& strGenre, VECSONGS& songs);
	bool		GetArtists(VECARTISTS& vecArtists);
	bool		GetArtistsByName(const CStdString& strArtist, VECARTISTS& artists);
	bool		GetAlbums(VECALBUMS& albums);
	bool		GetGenres(VECGENRES& genres);
	bool		GetTop100(VECSONGS& songs);
	bool		IncrTop100CounterByFileName(const CStdString& strFileName1);
	void		BeginTransaction();
	bool		CommitTransaction();
	void		RollbackTransaction();
	bool		InTransaction();
	void		EmptyCache();
	void		CheckVariousArtistsAndCoverArt();
	bool		GetRecentlyPlayedAlbums(VECALBUMS& albums);
	bool		GetRecentlyAddedAlbums(VECALBUMS& albums);
	bool		GetSongsByPathes(SETPATHES& pathes, MAPSONGS& songs);
	bool		GetAlbumByPath(const CStdString& strPath, CAlbum& album);
	bool		GetAlbumsByPath(const CStdString& strPath, VECALBUMS& albums);
	bool		GetAlbumsByArtist(const CStdString& strArtist, VECALBUMS& albums);
	bool		FindAlbumsByName(const CStdString& strSearch, VECALBUMS& albums);
	bool		FindSongsByName(const CStdString& strSearch, VECSONGS& songs);
	bool		FindSongsByNameAndArtist(const CStdString& strSearch, VECSONGS& songs);
	bool		GetSubpathsFromPath(const CStdString &strPath, CStdString& strPathIds);
	bool		RemoveSongsFromPaths(const CStdString &strPathIds);
	bool		CleanupAlbumsArtistsGenres(const CStdString &strPathIds);
	bool		Compress();
	int			Cleanup(CGUIDialogProgress *pDlgProgress);
	void		Clean();
	void		DeleteAlbumInfo();
	void		DeleteCDDBInfo();
	void		Interupt();

	bool		GetArtistsNav(VECARTISTS& artists, const CStdString &strGenre1);
	bool		GetAlbumsNav(VECALBUMS& albums, const CStdString &strGenre1, const CStdString &strArtist1);
	bool		GetSongsNav(VECSONGS& songs, const CStdString &strGenre1, const CStdString &strArtist1, const CStdString &strAlbum1);

protected:
	auto_ptr<SqliteDatabase> m_pDB;
	auto_ptr<Dataset>				 m_pDS;
	auto_ptr<Dataset>				 m_pDS2;
	map<CStdString, CArtistCache> m_artistCache;
	map<CStdString, CGenreCache> m_genreCache;
	map<CStdString, CPathCache> m_pathCache;
	map<CStdString, CPathCache> m_thumbCache;
	map<CStdString, CAlbumCache> m_albumCache;
	bool		m_bOpen;
	int			m_iRefCount;
	bool		CreateTables();
	long		AddAlbum(const CStdString& strAlbum, const long lArtistId, const int iNumArtists, const CStdString& strArtist, long lPathId, const CStdString& strPath);
	long		AddGenre(const CStdString& strGenre);
	long		AddArtist(const CStdString& strArtist);
	long		AddPath(const CStdString& strPath);
  long    AddThumb(const CStdString& strThumb1);
	void		AddExtraArtists(const VECARTISTS& vecArtists, long lSongId, long lAlbumId, bool bCheck=true);
	void		AddExtraGenres(const VECGENRES& vecGenres, long lSongId, long lAlbumId, bool bCheck=true);
	bool		AddAlbumInfoSongs(long idAlbumInfo, const VECSONGS& songs);
	bool		GetAlbumInfoSongs(long idAlbumInfo, VECSONGS& songs);
	bool		UpdateAlbumInfoSongs(long idAlbumInfo, const VECSONGS& songs);
	void		RemoveInvalidChars(CStdString& strTxt);

private:
	bool		GetExtraArtistsForAlbum(long lAlbumId, CStdString &strArtist);
	bool		GetExtraArtistsForSong(long lSongId, CStdString &strArtist);
	bool		GetExtraGenresForAlbum(long lAlbumId, CStdString &strGenre);
	bool		GetExtraGenresForSong(long lSongId, CStdString &strGenre);
	CSong		GetSongFromDataset();
	CAlbum		GetAlbumFromDataset();
	bool		CleanupSongs();
	bool		CleanupPaths();
	bool		CleanupAlbums();
	bool		CleanupArtists();
	bool		CleanupGenres();
	bool		CleanupAlbumsFromPaths(const CStdString &strPathIds);
  bool    UpdateOldVersion(float fVersion);

	int			m_iSongsBeforeCommit;
};

extern CMusicDatabase	g_musicDatabase;
