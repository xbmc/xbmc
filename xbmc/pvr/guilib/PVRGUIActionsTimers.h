/*
 *  Copyright (C) 2016-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/IPVRComponent.h"
#include "pvr/settings/PVRSettings.h"

#include <memory>

class CFileItem;

namespace PVR
{
class CPVRChannel;
class CPVRTimerInfoTag;

class CPVRGUIActionsTimers : public IPVRComponent
{
public:
  CPVRGUIActionsTimers();
  ~CPVRGUIActionsTimers() override = default;

  /*!
   * @brief Open the timer settings dialog to create a new tv or radio timer.
   * @param bRadio indicates whether a radio or tv timer shall be created.
   * @return true on success, false otherwise.
   */
  bool AddTimer(bool bRadio) const;

  /*!
   * @brief Create a new timer, either interactive or non-interactive.
   * @param item containing epg data to create a timer for. item must be an epg tag or a channel.
   * @param bShowTimerSettings is used to control whether a settings dialog will be opened prior
   * creating the timer.
   * @return true, if the timer was created successfully, false otherwise.
   */
  bool AddTimer(const CFileItem& item, bool bShowTimerSettings) const;

  /*!
   * @brief Add a timer to the client. Doesn't add the timer to the container. The backend will
   * do this.
   * @return True if it was sent correctly, false if not.
   */
  bool AddTimer(const std::shared_ptr<CPVRTimerInfoTag>& item) const;

  /*!
   * @brief Create a new timer rule, either interactive or non-interactive.
   * @param item containing epg data to create a timer rule for. item must be an epg tag or a
   * channel.
   * @param bShowTimerSettings is used to control whether a settings dialog will be opened prior
   * creating the timer rule.
   * @param bFallbackToOneShotTimer if no timer rule can be created, try to create a one-shot
   * timer instead.
   * @return true, if the timer rule was created successfully, false otherwise.
   */
  bool AddTimerRule(const CFileItem& item,
                    bool bShowTimerSettings,
                    bool bFallbackToOneShotTimer) const;

  /*!
   * @brief Creates or deletes a timer for the given epg tag.
   * @param item containing an epg tag.
   * @return true on success, false otherwise.
   */
  bool ToggleTimer(const CFileItem& item) const;

  /*!
   * @brief Toggles a given timer's enabled/disabled state.
   * @param item containing a timer.
   * @return true on success, false otherwise.
   */
  bool ToggleTimerState(const CFileItem& item) const;

  /*!
   * @brief Open the timer settings dialog to edit an existing timer.
   * @param item containing an epg tag or a timer.
   * @return true on success, false otherwise.
   */
  bool EditTimer(const CFileItem& item) const;

  /*!
   * @brief Open the timer settings dialog to edit an existing timer rule.
   * @param item containing an epg tag or a timer.
   * @return true on success, false otherwise.
   */
  bool EditTimerRule(const CFileItem& item) const;

  /*!
   * @brief Get the timer rule for a given timer
   * @param item containing an item to query the timer rule for. item must be a timer or an epg tag.
   * @return The timer rule item, or nullptr if none was found.
   */
  std::shared_ptr<CFileItem> GetTimerRule(const CFileItem& item) const;

  /*!
   * @brief Delete a timer, always showing a confirmation dialog.
   * @param item containing a timer to delete. item must be a timer, an epg tag or a channel.
   * @return true, if the timer was deleted successfully, false otherwise.
   */
  bool DeleteTimer(const CFileItem& item) const;

  /*!
   * @brief Delete a timer rule, always showing a confirmation dialog.
   * @param item containing a timer rule to delete. item must be a timer, an epg tag or a channel.
   * @return true, if the timer rule was deleted successfully, false otherwise.
   */
  bool DeleteTimerRule(const CFileItem& item) const;

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
  bool SetRecordingOnChannel(const std::shared_ptr<CPVRChannel>& channel, bool bOnOff);

  /*!
   * @brief Stop a currently active recording, always showing a confirmation dialog.
   * @param item containing a recording to stop. item must be a timer, an epg tag or a channel.
   * @return true, if the recording was stopped successfully, false otherwise.
   */
  bool StopRecording(const CFileItem& item) const;

  /*!
   * @brief Create a new reminder timer, non-interactive.
   * @param item containing epg data to create a reminder timer for. item must be an epg tag.
   * @return true, if the timer was created successfully, false otherwise.
   */
  bool AddReminder(const CFileItem& item) const;

  /*!
   * @brief Announce due reminders, if any.
   */
  void AnnounceReminders() const;

private:
  CPVRGUIActionsTimers(const CPVRGUIActionsTimers&) = delete;
  CPVRGUIActionsTimers const& operator=(CPVRGUIActionsTimers const&) = delete;

  /*!
   * @brief Open the timer settings dialog.
   * @param timer containing the timer the settings shall be displayed for.
   * @return true, if the dialog was ended successfully, false otherwise.
   */
  bool ShowTimerSettings(const std::shared_ptr<CPVRTimerInfoTag>& timer) const;

  /*!
   * @brief Add a timer or timer rule, either interactive or non-interactive.
   * @param item containing epg data to create a timer or timer rule for. item must be an epg tag
   * or a channel.
   * @param bCreateteRule denotes whether to create a one-shot timer or a timer rule.
   * @param bShowTimerSettings is used to control whether a settings dialog will be opened prior
   * creating the timer or timer rule.
   * @param bFallbackToOneShotTimer if bCreateteRule is true and no timer rule can be created, try
   * to create a one-shot timer instead.
   * @return true, if the timer or timer rule was created successfully, false otherwise.
   */
  bool AddTimer(const CFileItem& item,
                bool bCreateRule,
                bool bShowTimerSettings,
                bool bFallbackToOneShotTimer) const;

  /*!
   * @brief Delete a timer or timer rule, always showing a confirmation dialog.
   * @param item containing a timer or timer rule to delete. item must be a timer, an epg tag or
   * a channel.
   * @param bIsRecording denotes whether the timer is currently recording (controls correct
   * confirmation dialog).
   * @param bDeleteRule denotes to delete a timer rule. For convenience, one can pass a timer
   * created by a rule.
   * @return true, if the timer or timer rule was deleted successfully, false otherwise.
  */
  bool DeleteTimer(const CFileItem& item, bool bIsRecording, bool bDeleteRule) const;

  /*!
   * @brief Delete a timer or timer rule, showing a confirmation dialog in case a timer currently
   * recording shall be deleted.
   * @param timer containing a timer or timer rule to delete.
   * @param bIsRecording denotes whether the timer is currently recording (controls correct
   * confirmation dialog).
   * @param bDeleteRule denotes to delete a timer rule. For convenience, one can pass a timer
   * created by a rule.
   * @return true, if the timer or timer rule was deleted successfully, false otherwise.
   */
  bool DeleteTimer(const std::shared_ptr<CPVRTimerInfoTag>& timer,
                   bool bIsRecording,
                   bool bDeleteRule) const;

  /*!
   * @brief Open a dialog to confirm timer delete.
   * @param timer the timer to delete.
   * @param bDeleteRule in: ignored. out, for one shot timer scheduled by a timer rule: true to
   * also delete the timer rule that has scheduled this timer, false to only delete the one shot
   * timer. out, for one shot timer not scheduled by a timer rule: ignored
   * @return true, to proceed with delete, false otherwise.
   */
  bool ConfirmDeleteTimer(const std::shared_ptr<const CPVRTimerInfoTag>& timer,
                          bool& bDeleteRule) const;

  /*!
   * @brief Open a dialog to confirm stop recording.
   * @param timer the recording to stop (actually the timer to delete).
   * @return true, to proceed with delete, false otherwise.
   */
  bool ConfirmStopRecording(const std::shared_ptr<const CPVRTimerInfoTag>& timer) const;

  /*!
   * @brief Announce and process a reminder timer.
   * @param timer The reminder timer.
   */
  void AnnounceReminder(const std::shared_ptr<CPVRTimerInfoTag>& timer) const;

  CPVRSettings m_settings;
  mutable bool m_bReminderAnnouncementRunning{false};
};

namespace GUI
{
// pretty scope and name
using Timers = CPVRGUIActionsTimers;
} // namespace GUI

} // namespace PVR
