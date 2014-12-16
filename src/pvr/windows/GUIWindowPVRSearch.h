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

#include "GUIWindowPVRBase.h"
#include "epg/EpgSearchFilter.h"

namespace PVR
{
  class CGUIWindowPVRSearch : public CGUIWindowPVRBase
  {
  public:
    CGUIWindowPVRSearch(bool bRadio);
    virtual ~CGUIWindowPVRSearch(void) {};

    bool OnMessage(CGUIMessage& message);
    void OnWindowLoaded();
    void GetContextButtons(int itemNumber, CContextButtons &buttons);
    bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
    bool OnContextButton(const CFileItem &item, CONTEXT_BUTTON button);

  protected:
    void OnPrepareFileItems(CFileItemList &items);

  private:
    bool OnContextButtonClear(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonInfo(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonStartRecord(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonStopRecord(CFileItem *item, CONTEXT_BUTTON button);

    void OpenDialogSearch();

    bool                  m_bSearchConfirmed;
    EPG::EpgSearchFilter  m_searchfilter;
  };
}
