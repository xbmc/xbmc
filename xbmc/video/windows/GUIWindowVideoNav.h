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

#include "GUIWindowVideoBase.h"

class CFileItemList;

enum SelectFirstUnwatchedItem
{
  NEVER = 0,
  ON_FIRST_ENTRY = 1,
  ALWAYS = 2
};

enum IncludeAllSeasonsAndSpecials
{
  NEITHER = 0,
  BOTH = 1,
  ALL_SEASONS = 2,
  SPECIALS = 3
};

class CGUIWindowVideoNav : public CGUIWindowVideoBase
{
public:

  CGUIWindowVideoNav(void);
  virtual ~CGUIWindowVideoNav(void);

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);

  virtual void OnInfo(CFileItem* pItem, ADDON::ScraperPtr &info);

  /*! \brief Load video information from the database for these items (public static version)
   Useful for grabbing information for file listings, from watched status to full metadata
   \param items the items to load information for.
   \param database open database object to retrieve the data from
   \param allowReplaceLabels allow label replacement if according GUI setting is enabled
   */
  static void LoadVideoInfo(CFileItemList &items, CVideoDatabase &database, bool allowReplaceLabels = true);

protected:
  /*! \brief Load video information from the database for these items
   Useful for grabbing information for file listings, from watched status to full metadata
   \param items the items to load information for.
   */
  void LoadVideoInfo(CFileItemList &items);

  bool ApplyWatchedFilter(CFileItemList &items);
  virtual bool GetFilteredItems(const std::string &filter, CFileItemList &items);

  virtual void OnItemLoaded(CFileItem* pItem) {};

  // override base class methods
  virtual bool Update(const std::string &strDirectory, bool updateFilterPath = true);
  virtual bool GetDirectory(const std::string &strDirectory, CFileItemList &items);
  virtual void UpdateButtons();
  virtual void DoSearch(const std::string& strSearch, CFileItemList& items);
  virtual void PlayItem(int iItem);
  virtual void OnDeleteItem(CFileItemPtr pItem);
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  virtual bool OnClick(int iItem);
  virtual std::string GetStartFolder(const std::string &dir);

  VECSOURCES m_shares;

private:
  virtual SelectFirstUnwatchedItem GetSettingSelectFirstUnwatchedItem();
  virtual IncludeAllSeasonsAndSpecials GetSettingIncludeAllSeasonsAndSpecials();
  virtual int GetFirstUnwatchedItemIndex(bool includeAllSeasons, bool includeSpecials);
};
