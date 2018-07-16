/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "InfoScanner.h"
#include "MusicAlbumInfo.h"
#include "MusicInfoScraper.h"
#include "music/MusicDatabase.h"
#include "threads/Thread.h"
#include "threads/IRunnable.h"

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
  static void FileItemsToAlbums(CFileItemList& items, VECALBUMS& albums, MAPSONGS* songsMap = NULL);

  /*! \brief Scrape additional album information and update the music database with it.
  Given an album, search for it using the given scraper.
  If info is found, update the database and artwork with the new
  information.
  \param album [in/out] the album to update
  \param scraper [in] the album scraper to use
  \param bAllowSelection [in] should we allow the user to manually override the info with a GUI if the album is not found?
  \param pDialog [in] a progress dialog which this and downstream functions can update with status, if required
  */
  INFO_RET UpdateAlbumInfo(CAlbum& album, const ADDON::ScraperPtr& scraper, bool bAllowSelection, CGUIDialogProgress* pDialog = NULL);

  /*! \brief Scrape additional artist information and update the music database with it.
  Given an artist, search for it using the given scraper.
  If info is found, update the database and artwork with the new
  information.
  \param artist [in/out] the artist to update
  \param scraper [in] the artist scraper to use
  \param bAllowSelection [in] should we allow the user to manually override the info with a GUI if the album is not found?
  \param pDialog [in] a progress dialog which this and downstream functions can update with status, if required
  */
  INFO_RET UpdateArtistInfo(CArtist& artist, const ADDON::ScraperPtr& scraper, bool bAllowSelection, CGUIDialogProgress* pDialog = NULL);

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
  INFO_RET UpdateDatabaseAlbumInfo(CAlbum& album, const ADDON::ScraperPtr& scraper, bool bAllowSelection, CGUIDialogProgress* pDialog = NULL);

  /*! \brief Scrape additional artist information and update the database.
   Search for the given artist using the given scraper.
   If info is found, update the database and artwork with the new
   information.
   \param artist [in/out] the artist to update
   \param scraper [in] the artist scraper to use
   \param bAllowSelection [in] should we allow the user to manually override the info with a GUI if the album is not found?
   \param pDialog [in] a progress dialog which this and downstream functions can update with status, if required
   */
  INFO_RET UpdateDatabaseArtistInfo(CArtist& artist, const ADDON::ScraperPtr& scraper, bool bAllowSelection, CGUIDialogProgress* pDialog = NULL);

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
  INFO_RET DownloadAlbumInfo(const CAlbum& album, const ADDON::ScraperPtr& scraper, MUSIC_GRABBER::CMusicAlbumInfo& albumInfo, bool bUseScrapedMBID, CGUIDialogProgress* pDialog = NULL);

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
  INFO_RET DownloadArtistInfo(const CArtist& artist, const ADDON::ScraperPtr& scraper, MUSIC_GRABBER::CMusicArtistInfo& artistInfo, bool bUseScrapedMBID, CGUIDialogProgress* pDialog = NULL);


  /*! \brief Get the types of art for an artist or album that are to be
  automatically fetched from local files during scanning
  \param mediaType [in] artist or album
  \return vector of art types that are to be fetched during scanning
  */
  std::vector<std::string> GetArtTypesToScan(const MediaType& mediaType);

  /*! \brief Get the types of art for an artist or album that can be
   automatically found during scanning, and are not in the provided set of art
   \param mediaType [in] artist or album
   \param art [in] set of art type and file location (URL or path) pairs
   \return vector of art types that are missing from the set
   */
  std::vector<std::string> GetMissingArtTypes(const MediaType& mediaType, const std::map<std::string, std::string>& art);

  /*! \brief Set art for an artist
  Checks for the missing types of art in the given folder. If none is found
  there then it tries to use the first available art of that type from those
  listed in the artist structure. Art found is saved in the artist structure
  and written to the music database. The images found are cached.
  \param artist [in/out] an artist, the art is set
  \param missing [in] vector of art types that are missing
  \param artfolder [in] path of the location to search for local art files
  \return true when art is added
  */
  bool SetArtistArtwork(CArtist& artist, const std::vector<std::string>& missing, const std::string& artfolder);

  /*! \brief Set art for an album
  Checks for the missing types of art in the given folder. If none is found
  there then it tries to use the first available art of that type from those
  listed in the album structure. Art found is saved in the album structure
  and written to the music database. The images found are cached.
  \param artist [in/out] an album, the art is set
  \param missing [in] vector of art types that are missing
  \param artfolder [in] path of the location to search for local art files
  \return true when art is added
  */
  bool SetAlbumArtwork(CAlbum& album, std::vector<std::string>& missing, const std::string& artfolder);

  /*! \brief Set art for an album with local art from disc set subfolders
  When we have a true disc set - subfolders beneath the album folder AND the
  music files in each sub folder tagged with same unique - this checks for the
  all types of art in the given subfolders.
  Art found is saved in the album structure and written to the music database.
  The images found are cached.
  \param artist [in/out] an album, the art is modified
  \param paths [in] a set of pairs of disc subfolder path and disc number
  */
  void SetDiscSetArtwork(CAlbum& album, const std::vector<std::pair<std::string, int>>& paths);

  /*! \brief Scan in the ID3/Ogg/FLAC tags for a bunch of FileItems
   Given a list of FileItems, scan in the tags for those FileItems
   and populate a new FileItemList with the files that were successfully scanned.
   Add album to library, populate a list of album ids added for possible scraping later.
   Any files which couldn't be scanned (no/bad tags) are discarded in the process.
   \param items [in] list of FileItems to scan
   \param scannedItems [in] list to populate with the scannedItems
   */
  int RetrieveMusicInfo(const std::string& strDirectory, CFileItemList& items);

  void RetrieveLocalArt();
  void ScrapeInfoAddedAlbums();

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
  bool m_needsCleanup = false;
  int m_scanType = 0; // 0 - load from files, 1 - albums, 2 - artists
  int m_idSourcePath;
  CMusicDatabase m_musicDatabase;

  std::set<int> m_albumsAdded;

  std::set<std::string> m_seenPaths;
  int m_flags;
  CThread m_fileCountReader;
};
}
