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
#include "BackgroundInfoLoader.h"

class CGUIWindowMusicPlayList : public CGUIWindowMusicBase
{
public:
  CGUIWindowMusicPlayList(void);
  ~CGUIWindowMusicPlayList(void) override;

  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction &action) override;
  bool OnBack(int actionID) override;

  void RemovePlayListItem(int iItem);
  void MoveItem(int iStart, int iDest);

protected:
  bool GoParentFolder() override { return false; };
  void UpdateButtons() override;
  void OnItemLoaded(CFileItem* pItem) override;
  bool Update(const std::string& strDirectory, bool updateFilterPath = true) override;
  void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
  bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;
  void OnMove(int iItem, int iAction);
  bool OnPlayMedia(int iItem, const std::string &player = "") override;

  void SavePlayList();
  void ClearPlayList();
  void MarkPlaying();

  bool MoveCurrentPlayListItem(int iItem, int iAction, bool bUpdate = true);

  int m_movingFrom;
  VECSOURCES m_shares;
};
