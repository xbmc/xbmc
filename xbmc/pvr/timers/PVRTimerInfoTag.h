#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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

/*
 * DESCRIPTION:
 *
 * CPVRTimerInfoTag is part of the PVRManager to support sheduled recordings.
 *
 * The timer information tag holds data about current programmed timers for
 * the PVRManager. It is possible to create timers directly based upon
 * a EPG entry by giving the EPG information tag or as instant timer
 * on currently tuned channel, or give a blank tag to modify later.
 *
 * With exception of the blank one, the tag can easily and unmodified added
 * by the PVRManager function "bool AddTimer(const CFileItem &item)" to
 * the backend server.
 *
 * The filename inside the tag is for reference only and gives the index
 * number of the tag reported by the PVR backend and can not be played!
 */

#include "XBDateTime.h"
#include "addons/include/xbmc_pvr_types.h"
#include "utils/ISerializable.h"
#include "pvr/timers/PVRTimerType.h"

#include <memory>

class CFileItem;

namespace EPG
{
  class CEpgInfoTag;
  typedef std::shared_ptr<EPG::CEpgInfoTag> CEpgInfoTagPtr;
}

namespace PVR
{
  class CGUIDialogPVRTimerSettings;
  class CPVRTimers;
  class CPVRChannelGroupInternal;

  class CPVRChannel;
  typedef std::shared_ptr<PVR::CPVRChannel> CPVRChannelPtr;

  class CPVRTimerInfoTag;
  typedef std::shared_ptr<PVR::CPVRTimerInfoTag> CPVRTimerInfoTagPtr;

  class CPVRTimerInfoTag : public ISerializable
  {
    friend class CPVRTimers;

  public:
    CPVRTimerInfoTag(bool bRadio = false);
    CPVRTimerInfoTag(const PVR_TIMER &timer, const CPVRChannelPtr &channel, unsigned int iClientId);

  private:
    CPVRTimerInfoTag(const CPVRTimerInfoTag &tag); // intentionally not implemented.
    CPVRTimerInfoTag &operator=(const CPVRTimerInfoTag &orig); // intentionally not implemented.

  public:
    virtual ~CPVRTimerInfoTag(void);

    bool operator ==(const CPVRTimerInfoTag& right) const;
    bool operator !=(const CPVRTimerInfoTag& right) const;

    virtual void Serialize(CVariant &value) const;

    int Compare(const CPVRTimerInfoTag &timer) const;

    void UpdateSummary(void);

    void DisplayError(PVR_ERROR err) const;

    std::string GetStatus() const;
    std::string GetTypeAsString() const;

    bool SetDuration(int iDuration);

    static CPVRTimerInfoTagPtr CreateFromEpg(const EPG::CEpgInfoTagPtr &tag, bool bRepeating = false);
    EPG::CEpgInfoTagPtr GetEpgInfoTag(void) const;
    /*!
     * @return True if this timer has a corresponding epg info tag, false otherwise
     */
    bool HasEpgInfoTag() const;

    int ChannelNumber(void) const;
    std::string ChannelName(void) const;
    std::string ChannelIcon(void) const;
    CPVRChannelPtr ChannelTag(void) const;

    bool UpdateEntry(const CPVRTimerInfoTagPtr &tag);

    void UpdateEpgEvent(bool bClear = false);

    bool IsActive(void) const
    {
      return m_state == PVR_TIMER_STATE_SCHEDULED
        || m_state == PVR_TIMER_STATE_RECORDING
        || m_state == PVR_TIMER_STATE_CONFLICT_OK
        || m_state == PVR_TIMER_STATE_CONFLICT_NOK
        || m_state == PVR_TIMER_STATE_ERROR;
    }

    bool IsRecording(void) const { return m_state == PVR_TIMER_STATE_RECORDING; }

    /*!
      * @brief Checks whether is timer has a timer type. Can be false if
      *        no PVR client supporting timers is enabled.
      * @return True if this timer has a timer type, false otherwise
      */
    bool HasTimerType(void) const { return m_timerType.get() != NULL; }

    /*!
      * @brief Gets the type of this timer.
      * @return the timer type.
      */
    const CPVRTimerTypePtr GetTimerType() const { return m_timerType; }

    /*!
      * @brief Sets the type of this timer.
      * @param the new timer type.
      */
    void SetTimerType(const CPVRTimerTypePtr &type);

    /*!
      * @brief Checks whether this is a repeating (vs. one-shot) timer.
      * @return True if this is a repeating timer, false otherwise.
      */
    bool IsRepeating(void) const { return m_timerType && m_timerType->IsRepeating(); }

    /*!
      * @brief Checks whether this is a manual (vs. epg-based) timer.
      * @return True if this is a manual timer, false otherwise.
      */
    bool IsManual(void) const { return m_timerType && m_timerType->IsManual(); }

    CDateTime StartAsUTC(void) const;
    CDateTime StartAsLocalTime(void) const;
    bool IsStartAtAnyTime(void) const;
    void SetStartFromUTC(CDateTime &start) { m_StartTime = start; }
    void SetStartFromLocalTime(CDateTime &start) { m_StartTime = start.GetAsUTCDateTime(); }

    CDateTime EndAsUTC(void) const;
    CDateTime EndAsLocalTime(void) const;
    bool IsEndAtAnyTime(void) const;
    void SetEndFromUTC(CDateTime &end) { m_StopTime = end; }
    void SetEndFromLocalTime(CDateTime &end) { m_StopTime = end.GetAsUTCDateTime(); }

    CDateTime FirstDayAsUTC(void) const;
    CDateTime FirstDayAsLocalTime(void) const;
    void SetFirstDayFromUTC(CDateTime &firstDay) { m_FirstDay = firstDay; }
    void SetFirstDayFromLocalTime(CDateTime &firstDay) { m_FirstDay = firstDay.GetAsUTCDateTime(); }

    unsigned int MarginStart(void) const { return m_iMarginStart; }
    void SetMarginStart(unsigned int iMinutes) { m_iMarginStart = iMinutes; }

    unsigned int MarginEnd(void) const { return m_iMarginEnd; }
    void SetMarginEnd(unsigned int iMinutes) { m_iMarginEnd = iMinutes; }

    /*!
     * @brief Show a notification for this timer in the UI
     */
    void QueueNotification(void) const;

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

    /* Client control functions */
    bool AddToClient() const;
    bool DeleteFromClient(bool bForce = false, bool bDeleteSchedule = false) const;
    bool RenameOnClient(const std::string &strNewName);
    bool UpdateOnClient();

    void SetEpgInfoTag(EPG::CEpgInfoTagPtr &tag);
    void ClearEpgTag(void);

    void UpdateChannel(void);

    /*!
     * @brief Return string representation for any possible combination of weekdays.
     * @param iWeekdays weekdays as bit mask (0x01 = Mo, 0x02 = Tu, ...)
     * @param bEpgBased context is an epg-based timer
     * @param bLongMultiDaysFormat use long format. ("Mo-__-We-__-Fr-Sa-__" vs. "Mo-We-Fr-Sa")
     * @return the weekdays string representation
     */
    static std::string GetWeekdaysString(unsigned int iWeekdays, bool bEpgBased, bool bLongMultiDaysFormat);

    std::string           m_strTitle;            /*!< @brief name of this timer */
    std::string           m_strEpgSearchString;  /*!< @brief a epg data match string for repeating epg-based timers. Format is backend-dependent, for example regexp */
    bool                  m_bFullTextEpgSearch;  /*!< @brief indicates whether only epg episode title can be matched by the pvr backend or "more" (backend-dependent") data. */
    std::string           m_strDirectory;        /*!< @brief directory where the recording must be stored */
    std::string           m_strSummary;          /*!< @brief summary string with the time to show inside a GUI list */
    PVR_TIMER_STATE       m_state;               /*!< @brief the state of this timer */
    int                   m_iClientId;           /*!< @brief ID of the backend */
    unsigned int          m_iClientIndex;        /*!< @brief index number of the tag, given by the backend, -1 for new */
    unsigned int          m_iParentClientIndex;  /*!< @brief for timers scheduled by repeated timers, the index number of the parent, given by the backend, 0 for no parent */
    int                   m_iClientChannelUid;   /*!< @brief channel uid */
    int                   m_iPriority;           /*!< @brief priority of the timer */
    int                   m_iLifetime;           /*!< @brief lifetime of the timer in days */
    unsigned int          m_iWeekdays;           /*!< @brief bit based store of weekdays for repeating timers */
    unsigned int          m_iPreventDupEpisodes; /*!< @brief only record new episodes for repeating epg based timers */
    unsigned int          m_iRecordingGroup;     /*!< @brief (optional) if set, the addon/backend stores the recording to a group (sub-folder) */
    std::string           m_strFileNameAndPath;  /*!< @brief file name is only for reference */
    int                   m_iChannelNumber;      /*!< @brief integer value of the channel number */
    bool                  m_bIsRadio;            /*!< @brief is radio channel if set */
    unsigned int          m_iTimerId;            /*!< @brief id that won't change as long as XBMC is running */

    CPVRChannelPtr        m_channel;
    unsigned int          m_iMarginStart;        /*!< @brief (optional) if set, the backend starts the recording iMarginStart minutes before startTime. */
    unsigned int          m_iMarginEnd;          /*!< @brief (optional) if set, the backend ends the recording iMarginEnd minutes after endTime. */
    std::vector<std::string> m_genre;            /*!< @brief genre of the timer */
    int                   m_iGenreType;          /*!< @brief genre type of the timer */
    int                   m_iGenreSubType;       /*!< @brief genre subtype of the timer */

  private:
    std::string GetWeekdaysString() const;

    CCriticalSection      m_critSection;
    EPG::CEpgInfoTagPtr   m_epgTag;
    CDateTime             m_StartTime; /*!< start time */
    CDateTime             m_StopTime;  /*!< stop time */
    CDateTime             m_FirstDay;  /*!< if it is a manual repeating timer the first date it starts */
    CPVRTimerTypePtr      m_timerType; /*!< the type of this timer */
  };
}
