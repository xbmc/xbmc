/*
 *      Copyright (C) 2005-2018 Team Kodi
 *      http://kodi.tv
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

#pragma once

#include "guilib/GUIDialog.h"
#include "music/Song.h"
#include "music/Artist.h"
#include "music/Album.h"
#include "FileItem.h"
#include "MediaSource.h"
#include "threads/Event.h"

class CFileItem;
class CFileItemList;

class CGUIDialogMusicInfo :
      public CGUIDialog
{
public:
  CGUIDialogMusicInfo(void);
  ~CGUIDialogMusicInfo(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction &action) override;
  bool SetItem(CFileItem* item);
  void SetAlbum(const CAlbum& album, const std::string &path);
  void SetArtist(const CArtist& artist, const std::string &path);
  bool HasUpdatedUserrating() const { return m_hasUpdatedUserrating; };
  bool HasRefreshed() const { return m_hasRefreshed; };

  bool HasListItems() const override { return true; };
  CFileItemPtr GetCurrentListItem(int offset = 0) override;
  std::string GetContent();
  static void AddItemPathToFileBrowserSources(VECSOURCES &sources, const CFileItem &item);
  void SetDiscography(CMusicDatabase& database) const;
  void SetSongs(const VECSONGS &songs) const;
  void SetArtTypeList(CFileItemList& artlist);
  void SetScrapedInfo(bool bScraped) { m_scraperAddInfo = bScraped;  }
  CArtist& GetArtist() { return m_artist; };
  CAlbum& GetAlbum() { return m_album; };
  bool IsArtistInfo() const { return m_bArtistInfo; };
  bool IsCancelled() const { return m_cancelled; };
  bool HasScrapedInfo() const { return m_scraperAddInfo; };
  void FetchComplete();
  void RefreshInfo();

  static void ShowForAlbum(int idAlbum);
  static void ShowForArtist(int idArtist);
  static void ShowFor(CFileItem* pItem);
protected:
  void OnInitWindow() override;
  void Update();
  void SetLabel(int iControl, const std::string& strLabel);
  void OnGetArt();
  void OnAlbumInfo(int id);
  void OnArtistInfo(int id);
  void OnSetUserrating() const;
  void SetUserrating(int userrating) const;

  CAlbum m_album;
  CArtist m_artist;
  int m_startUserrating;
  bool m_hasUpdatedUserrating;
  bool m_hasRefreshed;
  bool m_bArtistInfo;
  bool m_cancelled;
  bool m_scraperAddInfo;
  CFileItemList* m_albumSongs;
  CFileItemPtr m_item;
  CFileItemList m_artTypeList;
  CEvent m_event;
  std::string m_fallbackartpath;
};
