/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "MediaSource.h"
#include "guilib/GUIDialog.h"
#include "music/Album.h"
#include "music/Artist.h"
#include "music/Song.h"
#include "threads/Event.h"

#include <memory>

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
  bool HasUpdatedUserrating() const { return m_hasUpdatedUserrating; }
  bool HasRefreshed() const { return m_hasRefreshed; }

  bool HasListItems() const override { return true; }
  CFileItemPtr GetCurrentListItem(int offset = 0) override;
  std::string GetContent();
  static void AddItemPathToFileBrowserSources(VECSOURCES &sources, const CFileItem &item);
  void SetDiscography(CMusicDatabase& database) const;
  void SetSongs(const VECSONGS &songs) const;
  void SetArtTypeList(CFileItemList& artlist);
  void SetScrapedInfo(bool bScraped) { m_scraperAddInfo = bScraped;  }
  CArtist& GetArtist() { return m_artist; }
  CAlbum& GetAlbum() { return m_album; }
  bool IsArtistInfo() const { return m_bArtistInfo; }
  bool IsCancelled() const { return m_cancelled; }
  bool HasScrapedInfo() const { return m_scraperAddInfo; }
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
  void OnPlayItem(const std::shared_ptr<CFileItem>& item);

  CAlbum m_album;
  CArtist m_artist;
  int m_startUserrating = -1;
  bool m_hasUpdatedUserrating = false;
  bool m_hasRefreshed = false;
  bool m_bArtistInfo = false;
  bool m_cancelled = false;
  bool m_scraperAddInfo = false;
  std::unique_ptr<CFileItemList> m_albumSongs;
  std::shared_ptr<CFileItem> m_item;
  std::unique_ptr<CFileItemList> m_artTypeList;
  CEvent m_event;
  std::string m_fallbackartpath;
};
