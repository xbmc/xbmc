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

class CGUIWindowMusicPlaylistEditor : public CGUIWindowMusicBase
{
public:
  CGUIWindowMusicPlaylistEditor(void);
  ~CGUIWindowMusicPlaylistEditor(void) override;

  bool OnMessage(CGUIMessage& message) override;
  bool OnBack(int actionID) override;

protected:
  bool GetDirectory(const std::string &strDirectory, CFileItemList &items) override;
  void UpdateButtons() override;
  bool Update(const std::string &strDirectory, bool updateFilterPath = true) override;
  void OnPrepareFileItems(CFileItemList &items) override;
  void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
  bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;
  void OnQueueItem(int iItem) override;
  std::string GetStartFolder(const std::string &dir) override { return ""; };

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
  void PlayItem(int iItem) override;

  void DeleteRemoveableMediaDirectoryCache();

  CMusicThumbLoader m_playlistThumbLoader;

  CFileItemList* m_playlist;
  std::string m_strLoadedPlaylist;
};
