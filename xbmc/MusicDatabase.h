/*!
 \file MusicDatabase.h
\brief
*/
#pragma once
#include "Database.h"

#include <set>

// return codes of Cleaning up the Database
// numbers are strings from strings.xml
#define ERROR_OK     317
#define ERROR_CANCEL    0
#define ERROR_DATABASE    315
#define ERROR_REORG_SONGS   319
#define ERROR_REORG_ARTIST   321
#define ERROR_REORG_GENRE   323
#define ERROR_REORG_PATH   325
#define ERROR_REORG_ALBUM   327
#define ERROR_WRITING_CHANGES  329
#define ERROR_COMPRESSING   332

#define NUM_SONGS_BEFORE_COMMIT 500

/*!
 \ingroup music
 \brief A set of CStdString objects, used for CMusicDatabase
 \sa ISETPATHES, CMusicDatabase
 */
typedef std::set<CStdString> SETPATHES;

/*!
 \ingroup music
 \brief The SETPATHES iterator
 \sa SETPATHES, CMusicDatabase
 */
typedef std::set<CStdString>::iterator ISETPATHES;

/*!
 \ingroup music
 \brief A vector of longs for iDs, used for CMusicDatabase's multiple artist/genre capability
 */
typedef std::vector<long> VECLONGS;

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
class CMusicDatabase : public CDatabase
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
  void EmptyCache();
  void Clean();
  int  Cleanup(CGUIDialogProgress *pDlgProgress);
  void DeleteAlbumInfo();
  bool LookupCDDBInfo(bool bRequery=false);
  void DeleteCDDBInfo();
  void AddSong(const CSong& song, bool bCheck = true);
  long AddAlbumInfo(const CAlbum& album, const VECSONGS& songs);
  long UpdateAlbumInfo(const CAlbum& album, const VECSONGS& songs);
  bool GetAlbumInfo(const CStdString& strAlbum, const CStdString& strPath, CAlbum& album, VECSONGS& songs);
  bool GetSong(const CStdString& strTitle, CSong& song);
  bool GetSongByFileName(const CStdString& strFileName, CSong& song);
  bool GetSongsByPath(const CStdString& strPath, VECSONGS& songs);
  bool GetSongsByPath(const CStdString& strPath, MAPSONGS& songs, bool bAppendToMap = false);
  bool GetSongsByAlbum(const CStdString& strAlbum, const CStdString& strPath, VECSONGS& songs);
  bool FindSongsByName(const CStdString& strSearch, VECSONGS& songs);
  bool FindSongsByNameAndArtist(const CStdString& strSearch, VECSONGS& songs);
  bool GetArtistsByName(const CStdString& strArtist, VECARTISTS& artists);
  bool GetGenresByName(const CStdString& strGenre1, VECGENRES& genres);
  bool GetAlbumsByName(const CStdString& strSearch1, VECALBUMS& albums);

  bool GetAlbumsByPath(const CStdString& strPath, VECALBUMS& albums);
  bool FindAlbumsByName(const CStdString& strSearch, VECALBUMS& albums);
  bool GetTop100(VECSONGS& songs);
  bool GetTop100Albums(VECALBUMS& albums);
  bool GetRecentAlbums(VECALBUMS& albums);
  bool IncrTop100CounterByFileName(const CStdString& strFileName1);
  bool GetSubpathsFromPath(const CStdString &strPath, CStdString& strPathIds);
  bool RemoveSongsFromPaths(const CStdString &strPathIds);
  bool CleanupAlbumsArtistsGenres(const CStdString &strPathIds);
  void CheckVariousArtistsAndCoverArt();
  bool GetGenresNav(VECGENRES& genres);
  bool GetArtistsNav(VECARTISTS& artists, const CStdString &strGenre1);
  bool GetAlbumsNav(VECALBUMS& albums, const CStdString &strGenre1, const CStdString &strArtist1);
  bool GetSongsNav(VECSONGS& songs, const CStdString &strGenre1, const CStdString &strArtist1, const CStdString &strAlbum1, const CStdString &strAlbumPath1);

protected:
  map<CStdString, CArtistCache> m_artistCache;
  map<CStdString, CGenreCache> m_genreCache;
  map<CStdString, CPathCache> m_pathCache;
  map<CStdString, CPathCache> m_thumbCache;
  map<CStdString, CAlbumCache> m_albumCache;
  virtual bool CreateTables();
  long AddAlbum(const CStdString& strAlbum, const long lArtistId, const int iNumArtists, const CStdString& strArtist, long lPathId, const CStdString& strPath);
  long AddGenre(const CStdString& strGenre);
  long AddArtist(const CStdString& strArtist);
  long AddPath(const CStdString& strPath);
  long AddThumb(const CStdString& strThumb1);
  void AddExtraArtists(const VECARTISTS& vecArtists, long lSongId, long lAlbumId, bool bCheck = true);
  void AddExtraGenres(const VECGENRES& vecGenres, long lSongId, long lAlbumId, bool bCheck = true);
  bool AddAlbumInfoSongs(long idAlbumInfo, const VECSONGS& songs);
  bool GetAlbumInfoSongs(long idAlbumInfo, VECSONGS& songs);
  bool UpdateAlbumInfoSongs(long idAlbumInfo, const VECSONGS& songs);

private:
  bool GetExtraArtistsForAlbum(long lAlbumId, CStdString &strArtist);
  bool GetExtraArtistsForSong(long lSongId, CStdString &strArtist);
  bool GetExtraGenresForAlbum(long lAlbumId, CStdString &strGenre);
  bool GetExtraGenresForSong(long lSongId, CStdString &strGenre);
  CSong GetSongFromDataset();
  CAlbum GetAlbumFromDataset();
  bool CleanupSongs();
  bool CleanupSongsByIds(const CStdString &strSongIds);
  bool CleanupPaths();
  bool CleanupThumbs();
  bool CleanupAlbums();
  bool CleanupArtists();
  bool CleanupGenres();
  bool CleanupAlbumsFromPaths(const CStdString &strPathIds);
  virtual bool UpdateOldVersion(float fVersion);

  int m_iSongsBeforeCommit;
};

extern CMusicDatabase g_musicDatabase;
