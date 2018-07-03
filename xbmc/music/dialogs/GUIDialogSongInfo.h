/*
 *      Copyright (C) 2005-2018 Team XBMC
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

#include "guilib/GUIDialog.h"
#include "FileItem.h"
#include "threads/Event.h"

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
  bool HasUpdatedUserrating() const { return m_hasUpdatedUserrating; };

  bool HasListItems() const override { return true; };
  CFileItemPtr GetCurrentListItem(int offset = 0) override;
  std::string GetContent();
  //const CFileItemList& CurrentDirectory() const { return m_artTypeList; };
  bool IsCancelled() const { return m_cancelled; };
  void FetchComplete();

  static void ShowFor(CFileItem* pItem);
protected:
  void OnInitWindow() override;
  void Update();
  void OnGetArt();
  void SetUserrating(int userrating);
  void OnSetUserrating();

  CFileItemPtr m_song;
  CFileItemList m_artTypeList;
  CEvent m_event;
  int m_startUserrating;
  bool m_cancelled;
  bool m_hasUpdatedUserrating;
  long m_albumId = -1;

};
