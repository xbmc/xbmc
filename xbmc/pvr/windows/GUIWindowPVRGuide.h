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
#include "epg/GUIEPGGridContainer.h"

class CSetting;

namespace PVR
{
  class CGUIWindowPVRGuide : public CGUIWindowPVRBase
  {
  public:
    CGUIWindowPVRGuide(bool bRadio);
    virtual ~CGUIWindowPVRGuide(void);

    bool OnMessage(CGUIMessage& message);
    bool OnAction(const CAction &action);
    void GetContextButtons(int itemNumber, CContextButtons &buttons);
    bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
    bool Update(const std::string &strDirectory, bool updateFilterPath = true);
    void ResetObservers(void);
    void UnregisterObservers(void);

  protected:
    void UpdateSelectedItemPath();

  private:
    bool SelectPlayingFile(void);

    bool OnContextButtonBegin(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonEnd(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonNow(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonInfo(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonPlay(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonStartRecord(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonStopRecord(CFileItem *item, CONTEXT_BUTTON button);

    void UpdateViewChannel();
    void UpdateViewNow();
    void UpdateViewNext();
    void UpdateViewTimeline();

    CFileItemList      *m_cachedTimeline;
    CPVRChannelGroupPtr m_cachedChannelGroup;

    bool m_bUpdateRequired;
  };
}
