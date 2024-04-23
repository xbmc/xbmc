/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItem.h"
#include "FileItemList.h"
#include "guilib/GUIDialog.h"
#include "threads/Event.h"

#include <memory>

class CGUIDialogSongInfo :
      public CGUIDialog
{
public:
  CGUIDialogSongInfo(void);
  ~CGUIDialogSongInfo(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool SetSong(CFileItem* item);
  void SetArtTypeList(CFileItemList& artlist);
  bool OnAction(const CAction& action) override;
  bool OnBack(int actionID) override;
  bool HasUpdatedUserrating() const { return m_hasUpdatedUserrating; }

  bool HasListItems() const override { return true; }
  CFileItemPtr GetCurrentListItem(int offset = 0) override;
  std::string GetContent();
  //const CFileItemList& CurrentDirectory() const { return m_artTypeList; }
  bool IsCancelled() const { return m_cancelled; }
  void FetchComplete();

  static void ShowFor(CFileItem* pItem);
protected:
  void OnInitWindow() override;
  void Update();
  void OnGetArt();
  void SetUserrating(int userrating);
  void OnSetUserrating();
  void OnPlaySong(const std::shared_ptr<CFileItem>& item);

  CFileItemPtr m_song;
  CFileItemList m_artTypeList;
  CEvent m_event;
  int m_startUserrating;
  bool m_cancelled;
  bool m_hasUpdatedUserrating;
  long m_albumId = -1;

};
