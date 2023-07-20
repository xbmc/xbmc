/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dialogs/GUIDialogContextMenu.h"
#include "pvr/dialogs/GUIDialogPVRGuideSearch.h"
#include "pvr/windows/GUIWindowPVRBase.h"

#include <memory>
#include <string>

class CFileItem;

namespace PVR
{
class CPVREpgSearchFilter;

class CGUIWindowPVRSearchBase : public CGUIWindowPVRBase
{
public:
  CGUIWindowPVRSearchBase(bool bRadio, int id, const std::string& xmlFile);
  ~CGUIWindowPVRSearchBase() override;

  bool OnAction(const CAction& action) override;
  bool OnMessage(CGUIMessage& message) override;
  void GetContextButtons(int itemNumber, CContextButtons& buttons) override;
  bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;
  bool Update(const std::string& strDirectory, bool updateFilterPath = true) override;
  void UpdateButtons() override;

  /*!
   * @brief set the item to search similar events for.
   * @param item the epg event to search similar events for.
   */
  void SetItemToSearch(const CFileItem& item);

  /*!
   * @brief Open the search dialog for the given search filter item.
   * @param item the epg search filter.
   * @return The result of the dialog
   */
  CGUIDialogPVRGuideSearch::Result OpenDialogSearch(const CFileItem& item);

protected:
  void OnPrepareFileItems(CFileItemList& items) override;

private:
  bool OnContextButtonClear(CFileItem* item, CONTEXT_BUTTON button);

  CGUIDialogPVRGuideSearch::Result OpenDialogSearch(
      const std::shared_ptr<CPVREpgSearchFilter>& searchFilter);

  void ExecuteSearch();

  void SetSearchFilter(const std::shared_ptr<CPVREpgSearchFilter>& searchFilter);

  bool m_bSearchConfirmed = false;
  std::shared_ptr<CPVREpgSearchFilter> m_searchfilter;
};

class CGUIWindowPVRTVSearch : public CGUIWindowPVRSearchBase
{
public:
  CGUIWindowPVRTVSearch() : CGUIWindowPVRSearchBase(false, WINDOW_TV_SEARCH, "MyPVRSearch.xml") {}

protected:
  std::string GetRootPath() const override;
  std::string GetStartFolder(const std::string& dir) override;
  std::string GetDirectoryPath() override;
};

class CGUIWindowPVRRadioSearch : public CGUIWindowPVRSearchBase
{
public:
  CGUIWindowPVRRadioSearch() : CGUIWindowPVRSearchBase(true, WINDOW_RADIO_SEARCH, "MyPVRSearch.xml")
  {
  }

protected:
  std::string GetRootPath() const override;
  std::string GetStartFolder(const std::string& dir) override;
  std::string GetDirectoryPath() override;
};
} // namespace PVR
