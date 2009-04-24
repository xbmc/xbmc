#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "VideoInfoTag.h"
#include "DateTime.h"
#include "FileItem.h"

class CTVTimerInfoTag : public CVideoInfoTag
{
public:
  CTVTimerInfoTag();
  CTVTimerInfoTag(const CFileItem& item);
  CTVTimerInfoTag(bool Init);

  bool operator ==(const CTVTimerInfoTag& right) const;
  bool operator !=(const CTVTimerInfoTag& right) const;

  void Reset();

  int             m_Index;                /// Index number of the tag, given by the backend, -1 for new
  bool            m_Active;               /// Active flag, if it is false backend ignore the timer

  CStdString      m_Summary;              /// Summary string with the time to show inside a GUI list
  /// see PVRManager.cpp for format.

  long            m_clientID;
  int             m_channelNum;           /// Integer value of the channel number
  int             m_clientNum;            /// Integer value of the client number
  CStdString      m_strChannel;           /// String name of the channel
  bool            m_Radio;                /// Is Radio channel if set

  bool            m_Repeat;               /// Repeating timer if true, use the m_FirstDay and repeat flags
  CDateTime       m_StartTime;            /// Start time
  time_t          m_iStartTime;           /// as time_t (if you get time_t from CDateTime it is always different)
  CDateTime       m_StopTime;             /// Stop time
  time_t          m_iStopTime;            /// as time_t (if you get time_t from CDateTime it is always different)
  CDateTime       m_FirstDay;             /// If it is a repeating timer the first date it starts
  time_t          m_iFirstDay;            ///
  bool            m_Repeat_Mon;           /// Repeat sheduled recording every monday
  bool            m_Repeat_Tue;           /// Repeat sheduled recording every tuesday
  bool            m_Repeat_Wed;           /// Repeat sheduled recording every wednesday
  bool            m_Repeat_Thu;           /// Repeat sheduled recording every thursday
  bool            m_Repeat_Fri;           /// Repeat sheduled recording every friday
  bool            m_Repeat_Sat;           /// Repeat sheduled recording every saturday
  bool            m_Repeat_Sun;           /// Repeat sheduled recording every sunday

  bool            m_recStatus;

  int             m_Priority;             /// Priority of the timer
  int             m_Lifetime;             /// Lifetime of the timer in days

  CStdString      m_strFileNameAndPath;   /// Filename is only for reference

};

typedef std::vector<CTVTimerInfoTag> VECTVTIMERS;
