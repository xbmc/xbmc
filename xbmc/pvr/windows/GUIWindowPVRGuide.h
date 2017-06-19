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
#include "threads/Event.h"
#include "threads/Thread.h"
#include "pvr/PVRChannelNumberInputHandler.h"
#include "GUIWindowPVRBase.h"

namespace PVR
{
  class CGUIEPGGridContainer;
  class CPVRRefreshTimelineItemsThread;

  class CGUIWindowPVRGuideBase : public CGUIWindowPVRBase, public CPVRChannelNumberInputHandler
  {
  public:
    CGUIWindowPVRGuideBase(bool bRadio, int id, const std::string &xmlFile);
    virtual ~CGUIWindowPVRGuideBase();

    void OnInitWindow() override;
    void OnDeinitWindow(int nextWindowID) override;
    bool OnMessage(CGUIMessage& message) override;
    bool OnAction(const CAction &action) override;
    void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
    bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;
    void UpdateButtons(void) override;
    void Notify(const Observable &obs, const ObservableMessage msg) override;
    void SetInvalid() override;
    bool Update(const std::string &strDirectory, bool updateFilterPath = true) override;

    bool RefreshTimelineItems();

    // CPVRChannelNumberInputHandler implementation
    void OnInputDone() override;

  protected:
    void UpdateSelectedItemPath() override;
    std::string GetDirectoryPath(void) override { return ""; }
    bool GetDirectory(const std::string &strDirectory, CFileItemList &items) override;

    void ClearData() override;

  private:
    void Init();

    CGUIEPGGridContainer* GetGridControl();

    bool SelectPlayingFile(void);

    bool OnContextButtonBegin(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonEnd(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonNow(CFileItem *item, CONTEXT_BUTTON button);

    void StartRefreshTimelineItemsThread();
    void StopRefreshTimelineItemsThread();

    std::unique_ptr<CPVRRefreshTimelineItemsThread> m_refreshTimelineItemsThread;
    std::atomic_bool m_bRefreshTimelineItems;

    CPVRChannelGroupPtr m_cachedChannelGroup;
    std::unique_ptr<CFileItemList> m_newTimeline;

    bool m_bChannelSelectionRestored;
  };

  class CGUIWindowPVRTVGuide : public CGUIWindowPVRGuideBase
  {
  public:
    CGUIWindowPVRTVGuide() : CGUIWindowPVRGuideBase(false, WINDOW_TV_GUIDE, "MyPVRGuide.xml") {}
  };

  class CGUIWindowPVRRadioGuide : public CGUIWindowPVRGuideBase
  {
  public:
    CGUIWindowPVRRadioGuide() : CGUIWindowPVRGuideBase(true, WINDOW_RADIO_GUIDE, "MyPVRGuide.xml") {}
  };

  class CPVRRefreshTimelineItemsThread : public CThread
  {
  public:
    CPVRRefreshTimelineItemsThread(CGUIWindowPVRGuideBase *pGuideWindow);
    virtual ~CPVRRefreshTimelineItemsThread();

    virtual void Process();

    void DoRefresh();
    void Stop();

  private:
    CGUIWindowPVRGuideBase *m_pGuideWindow;
    CEvent m_ready;
    CEvent m_done;
  };
}
