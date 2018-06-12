/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include "GUIWindowVideoBase.h"

class CGUIWindowVideoPlaylist : public CGUIWindowVideoBase
{
public:
  CGUIWindowVideoPlaylist(void);
  ~CGUIWindowVideoPlaylist(void) override;

  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction &action) override;
  bool OnBack(int actionID) override;

protected:
  bool OnPlayMedia(int iItem, const std::string &player = "") override;
  void UpdateButtons() override;
  void MarkPlaying();

  void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
  bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;

  void OnMove(int iItem, int iAction);

  void ClearPlayList();
  void RemovePlayListItem(int iItem);
  bool MoveCurrentPlayListItem(int iItem, int iAction, bool bUpdate = true);
  void MoveItem(int iStart, int iDest);

  void SavePlayList();

  int m_movingFrom;
  VECSOURCES m_shares;
};
