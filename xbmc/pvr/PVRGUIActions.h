/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>

#include "threads/CriticalSection.h"

#include "pvr/PVRChannelNumberInputHandler.h"
#include "pvr/PVRGUIChannelNavigator.h"
#include "pvr/PVRSettings.h"
#include "pvr/PVRTypes.h"

class CAction;
class CFileItem;
typedef std::shared_ptr<CFileItem> CFileItemPtr;

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

  class CPVRGUIActions
  {
  public:
    CPVRGUIActions();
    virtual ~CPVRGUIActions() = default;

    /*!
     * @brief Open a dialog with epg information for a given item.
     * @param item containing epg data to show. item must be an epg tag, a channel or a timer.
     * @return true on success, false otherwise.
     */
    bool ShowEPGInfo(const CFileItemPtr &item) const;

    /*!
     * @brief Open a dialog with the epg list for a given item.
     * @param item containing channel info. item must be an epg tag, a channel or a timer.
     * @return true on success, false otherwise.
     */
    bool ShowChannelEPG(const CFileItemPtr &item) const;

    /*!
     * @brief Open a window containing a list of epg tags 'similar' to a given item.
     * @param item containing epg data for matching. item must be an epg tag, a channel or a recording.
     * @return true on success, false otherwise.
     */
    bool FindSimilar(const std::shared_ptr<CFileItem>& item) const;

    /*!
     * @brief Open the timer settings dialog to create a new tv or radio timer.
     * @param bRadio indicates whether a radio or tv timer shall be created.
     * @return true on success, false otherwise.
     */
    bool AddTimer(bool bRadio) const;

    /*!
     * @brief Create a new timer, either interactive or non-interactive.
     * @param item containing epg data to create a timer for. item must be an epg tag or a channel.
     * @param bShowTimerSettings is used to control whether a settings dialog will be opened prior creating the timer.
     * @return true, if the timer was created successfully, false otherwise.
     */
    bool AddTimer(const CFileItemPtr &item, bool bShowTimerSettings) const;

    /*!
     * @brief Add a timer to the client. Doesn't add the timer to the container. The backend will do this.
     * @return True if it was sent correctly, false if not.
     */
    bool AddTimer(const CPVRTimerInfoTagPtr &item) const;

    /*!
     * @brief Create a new timer rule, either interactive or non-interactive.
     * @param item containing epg data to create a timer rule for. item must be an epg tag or a channel.
     * @param bShowTimerSettings is used to control whether a settings dialog will be opened prior creating the timer rule.
     * @return true, if the timer rule was created successfully, false otherwise.
     */
    bool AddTimerRule(const CFileItemPtr &item, bool bShowTimerSettings) const;

    /*!
     * @brief Creates or deletes a timer for the given epg tag.
     * @param item containing an epg tag.
     * @return true on success, false otherwise.
     */
    bool ToggleTimer(const CFileItemPtr &item) const;

    /*!
     * @brief Toggles a given timer's enabled/disabled state.
     * @param item containing a timer.
     * @return true on success, false otherwise.
     */
    bool ToggleTimerState(const CFileItemPtr &item) const;

    /*!
     * @brief Open the timer settings dialog to edit an existing timer.
     * @param item containing an epg tag or a timer.
     * @return true on success, false otherwise.
     */
    bool EditTimer(const CFileItemPtr &item) const;

    /*!
     * @brief Open the timer settings dialog to edit an existing timer rule.
     * @param item containing an epg tag or a timer.
     * @return true on success, false otherwise.
     */
    bool EditTimerRule(const CFileItemPtr &item) const;

    /*!
     * @brief Rename a timer, showing a text input dialog.
     * @param item containing a timer to rename.
     * @return true, if the timer was renamed successfully, false otherwise.
     */
    bool RenameTimer(const CFileItemPtr &item) const;

    /*!
     * @brief Delete a timer, always showing a confirmation dialog.
     * @param item containing a timer to delete. item must be a timer, an epg tag or a channel.
     * @return true, if the timer was deleted successfully, false otherwise.
     */
    bool DeleteTimer(const CFileItemPtr &item) const;

    /*!
     * @brief Delete a timer rule, always showing a confirmation dialog.
     * @param item containing a timer rule to delete. item must be a timer, an epg tag or a channel.
     * @return true, if the timer rule was deleted successfully, false otherwise.
     */
    bool DeleteTimerRule(const CFileItemPtr &item) const;

    /*!
     * @brief Open a dialog with information for a given recording.
     * @param item containing a recording.
     * @return true on success, false otherwise.
     */
    bool ShowRecordingInfo(const CFileItemPtr &item) const;

    /*!
     * @brief Toggle recording on the currently playing channel, if any.
     * @return True if the recording was started or stopped successfully, false otherwise.
     */
    bool ToggleRecordingOnPlayingChannel();

    /*!
     * @brief Start or stop recording on a given channel.
     * @param channel the channel to start/stop recording.
     * @param bOnOff True to start recording, false to stop.
     * @return True if the recording was started or stopped successfully, false otherwise.
     */
    bool SetRecordingOnChannel(const CPVRChannelPtr &channel, bool bOnOff);

    /*!
     * @brief Stop a currently active recording, always showing a confirmation dialog.
     * @param item containing a recording to stop. item must be a timer, an epg tag or a channel.
     * @return true, if the recording was stopped successfully, false otherwise.
     */
    bool StopRecording(const CFileItemPtr &item) const;

    /*!
     * @brief Open the recording settings dialog to edit a recording.
     * @param item containing the recording to edit.
     * @return true on success, false otherwise.
     */
    bool EditRecording(const CFileItemPtr &item) const;

    /*!
     * @brief Check if any recording settings can be edited.
     * @param item containing the recording to edit.
     * @return true on success, false otherwise.
     */
    bool CanEditRecording(const CFileItem& item) const;

    /*!
     * @brief Rename a recording, showing a text input dialog.
     * @param item containing a recording to rename.
     * @return true, if the recording was renamed successfully, false otherwise.
     */
    bool RenameRecording(const CFileItemPtr &item) const;

    /*!
     * @brief Delete a recording, always showing a confirmation dialog.
     * @param item containing a recording to delete.
     * @return true, if the recording was deleted successfully, false otherwise.
     */
    bool DeleteRecording(const CFileItemPtr &item) const;

    /*!
     * @brief Delete all recordings from trash, always showing a confirmation dialog.
     * @return true, if the recordings were permanently deleted successfully, false otherwise.
     */
    bool DeleteAllRecordingsFromTrash() const;

    /*!
     * @brief Undelete a recording.
     * @param item containing a recording to undelete.
     * @return true, if the recording was undeleted successfully, false otherwise.
     */
    bool UndeleteRecording(const CFileItemPtr &item) const;

    /*!
     * @brief Get a localized resume play label, if the given item can be resumed.
     * @param item containing a recording or an epg tag.
     * @return the localized resume play label that can be used for instance as context menu item label or an empty string if resume is not possible.
     */
    std::string GetResumeLabel(const CFileItem &item) const;

    /*!
     * @brief Resume a previously not completely played recording.
     * @param item containing a recording or an epg tag.
     * @param bFallbackToPlay controls whether playback of the recording should be started at the beginning ig no resume data are available.
     * @return true on success, false otherwise.
     */
    bool ResumePlayRecording(const CFileItemPtr &item, bool bFallbackToPlay) const;

    /*!
     * @brief Play recording.
     * @param item containing a recording or an epg tag.
     * @param bCheckResume controls resume check.
     * @return true on success, false otherwise.
     */
    bool PlayRecording(const CFileItemPtr &item, bool bCheckResume) const;

    /*!
     * @brief Play EPG tag.
     * @param item containing an epg tag.
     * @return true on success, false otherwise.
     */
    bool PlayEpgTag(const CFileItemPtr &item) const;

    /*!
     * @brief Switch channel.
     * @param item containing a channel or an epg tag.
     * @param bCheckResume controls resume check in case a recording for the current epg event is present.
     * @return true on success, false otherwise.
     */
    bool SwitchToChannel(const CFileItemPtr &item, bool bCheckResume) const;

    /*!
     * @brief Playback the given file item.
     * @param item containing a channel or a recording.
     * @return True if the item could be played, false otherwise.
     */
    bool PlayMedia(const CFileItemPtr &item) const;

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
    bool HideChannel(const CFileItemPtr &item) const;

    /*!
     * @brief Open a selection dialog and start a channel scan on the selected client.
     * @return true on success, false otherwise.
     */
    bool StartChannelScan();

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
    ParentalCheckResult CheckParentalLock(const CPVRChannelPtr &channel) const;

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
    void SetSelectedItemPath(bool bRadio, const std::string &path);

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
     * @brief Get the currently active channel number input handler.
     * @return the handler.
     */
    CPVRChannelNumberInputHandler &GetChannelNumberInputHandler();

    /*!
     * @brief Get the channel navigator.
     * @return the navigator.
     */
    CPVRGUIChannelNavigator &GetChannelNavigator();

    /*!
     * @brief Inform GUI actions that playback of an item just started.
     * @param item The item that started to play.
     */
    void OnPlaybackStarted(const CFileItemPtr &item);

    /*!
     * @brief Inform GUI actions that playback of an item was stopped due to user interaction.
     * @param item The item that stopped to play.
     */
    void OnPlaybackStopped(const CFileItemPtr &item);

  private:
    CPVRGUIActions(const CPVRGUIActions&) = delete;
    CPVRGUIActions const& operator=(CPVRGUIActions const&) = delete;

    /*!
     * @brief Open the timer settings dialog.
     * @param timer containing the timer the settings shall be displayed for.
     * @return true, if the dialog was ended successfully, false otherwise.
     */
    bool ShowTimerSettings(const CPVRTimerInfoTagPtr &timer) const;

    /*!
     * @brief Add a timer or timer rule, either interactive or non-interactive.
     * @param item containing epg data to create a timer or timer rule for. item must be an epg tag or a channel.
     * @param bCreateteRule denotes whether to create a one-shot timer or a timer rule.
     * @param bShowTimerSettings is used to control whether a settings dialog will be opened prior creating the timer or timer rule.
     * @return true, if the timer or timer rule was created successfully, false otherwise.
     */
    bool AddTimer(const CFileItemPtr &item, bool bCreateRule, bool bShowTimerSettings) const;

    /*!
     * @brief Delete a timer or timer rule, always showing a confirmation dialog.
     * @param item containing a timer or timer rule to delete. item must be a timer, an epg tag or a channel.
     * @param bIsRecording denotes whether the timer is currently recording (controls correct confirmation dialog).
     * @param bDeleteRule denotes to delete a timer rule. For convenience, one can pass a timer created by a rule.
     * @return true, if the timer or timer rule was deleted successfully, false otherwise.
    */
    bool DeleteTimer(const CFileItemPtr &item, bool bIsRecording, bool bDeleteRule) const;

    /*!
     * @brief Delete a timer or timer rule, showing a confirmation dialog in case a timer currently recording shall be deleted.
     * @param timer containing a timer or timer rule to delete.
     * @param bIsRecording denotes whether the timer is currently recording (controls correct confirmation dialog).
     * @param bDeleteRule denotes to delete a timer rule. For convenience, one can pass a timer created by a rule.
     * @return true, if the timer or timer rule was deleted successfully, false otherwise.
     */
    bool DeleteTimer(const CPVRTimerInfoTagPtr &timer, bool bIsRecording, bool bDeleteRule) const;

    /*!
     * @brief Open a dialog to confirm timer delete.
     * @param timer the timer to delete.
     * @param bDeleteRule in: ignored
     *                    out, for one shot timer scheduled by a timer rule: true to also delete the timer
     *                         rule that has scheduled this timer, false to only delete the one shot timer.
     *                    out, for one shot timer not scheduled by a timer rule: ignored
     * @return true, to proceed with delete, false otherwise.
     */
    bool ConfirmDeleteTimer(const CPVRTimerInfoTagPtr &timer, bool &bDeleteRule) const;

    /*!
     * @brief Open a dialog to confirm stop recording.
     * @param timer the recording to stop (actually the timer to delete).
     * @return true, to proceed with delete, false otherwise.
     */
    bool ConfirmStopRecording(const CPVRTimerInfoTagPtr &timer) const;

    /*!
     * @brief Open a dialog to confirm to delete a recording.
     * @param item the recording to delete.
     * @return true, to proceed with delete, false otherwise.
     */
    bool ConfirmDeleteRecording(const CFileItemPtr &item) const;

    /*!
     * @brief Open a dialog to confirm to permanently remove all deleted recordings.
     * @return true, to proceed with delete, false otherwise.
     */
    bool ConfirmDeleteAllRecordingsFromTrash() const;

    /*!
     * @brief Open the recording settings dialog.
     * @param recording containing the recording the settings shall be displayed for.
     * @return true, if the dialog was ended successfully, false otherwise.
     */
    bool ShowRecordingSettings(const CPVRRecordingPtr &recording) const;

    /*!
     * @brief Check whether resume play is possible for a given item, display "resume from ..."/"play from start" context menu in case.
     * @param item containing a recording or an epg tag.
     * @return true, to play/resume the item, false otherwise.
     */
    bool CheckResumeRecording(const CFileItemPtr &item) const;

    /*!
     * @brief Check "play minimized" settings value and switch to fullscreen if not set.
     * @param bFullscreen switch to fullscreen or set windowed playback.
     */
    void CheckAndSwitchToFullscreen(bool bFullscreen) const;

    /*!
     * @brief Start playback of the given item.
     * @param bFullscreen start playback fullscreen or not.
     * @param item containing a channel or a recording.
     */
    void StartPlayback(CFileItem *item, bool bFullscreen) const;

    bool AllLocalBackendsIdle(CPVRTimerInfoTagPtr& causingEvent) const;
    bool EventOccursOnLocalBackend(const CFileItemPtr& item) const;
    bool IsNextEventWithinBackendIdleTime(void) const;

    mutable CCriticalSection m_critSection;
    CPVRChannelSwitchingInputHandler m_channelNumberInputHandler;
    bool m_bChannelScanRunning = false;
    CPVRSettings m_settings;
    CPVRGUIChannelNavigator m_channelNavigator;
    std::string m_selectedItemPathTV;
    std::string m_selectedItemPathRadio;
  };

} // namespace PVR
