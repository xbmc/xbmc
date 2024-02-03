/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIWindowVideoBase.h"

class CGUIWindowVideoPlaylist : public CGUIWindowVideoBase
{
public:
  CGUIWindowVideoPlaylist(void);
  ~CGUIWindowVideoPlaylist(void) override;

  void OnPrepareFileItems(CFileItemList& items) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction &action) override;
  bool OnBack(int actionID) override;
  bool OnSelect(int iItem) override;

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
