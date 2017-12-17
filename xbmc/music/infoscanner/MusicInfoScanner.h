#pragma once
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
#include "InfoScanner.h"
#include "MusicAlbumInfo.h"
#include "MusicInfoScraper.h"
#include "music/MusicDatabase.h"
#include "threads/Thread.h"

class CAlbum;
class CArtist;
class CGUIDialogProgressBarHandle;

namespace MUSIC_INFO
{

class CMusicInfoScanner : public IRunnable, public CInfoScanner
{
public:
  /*! \brief Flags for controlling the scanning process
   */
  enum SCAN_FLAGS { SCAN_NORMAL     = 0,
                    SCAN_ONLINE     = 1 << 0,
                    SCAN_BACKGROUND = 1 << 1,
                    SCAN_RESCAN     = 1 << 2,
                    SCAN_ARTISTS    = 1 << 3,
                    SCAN_ALBUMS     = 1 << 4 };

  CMusicInfoScanner();
  ~CMusicInfoScanner() override;

  void Start(const std::string& strDirectory, int flags);
  void FetchAlbumInfo(const std::string& strDirectory, bool refresh = false);
  void FetchArtistInfo(const std::string& strDirectory, bool refresh = false);
  void Stop();

  /*! \brief Categorize FileItems into Albums, Songs, and Artists
   This takes a list of FileItems and turns it into a tree of Albums,
   Artists, and Songs.
   Albums are defined uniquely by the album name and album artist.
   
   \param songs [in/out] list of songs to categorise - albumartist field may be altered.
   \param albums [out] albums found within these songs.
   */
  static void FileItemsToAlbums(CFileItemList& items, VECALBUMS& albums, MAPSONGS* songsMap = nullptr);

  /*! \brief Scrape additional album information and update the music database with it.
  Given an album, search for it using the given scraper.
  If info is found, update the database and artwork with the new
  information.
  \param album [in/out] the album to update
  \param scraper [in] the album scraper to use
  \param bAllowSelection [in] should we allow the user to manually override the info with a GUI if the album is not found?
  \param pDialog [in] a progress dialog which this and downstream functions can update with status, if required
  */
  INFO_RET UpdateAlbumInfo(CAlbum& album, const ADDON::ScraperPtr& scraper, bool bAllowSelection, CGUIDialogProgress* pDialog = nullptr);

  /*! \brief Scrape additional artist information and update the music database with it.
  Given an artist, search for it using the given scraper.
  If info is found, update the database and artwork with the new
  information.
  \param artist [in/out] the artist to update
  \param scraper [in] the artist scraper to use
  \param bAllowSelection [in] should we allow the user to manually override the info with a GUI if the album is not found?
  \param pDialog [in] a progress dialog which this and downstream functions can update with status, if required
  */
  INFO_RET UpdateArtistInfo(CArtist& artist, const ADDON::ScraperPtr& scraper, bool bAllowSelection, CGUIDialogProgress* pDialog = nullptr);

protected:
  virtual void Process();
  bool DoScan(const std::string& strDirectory) override;

  /*! \brief Find art for albums
   Based on the albums in the folder, finds whether we have unique album art
   and assigns to the album if we do.

   In order of priority:
    1. If there is a single album in the folder, then the folder art is assigned to the album.
    2. We find the art for each song. A .tbn file takes priority over embedded art.
    3. If we have a unique piece of art for all songs in the album, we assign that to the album
       and remove that art from each song so that they inherit from the album.
    4. If there is not a unique piece of art for each song, then no art is assigned
       to the album.

   \param albums [in/out] list of albums to categorise - art field may be altered.
   \param path [in] path containing albums.
   */
  static void FindArtForAlbums(VECALBUMS &albums, const std::string &path);

  /*! \brief Scrape additional album information and update the database.
   Search for the given album using the given scraper.
   If info is found, update the database and artwork with the new
   information.
   \param album [in/out] the album to update
   \param scraper [in] the album scraper to use
   \param bAllowSelection [in] should we allow the user to manually override the info with a GUI if the album is not found?
   \param pDialog [in] a progress dialog which this and downstream functions can update with status, if required
   */
  INFO_RET UpdateDatabaseAlbumInfo(CAlbum& album, const ADDON::ScraperPtr& scraper, bool bAllowSelection, CGUIDialogProgress* pDialog = nullptr);
 
  /*! \brief Scrape additional artist information and update the database.
   Search for the given artist using the given scraper.
   If info is found, update the database and artwork with the new
   information.
   \param artist [in/out] the artist to update
   \param scraper [in] the artist scraper to use
   \param bAllowSelection [in] should we allow the user to manually override the info with a GUI if the album is not found?
   \param pDialog [in] a progress dialog which this and downstream functions can update with status, if required
   */
  INFO_RET UpdateDatabaseArtistInfo(CArtist& artist, const ADDON::ScraperPtr& scraper, bool bAllowSelection, CGUIDialogProgress* pDialog = nullptr);

  /*! \brief Using the scrapers download metadata for an album
   Given a CAlbum style struct containing some data about an album, query
   the scrapers to try and get more information about the album. The responsibility
   is with the caller to do something with that information. It will be passed back
   in a MusicInfo struct, which you can save, display to the user or throw away.
   \param album [in] a partially or fully filled out album structure containing the search query
   \param scraper [in] the scraper to query, usually the default or the relevant scraper for the musicdb path
   \param albumInfo [in/out] a CMusicAlbumInfo struct which will be populated with the output of the scraper
   \param bUseScrapedMBID [in] should scraper use any previously scraped mbid to identify the artist, or use artist name?
   \param pDialog [in] a progress dialog which this and downstream functions can update with status, if required
   */
  INFO_RET DownloadAlbumInfo(const CAlbum& album, const ADDON::ScraperPtr& scraper, MUSIC_GRABBER::CMusicAlbumInfo& albumInfo, bool bUseScrapedMBID, CGUIDialogProgress* pDialog = nullptr);

  /*! \brief Using the scrapers download metadata for an artist
   Given a CAlbum style struct containing some data about an artist, query
   the scrapers to try and get more information about the artist. The responsibility
   is with the caller to do something with that information. It will be passed back
   in a MusicInfo struct, which you can save, display to the user or throw away.
   \param artist [in] a partially or fully filled out artist structure containing the search query
   \param scraper [in] the scraper to query, usually the default or the relevant scraper for the musicdb path
   \param artistInfo [in/out] a CMusicAlbumInfo struct which will be populated with the output of the scraper
   \param bUseScrapedMBID [in] should scraper use any previously scraped mbid to identify the album, or use album and artist name?
   \param pDialog [in] a progress dialog which this and downstream functions can update with status, if required
   */
  INFO_RET DownloadArtistInfo(const CArtist& artist, const ADDON::ScraperPtr& scraper, MUSIC_GRABBER::CMusicArtistInfo& artistInfo, bool bUseScrapedMBID, CGUIDialogProgress* pDialog = nullptr);

  /*! \brief Get art for an artist
   Checks for thumb and fanart in given folder, and in parent folders back up the artist path (if non-empty).
   If none is found there then it tries to use the first available thumb and fanart from those listed in the
   artist structure. Images found are cached.
   \param artist [in] an artist
   \param level [in] how many levels of folders to search in. 1 => just the folder
   \return set of art type and file location (URL or path) pairs
   */
  std::map<std::string, std::string> GetArtistArtwork(const CArtist& artist, unsigned int level = 3);

  /*! \brief Scan in the ID3/Ogg/FLAC tags for a bunch of FileItems
   Given a list of FileItems, scan in the tags for those FileItems
   and populate a new FileItemList with the files that were successfully scanned.
   Add album to library, populate a list of album ids added for possible scraping later.
   Any files which couldn't be scanned (no/bad tags) are discarded in the process.
   \param items [in] list of FileItems to scan
   \param scannedItems [in] list to populate with the scannedItems
   */
  int RetrieveMusicInfo(const std::string& strDirectory, CFileItemList& items);

  void ScrapeInfoAddedAlbums();
  void RetrieveArtistArt();

  /*! \brief Scan in the ID3/Ogg/FLAC tags for a bunch of FileItems
    Given a list of FileItems, scan in the tags for those FileItems
   and populate a new FileItemList with the files that were successfully scanned.
   Any files which couldn't be scanned (no/bad tags) are discarded in the process.
   \param items [in] list of FileItems to scan
   \param scannedItems [in] list to populate with the scannedItems
   */
  INFO_RET ScanTags(const CFileItemList& items, CFileItemList& scannedItems);
  int GetPathHash(const CFileItemList &items, std::string &hash);
  void GetAlbumArtwork(long id, const CAlbum &artist);

  void Run() override;
  int CountFiles(const CFileItemList& items, bool recursive);
  int CountFilesRecursively(const std::string& strPath);

  /*! \brief Resolve a MusicBrainzID to a URL
   If we have a MusicBrainz ID for an artist or album, 
   resolve it to an MB URL and set up the scrapers accordingly.
   
   \param preferredScraper [in] A ScraperPtr to the preferred album/artist scraper.
   \param musicBrainzURL [out] will be populated with the MB URL for the artist/album.
   */
  bool ResolveMusicBrainz(const std::string &strMusicBrainzID, const ADDON::ScraperPtr &preferredScraper, CScraperUrl &musicBrainzURL);

  void ScannerWait(unsigned int milliseconds);

  int m_currentItem;
  int m_itemCount;
  bool m_bStop;
  bool m_needsCleanup;
  int m_scanType; // 0 - load from files, 1 - albums, 2 - artists
  CMusicDatabase m_musicDatabase;

  std::vector<int> m_albumsAdded;
  std::set<int> m_artistsArt;

  std::set<std::string> m_seenPaths;
  int m_flags;
  CThread m_fileCountReader;
};
}
