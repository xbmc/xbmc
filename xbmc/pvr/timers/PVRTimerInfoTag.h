/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

/*
 * DESCRIPTION:
 *
 * CPVRTimerInfoTag is part of the PVRManager to support scheduled recordings.
 *
 * The timer information tag holds data about current programmed timers for
 * the PVRManager. It is possible to create timers directly based upon
 * a EPG entry by giving the EPG information tag or as instant timer
 * on currently tuned channel, or give a blank tag to modify later.
 *
 * The filename inside the tag is for reference only and gives the index
 * number of the tag reported by the PVR backend and can not be played!
 */

#include "XBDateTime.h"
#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "threads/CriticalSection.h"
#include "utils/ISerializable.h"

#include "pvr/PVRTypes.h"
#include "pvr/timers/PVRTimerType.h"

class CVariant;

namespace PVR
{
  enum class TimerOperationResult
  {
    OK = 0,
    FAILED,
    RECORDING // The timer was not deleted because it is currently recording (see DeleteTimer).
  };

  class CPVRTimerInfoTag final : public ISerializable
  {
  public:
    explicit CPVRTimerInfoTag(bool bRadio = false);
    CPVRTimerInfoTag(const PVR_TIMER &timer, const CPVRChannelPtr &channel, unsigned int iClientId);

    bool operator ==(const CPVRTimerInfoTag& right) const;
    bool operator !=(const CPVRTimerInfoTag& right) const;

    void Serialize(CVariant &value) const override;

    void UpdateSummary(void);

    std::string GetStatus() const;
    std::string GetTypeAsString() const;

    static const int DEFAULT_PVRRECORD_INSTANTRECORDTIME = -1;

    /*!
     * @brief create a tag for an instant timer for a given channel
     * @param channel is the channel the instant timer is to be created for
     * @param iDuration is the duration for the instant timer, DEFAULT_PVRRECORD_INSTANTRECORDTIME denotes system default (setting value)
     * @return the timer or null if timer could not be created
     */
    static CPVRTimerInfoTagPtr CreateInstantTimerTag(const CPVRChannelPtr &channel, int iDuration = DEFAULT_PVRRECORD_INSTANTRECORDTIME);

    /*!
     * @brief create a timer or timer rule for the given epg info tag.
     * @param tag the epg info tag
     * @param bCreateRule if true, create a timer rule, create a one shot timer otherwise
     * @return the timer or null if timer could not be created
     */
    static CPVRTimerInfoTagPtr CreateFromEpg(const CPVREpgInfoTagPtr &tag, bool bCreateRule = false);

    /*!
     * @brief get the epg info tag associated with this timer, if any
     * @param bCreate if true, try to find the epg tag if not yet set (lazy evaluation)
     * @return the epg info tag associated with this timer or null if there is no tag
     */
    CPVREpgInfoTagPtr GetEpgInfoTag(bool bCreate = true) const;

    std::string ChannelName(void) const;
    std::string ChannelIcon(void) const;

    /*!
     * @brief Check whether this timer has an associated channel.
     * @return True if this timer has a channel set, false otherwise.
     */
    bool HasChannel() const;

    /*!
     * @brief Get the channel associated with this timer, if any.
     * @return the channel or null if non is associated with this timer.
     */
    CPVRChannelPtr Channel() const;

    /*!
     * @brief updates this timer excluding the state of any children. See UpdateChildState/ResetChildState.
     * @return true if the timer was updated successfully
     */
    bool UpdateEntry(const CPVRTimerInfoTagPtr &tag);

    /*!
     * @brief merge in the state of this child timer. Run for each child after using ResetChildState.
     * @return true if the child timer's state was merged successfully
     */
    bool UpdateChildState(const CPVRTimerInfoTagPtr &childTimer);

    /*!
     * @brief reset the state of children related to this timer. Run UpdateChildState for all children afterwards.
     */
    void ResetChildState();

    bool IsActive(void) const
    {
      return m_state == PVR_TIMER_STATE_SCHEDULED
        || m_state == PVR_TIMER_STATE_RECORDING
        || m_state == PVR_TIMER_STATE_CONFLICT_OK
        || m_state == PVR_TIMER_STATE_CONFLICT_NOK
        || m_state == PVR_TIMER_STATE_ERROR;
    }

    /*!
     * @return True if this timer won't result in a recording because it is broken for some reason, false otherwise
     */
    bool IsBroken(void) const
    {
      return m_state == PVR_TIMER_STATE_CONFLICT_NOK
        || m_state == PVR_TIMER_STATE_ERROR;
    }

    /*!
     * @return True if this timer won't result in a recording because it is in conflict with another timer or live stream, false otherwise
     */
    bool HasConflict(void) const { return m_state == PVR_TIMER_STATE_CONFLICT_NOK; }

    bool IsRecording(void) const { return m_state == PVR_TIMER_STATE_RECORDING; }

    /*!
      * @brief Checks whether this timer has a timer type.
      * @return True if this timer has a timer type, false otherwise
      */
    bool HasTimerType(void) const { return m_timerType.get() != NULL; }

    /*!
      * @brief Gets the type of this timer.
      * @return the timer type or NULL if this tag has no timer type.
      */
    const CPVRTimerTypePtr GetTimerType() const { return m_timerType; }

    /*!
      * @brief Sets the type of this timer.
      * @param the new timer type.
      */
    void SetTimerType(const CPVRTimerTypePtr &type);

    /*!
      * @brief Checks whether this is a timer rule (vs. one time timer).
      * @return True if this is a timer rule, false otherwise.
      */
    bool IsTimerRule(void) const { return m_timerType && m_timerType->IsTimerRule(); }

    /*!
      * @brief Checks whether this is a manual (vs. epg-based) timer.
      * @return True if this is a manual timer, false otherwise.
      */
    bool IsManual(void) const { return m_timerType && m_timerType->IsManual(); }

    CDateTime StartAsUTC(void) const;
    CDateTime StartAsLocalTime(void) const;
    void SetStartFromUTC(CDateTime &start) { m_StartTime = start; }
    void SetStartFromLocalTime(CDateTime &start) { m_StartTime = start.GetAsUTCDateTime(); }

    CDateTime EndAsUTC(void) const;
    CDateTime EndAsLocalTime(void) const;
    void SetEndFromUTC(CDateTime &end) { m_StopTime = end; }
    void SetEndFromLocalTime(CDateTime &end) { m_StopTime = end.GetAsUTCDateTime(); }

    CDateTime FirstDayAsUTC(void) const;
    CDateTime FirstDayAsLocalTime(void) const;
    void SetFirstDayFromLocalTime(CDateTime &firstDay) { m_FirstDay = firstDay.GetAsUTCDateTime(); }

    unsigned int MarginStart(void) const { return m_iMarginStart; }

    /*!
     * @brief Get the text for the notification.
     * @param strText The notification.
     */
    void GetNotificationText(std::string &strText) const;

    /*!
    * @brief Get the text for the notification when a timer has been deleted
    */
    std::string GetDeletedNotificationText() const;

    const std::string& Title(void) const;
    const std::string& Summary(void) const;
    const std::string& Path(void) const;

    /*!
     * @brief The series link for this timer.
     * @return The series link or empty string, if not available.
     */
    const std::string& SeriesLink() const;

    /*!
     * @brief Get the UID of the epg event associated with this timer tag, if any.
     * @return the UID or EPG_TAG_INVALID_UID.
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
     * @brief Rename this timer on the backend, transferring all local data of this timer to the backend.
     * @param strNewName The new name.
     * @return True on success, false otherwise.
     */
    bool RenameOnClient(const std::string &strNewName);

    /*!
     * @brief Update this timer on the backend, transferring all local data of this timer to the backend.
     * @return True on success, false otherwise.
     */
    bool UpdateOnClient();

    /*!
     * @brief Associate the given epg tag with this timer; before, clear old timer at associated epg tag, if any.
     * @param tag The epg tag to assign.
     */
    void SetEpgTag(const CPVREpgInfoTagPtr &tag);

    /*!
     * @brief Clear the epg tag associated with this timer; before, clear this timer at associated epg tag, if any.
     */
    void ClearEpgTag(void);

    /*!
     * @brief Update the channel associated with this timer.
     * @return the channel for the timer. Can be empty for epg based repeating timers (e.g. "match any channel" rules)
     */
    CPVRChannelPtr UpdateChannel(void);

    /*!
     * @brief Return string representation for any possible combination of weekdays.
     * @param iWeekdays weekdays as bit mask (0x01 = Mo, 0x02 = Tu, ...)
     * @param bEpgBased context is an epg-based timer
     * @param bLongMultiDaysFormat use long format. ("Mo-__-We-__-Fr-Sa-__" vs. "Mo-We-Fr-Sa")
     * @return the weekdays string representation
     */
    static std::string GetWeekdaysString(unsigned int iWeekdays, bool bEpgBased, bool bLongMultiDaysFormat);

    /*!
     * @brief For timers scheduled by a timer rule, return the id of the rule (aka the id of the "parent" of the timer).
     * @return the id of the timer rule or PVR_TIMER_NO_PARENT in case the timer was not scheduled by a timer rule.
     */
    unsigned int GetTimerRuleId() const { return m_iParentClientIndex; }

    std::string           m_strTitle;            /*!< @brief name of this timer */
    std::string           m_strEpgSearchString;  /*!< @brief a epg data match string for epg-based timer rules. Format is backend-dependent, for example regexp */
    bool                  m_bFullTextEpgSearch = false;  /*!< @brief indicates whether only epg episode title can be matched by the pvr backend or "more" (backend-dependent") data. */
    std::string           m_strDirectory;        /*!< @brief directory where the recording must be stored */
    std::string           m_strSummary;          /*!< @brief summary string with the time to show inside a GUI list */
    PVR_TIMER_STATE       m_state = PVR_TIMER_STATE_SCHEDULED;               /*!< @brief the state of this timer */
    int                   m_iClientId;           /*!< @brief ID of the backend */
    unsigned int          m_iClientIndex;        /*!< @brief index number of the tag, given by the backend, PVR_TIMER_NO_CLIENT_INDEX for new */
    unsigned int          m_iParentClientIndex;  /*!< @brief for timers scheduled by a timer rule, the index number of the parent, given by the backend, PVR_TIMER_NO_PARENT for no parent */
    int                   m_iClientChannelUid;   /*!< @brief channel uid */
    bool                  m_bStartAnyTime = false;       /*!< @brief Ignore start date and time clock. Record at 'Any Time' */
    bool                  m_bEndAnyTime = false;         /*!< @brief Ignore end date and time clock. Record at 'Any Time' */
    int                   m_iPriority;           /*!< @brief priority of the timer */
    int                   m_iLifetime;           /*!< @brief lifetime of the timer in days */
    int                   m_iMaxRecordings = 0;      /*!< @brief (optional) backend setting for maximum number of recordings to keep*/
    unsigned int          m_iWeekdays;           /*!< @brief bit based store of weekdays for timer rules */
    unsigned int          m_iPreventDupEpisodes; /*!< @brief only record new episodes for epg-based timer rules */
    unsigned int          m_iRecordingGroup = 0;     /*!< @brief (optional) if set, the addon/backend stores the recording to a group (sub-folder) */
    std::string           m_strFileNameAndPath;  /*!< @brief file name is only for reference */
    bool                  m_bIsRadio;            /*!< @brief is radio channel if set */
    unsigned int          m_iTimerId = 0;            /*!< @brief id that won't change as long as XBMC is running */

    unsigned int          m_iMarginStart;        /*!< @brief (optional) if set, the backend starts the recording iMarginStart minutes before startTime. */
    unsigned int          m_iMarginEnd;          /*!< @brief (optional) if set, the backend ends the recording iMarginEnd minutes after endTime. */

  private:
    CPVRTimerInfoTag(const CPVRTimerInfoTag &tag) = delete;
    CPVRTimerInfoTag &operator=(const CPVRTimerInfoTag &orig) = delete;

    std::string GetWeekdaysString() const;
    void UpdateEpgInfoTag(void);

    CCriticalSection      m_critSection;
    CDateTime             m_StartTime; /*!< start time */
    CDateTime             m_StopTime;  /*!< stop time */
    CDateTime             m_FirstDay;  /*!< if it is a manual timer rule the first date it starts */
    CPVRTimerTypePtr      m_timerType; /*!< the type of this timer */

    unsigned int          m_iActiveChildTimers;   /*!< @brief Number of active timers which have this timer as their m_iParentClientIndex */
    bool                  m_bHasChildConflictNOK; /*!< @brief Has at least one child timer with status PVR_TIMER_STATE_CONFLICT_NOK */
    bool                  m_bHasChildRecording;   /*!< @brief Has at least one child timer with status PVR_TIMER_STATE_RECORDING */
    bool                  m_bHasChildErrors;      /*!< @brief Has at least one child timer with status PVR_TIMER_STATE_ERROR */

    std::string m_strSeriesLink; /*!< series link */

    mutable unsigned int  m_iEpgUid;   /*!< id of epg event associated with this timer, EPG_TAG_INVALID_UID if none. */
    mutable CPVREpgInfoTagPtr m_epgTag; /*!< epg info tag matching m_iEpgUid. */
    mutable CPVRChannelPtr m_channel;
  };
}
