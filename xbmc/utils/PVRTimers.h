#pragma once
/*
 *      Copyright (C) 2005-2009 Team XBMC
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
 * for DESCRIPTION see 'TVTimerInfoTag.cpp'
 */

#include "DateTime.h"
#include "../addons/include/xbmc_pvr_types.h"

class CFileItem;
class CTVEPGInfoTag;

class cPVRTimerInfoTag
{
private:
  void DisplayError(PVR_ERROR err) const;

public:
  cPVRTimerInfoTag();
  cPVRTimerInfoTag(const CFileItem& item);
  cPVRTimerInfoTag(bool Init);

  bool operator ==(const cPVRTimerInfoTag& right) const;
  bool operator !=(const cPVRTimerInfoTag& right) const;

  void Reset();
  const CStdString GetStatus() const;
  int Compare(const cPVRTimerInfoTag &timer) const;
  time_t StartTime(void) const;
  time_t StopTime(void) const;

  CStdString      m_strTitle;             /// Name of this timer
  CStdString      m_Summary;              /// Summary string with the time to show inside a GUI list

  bool            m_Active;               /// Active flag, if it is false backend ignore the timer
  int             m_channelNum;           /// Integer value of the channel number
  int             m_clientID;             /// ID of the backend
  int             m_clientIndex;          /// Index number of the tag, given by the backend, -1 for new
  int             m_clientNum;            /// Integer value of the client number
  bool            m_Radio;                /// Is Radio channel if set
  bool            m_recStatus;            /// Is this timer recording?

  CDateTime       m_StartTime;            /// Start time
  CDateTime       m_StopTime;             /// Stop time

  bool            m_Repeat;               /// Repeating timer if true, use the m_FirstDay and repeat flags
  CDateTime       m_FirstDay;             /// If it is a repeating timer the first date it starts
  int             m_Weekdays;             /// Bit based store of weekdays to repeat

  int             m_Priority;             /// Priority of the timer
  int             m_Lifetime;             /// Lifetime of the timer in days

  CStdString      m_strFileNameAndPath;   /// Filename is only for reference

  CStdString Name(void) const { return m_strTitle; }
  void SetName(CStdString name) { m_strTitle = name; }
  int Number(void) const { return m_channelNum; }
  void SetNumber(int Number) { m_channelNum = Number; }
  bool IsRadio(void) const { return m_Radio; }
  void SetRadio(int Radio) { m_Radio = Radio; }
  int ClientNumber(void) const { return m_clientNum; }
  void SetClientNumber(int Number) { m_clientNum = Number; }
  long ClientID(void) const { return m_clientID; }
  void SetClientID(int ClientId) { m_clientID = ClientId; }

  bool Add() const;
  bool Delete(bool force = false) const;
  bool Rename(CStdString &newname) const;
  bool Update() const;
};

class cPVRTimers : public std::vector<cPVRTimerInfoTag> 
{
private:
  CCriticalSection  m_critSection;

public:
  cPVRTimers(void);
  bool Load() { return Update(); }
  bool Update();
  int GetNumTimers();
  int GetTimers(CFileItemList* results);
  cPVRTimerInfoTag *GetTimer(cPVRTimerInfoTag *Timer);
  cPVRTimerInfoTag *GetMatch(CDateTime t);
  cPVRTimerInfoTag *GetMatch(time_t t);
  cPVRTimerInfoTag *GetMatch(const CTVEPGInfoTag *Epg, int *Match = NULL);
  cPVRTimerInfoTag *GetNextActiveTimer(void);
  static bool AddTimer(const CFileItem &item);
  static bool DeleteTimer(const CFileItem &item, bool force = false);
  static bool RenameTimer(CFileItem &item, CStdString &newname);
  static bool UpdateTimer(const CFileItem &item);
  void Clear();
};

extern cPVRTimers PVRTimers;
