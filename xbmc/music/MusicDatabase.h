/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
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
#include "MusicDbUrl.h"

class CArtist;
class CFileItem;

namespace dbiplus
{
  class field_value;
  typedef std::vector<field_value> sql_record;
}

#include <set>
#include <string>

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
 \brief A set of std::string objects, used for CMusicDatabase
 \sa ISETPATHES, CMusicDatabase
 */
typedef std::set<std::string> SETPATHES;

/*!
 \ingroup music
 \brief The SETPATHES iterator
 \sa SETPATHES, CMusicDatabase
 */
typedef std::set<std::string>::iterator ISETPATHES;

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
  friend class TestDatabaseUtilsHelper;

public:
  CMusicDatabase(void);
  virtual ~CMusicDatabase(void);

  virtual bool Open();
  virtual bool CommitTransaction();
  void EmptyCache();
  void Clean();
  int  Cleanup(bool bShowProgress=true);
  bool LookupCDDBInfo(bool bRequery=false);
  void DeleteCDDBInfo();

  /////////////////////////////////////////////////
  // Song CRUD
  /////////////////////////////////////////////////
  /*! \brief Add a song to the database
   \param idAlbum [in] the database ID of the album for the song
   \param strTitle [in] the title of the song (required to be non-empty)
   \param strMusicBrainzTrackID [in] the MusicBrainz track ID of the song
   \param strPathAndFileName [in] the path and filename to the song
   \param strComment [in] the ids of the added songs
   \param strThumb [in] the ids of the added songs
   \param artistString [in] the assembled artist string, denormalized from CONCAT(strArtist||strJoinPhrase)
   \param genres [in] a vector of genres to which this song belongs
   \param iTrack [in] the track number and disc number of the song
   \param iDuration [in] the duration of the song
   \param iYear [in] the year of the song
   \param iTimesPlayed [in] the number of times the song has been played
   \param iStartOffset [in] the start offset of the song (when using a single audio file with a .cue)
   \param iEndOffset [in] the end offset of the song (when using a single audio file with .cue)
   \param dtLastPlayed [in] the time the song was last played
   \param rating [in] a rating for the song
   \param iKaraokeNumber [in] the karaoke id of the song
   \return the id of the song
   */
  int AddSong(const int idAlbum,
              const std::string& strTitle,
              const std::string& strMusicBrainzTrackID,
              const std::string& strPathAndFileName,
              const std::string& strComment,
              const std::string& strThumb,
              const std::string &artistString, const std::vector<std::string>& genres,
              int iTrack, int iDuration, int iYear,
              const int iTimesPlayed, int iStartOffset, int iEndOffset,
              const CDateTime& dtLastPlayed,
              char rating, int iKaraokeNumber);
  bool GetSong(int idSong, CSong& song);

  /*! \brief Update a song in the database.

   NOTE: This function assumes that song.artist contains the artist string to be concatenated.
         Most internal functions should instead use the long-form function as the artist string
         should be constructed from the artist credits.
         This function will eventually be demised.

   \param idSong  the database ID of the song to update
   \param song the song
   \return the id of the song.
   */
  int UpdateSong(int idSong, const CSong &song);

  /*! \brief Update a song in the database
   \param idSong [in] the database ID of the song to update
   \param strTitle [in] the title of the song (required to be non-empty)
   \param strMusicBrainzTrackID [in] the MusicBrainz track ID of the song
   \param strPathAndFileName [in] the path and filename to the song
   \param strComment [in] the ids of the added songs
   \param strThumb [in] the ids of the added songs
   \param artistString [in] the full artist string, denormalized from CONCAT(song_artist.strArtist || song_artist.strJoinPhrase)
   \param genres [in] a vector of genres to which this song belongs
   \param iTrack [in] the track number and disc number of the song
   \param iDuration [in] the duration of the song
   \param iYear [in] the year of the song
   \param iTimesPlayed [in] the number of times the song has been played
   \param iStartOffset [in] the start offset of the song (when using a single audio file with a .cue)
   \param iEndOffset [in] the end offset of the song (when using a single audio file with .cue)
   \param dtLastPlayed [in] the time the song was last played
   \param rating [in] a rating for the song
   \param iKaraokeNumber [in] the karaoke id of the song
   \return the id of the song
   */
  int UpdateSong(int idSong,
                 const std::string& strTitle, const std::string& strMusicBrainzTrackID,
                 const std::string& strPathAndFileName, const std::string& strComment,
                 const std::string& strThumb,
                 const std::string& artistString, const std::vector<std::string>& genres,
                 int iTrack, int iDuration, int iYear,
                 int iTimesPlayed, int iStartOffset, int iEndOffset,
                 const CDateTime& dtLastPlayed,
                 char rating, int iKaraokeNumber);

  //// Misc Song
  bool GetSongByFileName(const std::string& strFileName, CSong& song, int startOffset = 0);
  bool GetSongsByPath(const std::string& strPath, MAPSONGS& songs, bool bAppendToMap = false);
  bool Search(const std::string& search, CFileItemList &items);
  bool RemoveSongsFromPath(const std::string &path, MAPSONGS& songs, bool exact=true);
  bool SetSongRating(const std::string &filePath, char rating);
  int  GetSongByArtistAndAlbumAndTitle(const std::string& strArtist, const std::string& strAlbum, const std::string& strTitle);

  /////////////////////////////////////////////////
  // Album
  /////////////////////////////////////////////////
  bool AddAlbum(CAlbum& album);
  /*! \brief Update an album and all its nested entities (artists, songs, infoSongs, etc)
   \param album the album to update
   \return true or false
   */
  bool UpdateAlbum(CAlbum& album);

  /*! \brief Add an album and all its songs to the database
   \param album the album to add
   \param songIDs [out] the ids of the added songs
   \return the id of the album
   */
  int  AddAlbum(const std::string& strAlbum, const std::string& strMusicBrainzAlbumID,
                const std::string& strArtist, const std::string& strGenre,
                int year, bool bCompilation);
  /*! \brief retrieve an album, optionally with all songs.
   \param idAlbum the database id of the album.
   \param album [out] the album to fill.
   \param getSongs whether or not to retrieve songs, defaults to true.
   \return true if the album is retrieved, false otherwise.
   */
  bool GetAlbum(int idAlbum, CAlbum& album, bool getSongs = true);
  int  UpdateAlbum(int idAlbum, const CAlbum &album);
  int  UpdateAlbum(int idAlbum,
                   const std::string& strAlbum, const std::string& strMusicBrainzAlbumID,
                   const std::string& strArtist, const std::string& strGenre,
                   const std::string& strMoods, const std::string& strStyles,
                   const std::string& strThemes, const std::string& strReview,
                   const std::string& strImage, const std::string& strLabel,
                   const std::string& strType,
                   int iRating, int iYear, bool bCompilation);
  bool ClearAlbumLastScrapedTime(int idAlbum);
  bool HasAlbumBeenScraped(int idAlbum);
  int  AddAlbumInfoSong(int idAlbum, const CSong& song);

  /*! \brief Checks if the given path is inside a folder that has already been scanned into the library
   \param path the path we want to check
   */
  bool InsideScannedPath(const std::string& path);

  //// Misc Album
  int  GetAlbumIdByPath(const std::string& path);
  bool GetAlbumFromSong(int idSong, CAlbum &album);
  int  GetAlbumByName(const std::string& strAlbum, const std::string& strArtist="");
  int  GetAlbumByName(const std::string& strAlbum, const std::vector<std::string>& artist);
  std::string GetAlbumById(int id);

  /////////////////////////////////////////////////
  // Artist CRUD
  /////////////////////////////////////////////////
  bool UpdateArtist(const CArtist& artist);

  int  AddArtist(const std::string& strArtist, const std::string& strMusicBrainzArtistID);
  bool GetArtist(int idArtist, CArtist& artist, bool fetchAll = true);
  int  UpdateArtist(int idArtist,
                    const std::string& strArtist, const std::string& strMusicBrainzArtistID,
                    const std::string& strBorn, const std::string& strFormed,
                    const std::string& strGenres, const std::string& strMoods,
                    const std::string& strStyles, const std::string& strInstruments,
                    const std::string& strBiography, const std::string& strDied,
                    const std::string& strDisbanded, const std::string& strYearsActive,
                    const std::string& strImage, const std::string& strFanart);
  bool HasArtistBeenScraped(int idArtist);
  bool ClearArtistLastScrapedTime(int idArtist);
  int  AddArtistDiscography(int idArtist, const std::string& strAlbum, const std::string& strYear);
  bool DeleteArtistDiscography(int idArtist);

  std::string GetArtistById(int id);
  int GetArtistByName(const std::string& strArtist);

  /////////////////////////////////////////////////
  // Paths
  /////////////////////////////////////////////////
  int AddPath(const std::string& strPath);

  bool GetPaths(std::set<std::string> &paths);
  bool SetPathHash(const std::string &path, const std::string &hash);
  bool GetPathHash(const std::string &path, std::string &hash);
  bool GetAlbumPath(int idAlbum, std::string &path);
  bool GetArtistPath(int idArtist, std::string &path);

  /////////////////////////////////////////////////
  // Genres
  /////////////////////////////////////////////////
  int AddGenre(const std::string& strGenre);
  std::string GetGenreById(int id);
  int GetGenreByName(const std::string& strGenre);

  /////////////////////////////////////////////////
  // Link tables
  /////////////////////////////////////////////////
  bool AddAlbumArtist(int idArtist, int idAlbum, std::string strArtist, std::string joinPhrase, bool featured, int iOrder);
  bool GetAlbumsByArtist(int idArtist, bool includeFeatured, std::vector<int>& albums);
  bool GetArtistsByAlbum(int idAlbum, bool includeFeatured, std::vector<int>& artists);
  bool DeleteAlbumArtistsByAlbum(int idAlbum);

  bool AddSongArtist(int idArtist, int idSong, std::string strArtist, std::string joinPhrase, bool featured, int iOrder);
  bool GetSongsByArtist(int idArtist, bool includeFeatured, std::vector<int>& songs);
  bool GetArtistsBySong(int idSong, bool includeFeatured, std::vector<int>& artists);
  bool DeleteSongArtistsBySong(int idSong);

  bool AddSongGenre(int idGenre, int idSong, int iOrder);
  bool GetGenresBySong(int idSong, std::vector<int>& genres);
  bool DeleteSongGenresBySong(int idSong);

  bool AddAlbumGenre(int idGenre, int idAlbum, int iOrder);
  bool GetGenresByAlbum(int idAlbum, std::vector<int>& genres);
  bool DeleteAlbumGenresByAlbum(int idAlbum);

  /////////////////////////////////////////////////
  // Top 100
  /////////////////////////////////////////////////
  bool GetTop100(const std::string& strBaseDir, CFileItemList& items);
  bool GetTop100Albums(VECALBUMS& albums);
  bool GetTop100AlbumSongs(const std::string& strBaseDir, CFileItemList& item);

  /////////////////////////////////////////////////
  // Recently added
  /////////////////////////////////////////////////
  bool GetRecentlyAddedAlbums(VECALBUMS& albums, unsigned int limit=0);
  bool GetRecentlyAddedAlbumSongs(const std::string& strBaseDir, CFileItemList& item, unsigned int limit=0);
  bool GetRecentlyPlayedAlbums(VECALBUMS& albums);
  bool GetRecentlyPlayedAlbumSongs(const std::string& strBaseDir, CFileItemList& item);

  /////////////////////////////////////////////////
  // Compilations
  /////////////////////////////////////////////////
  bool GetCompilationAlbums(const std::string& strBaseDir, CFileItemList& items);
  bool GetCompilationSongs(const std::string& strBaseDir, CFileItemList& items);
  int  GetCompilationAlbumsCount();
  bool GetVariousArtistsAlbums(const std::string& strBaseDir, CFileItemList& items);
  bool GetVariousArtistsAlbumsSongs(const std::string& strBaseDir, CFileItemList& items);
  int GetVariousArtistsAlbumsCount();
  
  /*! \brief Increment the playcount of an item
   Increments the playcount and updates the last played date
   \param item CFileItem to increment the playcount for
   */
  void IncrementPlayCount(const CFileItem &item);
  bool CleanupOrphanedItems();

  /////////////////////////////////////////////////
  // VIEWS
  /////////////////////////////////////////////////
  bool GetGenresNav(const std::string& strBaseDir, CFileItemList& items, const Filter &filter = Filter(), bool countOnly = false);
  bool GetYearsNav(const std::string& strBaseDir, CFileItemList& items, const Filter &filter = Filter());
  bool GetArtistsNav(const std::string& strBaseDir, CFileItemList& items, bool albumArtistsOnly = false, int idGenre = -1, int idAlbum = -1, int idSong = -1, const Filter &filter = Filter(), const SortDescription &sortDescription = SortDescription(), bool countOnly = false);
  bool GetCommonNav(const std::string &strBaseDir, const std::string &table, const std::string &labelField, CFileItemList &items, const Filter &filter /* = Filter() */, bool countOnly /* = false */);
  bool GetAlbumTypesNav(const std::string &strBaseDir, CFileItemList &items, const Filter &filter = Filter(), bool countOnly = false);
  bool GetMusicLabelsNav(const std::string &strBaseDir, CFileItemList &items, const Filter &filter = Filter(), bool countOnly = false);
  bool GetAlbumsNav(const std::string& strBaseDir, CFileItemList& items, int idGenre = -1, int idArtist = -1, const Filter &filter = Filter(), const SortDescription &sortDescription = SortDescription(), bool countOnly = false);
  bool GetAlbumsByYear(const std::string &strBaseDir, CFileItemList& items, int year);
  bool GetSongsNav(const std::string& strBaseDir, CFileItemList& items, int idGenre, int idArtist,int idAlbum, const SortDescription &sortDescription = SortDescription());
  bool GetSongsByYear(const std::string& baseDir, CFileItemList& items, int year);
  bool GetSongsByWhere(const std::string &baseDir, const Filter &filter, CFileItemList& items, const SortDescription &sortDescription = SortDescription());
  bool GetAlbumsByWhere(const std::string &baseDir, const Filter &filter, CFileItemList &items, const SortDescription &sortDescription = SortDescription(), bool countOnly = false);
  bool GetArtistsByWhere(const std::string& strBaseDir, const Filter &filter, CFileItemList& items, const SortDescription &sortDescription = SortDescription(), bool countOnly = false);
  bool GetRandomSong(CFileItem* item, int& idSong, const Filter &filter);
  int GetSongsCount(const Filter &filter = Filter());
  unsigned int GetSongIDs(const Filter &filter, std::vector<std::pair<int,int> > &songIDs);
  virtual bool GetFilter(CDbUrl &musicUrl, Filter &filter, SortDescription &sorting);

  /////////////////////////////////////////////////
  // Scraper
  /////////////////////////////////////////////////
  bool SetScraperForPath(const std::string& strPath, const ADDON::ScraperPtr& info);
  bool GetScraperForPath(const std::string& strPath, ADDON::ScraperPtr& info, const ADDON::TYPE &type);
  
  /*! \brief Check whether a given scraper is in use.
   \param scraperID the scraper to check for.
   \return true if the scraper is in use, false otherwise.
   */
  bool ScraperInUse(const std::string &scraperID) const;

  /////////////////////////////////////////////////
  // Karaoke
  /////////////////////////////////////////////////
  void AddKaraokeData(int idSong, int iKaraokeNumber);
  bool GetSongByKaraokeNumber( int number, CSong& song );
  bool SetKaraokeSongDelay( int idSong, int delay );
  int GetKaraokeSongsCount();
  void ExportKaraokeInfo(const std::string &outFile, bool asHTML );
  void ImportKaraokeInfo(const std::string &inputFile );

  /////////////////////////////////////////////////
  // Filters
  /////////////////////////////////////////////////
  bool GetItems(const std::string &strBaseDir, CFileItemList &items, const Filter &filter = Filter(), const SortDescription &sortDescription = SortDescription());
  bool GetItems(const std::string &strBaseDir, const std::string &itemType, CFileItemList &items, const Filter &filter = Filter(), const SortDescription &sortDescription = SortDescription());
  std::string GetItemById(const std::string &itemType, int id);

  /////////////////////////////////////////////////
  // XML
  /////////////////////////////////////////////////
  void ExportToXML(const std::string &xmlFile, bool singleFiles = false, bool images=false, bool overwrite=false);
  void ImportFromXML(const std::string &xmlFile);

  /////////////////////////////////////////////////
  // Properties
  /////////////////////////////////////////////////
  void SetPropertiesForFileItem(CFileItem& item);
  static void SetPropertiesFromArtist(CFileItem& item, const CArtist& artist);
  static void SetPropertiesFromAlbum(CFileItem& item, const CAlbum& album);

  /////////////////////////////////////////////////
  // Art
  /////////////////////////////////////////////////
  bool SaveAlbumThumb(int idAlbum, const std::string &thumb);
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
  std::map<std::string, int> m_artistCache;
  std::map<std::string, int> m_genreCache;
  std::map<std::string, int> m_pathCache;
  std::map<std::string, int> m_thumbCache;
  std::map<std::string, CAlbum> m_albumCache;

  virtual void CreateTables();
  virtual void CreateAnalytics();
  virtual int GetMinSchemaVersion() const { return 18; }
  virtual int GetSchemaVersion() const;

  const char *GetBaseDBName() const { return "MyMusic"; };


private:
  /*! \brief (Re)Create the generic database views for songs and albums
   */
  virtual void CreateViews();

  CSong GetSongFromDataset();
  CSong GetSongFromDataset(const dbiplus::sql_record* const record, int offset = 0);
  CArtist GetArtistFromDataset(dbiplus::Dataset* pDS, int offset = 0, bool needThumb = true);
  CArtist GetArtistFromDataset(const dbiplus::sql_record* const record, int offset = 0, bool needThumb = true);
  CAlbum GetAlbumFromDataset(dbiplus::Dataset* pDS, int offset = 0, bool imageURL = false);
  CAlbum GetAlbumFromDataset(const dbiplus::sql_record* const record, int offset = 0, bool imageURL = false);
  CArtistCredit GetArtistCreditFromDataset(const dbiplus::sql_record* const record, int offset = 0);
  void GetFileItemFromDataset(CFileItem* item, const CMusicDbUrl &baseUrl);
  void GetFileItemFromDataset(const dbiplus::sql_record* const record, CFileItem* item, const CMusicDbUrl &baseUrl);
  CSong GetAlbumInfoSongFromDataset(const dbiplus::sql_record* const record, int offset = 0);
  bool CleanupSongs();
  bool CleanupSongsByIds(const std::string &strSongIds);
  bool CleanupPaths();
  bool CleanupAlbums();
  bool CleanupArtists();
  bool CleanupGenres();
  virtual void UpdateTables(int version);
  bool SearchArtists(const std::string& search, CFileItemList &artists);
  bool SearchAlbums(const std::string& search, CFileItemList &albums);
  bool SearchSongs(const std::string& strSearch, CFileItemList &songs);
  int GetSongIDFromPath(const std::string &filePath);

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
    song_strFileName,
    song_strMusicBrainzTrackID,
    song_iTimesPlayed,
    song_iStartOffset,
    song_iEndOffset,
    song_lastplayed,
    song_rating,
    song_comment,
    song_idAlbum,
    song_strAlbum,
    song_strPath,
    song_iKarNumber,
    song_iKarDelay,
    song_strKarEncoding,
    song_bCompilation,
    song_strAlbumArtists,
    song_enumCount // end of the enum, do not add past here
  } SongFields;

  // Fields should be ordered as they
  // appear in the albumview
  enum _AlbumFields
  {
    album_idAlbum=0,
    album_strAlbum,
    album_strMusicBrainzAlbumID,
    album_strArtists,
    album_strGenres,
    album_iYear,
    album_strMoods,
    album_strStyles,
    album_strThemes,
    album_strReview,
    album_strLabel,
    album_strType,
    album_strThumbURL,
    album_iRating,
    album_bCompilation,
    album_iTimesPlayed,
    album_enumCount // end of the enum, do not add past here
  } AlbumFields;

  enum _ArtistCreditFields
  {
    // used for GetAlbum to get the cascaded album/song artist credits
    artistCredit_idEntity = 0,  // can be idSong or idAlbum depending on context
    artistCredit_idArtist,
    artistCredit_strArtist,
    artistCredit_strMusicBrainzArtistID,
    artistCredit_bFeatured,
    artistCredit_strJoinPhrase,
    artistCredit_iOrder,
    artistCredit_enumCount
  } ArtistCreditFields;

  enum _ArtistFields
  {
    artist_idArtist=0,
    artist_strArtist,
    artist_strMusicBrainzArtistID,
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
    artist_strFanart,
    artist_enumCount // end of the enum, do not add past here
  } ArtistFields;

  enum _AlbumInfoSongFields
  {
    albumInfoSong_idAlbumInfoSong=0,
    albumInfoSong_idAlbumInfo,
    albumInfoSong_iTrack,
    albumInfoSong_strTitle,
    albumInfoSong_iDuration,
    albumInfoSong_enumCount // end of the enum, do not add past here
  } AlbumInfoSongFields;
};
