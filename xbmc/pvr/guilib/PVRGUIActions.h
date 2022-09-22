/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/PVRChannelNumberInputHandler.h"
#include "pvr/guilib/PVRGUIActionsEPG.h"
#include "pvr/guilib/PVRGUIActionsPlayback.h"
#include "pvr/guilib/PVRGUIActionsRecordings.h"
#include "pvr/guilib/PVRGUIActionsTimers.h"
#include "pvr/guilib/PVRGUIChannelNavigator.h"
#include "pvr/settings/PVRSettings.h"
#include "threads/CriticalSection.h"

#include <memory>
#include <string>
#include <vector>

class CFileItem;

namespace PVR
{
enum class ParentalCheckResult
{
  CANCELED,
  FAILED,
  SUCCESS
};

class CPVRChannel;
class CPVRChannelGroupMember;
class CPVRTimerInfoTag;

class CPVRChannelSwitchingInputHandler : public CPVRChannelNumberInputHandler
{
public:
  // CPVRChannelNumberInputHandler implementation
  void GetChannelNumbers(std::vector<std::string>& channelNumbers) override;
  void AppendChannelNumberCharacter(char cCharacter) override;
  void OnInputDone() override;

private:
  /*!
   * @brief Switch to the channel with the given number.
   * @param channelNumber the channel number
   */
  void SwitchToChannel(const CPVRChannelNumber& channelNumber);

  /*!
   * @brief Switch to the previously played channel.
   */
  void SwitchToPreviousChannel();
};

class CPVRGUIActions : public CPVRGUIActionsEPG,
                       public CPVRGUIActionsPlayback,
                       public CPVRGUIActionsRecordings,
                       public CPVRGUIActionsTimers
{
public:
  CPVRGUIActions();
  virtual ~CPVRGUIActions() = default;

  /*!
   * @brief Hide a channel, always showing a confirmation dialog.
   * @param item containing a channel or an epg tag.
   * @return true on success, false otherwise.
   */
  bool HideChannel(const std::shared_ptr<CFileItem>& item) const;

  /*!
   * @brief Open a selection dialog and start a channel scan on the selected client.
   * @return true on success, false otherwise.
   */
  bool StartChannelScan();

  /*!
   * @brief Start a channel scan on the specified client or open a dialog to select a client
   * @param clientId the id of client to scan or PVR_INVALID_CLIENT_ID if a dialog will be opened
   * @return true on success, false otherwise.
   */
  bool StartChannelScan(int clientId);

  /*!
   * @return True when a channel scan is currently running, false otherwise.
   */
  bool IsRunningChannelScan() const { return m_bChannelScanRunning; }

  /*!
   * @brief Select and invoke client-specific settings actions
   * @return true on success, false otherwise.
   */
  bool ProcessSettingsMenuHooks();

  /*!
   * @brief Reset the TV database to it's initial state and delete all the data.
   * @param bResetEPGOnly True to only reset the EPG database, false to reset both PVR and EPG database.
   * @return true on success, false otherwise.
   */
  bool ResetPVRDatabase(bool bResetEPGOnly);

  /*!
   * @brief Check if channel is parental locked. Ask for PIN if necessary.
   * @param channel The channel to do the check for.
   * @return the result of the check (success, failed, or canceled by user).
   */
  ParentalCheckResult CheckParentalLock(const std::shared_ptr<CPVRChannel>& channel) const;

  /*!
   * @brief Open Numeric dialog to check for parental PIN.
   * @return the result of the check (success, failed, or canceled by user).
   */
  ParentalCheckResult CheckParentalPIN() const;

  /*!
   * @brief Check whether the system Kodi is running on can be powered down
   *        (shutdown/reboot/suspend/hibernate) without stopping any active
   *        recordings and/or without preventing the start of recordings
   *        scheduled for now + pvrpowermanagement.backendidletime.
   * @param bAskUser True to informs user in case of potential
   *        data loss. User can decide to allow powerdown anyway. False to
   *        not to ask user and to not confirm power down.
   * @return True if system can be safely powered down, false otherwise.
   */
  bool CanSystemPowerdown(bool bAskUser = true) const;

  /*!
   * @brief Get the currently selected item path; used across several windows/dialogs to share item selection.
   * @param bRadio True to query the selected path for PVR radio, false for Live TV.
   * @return the path.
   */
  std::string GetSelectedItemPath(bool bRadio) const;

  /*!
   * @brief Set the currently selected item path; used across several windows/dialogs to share item selection.
   * @param bRadio True to set the selected path for PVR radio, false for Live TV.
   * @param path The new path to set.
   */
  void SetSelectedItemPath(bool bRadio, const std::string& path);

  /*!
   * @brief Get a channel group member for the given channel, either from the currently active
   * group or if not found there, from the 'all channels' group.
   * @param channel the channel.
   * @return the group member or nullptr if not found.
   */
  std::shared_ptr<CPVRChannelGroupMember> GetChannelGroupMember(
      const std::shared_ptr<CPVRChannel>& channel) const;

  /*!
   * @brief Get a channel group member for the given item, either from the currently active group
   * or if not found there, from the 'all channels' group.
   * @param item the item containing a channel, channel group, recording, timer or epg tag.
   * @return the group member or nullptr if not found.
   */
  std::shared_ptr<CPVRChannelGroupMember> GetChannelGroupMember(const CFileItem& item) const;

  /*!
   * @brief Get the currently active channel number input handler.
   * @return the handler.
   */
  CPVRChannelNumberInputHandler& GetChannelNumberInputHandler();

  /*!
   * @brief Get the channel navigator.
   * @return the navigator.
   */
  CPVRGUIChannelNavigator& GetChannelNavigator();

  /*!
   * @brief Inform GUI actions that playback of an item just started.
   * @param item The item that started to play.
   */
  void OnPlaybackStarted(const std::shared_ptr<CFileItem>& item);

  /*!
   * @brief Inform GUI actions that playback of an item was stopped due to user interaction.
   * @param item The item that stopped to play.
   */
  void OnPlaybackStopped(const std::shared_ptr<CFileItem>& item);

  /*!
   * @brief Process info action for the given item.
   * @param item The item.
   */
  bool OnInfo(const std::shared_ptr<CFileItem>& item);

private:
  CPVRGUIActions(const CPVRGUIActions&) = delete;
  CPVRGUIActions const& operator=(CPVRGUIActions const&) = delete;

  bool AllLocalBackendsIdle(std::shared_ptr<CPVRTimerInfoTag>& causingEvent) const;
  bool EventOccursOnLocalBackend(const std::shared_ptr<CFileItem>& item) const;
  bool IsNextEventWithinBackendIdleTime() const;

  mutable CCriticalSection m_critSection;
  CPVRChannelSwitchingInputHandler m_channelNumberInputHandler;
  bool m_bChannelScanRunning = false;
  CPVRSettings m_settings;
  CPVRGUIChannelNavigator m_channelNavigator;
  std::string m_selectedItemPathTV;
  std::string m_selectedItemPathRadio;
};

} // namespace PVR
