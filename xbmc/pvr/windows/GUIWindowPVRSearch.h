/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

#include "pvr/windows/GUIWindowPVRBase.h"

namespace PVR
{
  class CPVREpgSearchFilter;

  class CGUIWindowPVRSearchBase : public CGUIWindowPVRBase
  {
  public:
    CGUIWindowPVRSearchBase(bool bRadio, int id, const std::string &xmlFile);
    ~CGUIWindowPVRSearchBase() override = default;

    bool OnMessage(CGUIMessage& message)  override;
    void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
    bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;

    /*!
     * @brief set the item to search similar events for.
     * @param item the epg event to search similar events for.
     */
    void SetItemToSearch(const CFileItemPtr &item);

  protected:
    void OnPrepareFileItems(CFileItemList &items) override;
    std::string GetDirectoryPath(void) override { return ""; }

  private:
    bool OnContextButtonClear(CFileItem *item, CONTEXT_BUTTON button);

    void OpenDialogSearch();

    bool m_bSearchConfirmed;
    std::unique_ptr<CPVREpgSearchFilter> m_searchfilter;
  };

  class CGUIWindowPVRTVSearch : public CGUIWindowPVRSearchBase
  {
  public:
    CGUIWindowPVRTVSearch() : CGUIWindowPVRSearchBase(false, WINDOW_TV_SEARCH, "MyPVRSearch.xml") {}
  };

  class CGUIWindowPVRRadioSearch : public CGUIWindowPVRSearchBase
  {
  public:
    CGUIWindowPVRRadioSearch() : CGUIWindowPVRSearchBase(true, WINDOW_RADIO_SEARCH, "MyPVRSearch.xml") {}
  };
}
