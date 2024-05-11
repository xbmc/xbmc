/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"
#include "playlists/SmartPlayList.h"

class CFileItemList;

class CGUIDialogSmartPlaylistEditor :
      public CGUIDialog
{
public:
  enum PLAYLIST_TYPE { TYPE_SONGS = 1, TYPE_ALBUMS, TYPE_ARTISTS, TYPE_MIXED, TYPE_MUSICVIDEOS, TYPE_MOVIES, TYPE_TVSHOWS, TYPE_EPISODES };

  CGUIDialogSmartPlaylistEditor(void);
  ~CGUIDialogSmartPlaylistEditor(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnBack(int actionID) override;
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;

  static bool EditPlaylist(const std::string &path, const std::string &type = "");
  static bool NewPlaylist(const std::string &type);

protected:
  void OnRuleList(int item);
  void OnRuleAdd();
  void OnRuleRemove(int item);
  void OnMatch();
  void OnLimit();
  void OnName();
  void OnType();
  void OnOrder();
  void OnOrderDirection();
  void OnGroupBy();
  void OnGroupMixed();
  void OnOK();
  void OnCancel();
  void OnPopupMenu(int item);
  void UpdateButtons();
  void UpdateRuleControlButtons();
  int GetSelectedItem();
  void HighlightItem(int item);
  std::vector<PLAYLIST_TYPE> GetAllowedTypes(const std::string& mode);
  PLAYLIST_TYPE ConvertType(const std::string &type);
  std::string ConvertType(PLAYLIST_TYPE type);
  std::string GetLocalizedType(PLAYLIST_TYPE type);

  KODI::PLAYLIST::CSmartPlaylist m_playlist;

  // our list of rules for display purposes
  CFileItemList* m_ruleLabels;

  std::string m_path;
  bool m_cancelled;
  std::string m_mode;  // mode we're in (partymode etc.)
};
