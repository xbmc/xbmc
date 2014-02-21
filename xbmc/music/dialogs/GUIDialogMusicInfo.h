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
  virtual ~CGUIDialogMusicInfo(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  void SetAlbum(const CAlbum& album, const std::string &path);
  void SetArtist(const CArtist& artist, const std::string &path);
  bool NeedRefresh() const;
  bool HasUpdatedThumb() const { return m_hasUpdatedThumb; };

  virtual bool HasListItems() const { return true; };
  virtual CFileItemPtr GetCurrentListItem(int offset = 0);
  const CFileItemList& CurrentDirectory() const { return *m_albumSongs; };
  static void AddItemPathToFileBrowserSources(VECSOURCES &sources, const CFileItem &item);
protected:
  virtual void OnInitWindow();
  void Update();
  void SetLabel(int iControl, const std::string& strLabel);
  void OnGetThumb();
  void OnGetFanart();
  void SetSongs(const VECSONGS &songs);
  void SetDiscography();
  void OnSearch(const CFileItem* pItem);

  CAlbum m_album;
  CArtist m_artist;
  bool m_bViewReview;
  bool m_bRefresh;
  bool m_hasUpdatedThumb;
  bool m_bArtistInfo;
  CFileItemPtr   m_albumItem;
  CFileItemList* m_albumSongs;
};
