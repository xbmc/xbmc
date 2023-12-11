/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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

protected:
  bool ApplyWatchedFilter(CFileItemList &items);
  bool GetFilteredItems(const std::string &filter, CFileItemList &items) override;

  void OnItemLoaded(CFileItem* pItem) override {};

  // override base class methods
  bool Update(const std::string &strDirectory, bool updateFilterPath = true) override;
  bool GetDirectory(const std::string &strDirectory, CFileItemList &items) override;
  void UpdateButtons() override;
  void DoSearch(const std::string& strSearch, CFileItemList& items) override;
  void OnDeleteItem(const CFileItemPtr& pItem) override;
  void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
  bool OnPopupMenu(int iItem) override;
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
