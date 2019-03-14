/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <atomic>
#include <memory>

#include "threads/Event.h"
#include "threads/Thread.h"

#include "pvr/PVRChannelNumberInputHandler.h"
#include "pvr/windows/GUIWindowPVRBase.h"

namespace PVR
{
  class CGUIEPGGridContainer;
  class CPVRRefreshTimelineItemsThread;

  class CGUIWindowPVRGuideBase : public CGUIWindowPVRBase, public CPVRChannelNumberInputHandler
  {
  public:
    CGUIWindowPVRGuideBase(bool bRadio, int id, const std::string &xmlFile);
    ~CGUIWindowPVRGuideBase() override;

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
    void GetChannelNumbers(std::vector<std::string>& channelNumbers) override;
    void OnInputDone() override;

  protected:
    void UpdateSelectedItemPath() override;
    std::string GetDirectoryPath(void) override { return ""; }
    bool GetDirectory(const std::string &strDirectory, CFileItemList &items) override;
    void FormatAndSort(CFileItemList &items) override;
    CFileItemPtr GetCurrentListItem(int offset = 0) override;

    void ClearData() override;

  private:
    CGUIEPGGridContainer* GetGridControl();
    void InitEpgGridControl();

    bool OnContextButtonBegin();
    bool OnContextButtonEnd();
    bool OnContextButtonNow();
    bool OnContextButtonDate();

    bool ShouldNavigateToGridContainer(int iAction);

    void StartRefreshTimelineItemsThread();
    void StopRefreshTimelineItemsThread();

    void RefreshView(CGUIMessage& message, bool bInitGridControl);

    std::unique_ptr<CPVRRefreshTimelineItemsThread> m_refreshTimelineItemsThread;
    std::atomic_bool m_bRefreshTimelineItems;
    std::atomic_bool m_bSyncRefreshTimelineItems;

    CPVRChannelGroupPtr m_cachedChannelGroup;
    std::unique_ptr<CFileItemList> m_newTimeline;

    bool m_bChannelSelectionRestored;
    std::atomic_bool m_bFirstOpen;
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
    explicit CPVRRefreshTimelineItemsThread(CGUIWindowPVRGuideBase *pGuideWindow);
    ~CPVRRefreshTimelineItemsThread() override;

    void Process() override;

    void DoRefresh(bool bWait);
    void Stop();

  private:
    CGUIWindowPVRGuideBase *m_pGuideWindow;
    CEvent m_ready;
    CEvent m_done;
  };
}
