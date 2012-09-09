#pragma once

/*
 *      Copyright (C) 2012 Team XBMC
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

#include "GUIWindowPVRCommon.h"
#include "epg/GUIEPGGridContainer.h"
#include "threads/CriticalSection.h"
#include "utils/Observer.h"
#include "../channels/PVRChannelGroup.h"

namespace PVR
{
  class CGUIWindowPVR;

  class CGUIWindowPVRGuide : public CGUIWindowPVRCommon, public Observer
  {
    friend class CGUIWindowPVR;

  public:
    CGUIWindowPVRGuide(CGUIWindowPVR *parent);
    virtual ~CGUIWindowPVRGuide(void);

    void GetContextButtons(int itemNumber, CContextButtons &buttons) const;
    bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
    void UpdateData(bool bUpdateSelectedFile = true);
    void Notify(const Observable &obs, const ObservableMessage msg);
    void SetInvalid(void) { UpdateData(); }
    void UnregisterObservers(void);
    void ResetObservers(void);

  private:
    bool SelectPlayingFile(void);
    bool IsSelectedButton(CGUIMessage &message) const;
    bool IsSelectedList(CGUIMessage &message) const;
    bool OnClickButton(CGUIMessage &message);
    bool OnClickList(CGUIMessage &message);
    bool PlayEpgItem(CFileItem *item);

    bool OnContextButtonBegin(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonEnd(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonInfo(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonPlay(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonStartRecord(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonStopRecord(CFileItem *item, CONTEXT_BUTTON button);

    void UpdateButtons(void);
    void UpdateViewChannel(bool bUpdateSelectedFile);
    void UpdateViewNow(bool bUpdateSelectedFile);
    void UpdateViewNext(bool bUpdateSelectedFile);
    void UpdateViewTimeline(bool bUpdateSelectedFile);

    int               m_iGuideView;
    CFileItemList    *m_cachedTimeline;
    CPVRChannelGroupPtr m_cachedChannelGroup;
  };
}
