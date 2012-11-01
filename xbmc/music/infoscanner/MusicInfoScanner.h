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
class CGUIDialogProgressBarHandle;

namespace MUSIC_INFO
{
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

  //! \brief Set whether or not to show a progress dialog
  void ShowDialog(bool show) { m_showDialog = show; }

  /*! \brief Categorise songs into albums
   Albums are defined uniquely by the album name and album artist.

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
   we assume it is a various artists album, and set the albumartist field accordingly.

   \param songs [in/out] list of songs to categorise - albumartist field may be altered.
   \param albums [out] albums found within these songs.
   */
  static void CategoriseAlbums(VECSONGS &songs, VECALBUMS &albums);

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
  static void FindArtForAlbums(VECALBUMS &albums, const CStdString &path);

  bool DownloadAlbumInfo(const CStdString& strPath, const CStdString& strArtist, const CStdString& strAlbum, bool& bCanceled, MUSIC_GRABBER::CMusicAlbumInfo& album, CGUIDialogProgress* pDialog=NULL);
  bool DownloadArtistInfo(const CStdString& strPath, const CStdString& strArtist, bool& bCanceled, CGUIDialogProgress* pDialog=NULL);

  std::map<std::string, std::string> GetArtistArtwork(long id, const CArtist *artist = NULL);
protected:
  virtual void Process();
  int RetrieveMusicInfo(CFileItemList& items, const CStdString& strDirectory);
  int GetPathHash(const CFileItemList &items, CStdString &hash);
  void GetAlbumArtwork(long id, const CAlbum &artist);

  bool DoScan(const CStdString& strDirectory);

  virtual void Run();
  int CountFiles(const CFileItemList& items, bool recursive);
  int CountFilesRecursively(const CStdString& strPath);

protected:
  bool m_showDialog;
  CGUIDialogProgressBarHandle* m_handle;
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
