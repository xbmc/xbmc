/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/IPVRComponent.h"
#include "pvr/PVRChannelNumberInputHandler.h"
#include "pvr/guilib/PVRGUIChannelNavigator.h"
#include "pvr/settings/PVRSettings.h"
#include "threads/CriticalSection.h"

#include <memory>
#include <string>
#include <vector>

class CFileItem;

namespace PVR
{
class CPVRChannel;
class CPVRChannelGroupMember;

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

class CPVRGUIActionsChannels : public IPVRComponent
{
public:
  CPVRGUIActionsChannels();
  ~CPVRGUIActionsChannels() override;

  /*!
   * @brief Get the events available for CEventStream.
   * @return The events.
   */
  CEventStream<PVRChannelNumberInputChangedEvent>& Events() { return m_events; }

  /*!
   * @brief Register a handler for channel number input.
   * @param handler The handler to register.
   */
  void RegisterChannelNumberInputHandler(CPVRChannelNumberInputHandler* handler);

  /*!
   * @brief Deregister a handler for channel number input.
   * @param handler The handler to deregister.
   */
  void DeregisterChannelNumberInputHandler(CPVRChannelNumberInputHandler* handler);

  /*!
   * @brief CEventStream callback for channel number input changes.
   * @param event The event.
   */
  void Notify(const PVRChannelNumberInputChangedEvent& event);

  /*!
   * @brief Hide a channel, always showing a confirmation dialog.
   * @param item containing a channel or an epg tag.
   * @return true on success, false otherwise.
   */
  bool HideChannel(const CFileItem& item) const;

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
   * @brief Get a channel group member for the given channel, either from the currently active
   * group or if not found there, from the 'all channels' group.
   * @param channel the channel.
   * @return the group member or nullptr if not found.
   */
  std::shared_ptr<CPVRChannelGroupMember> GetChannelGroupMember(
      const std::shared_ptr<const CPVRChannel>& channel) const;

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
  void OnPlaybackStarted(const CFileItem& item);

  /*!
   * @brief Inform GUI actions that playback of an item was stopped due to user interaction.
   * @param item The item that stopped to play.
   */
  void OnPlaybackStopped(const CFileItem& item);

  /*!
   * @brief Get the currently selected channel item path; used across several windows/dialogs to
   * share item selection.
   * @param bRadio True to query the selected path for PVR radio, false for Live TV.
   * @return the path.
   */
  std::string GetSelectedChannelPath(bool bRadio) const;

  /*!
   * @brief Set the currently selected channel item path; used across several windows/dialogs to
   * share item selection.
   * @param bRadio True to set the selected path for PVR radio, false for Live TV.
   * @param path The new path to set.
   */
  void SetSelectedChannelPath(bool bRadio, const std::string& path);

private:
  CPVRGUIActionsChannels(const CPVRGUIActionsChannels&) = delete;
  CPVRGUIActionsChannels const& operator=(CPVRGUIActionsChannels const&) = delete;

  CPVRChannelSwitchingInputHandler m_channelNumberInputHandler;
  bool m_bChannelScanRunning{false};
  CPVRGUIChannelNavigator m_channelNavigator;
  CEventSource<PVRChannelNumberInputChangedEvent> m_events;

  mutable CCriticalSection m_critSection;
  CPVRSettings m_settings;
  std::string m_selectedChannelPathTV;
  std::string m_selectedChannelPathRadio;
};

namespace GUI
{
// pretty scope and name
using Channels = CPVRGUIActionsChannels;
} // namespace GUI

} // namespace PVR
