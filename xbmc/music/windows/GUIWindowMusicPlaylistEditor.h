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

#include "GUIWindowMusicBase.h"

class CFileItemList;

class CGUIWindowMusicPlaylistEditor : public CGUIWindowMusicBase, public IBackgroundLoaderObserver
{
public:
  CGUIWindowMusicPlaylistEditor(void);
  virtual ~CGUIWindowMusicPlaylistEditor(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnBack(int actionID);

protected:
  virtual void OnItemLoaded(CFileItem* pItem) {};
  virtual bool GetDirectory(const std::string &strDirectory, CFileItemList &items);
  virtual void UpdateButtons();
  virtual bool Update(const std::string &strDirectory, bool updateFilterPath = true);
  virtual void OnPrepareFileItems(CFileItemList &items);
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  virtual void OnQueueItem(int iItem);
  virtual std::string GetStartFolder(const std::string &dir) { return ""; };

  void OnPlaylistContext();
  int GetCurrentPlaylistItem();
  void OnDeletePlaylistItem(int item);
  void UpdatePlaylist();
  void ClearPlaylist();
  void OnSavePlaylist();
  void OnLoadPlaylist();
  void AppendToPlaylist(CFileItemList &newItems);
  void OnMovePlaylistItem(int item, int direction);

  void LoadPlaylist(const std::string &playlist);

  // new method
  virtual void PlayItem(int iItem);

  void DeleteRemoveableMediaDirectoryCache();

  CMusicThumbLoader m_thumbLoader;
  CMusicThumbLoader m_playlistThumbLoader;

  CFileItemList* m_playlist;
  std::string m_strLoadedPlaylist;
};
