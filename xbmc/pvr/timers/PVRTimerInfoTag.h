#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
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
 *
 *
 * USED SETUP VARIABLES:
 *
 * ------------- Name -------------|---Type--|-default-|--Description-----
 * pvrmanager.instantrecordtime    = Integer = 180     = Length of a instant timer in minutes
 * pvrmanager.defaultpriority      = Integer = 50      = Default Priority
 * pvrmanager.defaultlifetime      = Integer = 99      = Liftime of the timer in days
 * pvrmanager.marginstart          = Integer = 2       = Minutes to start record earlier
 * pvrmanager.marginstop           = Integer = 10      = Minutes to stop record later
 *
 */

#include "XBDateTime.h"
#include "../addons/include/xbmc_pvr_types.h"

class CFileItem;

namespace EPG
{
  class CEpgInfoTag;
}

namespace PVR
{
  class CGUIDialogPVRTimerSettings;
  class CPVRChannel;

  class CPVRTimerInfoTag
  {
    friend class EPG::CEpgInfoTag;

  public:
    CStdString            m_strTitle;           /*!< @brief name of this timer */
    CStdString            m_strDirectory;       /*!< @brief directory where the recording must be stored */
    CStdString            m_strSummary;         /*!< @brief summary string with the time to show inside a GUI list */
    PVR_TIMER_STATE       m_state;              /*!< @brief the state of this timer */
    int                   m_iClientId;          /*!< @brief ID of the backend */
    int                   m_iClientIndex;       /*!< @brief index number of the tag, given by the backend, -1 for new */
    int                   m_iClientChannelUid;  /*!< @brief channel uid */
    int                   m_iPriority;          /*!< @brief priority of the timer */
    int                   m_iLifetime;          /*!< @brief lifetime of the timer in days */
    bool                  m_bIsRepeating;       /*!< @brief repeating timer if true, use the m_FirstDay and repeat flags */
    int                   m_iWeekdays;          /*!< @brief bit based store of weekdays to repeat */
    CStdString            m_strFileNameAndPath; /*!< @brief filename is only for reference */
    int                   m_iChannelNumber;     /*!< @brief integer value of the channel number */
    bool                  m_bIsRadio;           /*!< @brief is radio channel if set */
    const CPVRChannel *   m_channel;
    unsigned int          m_iMarginStart;       /*!< @brief (optional) if set, the backend starts the recording iMarginStart minutes before startTime. */
    unsigned int          m_iMarginEnd;         /*!< @brief (optional) if set, the backend ends the recording iMarginEnd minutes after endTime. */
    CStdString            m_strGenre;           /*!< @brief genre of the timer */
    int                   m_iGenreType;         /*!< @brief genre type of the timer */
    int                   m_iGenreSubType;      /*!< @brief genre subtype of the timer */

    CPVRTimerInfoTag(void);
    CPVRTimerInfoTag(const PVR_TIMER &timer, CPVRChannel *channel, unsigned int iClientId);
    virtual ~CPVRTimerInfoTag(void);

    void Reset();

    bool operator ==(const CPVRTimerInfoTag& right) const;
    bool operator !=(const CPVRTimerInfoTag& right) const;
    CPVRTimerInfoTag &operator=(const CPVRTimerInfoTag &orig);

    int Compare(const CPVRTimerInfoTag &timer) const;

    void UpdateSummary(void);

    void DisplayError(PVR_ERROR err) const;

    const CStdString &GetStatus() const;

    bool SetDuration(int iDuration);

    static CPVRTimerInfoTag *CreateFromEpg(const EPG::CEpgInfoTag &tag);
    void SetEpgInfoTag(EPG::CEpgInfoTag *tag);
    EPG::CEpgInfoTag *GetEpgInfoTag(void) const;

    int ChannelNumber(void) const;
    CStdString ChannelName(void) const;
    CStdString ChannelIcon(void) const;

    bool UpdateEntry(const CPVRTimerInfoTag &tag);

    void UpdateEpgEvent(bool bClear = false);

    bool IsActive(void) const { return m_state == PVR_TIMER_STATE_SCHEDULED || m_state == PVR_TIMER_STATE_RECORDING; }
    bool IsRecording(void) const { return m_state == PVR_TIMER_STATE_RECORDING; }

    const CDateTime &StartAsUTC(void) const { return m_StartTime; }
    const CDateTime &StartAsLocalTime(void) const;
    void SetStartFromUTC(CDateTime &start) { m_StartTime = start; }
    void SetStartFromLocalTime(CDateTime &start) { m_StartTime = start.GetAsUTCDateTime(); }

    const CDateTime &EndAsUTC(void) const { return m_StopTime; }
    const CDateTime &EndAsLocalTime(void) const;
    void SetEndFromUTC(CDateTime &end) { m_StopTime = end; }
    void SetEndFromLocalTime(CDateTime &end) { m_StopTime = end.GetAsUTCDateTime(); }

    const CDateTime &FirstDayAsUTC(void) const { return m_FirstDay; }
    const CDateTime &FirstDayAsLocalTime(void) const;
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
    void GetNotificationText(CStdString &strText) const;

    /* Client control functions */
    bool AddToClient();
    bool DeleteFromClient(bool bForce = false);
    bool RenameOnClient(const CStdString &strNewName);
    bool UpdateOnClient();

  protected:
    /*!
     * @brief Called by the CEpgInfoTag destructor
     */
    virtual void OnEpgTagDeleted(void);

    CCriticalSection      m_critSection;
    int                   m_iEpgId;    /*!< the id of the epg table or -1 if none */
    CDateTime             m_epgStart;  /*!< the start time of the epg tag */
    CDateTime             m_StartTime; /*!< start time */
    CDateTime             m_StopTime;  /*!< stop time */
    CDateTime             m_FirstDay;  /*!< if it is a repeating timer the first date it starts */
  };
}
