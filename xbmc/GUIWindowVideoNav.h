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

#include "GUIWindowVideoBase.h"
#include "ThumbLoader.h"

class CFileItemList;

class CGUIWindowVideoNav : public CGUIWindowVideoBase
{
public:

  CGUIWindowVideoNav(void);
  virtual ~CGUIWindowVideoNav(void);

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void Render();

  virtual void ClearFileItems();
  virtual void OnFinalizeFileItems(CFileItemList &items);
  virtual void OnInfo(CFileItem* pItem, const SScraperInfo&info);
  static bool DeleteItem(CFileItem* pItem, bool bUnavailable=false);

protected:
  virtual void OnItemLoaded(CFileItem* pItem);
  void OnLinkMovieToTvShow(int itemnumber, bool bRemove);
  // override base class methods
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual void UpdateButtons();
  virtual void DoSearch(const CStdString& strSearch, CFileItemList& items);
  virtual void PlayItem(int iItem);
  virtual void OnDeleteItem(int iItem);
  virtual void OnWindowLoaded();
  virtual void OnFilterItems();
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  virtual bool OnClick(int iItem);

  virtual CStdString GetQuickpathName(const CStdString& strPath) const;
  void FilterItems(CFileItemList &items);

  void DisplayEmptyDatabaseMessage(bool bDisplay);

  VECSOURCES m_shares;

  bool m_bDisplayEmptyDatabaseMessage;  ///< If true we display a message informing the user to switch back to the Files view.

  // filtered item views
  CFileItemList* m_unfilteredItems;
  CStdString m_filter;

  CStdString m_startDirectory;
};
