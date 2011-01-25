/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
/*!
 \file MusicDatabase.h
\brief
*/
#pragma once
#include "dbwrappers/Database.h"
#include "Album.h"
#include "addons/Scraper.h"

class CArtist;
class CFileItem;

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

class CGUIDialogProgress;
class CFileItemList;

/*!
 \ingroup music
 \brief Class to store and read tag information

 CMusicDatabase can be used to read and store
 tag information for faster access. It is based on
 sqlite (http://www.sqlite.org).

 Here is the database layout:
  \image html musicdatabase.png

 \sa CAlbum, CSong, VECSONGS, CMapSong, VECARTISTS, VECALBUMS, VECGENRES
 */
class CMusicDatabase : public CDatabase
{
  class CArtistCache
  {
  public:
    int idArtist;
    CStdString strArtist;
  };

  class CPathCache
  {
  public:
    int idPath;
    CStdString strPath;
  };

  class CGenreCache
  {
  public:
    int idGenre;
    CStdString strGenre;
  };

  class CAlbumCache : public CAlbum
  {
  public:
    int idAlbum;
    int idArtist;
  };

public:
  CMusicDatabase(void);
  virtual ~CMusicDatabase(void);

  virtual bool Open();
  virtual bool CommitTransaction();
  void EmptyCache();
  void Clean();
  int  Cleanup(CGUIDialogProgress *pDlgProgress=NULL);
  void DeleteAlbumInfo();
  bool LookupCDDBInfo(bool bRequery=false);
  void DeleteCDDBInfo();
  void AddSong(CSong& song, bool bCheck = true);
  int SetAlbumInfo(int idAlbum, const CAlbum& album, const VECSONGS& songs, bool bTransaction=true);
  bool DeleteAlbumInfo(int idArtist);
  int SetArtistInfo(int idArtist, const CArtist& artist);
  bool DeleteArtistInfo(int idArtist);
  bool GetAlbumInfo(int idAlbum, CAlbum &info, VECSONGS* songs);
  bool HasAlbumInfo(int idAlbum);
  bool GetArtistInfo(int idArtist, CArtist &info, bool needAll=true);
  bool GetSongByFileName(const CStdString& strFileName, CSong& song);
  int GetAlbumIdByPath(const CStdString& path);
  bool GetSongById(int idSong, CSong& song);
  bool GetSongByKaraokeNumber( int number, CSong& song );
  bool SetKaraokeSongDelay( int idSong, int delay );
  bool GetSongsByPath(const CStdString& strPath, CSongMap& songs, bool bAppendToMap = false);
  bool Search(const CStdString& search, CFileItemList &items);

  bool GetAlbumFromSong(int idSong, CAlbum &album);
  bool GetAlbumFromSong(const CSong &song, CAlbum &album);

  bool GetArbitraryQuery(const CStdString& strQuery, const CStdString& strOpenRecordSet, const CStdString& strCloseRecordSet,
                         const CStdString& strOpenRecord, const CStdString& strCloseRecord, const CStdString& strOpenField, const CStdString& strCloseField, CStdString& strResult);
  bool ArbitraryExec(const CStdString& strExec);

  bool GetTop100(const CStdString& strBaseDir, CFileItemList& items);
  bool GetTop100Albums(VECALBUMS& albums);
  bool GetTop100AlbumSongs(const CStdString& strBaseDir, CFileItemList& item);
  bool GetRecentlyAddedAlbums(VECALBUMS& albums);
  bool GetRecentlyAddedAlbumSongs(const CStdString& strBaseDir, CFileItemList& item);
  bool GetRecentlyPlayedAlbums(VECALBUMS& albums);
  bool GetRecentlyPlayedAlbumSongs(const CStdString& strBaseDir, CFileItemList& item);
  bool IncrTop100CounterByFileName(const CStdString& strFileName1);
  bool RemoveSongsFromPath(const CStdString &path, CSongMap &songs, bool exact=true);
  bool CleanupOrphanedItems();
  bool GetPaths(std::set<CStdString> &paths);
  bool SetPathHash(const CStdString &path, const CStdString &hash);
  bool GetPathHash(const CStdString &path, CStdString &hash);
  bool GetGenresNav(const CStdString& strBaseDir, CFileItemList& items);
  bool GetYearsNav(const CStdString& strBaseDir, CFileItemList& items);
  bool GetArtistsNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre, bool albumArtistsOnly);
  bool GetAlbumsNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre, int idArtist, int start, int end);
  bool GetAlbumsByYear(const CStdString &strBaseDir, CFileItemList& items, int year);
  bool GetSongsNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre, int idArtist,int idAlbum);
  bool GetSongsByYear(const CStdString& baseDir, CFileItemList& items, int year);
  bool GetSongsByWhere(const CStdString &baseDir, const CStdString &whereClause, CFileItemList& items);
  bool GetAlbumsByWhere(const CStdString &baseDir, const CStdString &where, const CStdString &order, CFileItemList &items);
  bool GetRandomSong(CFileItem* item, int& idSong, const CStdString& strWhere);
  int GetKaraokeSongsCount();
  int GetSongsCount(const CStdString& strWhere = "");
  unsigned int GetSongIDs(const CStdString& strWhere, std::vector<std::pair<int,int> > &songIDs);

  bool GetAlbumPath(int idAlbum, CStdString &path);
  bool SaveAlbumThumb(int idAlbum, const CStdString &thumb);
  bool GetAlbumThumb(int idAlbum, CStdString &thumb);
  bool GetArtistPath(int idArtist, CStdString &path);

  bool GetGenreById(int idGenre, CStdString& strGenre);
  bool GetArtistById(int idArtist, CStdString& strArtist);
  bool GetAlbumById(int idAlbum, CStdString& strAlbum);

  int GetArtistByName(const CStdString& strArtist);
  int GetAlbumByName(const CStdString& strAlbum, const CStdString& strArtist="");
  int GetGenreByName(const CStdString& strGenre);
  int GetSongByArtistAndAlbumAndTitle(const CStdString& strArtist, const CStdString& strAlbum, const CStdString& strTitle);

  bool GetVariousArtistsAlbums(const CStdString& strBaseDir, CFileItemList& items);
  bool GetVariousArtistsAlbumsSongs(const CStdString& strBaseDir, CFileItemList& items);
  int GetVariousArtistsAlbumsCount();

  bool SetSongRating(const CStdString &filePath, char rating);
  bool SetScraperForPath(const CStdString& strPath, const ADDON::ScraperPtr& info);
  bool GetScraperForPath(const CStdString& strPath, ADDON::ScraperPtr& info, const ADDON::TYPE &type);

  /*! \brief Check whether a given scraper is in use.
   \param scraperID the scraper to check for.
   \return true if the scraper is in use, false otherwise.
   */
  bool ScraperInUse(const CStdString &scraperID) const;

  void ExportToXML(const CStdString &xmlFile, bool singleFiles = false, bool images=false, bool overwrite=false);
  void ImportFromXML(const CStdString &xmlFile);

  void ExportKaraokeInfo(const CStdString &outFile, bool asHTML );
  void ImportKaraokeInfo(const CStdString &inputFile );

  void SetPropertiesForFileItem(CFileItem& item);
  static void SetPropertiesFromArtist(CFileItem& item, const CArtist& artist);
  static void SetPropertiesFromAlbum(CFileItem& item, const CAlbum& album);
protected:
  std::map<CStdString, int /*CArtistCache*/> m_artistCache;
  std::map<CStdString, int /*CGenreCache*/> m_genreCache;
  std::map<CStdString, int /*CPathCache*/> m_pathCache;
  std::map<CStdString, int /*CPathCache*/> m_thumbCache;
  std::map<CStdString, CAlbumCache> m_albumCache;

  virtual bool CreateTables();
  virtual int GetMinVersion() const { return 15; };
  const char *GetDefaultDBName() const { return "MyMusic7"; };

  int AddAlbum(const CStdString& strAlbum1, int idArtist, const CStdString &extraArtists, const CStdString &strArtist1, int idThumb, int idGenre, const CStdString &extraGenres, int year);
  int AddGenre(const CStdString& strGenre);
  int AddArtist(const CStdString& strArtist);
  int AddPath(const CStdString& strPath);
  int AddThumb(const CStdString& strThumb1);
  void AddExtraAlbumArtists(const CStdStringArray& vecArtists, int idAlbum);
  void AddExtraSongArtists(const CStdStringArray& vecArtists, int idSong, bool bCheck = true);
  void AddKaraokeData(const CSong& song);
  void AddExtraGenres(const CStdStringArray& vecGenres, int idSong, int idAlbum, bool bCheck = true);
  bool SetAlbumInfoSongs(int idAlbumInfo, const VECSONGS& songs);
  bool GetAlbumInfoSongs(int idAlbumInfo, VECSONGS& songs);
private:
  void SplitString(const CStdString &multiString, std::vector<CStdString> &vecStrings, CStdString &extraStrings);
  CSong GetSongFromDataset(bool bWithMusicDbPath=false);
  CArtist GetArtistFromDataset(dbiplus::Dataset* pDS, bool needThumb=true);
  CAlbum GetAlbumFromDataset(dbiplus::Dataset* pDS, bool imageURL=false);
  void GetFileItemFromDataset(CFileItem* item, const CStdString& strMusicDBbasePath);
  bool CleanupSongs();
  bool CleanupSongsByIds(const CStdString &strSongIds);
  bool CleanupPaths();
  bool CleanupThumbs();
  bool CleanupAlbums();
  bool CleanupArtists();
  bool CleanupGenres();
  virtual bool UpdateOldVersion(int version);
  bool SearchArtists(const CStdString& search, CFileItemList &artists);
  bool SearchAlbums(const CStdString& search, CFileItemList &albums);
  bool SearchSongs(const CStdString& strSearch, CFileItemList &songs);
  int GetSongIDFromPath(const CStdString &filePath);

  // Fields should be ordered as they
  // appear in the songview
  enum _SongFields
  {
    song_idSong=0,
    song_strExtraArtists,
    song_strExtraGenres,
    song_strTitle,
    song_iTrack,
    song_iDuration,
    song_iYear,
    song_dwFileNameCRC,
    song_strFileName,
    song_strMusicBrainzTrackID,
    song_strMusicBrainzArtistID,
    song_strMusicBrainzAlbumID,
    song_strMusicBrainzAlbumArtistID,
    song_strMusicBrainzTRMID,
    song_iTimesPlayed,
    song_iStartOffset,
    song_iEndOffset,
    song_lastplayed,
    song_rating,
    song_comment,
    song_idAlbum,
    song_strAlbum,
    song_strPath,
    song_idArtist,
    song_strArtist,
    song_idGenre,
    song_strGenre,
    song_strThumb,
    song_iKarNumber,
    song_iKarDelay,
    song_strKarEncoding
  } SongFields;

  // Fields should be ordered as they
  // appear in the albumview
  enum _AlbumFields
  {
    album_idAlbum=0,
    album_strAlbum,
    album_strExtraArtists,
    album_idArtist,
    album_strExtraGenres,
    album_idGenre,
    album_strArtist,
    album_strGenre,
    album_iYear,
    album_strThumb,
    album_idAlbumInfo,
    album_strMoods,
    album_strStyles,
    album_strThemes,
    album_strReview,
    album_strLabel,
    album_strType,
    album_strThumbURL,
    album_iRating
  } AlbumFields;

  enum _ArtistFields
  {
    artist_idArtist=1, // not a typo - we have the primary key @ 0
    artist_strBorn,
    artist_strFormed,
    artist_strGenres,
    artist_strMoods,
    artist_strStyles,
    artist_strInstruments,
    artist_strBiography,
    artist_strDied,
    artist_strDisbanded,
    artist_strYearsActive,
    artist_strImage,
    artist_strFanart
  } ArtistFields;

  void AnnounceRemove(std::string content, int id);
  void AnnounceUpdate(std::string content, int id);
};
