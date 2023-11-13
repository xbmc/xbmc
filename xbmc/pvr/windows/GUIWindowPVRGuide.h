/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/PVRChannelNumberInputHandler.h"
#include "pvr/windows/GUIWindowPVRBase.h"
#include "threads/Event.h"
#include "threads/Thread.h"

#include <atomic>
#include <memory>
#include <string>

class CFileItemList;
class CGUIMessage;

namespace PVR
{
enum class PVREvent;

class CPVRChannelGroup;
class CGUIEPGGridContainer;
class CPVRRefreshTimelineItemsThread;

class CGUIWindowPVRGuideBase : public CGUIWindowPVRBase, public CPVRChannelNumberInputHandler
{
public:
  CGUIWindowPVRGuideBase(bool bRadio, int id, const std::string& xmlFile);
  ~CGUIWindowPVRGuideBase() override;

  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction& action) override;
  void GetContextButtons(int itemNumber, CContextButtons& buttons) override;
  bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;
  void UpdateButtons() override;
  void SetInvalid() override;
  bool Update(const std::string& strDirectory, bool updateFilterPath = true) override;

  void NotifyEvent(const PVREvent& event) override;

  bool RefreshTimelineItems();

  // CPVRChannelNumberInputHandler implementation
  void GetChannelNumbers(std::vector<std::string>& channelNumbers) override;
  void OnInputDone() override;

  bool GotoBegin();
  bool GotoEnd();
  bool GotoCurrentProgramme();
  bool GotoDate(int deltaHours);
  bool OpenDateSelectionDialog();
  bool Go12HoursBack();
  bool Go12HoursForward();
  bool GotoFirstChannel();
  bool GotoLastChannel();
  bool GotoPlayingChannel();

protected:
  void UpdateSelectedItemPath() override;
  std::string GetDirectoryPath() override { return ""; }
  bool GetDirectory(const std::string& strDirectory, CFileItemList& items) override;
  void FormatAndSort(CFileItemList& items) override;
  CFileItemPtr GetCurrentListItem(int offset = 0) override;

  void ClearData() override;

private:
  CGUIEPGGridContainer* GetGridControl();
  void InitEpgGridControl();

  bool OnContextButtonNavigate(CONTEXT_BUTTON button);

  bool ShouldNavigateToGridContainer(int iAction);

  void StartRefreshTimelineItemsThread();
  void StopRefreshTimelineItemsThread();

  void RefreshView(CGUIMessage& message, bool bInitGridControl);

  int GetCurrentListItemIndex(const std::shared_ptr<const CFileItem>& item);

  std::unique_ptr<CPVRRefreshTimelineItemsThread> m_refreshTimelineItemsThread;
  std::atomic_bool m_bRefreshTimelineItems{false};
  std::atomic_bool m_bSyncRefreshTimelineItems{false};

  std::shared_ptr<CPVRChannelGroup> m_cachedChannelGroup;

  bool m_bChannelSelectionRestored{false};
};

class CGUIWindowPVRTVGuide : public CGUIWindowPVRGuideBase
{
public:
  CGUIWindowPVRTVGuide() : CGUIWindowPVRGuideBase(false, WINDOW_TV_GUIDE, "MyPVRGuide.xml") {}
  std::string GetRootPath() const override;
};

class CGUIWindowPVRRadioGuide : public CGUIWindowPVRGuideBase
{
public:
  CGUIWindowPVRRadioGuide() : CGUIWindowPVRGuideBase(true, WINDOW_RADIO_GUIDE, "MyPVRGuide.xml") {}
  std::string GetRootPath() const override;
};

class CPVRRefreshTimelineItemsThread : public CThread
{
public:
  explicit CPVRRefreshTimelineItemsThread(CGUIWindowPVRGuideBase* pGuideWindow);
  ~CPVRRefreshTimelineItemsThread() override;

  void Process() override;

  void DoRefresh(bool bWait);
  void Stop();

private:
  CGUIWindowPVRGuideBase* m_pGuideWindow;
  CEvent m_ready;
  CEvent m_done;
};
} // namespace PVR
