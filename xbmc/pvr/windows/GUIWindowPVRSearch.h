#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "pvr/epg/EpgSearchFilter.h"
#include "GUIWindowPVRBase.h"

namespace PVR
{
  class CGUIWindowPVRSearchBase : public CGUIWindowPVRBase
  {
  public:
    CGUIWindowPVRSearchBase(bool bRadio, int id, const std::string &xmlFile);
    virtual ~CGUIWindowPVRSearchBase() = default;

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
    CPVREpgSearchFilter m_searchfilter;
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
