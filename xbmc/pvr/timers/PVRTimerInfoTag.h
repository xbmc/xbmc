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
  #define PVR_VIRTUAL_CHANNEL_UID (-1)

  class CPVRTimerInfoTag : public ISerializable
  {
    friend class CPVRTimers;
    friend class CGUIDialogPVRTimerSettings;

  public:
    CPVRTimerInfoTag(void);
    CPVRTimerInfoTag(const PVR_TIMER &timer, CPVRChannelPtr channel, unsigned int iClientId);
    virtual ~CPVRTimerInfoTag(void);

    bool operator ==(const CPVRTimerInfoTag& right) const;
    bool operator !=(const CPVRTimerInfoTag& right) const;
    CPVRTimerInfoTag &operator=(const CPVRTimerInfoTag &orig);

    virtual void Serialize(CVariant &value) const;

    int Compare(const CPVRTimerInfoTag &timer) const;

    void UpdateSummary(void);

    void DisplayError(PVR_ERROR err) const;

    std::string GetStatus() const;

    bool SetDuration(int iDuration);

    static CPVRTimerInfoTag *CreateFromEpg(const EPG::CEpgInfoTag &tag);
    EPG::CEpgInfoTagPtr GetEpgInfoTag(void) const;

    int ChannelNumber(void) const;
    std::string ChannelName(void) const;
    std::string ChannelIcon(void) const;
    CPVRChannelPtr ChannelTag(void) const;

    bool UpdateEntry(const CPVRTimerInfoTag &tag);

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
    void SetFirstDayFromUTC(CDateTime &firstDay) { m_FirstDay = firstDay; }
    void SetFirstDayFromLocalTime(CDateTime &firstDay) { m_FirstDay = firstDay.GetAsUTCDateTime(); }

    unsigned int MarginStart(void) const { return m_iMarginStart; }
    void SetMarginStart(unsigned int iMinutes) { m_iMarginStart = iMinutes; }

    unsigned int MarginEnd(void) const { return m_iMarginEnd; }
    void SetMarginEnd(unsigned int iMinutes) { m_iMarginEnd = iMinutes; }

    bool SupportsFolders() const;

    /*!
     * @brief Show a notification for this timer in the UI
     */
    void QueueNotification(void) const;

    /*!
     * @brief Get the text for the notification.
     * @param strText The notification.
     */
    void GetNotificationText(std::string &strText) const;

    const std::string& Title(void) const;
    const std::string& Summary(void) const;
    const std::string& Path(void) const;

    /* Client control functions */
    bool AddToClient() const;
    bool DeleteFromClient(bool bForce = false) const;
    bool RenameOnClient(const std::string &strNewName);
    bool UpdateOnClient();

    void SetEpgInfoTag(EPG::CEpgInfoTagPtr tag);
    void ClearEpgTag(void);

    void UpdateChannel(void);

    std::string           m_strTitle;           /*!< @brief name of this timer */
    std::string           m_strDirectory;       /*!< @brief directory where the recording must be stored */
    std::string           m_strSummary;         /*!< @brief summary string with the time to show inside a GUI list */
    PVR_TIMER_STATE       m_state;              /*!< @brief the state of this timer */
    int                   m_iClientId;          /*!< @brief ID of the backend */
    int                   m_iClientIndex;       /*!< @brief index number of the tag, given by the backend, -1 for new */
    int                   m_iClientChannelUid;  /*!< @brief channel uid */
    int                   m_iPriority;          /*!< @brief priority of the timer */
    int                   m_iLifetime;          /*!< @brief lifetime of the timer in days */
    bool                  m_bIsRepeating;       /*!< @brief repeating timer if true, use the m_FirstDay and repeat flags */
    int                   m_iWeekdays;          /*!< @brief bit based store of weekdays to repeat */
    std::string           m_strFileNameAndPath; /*!< @brief filename is only for reference */
    int                   m_iChannelNumber;     /*!< @brief integer value of the channel number */
    bool                  m_bIsRadio;           /*!< @brief is radio channel if set */
    unsigned int          m_iTimerId;           /*!< @brief id that won't change as long as XBMC is running */

    CPVRChannelPtr        m_channel;
    unsigned int          m_iMarginStart;       /*!< @brief (optional) if set, the backend starts the recording iMarginStart minutes before startTime. */
    unsigned int          m_iMarginEnd;         /*!< @brief (optional) if set, the backend ends the recording iMarginEnd minutes after endTime. */
    std::vector<std::string> m_genre;           /*!< @brief genre of the timer */
    int                   m_iGenreType;         /*!< @brief genre type of the timer */
    int                   m_iGenreSubType;      /*!< @brief genre subtype of the timer */

  private:
    CCriticalSection      m_critSection;
    EPG::CEpgInfoTagPtr   m_epgTag;
    CDateTime             m_StartTime; /*!< start time */
    CDateTime             m_StopTime;  /*!< stop time */
    CDateTime             m_FirstDay;  /*!< if it is a repeating timer the first date it starts */
  };
}
