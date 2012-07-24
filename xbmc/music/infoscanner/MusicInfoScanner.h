#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "threads/Thread.h"
#include "music/MusicDatabase.h"
#include "MusicAlbumInfo.h"

class CAlbum;
class CArtist;

namespace MUSIC_INFO
{
enum SCAN_STATE { PREPARING = 0, REMOVING_OLD, CLEANING_UP_DATABASE, READING_MUSIC_INFO, DOWNLOADING_ALBUM_INFO, DOWNLOADING_ARTIST_INFO, COMPRESSING_DATABASE, WRITING_CHANGES };

class IMusicInfoScannerObserver
{
public:
  virtual ~IMusicInfoScannerObserver() {}
  virtual void OnStateChanged(SCAN_STATE state) = 0;
  virtual void OnDirectoryChanged(const CStdString& strDirectory) = 0;
  virtual void OnDirectoryScanned(const CStdString& strDirectory) = 0;
  virtual void OnSetProgress(int currentItem, int itemCount)=0;
  virtual void OnFinished() = 0;
};

/*! \brief return values from the information lookup functions
 */
enum INFO_RET 
{ 
  INFO_CANCELLED,
  INFO_ERROR,
  INFO_NOT_NEEDED,
  INFO_HAVE_ALREADY,
  INFO_NOT_FOUND,
  INFO_ADDED 
};

class CMusicInfoScanner : CThread, public IRunnable
{
public:
  /*! \brief Flags for controlling the scanning process
   */
  enum SCAN_FLAGS { SCAN_NORMAL     = 0,
                    SCAN_ONLINE     = 1 << 0,
                    SCAN_BACKGROUND = 1 << 1,
                    SCAN_RESCAN     = 1 << 2 };

  CMusicInfoScanner();
  virtual ~CMusicInfoScanner();

  void Start(const CStdString& strDirectory, int flags);
  void FetchAlbumInfo(const CStdString& strDirectory, bool refresh=false);
  void FetchArtistInfo(const CStdString& strDirectory, bool refresh=false);
  bool IsScanning();
  void Stop();
  void SetObserver(IMusicInfoScannerObserver* pObserver);

  /*! \brief Categorise songs into albums
   For users that don't have correct tags, we need to provide some hints
   to the DB about the correct album artist and compilation settings.

   In this case, albums are defined uniquely by the album name and album artist.

   If albumartist is not available in a song, we determine it from the
   common portion of each song's artist list.

   eg the common artist for
     Bob Dylan / Tom Petty / Roy Orbison
     Bob Dylan / Tom Petty
   would be "Bob Dylan / Tom Petty".

   If all songs that share an album
    1. have a non-empty album name
    2. have at least two different primary artists
    3. have no album artist set
    4. and no track numbers overlap
   we assume it is a various artists album.

   \param items [in] list of file items (songs) to categorise.
   \param albumHints [out] list of albums with our guess at artist and compilation flags.
   */
  static void CategoriseAlbums(const CFileItemList& items, VECALBUMS& albumHints);

  INFO_RET DownloadAlbumInfo(const CAlbum& album, MUSIC_GRABBER::CMusicAlbumInfo& albumInfo, CGUIDialogProgress* pDialog = NULL);
  INFO_RET DownloadArtistInfo(const CArtist& artist, MUSIC_GRABBER::CMusicArtistInfo& artistInfo, CGUIDialogProgress* pDialog = NULL);

  std::map<std::string, std::string> GetArtistArtwork(long id, const CArtist *artist = NULL);
protected:
  virtual void Process();
  int RetrieveMusicInfo(CFileItemList& items, const CStdString& strDirectory);
  INFO_RET ScanTags(const CFileItemList& items, CFileItemList& scannedItems);
  int GetPathHash(const CFileItemList &items, CStdString &hash);
  void GetAlbumArtwork(long id, const CAlbum &artist);

  bool DoScan(const CStdString& strDirectory);

  virtual void Run();
  int CountFiles(const CFileItemList& items, bool recursive);
  int CountFilesRecursively(const CStdString& strPath);

protected:
  IMusicInfoScannerObserver* m_pObserver;
  int m_currentItem;
  int m_itemCount;
  bool m_bRunning;
  bool m_bCanInterrupt;
  bool m_needsCleanup;
  int m_scanType; // 0 - load from files, 1 - albums, 2 - artists
  CMusicDatabase m_musicDatabase;

  std::set<CStdString> m_pathsToScan;
  std::set<CAlbum> m_albumsToScan;
  std::set<CArtist> m_artistsToScan;
  std::set<CStdString> m_pathsToCount;
  std::vector<long> m_artistsScanned;
  std::vector<long> m_albumsScanned;
  int m_flags;
};
}

