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

#include "DateTime.h"
#include "../addons/include/xbmc_pvr_types.h"

class CFileItem;
class CPVREpgInfoTag;
class CGUIDialogPVRTimerSettings;
class CPVRChannel;

class CPVRTimerInfoTag
{
private:
  friend class CGUIDialogPVRTimerSettings;

  CStdString      m_strTitle;           /* name of this timer */
  CStdString      m_strDir;             /* directory where the recording must be stored */

  CStdString      m_strSummary;         /* summary string with the time to show inside a GUI list */
  bool            m_bValidSummary;

  bool            m_bIsActive;          /* active flag, if it is false backend ignore the timer */
  int             m_iChannelNumber;     /* integer value of the channel number */
  int             m_iClientID;          /* ID of the backend */
  int             m_iClientIndex;       /* index number of the tag, given by the backend, -1 for new */
  int             m_iClientNumber;      /* integer value of the client number */
  bool            m_bIsRadio;           /* is radio channel if set */
  bool            m_bIsRecording;       /* is this timer recording? */
  int             m_iPriority;          /* priority of the timer */
  int             m_iLifetime;          /* lifetime of the timer in days */
  bool            m_bIsRepeating;       /* repeating timer if true, use the m_FirstDay and repeat flags */
  CDateTime       m_StartTime;          /* start time */
  CDateTime       m_StopTime;           /* stop time */
  CDateTime       m_FirstDay;           /* if it is a repeating timer the first date it starts */
  int             m_iWeekdays;          /* bit based store of weekdays to repeat */
  CStdString      m_strFileNameAndPath; /* filename is only for reference */

  const CPVREpgInfoTag *m_EpgInfo;

  void UpdateSummary(void);

  void DisplayError(PVR_ERROR err) const;

public:
  CPVRTimerInfoTag();

  bool operator ==(const CPVRTimerInfoTag& right) const;
  bool operator !=(const CPVRTimerInfoTag& right) const;
  int Compare(const CPVRTimerInfoTag &timer) const;

  void Reset();

  const CStdString &GetStatus() const;

  const CDateTime &Start(void) const { return m_StartTime; }
  time_t StartTime(void) const;
  void SetStart(const CDateTime &Start);

  const CDateTime &Stop(void) const { return m_StopTime; }
  time_t StopTime(void) const;
  void SetStop(const CDateTime &Stop);

  bool SetDuration(int iDuration);

  const CStdString &Title(void) const { return m_strTitle; }
  void SetTitle(const CStdString &name) { m_strTitle = name; }

  const CStdString &Dir(void) const { return m_strDir; }
  void SetDir(const CStdString &dir) { m_strDir = dir; }

  int Number(void) const { return m_iChannelNumber; }
  void SetNumber(int Number) { m_iChannelNumber = Number; }

  bool Active(void) const { return m_bIsActive; }
  void SetActive(bool Active) { m_bIsActive = Active; }

  bool IsRadio(void) const { return m_bIsRadio; }
  void SetRadio(bool Radio) { m_bIsRadio = Radio; }

  int ClientNumber(void) const { return m_iClientNumber; }
  void SetClientNumber(int Number) { m_iClientNumber = Number; }

  long ClientIndex(void) const { return m_iClientIndex; }
  void SetClientIndex(int ClientIndex) { m_iClientIndex = ClientIndex; }

  long ClientID(void) const { return m_iClientID; }
  void SetClientID(int ClientId) { m_iClientID = ClientId; }

  bool IsRecording(void) const { return m_bIsRecording; }
  void SetRecording(bool Recording) { m_bIsRecording = Recording; }

  int Lifetime(void) const { return m_iLifetime; }
  void SetLifetime(int Lifetime) { m_iLifetime = Lifetime; }

  int Priority(void) const { return m_iPriority; }
  void SetPriority(int Priority) { m_iPriority = Priority; }

  bool IsRepeating(void) const { return m_bIsRepeating; }
  void SetRepeating(bool Repeating) { m_bIsRepeating = Repeating; }

  int Weekdays(void) const { return m_iWeekdays; }
  void SetWeekdays(int Weekdays);

  const CDateTime &FirstDay(void) const { return m_FirstDay; }
  time_t FirstDayTime(void) const;
  void SetFirstDay(const CDateTime &FirstDay) { m_FirstDay = FirstDay; }

  const CStdString &Summary(void) const { return m_strSummary; }
  void SetSummary(const CStdString &Summary) { m_strSummary = Summary; }

  const CStdString &Path(void) const { return m_strFileNameAndPath; }
  void SetPath(const CStdString &path) { m_strFileNameAndPath = path; }

  static CPVRTimerInfoTag *InstantTimer();
  static CPVRTimerInfoTag *CreateFromEpg(const CPVREpgInfoTag &tag);

  const CPVREpgInfoTag *EpgInfoTag() const { return m_EpgInfo;}
  void SetEpgInfoTag(const CPVREpgInfoTag *tag);

  /* Channel related Info data */
  int ChannelNumber(void) const;
  CStdString ChannelName(void) const;

  /* Client control functions */
  bool AddToClient() const;
  bool DeleteFromClient(bool force = false) const;
  bool RenameOnClient(const CStdString &newname) const;
  bool UpdateOnClient() const;
};
