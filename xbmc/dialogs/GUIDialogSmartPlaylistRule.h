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

class CGUIDialogSmartPlaylistRule :
      public CGUIDialog
{
public:
  CGUIDialogSmartPlaylistRule(void);
  ~CGUIDialogSmartPlaylistRule(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnBack(int actionID) override;
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;

  static bool EditRule(KODI::PLAYLIST::CSmartPlaylistRule& rule, const std::string& type = "songs");

protected:
  void OnField();
  void OnOperator();
  void OnOK();
  void OnCancel();
  void UpdateButtons();
  void OnBrowse();
  std::vector<std::pair<std::string, int>> GetValidOperators(
      const KODI::PLAYLIST::CSmartPlaylistRule& rule);
  KODI::PLAYLIST::CSmartPlaylistRule m_rule;
  bool m_cancelled;
  std::string m_type;
};
