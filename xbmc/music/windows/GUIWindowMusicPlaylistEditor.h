/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIWindowMusicBase.h"

class CFileItemList;

class CGUIWindowMusicPlaylistEditor : public CGUIWindowMusicBase
{
public:
  CGUIWindowMusicPlaylistEditor(void);
  ~CGUIWindowMusicPlaylistEditor(void) override;

  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction &action) override;
  bool OnClick(int iItem, const std::string &player = "") override;
  bool OnBack(int actionID) override;

protected:
  bool GetDirectory(const std::string &strDirectory, CFileItemList &items) override;
  void UpdateButtons() override;
  bool Update(const std::string &strDirectory, bool updateFilterPath = true) override;
  void OnPrepareFileItems(CFileItemList& items) override;
  void OnQueueItem(int iItem, bool first = false) override;
  std::string GetStartFolder(const std::string& dir) override { return ""; }

  void OnSourcesContext();
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
