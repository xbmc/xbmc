/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIWindowMusicBase.h"

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
