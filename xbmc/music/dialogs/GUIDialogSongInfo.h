#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
  virtual ~CGUIDialogSongInfo(void);
  virtual bool OnMessage(CGUIMessage& message);
  void SetSong(CFileItem *item);
  virtual bool OnAction(const CAction &action);
  virtual bool OnBack(int actionID);
  bool NeedsUpdate() const { return m_needsUpdate; };

  virtual bool HasListItems() const { return true; };
  virtual CFileItemPtr GetCurrentListItem(int offset = 0);
protected:
  virtual void OnInitWindow();
  bool DownloadThumbnail(const CStdString &thumbFile);
  void OnGetThumb();
  void SetRating(char rating);

  CFileItemPtr m_song;
  char m_startRating;
  bool m_cancelled;
  bool m_needsUpdate;
  long m_albumId;
};
