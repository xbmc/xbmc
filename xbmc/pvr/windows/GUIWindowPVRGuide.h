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

#include <atomic>
#include <memory>
#include "threads/Thread.h"
#include "GUIWindowPVRBase.h"

class CSetting;

namespace EPG
{
  class CGUIEPGGridContainer;
}

namespace PVR
{
  class CPVRRefreshTimelineItemsThread;

  class CGUIWindowPVRGuide : public CGUIWindowPVRBase
  {
  public:
    CGUIWindowPVRGuide(bool bRadio);
    virtual ~CGUIWindowPVRGuide(void);

    virtual void OnInitWindow() override;
    virtual void OnDeinitWindow(int nextWindowID) override;
    virtual bool OnMessage(CGUIMessage& message) override;
    virtual bool OnAction(const CAction &action) override;
    virtual void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
    virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;
    virtual void UpdateButtons(void) override;
    virtual void Notify(const Observable &obs, const ObservableMessage msg) override;
    virtual void SetInvalid() override;

    bool RefreshTimelineItems();

  protected:
    virtual void UpdateSelectedItemPath() override;
    virtual std::string GetDirectoryPath(void) override { return ""; }
    virtual bool GetDirectory(const std::string &strDirectory, CFileItemList &items) override;
    virtual void RegisterObservers(void) override;
    virtual void UnregisterObservers(void) override;

  private:
    EPG::CGUIEPGGridContainer* GetGridControl();

    bool SelectPlayingFile(void);

    bool OnContextButtonBegin(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonEnd(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonNow(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonInfo(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonPlay(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonStartRecord(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonStopRecord(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonDeleteTimer(CFileItem *item, CONTEXT_BUTTON button);

    void GetViewChannelItems(CFileItemList &items);
    void GetViewNowItems(CFileItemList &items);
    void GetViewNextItems(CFileItemList &items);
    void GetViewTimelineItems(CFileItemList &items);

    void StartRefreshTimelineItemsThread();
    void StopRefreshTimelineItemsThread();

    std::unique_ptr<CPVRRefreshTimelineItemsThread> m_refreshTimelineItemsThread;
    std::atomic_bool m_bRefreshTimelineItems;

    CPVRChannelGroupPtr m_cachedChannelGroup;
    std::unique_ptr<CFileItemList> m_newTimeline;
  };

  class CPVRRefreshTimelineItemsThread : public CThread
  {
  public:
    CPVRRefreshTimelineItemsThread(CGUIWindowPVRGuide *pGuideWindow);
    virtual ~CPVRRefreshTimelineItemsThread() {}

    virtual void Process();

  private:
    CGUIWindowPVRGuide *m_pGuideWindow;
  };
}
