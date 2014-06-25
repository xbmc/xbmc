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
#include "playlists/SmartPlayList.h"

class CGUIDialogSmartPlaylistRule :
      public CGUIDialog
{
public:
  CGUIDialogSmartPlaylistRule(void);
  virtual ~CGUIDialogSmartPlaylistRule(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnBack(int actionID);
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

  static bool EditRule(CSmartPlaylistRule &rule, const std::string& type="songs");

protected:
  void OnField();
  void OnOperator();
  void OnOK();
  void OnCancel();
  void UpdateButtons();
  void OnBrowse();

  CSmartPlaylistRule m_rule;
  bool m_cancelled;
  std::string m_type;
};
