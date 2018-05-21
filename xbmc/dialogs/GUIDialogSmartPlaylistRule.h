#pragma once

/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

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

  static bool EditRule(CSmartPlaylistRule &rule, const std::string& type="songs");

protected:
  void OnField();
  void OnOperator();
  void OnOK();
  void OnCancel();
  void UpdateButtons();
  void OnBrowse();
  std::vector< std::pair<std::string, int> > GetValidOperators(const CSmartPlaylistRule& rule);
  CSmartPlaylistRule m_rule;
  bool m_cancelled;
  std::string m_type;
};
