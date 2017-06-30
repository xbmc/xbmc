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

class CFileItem;

class CGUIDialogSongInfo :
      public CGUIDialog
{
public:
  CGUIDialogSongInfo(void);
  ~CGUIDialogSongInfo(void) override;
  bool OnMessage(CGUIMessage& message) override;
  void SetSong(CFileItem *item);
  bool OnAction(const CAction &action) override;
  bool OnBack(int actionID) override;
  bool NeedsUpdate() const { return m_needsUpdate; };

  bool HasListItems() const override { return true; };
  CFileItemPtr GetCurrentListItem(int offset = 0) override;
protected:
  void OnInitWindow() override;
  void Update();
  bool DownloadThumbnail(const std::string &thumbFile);
  void OnGetThumb();
  void SetUserrating(int userrating);
  void OnSetUserrating();

  CFileItemPtr m_song;
  int m_startUserrating;
  bool m_cancelled;
  bool m_needsUpdate;
  long m_albumId;
};
