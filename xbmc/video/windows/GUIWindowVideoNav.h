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
  ~CGUIWindowVideoNav(void) override;

  bool OnAction(const CAction &action) override;
  bool OnMessage(CGUIMessage& message) override;

  void OnItemInfo(const CFileItem& fileItem, ADDON::ScraperPtr &info) override;

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
  bool GetFilteredItems(const std::string &filter, CFileItemList &items) override;

  void OnItemLoaded(CFileItem* pItem) override {};

  // override base class methods
  bool Update(const std::string &strDirectory, bool updateFilterPath = true) override;
  bool GetDirectory(const std::string &strDirectory, CFileItemList &items) override;
  void UpdateButtons() override;
  void DoSearch(const std::string& strSearch, CFileItemList& items) override;
  virtual void PlayItem(int iItem);
  void OnDeleteItem(CFileItemPtr pItem) override;
  void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
  bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;
  bool OnAddMediaSource() override;
  bool OnClick(int iItem, const std::string &player = "") override;
  std::string GetStartFolder(const std::string &dir) override;

  VECSOURCES m_shares;

private:
  virtual SelectFirstUnwatchedItem GetSettingSelectFirstUnwatchedItem();
  virtual IncludeAllSeasonsAndSpecials GetSettingIncludeAllSeasonsAndSpecials();
  virtual int GetFirstUnwatchedItemIndex(bool includeAllSeasons, bool includeSpecials);
  void SelectFirstUnwatched();
};
