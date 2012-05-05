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
#include "utils/SortUtils.h"

class CArtist;
class CFileItem;

namespace dbiplus
{
  class field_value;
  typedef std::vector<field_value> sql_record;
}

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
  friend class DatabaseUtils;

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
  int UpdateSong(const CSong& song, int idSong = -1);
  int SetAlbumInfo(int idAlbum, const CAlbum& album, const VECSONGS& songs, bool bTransaction=true);
  bool DeleteAlbumInfo(int idArtist);
  int SetArtistInfo(int idArtist, const CArtist& artist);
  bool DeleteArtistInfo(int idArtist);
  bool GetAlbumInfo(int idAlbum, CAlbum &info, VECSONGS* songs);
  bool HasAlbumInfo(int idAlbum);
  bool GetArtistInfo(int idArtist, CArtist &info, bool needAll=true);
  bool GetSongByFileName(const CStdString& strFileName, CSong& song, int startOffset = 0);
  int GetAlbumIdByPath(const CStdString& path);
  bool GetSongById(int idSong, CSong& song);
  bool GetSongByKaraokeNumber( int number, CSong& song );
  bool SetKaraokeSongDelay( int idSong, int delay );
  bool GetSongsByPath(const CStdString& strPath, CSongMap& songs, bool bAppendToMap = false);
  bool Search(const CStdString& search, CFileItemList &items);

  bool GetAlbumFromSong(int idSong, CAlbum &album);
  bool GetAlbumFromSong(const CSong &song, CAlbum &album);
  
  bool GetAlbumsByArtist(int idArtist, bool includeFeatured, std::vector<long>& albums);
  bool GetArtistsByAlbum(int idAlbum, bool includeFeatured, std::vector<long>& artists);
  bool GetSongsByArtist(int idArtist, bool includeFeatured, std::vector<long>& songs);
  bool GetArtistsBySong(int idSong, bool includeFeatured, std::vector<long>& artists);

  bool GetArbitraryQuery(const CStdString& strQuery, const CStdString& strOpenRecordSet, const CStdString& strCloseRecordSet,
                         const CStdString& strOpenRecord, const CStdString& strCloseRecord, const CStdString& strOpenField, const CStdString& strCloseField, CStdString& strResult);
  bool ArbitraryExec(const CStdString& strExec);

  bool GetTop100(const CStdString& strBaseDir, CFileItemList& items);
  bool GetTop100Albums(VECALBUMS& albums);
  bool GetTop100AlbumSongs(const CStdString& strBaseDir, CFileItemList& item);
  bool GetRecentlyAddedAlbums(VECALBUMS& albums, unsigned int limit=0);
  bool GetRecentlyAddedAlbumSongs(const CStdString& strBaseDir, CFileItemList& item, unsigned int limit=0);
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
  bool GetAlbumsNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre, int idArtist, int start, int end, const SortDescription &sortDescription = SortDescription());
  bool GetAlbumsByYear(const CStdString &strBaseDir, CFileItemList& items, int year);
  bool GetSongsNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre, int idArtist,int idAlbum, const SortDescription &sortDescription = SortDescription());
  bool GetSongsByYear(const CStdString& baseDir, CFileItemList& items, int year);
  bool GetSongsByWhere(const CStdString &baseDir, const CStdString &whereClause, CFileItemList& items, const SortDescription &sortDescription = SortDescription());
  bool GetAlbumsByWhere(const CStdString &baseDir, const CStdString &where, const CStdString &order, CFileItemList &items, const SortDescription &sortDescription = SortDescription());
  bool GetArtistsByWhere(const CStdString& strBaseDir, const CStdString &where, CFileItemList& items);
  bool GetRandomSong(CFileItem* item, int& idSong, const CStdString& strWhere);
  int GetKaraokeSongsCount();
  int GetSongsCount(const CStdString& strWhere = "");
  unsigned int GetSongIDs(const CStdString& strWhere, std::vector<std::pair<int,int> > &songIDs);

  bool GetAlbumPath(int idAlbum, CStdString &path);
  bool SaveAlbumThumb(int idAlbum, const CStdString &thumb);
  bool GetAlbumThumb(int idAlbum, CStdString &thumb);
  bool GetArtistPath(int idArtist, CStdString &path);

  CStdString GetGenreById(int id);
  CStdString GetArtistById(int id);
  CStdString GetAlbumById(int id);

  int GetArtistByName(const CStdString& strArtist);
  int GetAlbumByName(const CStdString& strAlbum, const CStdString& strArtist="");
  int GetAlbumByName(const CStdString& strAlbum, const std::vector<std::string>& artist);
  int GetGenreByName(const CStdString& strGenre);
  int GetSongByArtistAndAlbumAndTitle(const CStdString& strArtist, const CStdString& strAlbum, const CStdString& strTitle);

  bool GetCompilationAlbums(const CStdString& strBaseDir, CFileItemList& items);
  bool GetCompilationSongs(const CStdString& strBaseDir, CFileItemList& items);
  int  GetCompilationAlbumsCount();
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

  /*! \brief Sets art for a database item.
   Sets a single piece of art for a database item.
   \param mediaId the id in the media (song/artist/album) table.
   \param mediaType the type of media, which corresponds to the table the item resides in (song/artist/album).
   \param artType the type of art to set, e.g. "thumb", "fanart"
   \param url the url to the art (this is the original url, not a cached url).
   \sa GetArtForItem
   */
  void SetArtForItem(int mediaId, const std::string &mediaType, const std::string &artType, const std::string &url);

  /*! \brief Sets art for a database item.
   Sets multiple pieces of art for a database item.
   \param mediaId the id in the media (song/artist/album) table.
   \param mediaType the type of media, which corresponds to the table the item resides in (song/artist/album).
   \param art a map of <type, url> where type is "thumb", "fanart", etc. and url is the original url of the art.
   \sa GetArtForItem
   */
  void SetArtForItem(int mediaId, const std::string &mediaType, const std::map<std::string, std::string> &art);

  /*! \brief Fetch art for a database item.
   Fetches multiple pieces of art for a database item.
   \param mediaId the id in the media (song/artist/album) table.
   \param mediaType the type of media, which corresponds to the table the item resides in (song/artist/album).
   \param art [out] a map of <type, url> where type is "thumb", "fanart", etc. and url is the original url of the art.
   \return true if art is retrieved, false if no art is found.
   \sa SetArtForItem
   */
  bool GetArtForItem(int mediaId, const std::string &mediaType, std::map<std::string, std::string> &art);

  /*! \brief Fetch art for a database item.
   Fetches a single piece of art for a database item.
   \param mediaId the id in the media (song/artist/album) table.
   \param mediaType the type of media, which corresponds to the table the item resides in (song/artist/album).
   \param artType the type of art to retrieve, eg "thumb", "fanart".
   \return the original URL to the piece of art, if available.
   \sa SetArtForItem
   */
  std::string GetArtForItem(int mediaId, const std::string &mediaType, const std::string &artType);

  /*! \brief Fetch artist art for a song or album item.
   Fetches the art associated with the primary artist for the song or album.
   \param mediaId the id in the media (song/album) table.
   \param mediaType the type of media, which corresponds to the table the item resides in (song/album).
   \param art [out] the art map <type, url> of artist art.
   \return true if artist art is found, false otherwise.
   \sa GetArtForItem
   */
  bool GetArtistArtForItem(int mediaId, const std::string &mediaType, std::map<std::string, std::string> &art);

  /*! \brief Fetch artist art for a song or album item.
   Fetches a single piece of art associated with the primary artist for the song or album.
   \param mediaId the id in the media (song/album) table.
   \param mediaType the type of media, which corresponds to the table the item resides in (song/album).
   \param artType the type of art to retrieve, eg "thumb", "fanart".
   \return the original URL to the piece of art, if available.
   \sa GetArtForItem
   */
  std::string GetArtistArtForItem(int mediaId, const std::string &mediaType, const std::string &artType);

protected:
  std::map<CStdString, int /*CArtistCache*/> m_artistCache;
  std::map<CStdString, int /*CGenreCache*/> m_genreCache;
  std::map<CStdString, int /*CPathCache*/> m_pathCache;
  std::map<CStdString, int /*CPathCache*/> m_thumbCache;
  std::map<CStdString, CAlbumCache> m_albumCache;

  virtual bool CreateTables();
  virtual int GetMinVersion() const { return 26; };
  const char *GetBaseDBName() const { return "MyMusic"; };

  int AddAlbum(const CStdString& strAlbum1, const CStdString &strArtist1, int idThumb, const CStdString& strGenre, int year, bool bCompilation);
  int AddGenre(const CStdString& strGenre);
  int AddArtist(const CStdString& strArtist);
  int AddPath(const CStdString& strPath);
  int AddThumb(const CStdString& strThumb1);
  bool AddAlbumArtist(int idArtist, int idAlbum, bool featured, int iOrder);
  bool AddSongArtist(int idArtist, int idSong, bool featured, int iOrder);
  bool AddSongGenre(int idGenre, int idSong, int iOrder);
  bool AddAlbumGenre(int idGenre, int idAlbum, int iOrder);

  void AddKaraokeData(const CSong& song);
  bool SetAlbumInfoSongs(int idAlbumInfo, const VECSONGS& songs);
  bool GetAlbumInfoSongs(int idAlbumInfo, VECSONGS& songs);
private:
  /*! \brief (Re)Create the generic database views for songs and albums
   */
  virtual void CreateViews();

  void SplitString(const CStdString &multiString, std::vector<std::string> &vecStrings, CStdString &extraStrings);
  CSong GetSongFromDataset(bool bWithMusicDbPath=false);
  CArtist GetArtistFromDataset(dbiplus::Dataset* pDS, bool needThumb=true);
  CAlbum GetAlbumFromDataset(dbiplus::Dataset* pDS, bool imageURL=false);
  CAlbum GetAlbumFromDataset(const dbiplus::sql_record* const record, bool imageURL=false);
  void GetFileItemFromDataset(CFileItem* item, const CStdString& strMusicDBbasePath);
  void GetFileItemFromDataset(const dbiplus::sql_record* const record, CFileItem* item, const CStdString& strMusicDBbasePath);
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
    song_strArtists,
    song_strGenres,
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
    song_strThumb,
    song_iKarNumber,
    song_iKarDelay,
    song_strKarEncoding,
    song_bCompilation
  } SongFields;

  // Fields should be ordered as they
  // appear in the albumview
  enum _AlbumFields
  {
    album_idAlbum=0,
    album_strAlbum,
    album_strArtists,
    album_strGenres,
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
    album_iRating,
    album_bCompilation
  } AlbumFields;

  enum _ArtistFields
  {
    artist_idArtist=0,
    artist_strArtist,
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
