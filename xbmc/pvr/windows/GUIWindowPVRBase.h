/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "threads/SystemClock.h"
#include "windows/GUIMediaWindow.h"

#include <atomic>
#include <memory>
#include <string>

class CGUIDialogProgressBarHandle;

namespace PVR
{
enum class PVREvent;

enum EPGSelectAction
{
  EPG_SELECT_ACTION_CONTEXT_MENU = 0,
  EPG_SELECT_ACTION_SWITCH = 1,
  EPG_SELECT_ACTION_INFO = 2,
  EPG_SELECT_ACTION_RECORD = 3,
  EPG_SELECT_ACTION_PLAY_RECORDING = 4,
  EPG_SELECT_ACTION_SMART_SELECT = 5
};

class CPVRChannelGroup;
class CGUIPVRChannelGroupsSelector;

class CGUIWindowPVRBase : public CGUIMediaWindow
{
public:
  ~CGUIWindowPVRBase() override;

  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;
  bool OnMessage(CGUIMessage& message) override;
  bool Update(const std::string& strDirectory, bool updateFilterPath = true) override;
  void UpdateButtons() override;
  bool OnAction(const CAction& action) override;
  void SetInvalid() override;
  bool CanBeActivated() const override;

  bool UseFileDirectories() override { return false; }

  /*!
   * @brief CEventStream callback for PVR events.
   * @param event The event.
   */
  virtual void NotifyEvent(const PVREvent& event);

  bool ActivatePreviousChannelGroup();
  bool ActivateNextChannelGroup();
  bool OpenChannelGroupSelectionDialog();

protected:
  CGUIWindowPVRBase(bool bRadio, int id, const std::string& xmlFile);

  virtual std::string GetDirectoryPath() = 0;

  virtual void ClearData();

  /*!
   * @brief Init this window's channel group with the currently active (the "playing") channel group.
   * @return true if group could be set, false otherwise.
   */
  bool InitChannelGroup();

  /*!
   * @brief Get the channel group for this window.
   * @return the group or null, if no group set.
   */
  std::shared_ptr<CPVRChannelGroup> GetChannelGroup();

  /*!
   * @brief Set a new channel group, start listening to this group, optionally update window content.
   * @param group The new group.
   * @param bUpdate if true, window content will be updated.
   */
  void SetChannelGroup(const std::shared_ptr<CPVRChannelGroup>& group, bool bUpdate = true);

  void SetChannelGroupPath(const std::string& path);

  virtual void UpdateSelectedItemPath();

  bool IsRadio() const { return m_bRadio; }

  bool IsUpdating() const { return m_bUpdating; }

  CCriticalSection m_critSection;

private:
  /*!
   * @brief Show or update the progress dialog.
   * @param strText The current status.
   * @param iProgress The current progress in %.
   */
  void ShowProgressDialog(const std::string& strText, int iProgress);

  /*!
   * @brief Hide the progress dialog if it's visible.
   */
  void HideProgressDialog();

  bool m_bRadio{false};
  std::atomic_bool m_bUpdating{false};

  std::unique_ptr<CGUIPVRChannelGroupsSelector> m_channelGroupsSelector;
  std::shared_ptr<CPVRChannelGroup> m_channelGroup;
  XbmcThreads::EndTime<> m_refreshTimeout;
  CGUIDialogProgressBarHandle* m_progressHandle =
      nullptr; /*!< progress dialog that is displayed while the pvr manager is loading */
  std::string m_channelGroupPath;
};
} // namespace PVR
