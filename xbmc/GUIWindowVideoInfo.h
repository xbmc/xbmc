#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIDialog.h"
#include "GUIListItem.h"
#include "GUIWindowVideoBase.h"
#include "GUIWindowVideoFiles.h"

class CFileItem;

class CGUIWindowVideoInfo :
      public CGUIDialog
{
public:
  CGUIWindowVideoInfo(void);
  virtual ~CGUIWindowVideoInfo(void);
  virtual bool OnMessage(CGUIMessage& message);
  void SetMovie(const CFileItem *item);
  bool NeedRefresh() const;
  bool RefreshAll() const;
  bool HasUpdatedThumb() const { return m_hasUpdatedThumb; };

  const CStdString &GetThumbnail() const;
  virtual CFileItemPtr GetCurrentListItem(int offset = 0) { return m_movieItem; }
  const CFileItemList& CurrentDirectory() const { return *m_castList; };
  virtual bool HasListItems() const { return true; };
protected:
  void Refresh();
  void Update();
  void SetLabel(int iControl, const CStdString& strLabel);
  VIDEODB_CONTENT_TYPE GetContentType(const CFileItem *pItem) const;

  // link cast to movies
  void ClearCastList();
  void OnSearch(CStdString& strSearch);
  void DoSearch(CStdString& strSearch, CFileItemList& items);
  void OnSearchItemFound(const CFileItem* pItem);
  void Play(bool resume = false);
  void OnGetThumb();
  void OnGetFanart();
  void PlayTrailer();

  CFileItemPtr m_movieItem;
  CFileItemList *m_castList;
  bool m_bViewReview;
  bool m_bRefresh;
  bool m_bRefreshAll;
  bool m_hasUpdatedThumb;
  CGUIDialogProgress* m_dlgProgress;
};
