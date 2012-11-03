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
#include "FileItem.h"

class CGUIDialogVideoInfo :
      public CGUIDialog
{
public:
  CGUIDialogVideoInfo(void);
  virtual ~CGUIDialogVideoInfo(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  void SetMovie(const CFileItem *item);
  bool NeedRefresh() const;
  bool RefreshAll() const;
  bool HasUpdatedThumb() const { return m_hasUpdatedThumb; };

  std::string GetThumbnail() const;
  virtual CFileItemPtr GetCurrentListItem(int offset = 0) { return m_movieItem; }
  const CFileItemList& CurrentDirectory() const { return *m_castList; };
  virtual bool HasListItems() const { return true; };

  static std::string ChooseArtType(const CFileItem &item, std::map<std::string, std::string> &currentArt);
protected:
  virtual void OnInitWindow();
  void Update();
  void SetLabel(int iControl, const CStdString& strLabel);

  // link cast to movies
  void ClearCastList();
  void OnSearch(CStdString& strSearch);
  void DoSearch(CStdString& strSearch, CFileItemList& items);
  void OnSearchItemFound(const CFileItem* pItem);
  void Play(bool resume = false);
  void OnGetArt();
  void OnGetFanart();
  void PlayTrailer();

  CFileItemPtr m_movieItem;
  CFileItemList *m_castList;
  bool m_bViewReview;
  bool m_bRefresh;
  bool m_bRefreshAll;
  bool m_hasUpdatedThumb;
};
