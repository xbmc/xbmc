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

#include "guilib/GUIDialog.h"
#include "music/Song.h"
#include "music/Artist.h"
#include "music/Album.h"
#include "FileItem.h"

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
  void SetAlbum(const CAlbum& album, const std::string &path);
  void SetArtist(const CArtist& artist, const std::string &path);
  bool NeedRefresh() const { return m_bRefresh; };
  bool NeedsUpdate() const { return m_needsUpdate; };
  bool HasUpdatedThumb() const { return m_hasUpdatedThumb; };

  bool HasListItems() const override { return true; };
  CFileItemPtr GetCurrentListItem(int offset = 0) override;
  const CFileItemList& CurrentDirectory() const { return *m_albumSongs; };
  static void AddItemPathToFileBrowserSources(VECSOURCES &sources, const CFileItem &item);

  static void ShowFor(CFileItem item);
protected:
  void OnInitWindow() override;
  void Update();
  void SetLabel(int iControl, const std::string& strLabel);
  void OnGetThumb();
  void OnGetFanart();
  void SetSongs(const VECSONGS &songs) const;
  void SetDiscography() const;
  void OnSearch(const CFileItem* pItem);
  void OnSetUserrating() const;
  void SetUserrating(int userrating) const;

  CAlbum m_album;
  CArtist m_artist;
  int m_startUserrating;
  bool m_bViewReview;
  bool m_bRefresh;
  bool m_needsUpdate;
  bool m_hasUpdatedThumb;
  bool m_bArtistInfo;
  CFileItemPtr   m_albumItem;
  CFileItemList* m_albumSongs;
};
