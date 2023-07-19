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

#define CONTROL_BTNVIEWASICONS            2
#define CONTROL_BTNSORTBY                 3
#define CONTROL_BTNSORTASC                4
#define CONTROL_BTNGROUPITEMS             5
#define CONTROL_BTNSHOWHIDDEN             6
#define CONTROL_BTNSHOWDELETED            7
#define CONTROL_BTNHIDEDISABLEDTIMERS     8
#define CONTROL_BTNSHOWMODE               10
#define CONTROL_LSTCHANNELGROUPS          11

#define CONTROL_BTNCHANNELGROUPS          28
#define CONTROL_BTNFILTERCHANNELS         31

#define CONTROL_LABEL_HEADER1             29
#define CONTROL_LABEL_HEADER2             30

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
    void Notify(const PVREvent& event);
    virtual void NotifyEvent(const PVREvent& event);

    /*!
     * @brief Refresh window content.
     * @return true, if refresh succeeded, false otherwise.
     */
    bool DoRefresh() { return Refresh(true); }

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
    void SetChannelGroup(std::shared_ptr<CPVRChannelGroup> &&group, bool bUpdate = true);

    virtual void UpdateSelectedItemPath();

    CCriticalSection m_critSection;
    std::string m_channelGroupPath;
    bool m_bRadio;
    std::atomic_bool m_bUpdating = {false};

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

    std::unique_ptr<CGUIPVRChannelGroupsSelector> m_channelGroupsSelector;
    std::shared_ptr<CPVRChannelGroup> m_channelGroup;
    XbmcThreads::EndTime<> m_refreshTimeout;
    CGUIDialogProgressBarHandle* m_progressHandle =
        nullptr; /*!< progress dialog that is displayed while the pvr manager is loading */
  };
}
