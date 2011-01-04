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

class CPVRTimerInfoTag
{
private:
  friend class CGUIDialogPVRTimerSettings;

  CStdString      m_strTitle;             /// Name of this timer
  CStdString      m_strDir;               /// Directory where the recording must be stored
  CStdString      m_Summary;              /// Summary string with the time to show inside a GUI list
  bool            m_Active;               /// Active flag, if it is false backend ignore the timer
  int             m_channelNum;           /// Integer value of the channel number
  int             m_clientID;             /// ID of the backend
  int             m_clientIndex;          /// Index number of the tag, given by the backend, -1 for new
  int             m_clientNum;            /// Integer value of the client number
  bool            m_Radio;                /// Is Radio channel if set
  bool            m_recStatus;            /// Is this timer recording?
  int             m_Priority;             /// Priority of the timer
  int             m_Lifetime;             /// Lifetime of the timer in days
  bool            m_Repeat;               /// Repeating timer if true, use the m_FirstDay and repeat flags
  CDateTime       m_StartTime;            /// Start time
  CDateTime       m_StopTime;             /// Stop time
  CDateTime       m_FirstDay;             /// If it is a repeating timer the first date it starts
  int             m_Weekdays;             /// Bit based store of weekdays to repeat
  CStdString      m_strFileNameAndPath;   /// Filename is only for reference

  const CPVREpgInfoTag *m_EpgInfo;

  void DisplayError(PVR_ERROR err) const;

public:
  CPVRTimerInfoTag();
  CPVRTimerInfoTag(const CFileItem& item);
  CPVRTimerInfoTag(bool Init);

  bool operator ==(const CPVRTimerInfoTag& right) const;
  bool operator !=(const CPVRTimerInfoTag& right) const;

  void Reset();
  const CStdString GetStatus() const;
  int Compare(const CPVRTimerInfoTag &timer) const;
  time_t StartTime(void) const;
  time_t StopTime(void) const;
  time_t FirstDayTime(void) const;
  CDateTime Start(void) const { return m_StartTime; }
  void SetStart(CDateTime Start) { m_StartTime = Start; }
  CDateTime Stop(void) const { return m_StopTime; }
  void SetStop(CDateTime Stop) { m_StopTime = Stop; }
  CStdString Title(void) const { return m_strTitle; }
  void SetTitle(CStdString name) { m_strTitle = name; }
  CStdString Dir(void) const { return m_strDir; }
  void SetDir(CStdString dir) { m_strDir = dir; }
  int Number(void) const { return m_channelNum; }
  void SetNumber(int Number) { m_channelNum = Number; }
  bool Active(void) const { return m_Active; }
  void SetActive(bool Active) { m_Active = Active; }
  bool IsRadio(void) const { return m_Radio; }
  void SetRadio(bool Radio) { m_Radio = Radio; }
  int ClientNumber(void) const { return m_clientNum; }
  void SetClientNumber(int Number) { m_clientNum = Number; }
  long ClientIndex(void) const { return m_clientIndex; }
  void SetClientIndex(int ClientIndex) { m_clientIndex = ClientIndex; }
  long ClientID(void) const { return m_clientID; }
  void SetClientID(int ClientId) { m_clientID = ClientId; }
  bool IsRecording(void) const { return m_recStatus; }
  void SetRecording(bool Recording) { m_recStatus = Recording; }
  int Lifetime(void) const { return m_Lifetime; }
  void SetLifetime(int Lifetime) { m_Lifetime = Lifetime; }
  int Priority(void) const { return m_Priority; }
  void SetPriority(int Priority) { m_Priority = Priority; }
  bool IsRepeating(void) const { return m_Repeat; }
  void SetRepeating(bool Repeating) { m_Repeat = Repeating; }
  int Weekdays(void) const { return m_Weekdays; }
  void SetWeekdays(int Weekdays) { m_Weekdays = Weekdays; }
  CDateTime FirstDay(void) const { return m_FirstDay; }
  void SetFirstDay(CDateTime FirstDay) { m_FirstDay = FirstDay; }
  CStdString Summary(void) const { return m_Summary; }
  void SetSummary(CStdString Summary) { m_Summary = Summary; }
  CStdString Path(void) const { return m_strFileNameAndPath; }
  void SetPath(CStdString path) { m_strFileNameAndPath = path; }
  const CPVREpgInfoTag *Epg() const { return m_EpgInfo;}
  void SetEpg(const CPVREpgInfoTag *tag);

  /* Channel related Info data */
  int ChannelNumber(void) const;
  CStdString ChannelName(void) const;

  /* Client control functions */
  bool Add() const;
  bool Delete(bool force = false) const;
  bool Rename(CStdString &newname) const;
  bool Update() const;
};
