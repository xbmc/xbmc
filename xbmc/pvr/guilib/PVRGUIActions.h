/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/PVRChannelNumberInputHandler.h"
#include "pvr/guilib/PVRGUIActionsRecordings.h"
#include "pvr/guilib/PVRGUIActionsTimers.h"
#include "pvr/guilib/PVRGUIChannelNavigator.h"
#include "pvr/settings/PVRSettings.h"
#include "threads/CriticalSection.h"

#include <memory>
#include <string>
#include <vector>

class CFileItem;

class CGUIWindow;

namespace PVR
{
  enum PlaybackType
  {
    PlaybackTypeAny = 0,
    PlaybackTypeTV,
    PlaybackTypeRadio
  };

  enum class ParentalCheckResult
  {
    CANCELED,
    FAILED,
    SUCCESS
  };

  class CPVRChannel;
  class CPVRChannelGroupMember;
  class CPVRStreamProperties;
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

  class CPVRGUIActions : public CPVRGUIActionsRecordings, public CPVRGUIActionsTimers
  {
  public:
    CPVRGUIActions();
    virtual ~CPVRGUIActions() = default;

    /*!
     * @brief Open a dialog with epg information for a given item.
     * @param item containing epg data to show. item must be an epg tag, a channel or a timer.
     * @return true on success, false otherwise.
     */
    bool ShowEPGInfo(const std::shared_ptr<CFileItem>& item) const;

    /*!
     * @brief Open a dialog with the epg list for a given item.
     * @param item containing channel info. item must be an epg tag, a channel or a timer.
     * @return true on success, false otherwise.
     */
    bool ShowChannelEPG(const std::shared_ptr<CFileItem>& item) const;

    /*!
     * @brief Open a window containing a list of epg tags 'similar' to a given item.
     * @param item containing epg data for matching. item must be an epg tag, a channel or a recording.
     * @return true on success, false otherwise.
     */
    bool FindSimilar(const std::shared_ptr<CFileItem>& item) const;

    /*!
     * @brief Get a localized resume play label, if the given item can be resumed.
     * @param item containing a recording or an epg tag.
     * @return the localized resume play label that can be used for instance as context menu item label or an empty string if resume is not possible.
     */
    std::string GetResumeLabel(const CFileItem& item) const;

    /*!
     * @brief Resume a previously not completely played recording.
     * @param item containing a recording or an epg tag.
     * @param bFallbackToPlay controls whether playback of the recording should be started at the beginning ig no resume data are available.
     * @return true on success, false otherwise.
     */
    bool ResumePlayRecording(const std::shared_ptr<CFileItem>& item, bool bFallbackToPlay) const;

    /*!
     * @brief Play recording.
     * @param item containing a recording or an epg tag.
     * @param bCheckResume controls resume check.
     * @return true on success, false otherwise.
     */
    bool PlayRecording(const std::shared_ptr<CFileItem>& item, bool bCheckResume) const;

    /*!
     * @brief Play EPG tag.
     * @param item containing an epg tag.
     * @return true on success, false otherwise.
     */
    bool PlayEpgTag(const std::shared_ptr<CFileItem>& item) const;

    /*!
     * @brief Switch channel.
     * @param item containing a channel or an epg tag.
     * @param bCheckResume controls resume check in case a recording for the current epg event is present.
     * @return true on success, false otherwise.
     */
    bool SwitchToChannel(const std::shared_ptr<CFileItem>& item, bool bCheckResume) const;

    /*!
     * @brief Playback the given file item.
     * @param item containing a channel or a recording.
     * @return True if the item could be played, false otherwise.
     */
    bool PlayMedia(const std::shared_ptr<CFileItem>& item) const;

    /*!
     * @brief Start playback of the last played channel, and if there is none, play first channel in the current channelgroup.
     * @param type The type of playback to be started (any, radio, tv). See PlaybackType enum
     * @return True if playback was started, false otherwise.
     */
    bool SwitchToChannel(PlaybackType type) const;

    /*!
     * @brief Plays the last played channel or the first channel of TV or Radio on startup.
     * @return True if playback was started, false otherwise.
     */
    bool PlayChannelOnStartup() const;

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
     * @brief Seek to the start of the next epg event in timeshift buffer, relative to the currently playing event.
     *        If there is no next event, seek to the end of the currently playing event (to the 'live' position).
     */
    void SeekForward();

    /*!
     * @brief Seek to the start of the previous epg event in timeshift buffer, relative to the currently playing event
     *        or if there is no previous event or if playback time is greater than given threshold, seek to the start
     *        of the playing event.
     * @param iThreshold the value in seconds to trigger seek to start of current event instead of start of previous event.
     */
    void SeekBackward(unsigned int iThreshold);

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

    /*!
     * @brief Execute a saved search. Displays result in search window if it is open.
     * @param item The item containing a search filter.
     * @return True on success, false otherwise.
     */
    bool ExecuteSavedSearch(const std::shared_ptr<CFileItem>& item);

    /*!
     * @brief Edit a saved search. Opens the search dialog.
     * @param item The item containing a search filter.
     * @return True on success, false otherwise.
     */
    bool EditSavedSearch(const std::shared_ptr<CFileItem>& item);

    /*!
     * @brief Rename a saved search. Opens a title input dialog.
     * @param item The item containing a search filter.
     * @return True on success, false otherwise.
     */
    bool RenameSavedSearch(const std::shared_ptr<CFileItem>& item);

    /*!
     * @brief Delete a saved search. Opens confirmation dialog before deleting.
     * @param item The item containing a search filter.
     * @return True on success, false otherwise.
     */
    bool DeleteSavedSearch(const std::shared_ptr<CFileItem>& item);

  private:
    CPVRGUIActions(const CPVRGUIActions&) = delete;
    CPVRGUIActions const& operator=(CPVRGUIActions const&) = delete;

    /*!
     * @brief Check whether resume play is possible for a given item, display "resume from ..."/"play from start" context menu in case.
     * @param item containing a recording or an epg tag.
     * @return true, to play/resume the item, false otherwise.
     */
    bool CheckResumeRecording(const std::shared_ptr<CFileItem>& item) const;

    /*!
     * @brief Check "play minimized" settings value and switch to fullscreen if not set.
     * @param bFullscreen switch to fullscreen or set windowed playback.
     */
    void CheckAndSwitchToFullscreen(bool bFullscreen) const;

    /*!
     * @brief Start playback of the given item.
     * @param bFullscreen start playback fullscreen or not.
     * @param epgProps properties to be used instead of calling to the client if supplied.
     * @param item containing a channel or a recording.
     */
    void StartPlayback(CFileItem* item,
                       bool bFullscreen,
                       const CPVRStreamProperties* epgProps = nullptr) const;

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
