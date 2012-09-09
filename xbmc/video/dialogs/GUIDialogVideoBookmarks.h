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
#include "GUIViewControl.h"
#include "video/VideoDatabase.h"

class CFileItemList;

class CGUIDialogVideoBookmarks : public CGUIDialog
{
public:
  CGUIDialogVideoBookmarks(void);
  virtual ~CGUIDialogVideoBookmarks(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnWindowLoaded();
  virtual void OnWindowUnload();

protected:
  void GotoBookmark(int iItem);
  void ClearBookmarks();
  void AddBookmark(CVideoInfoTag* tag=NULL);
  void Clear();
  void Update();
  void AddEpisodeBookmark();

  CGUIControl *GetFirstFocusableControl(int id);

  CFileItemList* m_vecItems;
  CGUIViewControl m_viewControl;
  VECBOOKMARKS m_bookmarks;
};
