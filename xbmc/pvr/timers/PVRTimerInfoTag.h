/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "pvr/timers/PVRTimerType.h"
#include "threads/CriticalSection.h"
#include "utils/ISerializable.h"

#include <memory>
#include <string>

struct PVR_TIMER;

namespace PVR
{
class CPVRChannel;
class CPVREpgInfoTag;

enum class TimerOperationResult
{
  OK = 0,
  FAILED,
  RECORDING // The timer was not deleted because it is currently recording (see DeleteTimer).
};

class CPVRTimerInfoTag final : public ISerializable
{
  // allow these classes direct access to members as they act as timer tag instance factories.
  friend class CGUIDialogPVRTimerSettings;
  friend class CPVRDatabase;

public:
  explicit CPVRTimerInfoTag(bool bRadio = false);
  CPVRTimerInfoTag(const PVR_TIMER& timer,
                   const std::shared_ptr<CPVRChannel>& channel,
                   unsigned int iClientId);

  bool operator==(const CPVRTimerInfoTag& right) const;
  bool operator!=(const CPVRTimerInfoTag& right) const;

  /*!
   * @brief Copy over data to the given PVR_TIMER instance.
   * @param timer The timer instance to fill.
   */
  void FillAddonData(PVR_TIMER& timer) const;

  // ISerializable implementation
  void Serialize(CVariant& value) const override;

  static constexpr int DEFAULT_PVRRECORD_INSTANTRECORDTIME = -1;

  /*!
   * @brief create a tag for an instant timer for a given channel
   * @param channel is the channel the instant timer is to be created for
   * @param iDuration is the duration for the instant timer, DEFAULT_PVRRECORD_INSTANTRECORDTIME
   * denotes system default (setting value)
   * @return the timer or null if timer could not be created
   */
  static std::shared_ptr<CPVRTimerInfoTag> CreateInstantTimerTag(
      const std::shared_ptr<CPVRChannel>& channel,
      int iDuration = DEFAULT_PVRRECORD_INSTANTRECORDTIME);

  /*!
   * @brief create a tag for a timer for a given channel at given time for given duration
   * @param channel is the channel the timer is to be created for
   * @param start is the start time for the recording
   * @param iDuration is the duration of the recording
   * @return the timer or null if timer could not be created
   */
  static std::shared_ptr<CPVRTimerInfoTag> CreateTimerTag(
      const std::shared_ptr<CPVRChannel>& channel, const CDateTime& start, int iDuration);

  /*!
   * @brief create a recording or reminder timer or timer rule for the given epg info tag.
   * @param tag the epg info tag
   * @param bCreateRule if true, create a timer rule, create a one shot timer otherwise
   * @param bCreateReminder if true, create a reminder timer or rule, create a recording timer
   * or rule otherwise
   * @param bReadOnly whether the timer/rule is read only
   * @return the timer or null if timer could not be created
   */
  static std::shared_ptr<CPVRTimerInfoTag> CreateFromEpg(const std::shared_ptr<CPVREpgInfoTag>& tag,
                                                         bool bCreateRule,
                                                         bool bCreateReminder,
                                                         bool bReadOnly = false);

  /*!
   * @brief create a timer or timer rule for the given epg info tag.
   * @param tag the epg info tag
   * @param bCreateRule if true, create a timer rule, create a one shot timer otherwise
   * @return the timer or null if timer could not be created
   */
  static std::shared_ptr<CPVRTimerInfoTag> CreateFromEpg(const std::shared_ptr<CPVREpgInfoTag>& tag,
                                                         bool bCreateRule = false);

  /*!
   * @brief create a reminder timer for the given start date.
   * @param start the start date
   * @param iDuration the duration the reminder is valid
   * @param parent If non-zero, the new timer will be made a child of the given timer rule
   * @return the timer or null if timer could not be created
   */
  static std::shared_ptr<CPVRTimerInfoTag> CreateReminderFromDate(
      const CDateTime& start,
      int iDuration,
      const std::shared_ptr<CPVRTimerInfoTag>& parent = std::shared_ptr<CPVRTimerInfoTag>());

  /*!
   * @brief create a reminder timer for the given epg info tag.
   * @param tag the epg info tag
   * @param parent If non-zero, the new timer will be made a child of the given timer rule
   * @return the timer or null if timer could not be created
   */
  static std::shared_ptr<CPVRTimerInfoTag> CreateReminderFromEpg(
      const std::shared_ptr<CPVREpgInfoTag>& tag,
      const std::shared_ptr<CPVRTimerInfoTag>& parent = std::shared_ptr<CPVRTimerInfoTag>());

  /*!
   * @brief Associate the given epg tag with this timer.
   * @param tag The epg tag to assign.
   */
  void SetEpgInfoTag(const std::shared_ptr<CPVREpgInfoTag>& tag);

  /*!
   * @brief get the epg info tag associated with this timer, if any
   * @param bCreate if true, try to find the epg tag if not yet set (lazy evaluation)
   * @return the epg info tag associated with this timer or null if there is no tag
   */
  std::shared_ptr<CPVREpgInfoTag> GetEpgInfoTag(bool bCreate = true) const;

  /*!
   * @brief updates this timer excluding the state of any children.
   * @param tag A timer containing the data that shall be merged into this timer's data.
   * @return true if the timer was updated successfully
   */
  bool UpdateEntry(const std::shared_ptr<const CPVRTimerInfoTag>& tag);

  /*!
   * @brief merge in the state of this child timer.
   * @param childTimer The child timer
   * @param bAdd If true, add child's data to parent's state, otherwise subtract.
   * @return true if the child timer's state was merged successfully
   */
  bool UpdateChildState(const std::shared_ptr<const CPVRTimerInfoTag>& childTimer, bool bAdd);

  /*!
   * @brief reset the state of children related to this timer.
   */
  void ResetChildState();

  /*!
   * @brief Whether this timer is active.
   * @return True if this timer is active, false otherwise.
   */
  bool IsActive() const
  {
    return m_state == PVR_TIMER_STATE_SCHEDULED || m_state == PVR_TIMER_STATE_RECORDING ||
           m_state == PVR_TIMER_STATE_CONFLICT_OK || m_state == PVR_TIMER_STATE_CONFLICT_NOK ||
           m_state == PVR_TIMER_STATE_ERROR;
  }

  /*!
   * @brief Whether this timer is broken.
   * @return True if this timer won't result in a recording because it is broken for some reason,
   * false otherwise
   */
  bool IsBroken() const
  {
    return m_state == PVR_TIMER_STATE_CONFLICT_NOK || m_state == PVR_TIMER_STATE_ERROR;
  }

  /*!
   * @brief Whether this timer has a conflict.
   * @return True if this timer won't result in a recording because it is in conflict with another
   * timer or live stream, false otherwise.
   */
  bool HasConflict() const { return m_state == PVR_TIMER_STATE_CONFLICT_NOK; }

  /*!
   * @brief Whether this timer is currently recording.
   * @return True if recording, false otherwise.
   */
  bool IsRecording() const { return m_state == PVR_TIMER_STATE_RECORDING; }

  /*!
   * @brief Whether this timer is disabled, for example by the user.
   * @return True if disabled, false otherwise.
   */
  bool IsDisabled() const { return m_state == PVR_TIMER_STATE_DISABLED; }

  /*!
   * @brief Gets the type of this timer.
   * @return the timer type or NULL if this tag has no timer type.
   */
  const std::shared_ptr<CPVRTimerType> GetTimerType() const { return m_timerType; }

  /*!
   * @brief Sets the type of this timer.
   * @param the new timer type.
   */
  void SetTimerType(const std::shared_ptr<CPVRTimerType>& type);

  /*!
   * @brief Checks whether this is a timer rule (vs. one time timer).
   * @return True if this is a timer rule, false otherwise.
   */
  bool IsTimerRule() const { return m_timerType && m_timerType->IsTimerRule(); }

  /*!
   * @brief Checks whether this is a reminder timer (vs. recording timer).
   * @return True if this is a reminder timer, false otherwise.
   */
  bool IsReminder() const { return m_timerType && m_timerType->IsReminder(); }

  /*!
   * @brief Checks whether this is a manual (vs. epg-based) timer.
   * @return True if this is a manual timer, false otherwise.
   */
  bool IsManual() const { return m_timerType && m_timerType->IsManual(); }

  /*!
   * @brief Checks whether this is an epg-based (vs. manual) timer.
   * @return True if this is an epg-Based timer, false otherwise.
   */
  bool IsEpgBased() const { return !IsManual(); }

  /*!
   * @brief The ID of the client for this timer.
   * @return The client ID or -1  if this is a local timer.
   */
  int ClientID() const { return m_iClientId; }

  /*!
   * @brief Check, whether this timer is owned by a pvr client or by Kodi.
   * @return True, if owned by a pvr client, false otherwise.
   */
  bool IsOwnedByClient() const;

  /*!
   * @brief Whether this timer is for Radio or TV.
   * @return True if Radio, false otherwise.
   */
  bool IsRadio() const { return m_bIsRadio; }

  /*!
   * @brief The path that identifies this timer.
   * @return The path.
   */
  const std::string& Path() const;

  /*!
   * @brief The index for this timer, as given by the client.
   * @return The client index or PVR_TIMER_NO_CLIENT_INDEX if the timer was just created locally
   * by Kodi and was not yet added by the client.
   */
  int ClientIndex() const { return m_iClientIndex; }

  /*!
   * @brief The index for the parent of this timer, as given by the client. Timers scheduled by a
   * timer rule will have a parant index != PVR_TIMER_NO_PARENT.
   * @return The client index or PVR_TIMER_NO_PARENT if the timer has no parent.
   */
  int ParentClientIndex() const { return m_iParentClientIndex; }

  /*!
   * @brief Whether this timer has a parent.
   * @return True if timer has a parent, false otherwise.
   */
  bool HasParent() const { return m_iParentClientIndex != PVR_TIMER_NO_PARENT; }

  /*!
   * @brief The local ID for this timer, as given by Kodi.
   * @return The ID or 0 if not yet set.
   */
  unsigned int TimerID() const { return m_iTimerId; }

  /*!
   * @brief Set the local ID for this timer.
   * @param id The ID to set.
   */
  void SetTimerID(unsigned int id) { m_iTimerId = id; }

  /*!
   * @brief The UID of the channel for this timer.
   * @return The channel UID or PVR_CHANNEL_INVALID_UID if not available.
   */
  int ClientChannelUID() const { return m_iClientChannelUid; }

  /*!
   * @brief The state for this timer.
   * @return The state.
   */
  PVR_TIMER_STATE State() const { return m_state; }

  /*!
   * @brief Set the state for this timer.
   * @param state The state to set.
   */
  void SetState(PVR_TIMER_STATE state) { m_state = state; }

  /*!
   * @brief The title for this timer.
   * @return The title.
   */
  const std::string& Title() const;

  /*!
   * @brief Check whether this timer has an associated channel.
   * @return True if this timer has a channel set, false otherwise.
   */
  bool HasChannel() const;

  /*!
   * @brief Get the channel associated with this timer, if any.
   * @return the channel or null if non is associated with this timer.
   */
  std::shared_ptr<CPVRChannel> Channel() const;

  /*!
   * @brief Update the channel associated with this timer, based on current client ID and
   * channel UID.
   */
  void UpdateChannel();

  /*!
   * @brief The name of the channel associated with this timer, if any.
   * @return The channel name.
   */
  std::string ChannelName() const;

  /*!
   * @brief The path for the channel icon associated with this timer, if any.
   * @return The channel icon path.
   */
  std::string ChannelIcon() const;

  /*!
   * @brief The start date and time for this timer, as UTC.
   * @return The start date and time.
   */
  CDateTime StartAsUTC() const;

  /*!
   * @brief The start date and time for this timer, as local time.
   * @return The start date and time.
   */
  CDateTime StartAsLocalTime() const;

  /*!
   * @brief Set the start date and time from a CDateTime instance carrying the data as UTC.
   * @param start The start date and time as UTC.
   */
  void SetStartFromUTC(const CDateTime& start);

  /*!
   * @brief Set the start date and time from a CDateTime instance carrying the data as local time.
   * @param start The start date and time as local time.
   */
  void SetStartFromLocalTime(const CDateTime& start);

  /*!
   * @brief The end date and time for this timer, as UTC.
   * @return The start date and time.
   */
  CDateTime EndAsUTC() const;

  /*!
   * @brief The end date and time for this timer, as local time.
   * @return The start date and time.
   */
  CDateTime EndAsLocalTime() const;

  /*!
   * @brief Set the end date and time from a CDateTime instance carrying the data as UTC.
   * @param start The end date and time as UTC.
   */
  void SetEndFromUTC(const CDateTime& end);

  /*!
   * @brief Set the end date and time from a CDateTime instance carrying the data as local time.
   * @param start The end date and time as local time.
   */
  void SetEndFromLocalTime(const CDateTime& end);

  /*!
   * @brief The first day for this timer, as UTC.
   * @return The first day.
   */
  CDateTime FirstDayAsUTC() const;

  /*!
   * @brief The first day for this timer, as local time.
   * @return The first day.
   */
  CDateTime FirstDayAsLocalTime() const;

  /*!
   * @brief Set the first dday from a CDateTime instance carrying the data as UTC.
   * @param start The first day as UTC.
   */
  void SetFirstDayFromUTC(const CDateTime& firstDay);

  /*!
   * @brief Set the first dday from a CDateTime instance carrying the data as local time.
   * @param start The first day as local time.
   */
  void SetFirstDayFromLocalTime(const CDateTime& firstDay);

  /*!
   * @brief Helper function to convert a given CDateTime containing data as UTC to local time.
   * @param utc A CDateTime instance carrying data as UTC.
   * @return A CDateTime instance carrying data as local time.
   */
  static CDateTime ConvertUTCToLocalTime(const CDateTime& utc);

  /*!
   * @brief Helper function to convert a given CDateTime containing data as local time to UTC.
   * @param local A CDateTime instance carrying data as local time.
   * @return A CDateTime instance carrying data as UTC.
   */
  static CDateTime ConvertLocalTimeToUTC(const CDateTime& local);

  /*!
   * @brief Get the duration of this timer in seconds, excluding padding times.
   * @return The duration.
   */
  int GetDuration() const;

  /*!
   * @brief Get time in minutes to start the recording before the start time of the programme.
   * @return The start padding time.
   */
  unsigned int MarginStart() const { return m_iMarginStart; }

  /*!
   * @brief Get time in minutes to end the recording after the end time of the programme.
   * @return The end padding time.
   */
  unsigned int MarginEnd() const { return m_iMarginEnd; }

  /*!
   * @brief For timer rules, the days of week this timer rule is scheduled for.
   * @return The days of week.
   */
  unsigned int WeekDays() const { return m_iWeekdays; }

  /*!
   * @brief For timer rules, whether start time is "any time", not a particular time.
   * @return True, if timer start is "any time", false otherwise.
   */
  bool IsStartAnyTime() const { return m_bStartAnyTime; }

  /*!
   * @brief For timer rules, whether end time is "any time", not a particular time.
   * @return True, if timer end is "any time", false otherwise.
   */
  bool IsEndAnyTime() const { return m_bEndAnyTime; }

  /*!
   * @brief For timer rules, whether only the EPG programme title shall be searched or also other
   * data like the programme's plot, if available.
   * @return True, if not only the programme's title shall be included in EPG search,
   * false otherwise.
   */
  bool IsFullTextEpgSearch() const { return m_bFullTextEpgSearch; }

  /*!
   * @brief For timer rules, the epg data match string for searches. Format is backend-dependent,
   * for example regexp.
   * @return The search string
   */
  const std::string& EpgSearchString() const { return m_strEpgSearchString; }

  /*!
   * @brief The series link for this timer.
   * @return The series link or empty string, if not available.
   */
  const std::string& SeriesLink() const;

  /*!
   * @brief Get the UID of the epg event associated with this timer tag, if any.
   * @return The UID or EPG_TAG_INVALID_UID.
   */
  unsigned int UniqueBroadcastID() const { return m_iEpgUid; }

  /*!
   * @brief Add this timer to the backend, transferring all local data of this timer to the backend.
   * @return True on success, false otherwise.
   */
  bool AddToClient() const;

  /*!
   * @brief Delete this timer on the backend.
   * @param bForce Control what to do in case the timer is currently recording.
   *        True to force to delete the timer, false to return TimerDeleteResult::RECORDING.
   * @return The result.
   */
  TimerOperationResult DeleteFromClient(bool bForce = false) const;

  /*!
   * @brief Update this timer on the backend, transferring all local data of this timer to
   * the backend.
   * @return True on success, false otherwise.
   */
  bool UpdateOnClient();

  /*!
   * @brief Persist this timer in the local database.
   * @return True on success, false otherwise.
   */
  bool Persist();

  /*!
   * @brief Delete this timer from the local database.
   * @return True on success, false otherwise.
   */
  bool DeleteFromDatabase();

  /*!
   * @brief GUI support: Get the text for the timer GUI notification.
   * @return The notification text.
   */
  std::string GetNotificationText() const;

  /*!
   * @brief GUI support: Get the text for the timer GUI notification when a timer has been deleted.
   * @return The notification text.
   */
  std::string GetDeletedNotificationText() const;

  /*!
   * @brief GUI support: Get the summary text for this timer, reflecting the timer schedule in a
   * human readable form.
   * @return The summary string.
   */
  const std::string& Summary() const;

  /*!
   * @brief GUI support: Update the summary text for this timer.
   */
  void UpdateSummary();

  /*!
   * @brief GUI support: Get the status text for this timer, reflecting its current state in a
   * human readable form.
   * @return The status string.
   */
  std::string GetStatus(bool bRadio) const;

  /*!
   * @brief GUI support: Get the timer string in a human readable form.
   * @return The type string.
   */
  std::string GetTypeAsString() const;

  /*!
   * @brief GUI support: Return string representation for any possible combination of weekdays.
   * @param iWeekdays weekdays as bit mask (0x01 = Mo, 0x02 = Tu, ...)
   * @param bEpgBased context is an epg-based timer
   * @param bLongMultiDaysFormat use long format. ("Mo-__-We-__-Fr-Sa-__" vs. "Mo-We-Fr-Sa")
   * @return The weekdays string representation.
   */
  static std::string GetWeekdaysString(unsigned int iWeekdays,
                                       bool bEpgBased,
                                       bool bLongMultiDaysFormat);

private:
  CPVRTimerInfoTag(const CPVRTimerInfoTag& tag) = delete;
  CPVRTimerInfoTag& operator=(const CPVRTimerInfoTag& orig) = delete;

  std::string GetWeekdaysString() const;
  void UpdateEpgInfoTag();

  static std::shared_ptr<CPVRTimerInfoTag> CreateFromDate(
      const std::shared_ptr<CPVRChannel>& channel,
      const CDateTime& start,
      int iDuration,
      bool bCreateReminder,
      bool bReadOnly);

  mutable CCriticalSection m_critSection;

  std::string m_strTitle; /*!< @brief name of this timer */
  std::string
      m_strEpgSearchString; /*!< @brief a epg data match string for epg-based timer rules. Format is backend-dependent, for example regexp */
  bool m_bFullTextEpgSearch =
      false; /*!< @brief indicates whether only epg episode title can be matched by the pvr backend or "more" (backend-dependent") data. */
  std::string m_strDirectory; /*!< @brief directory where the recording must be stored */
  std::string m_strSummary; /*!< @brief summary string with the time to show inside a GUI list */
  PVR_TIMER_STATE m_state = PVR_TIMER_STATE_SCHEDULED; /*!< @brief the state of this timer */
  int m_iClientId; /*!< @brief ID of the backend */
  int m_iClientIndex; /*!< @brief index number of the tag, given by the backend, PVR_TIMER_NO_CLIENT_INDEX for new */
  int m_iParentClientIndex; /*!< @brief for timers scheduled by a timer rule, the index number of the parent, given by the backend, PVR_TIMER_NO_PARENT for no parent */
  int m_iClientChannelUid; /*!< @brief channel uid */
  bool m_bStartAnyTime =
      false; /*!< @brief Ignore start date and time clock. Record at 'Any Time' */
  bool m_bEndAnyTime = false; /*!< @brief Ignore end date and time clock. Record at 'Any Time' */
  int m_iPriority; /*!< @brief priority of the timer */
  int m_iLifetime; /*!< @brief lifetime of the timer in days */
  int m_iMaxRecordings =
      0; /*!< @brief (optional) backend setting for maximum number of recordings to keep*/
  unsigned int m_iWeekdays; /*!< @brief bit based store of weekdays for timer rules */
  unsigned int
      m_iPreventDupEpisodes; /*!< @brief only record new episodes for epg-based timer rules */
  unsigned int m_iRecordingGroup =
      0; /*!< @brief (optional) if set, the addon/backend stores the recording to a group (sub-folder) */
  std::string m_strFileNameAndPath; /*!< @brief file name is only for reference */
  bool m_bIsRadio; /*!< @brief is radio channel if set */
  unsigned int m_iTimerId = 0; /*!< @brief id that won't change as long as Kodi is running */
  unsigned int
      m_iMarginStart; /*!< @brief (optional) if set, the backend starts the recording iMarginStart minutes before startTime. */
  unsigned int
      m_iMarginEnd; /*!< @brief (optional) if set, the backend ends the recording iMarginEnd minutes after endTime. */
  mutable unsigned int
      m_iEpgUid; /*!< id of epg event associated with this timer, EPG_TAG_INVALID_UID if none. */
  std::string m_strSeriesLink; /*!< series link */

  CDateTime m_StartTime; /*!< start time */
  CDateTime m_StopTime; /*!< stop time */
  CDateTime m_FirstDay; /*!< if it is a manual timer rule the first date it starts */
  std::shared_ptr<CPVRTimerType> m_timerType; /*!< the type of this timer */

  unsigned int m_iTVChildTimersActive = 0;
  unsigned int m_iTVChildTimersConflictNOK = 0;
  unsigned int m_iTVChildTimersRecording = 0;
  unsigned int m_iTVChildTimersErrors = 0;
  unsigned int m_iRadioChildTimersActive = 0;
  unsigned int m_iRadioChildTimersConflictNOK = 0;
  unsigned int m_iRadioChildTimersRecording = 0;
  unsigned int m_iRadioChildTimersErrors = 0;

  mutable std::shared_ptr<CPVREpgInfoTag> m_epgTag; /*!< epg info tag matching m_iEpgUid. */
  mutable std::shared_ptr<CPVRChannel> m_channel;

  mutable bool m_bProbedEpgTag = false;
};
} // namespace PVR
