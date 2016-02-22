#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#ifndef __TIMERS_H
#define __TIMERS_H

#include "libXBMC_pvr.h"
#include <stdlib.h>
#include <string>
#include <ctime>

/* VDR:
enum eTimerFlags { tfNone      = 0x0000,
                   tfActive    = 0x0001,
                   tfInstant   = 0x0002,
                   tfVps       = 0x0004,
                   tfRecording = 0x0008,
                   tfAll       = 0xFFFF,
                 };
*/

// From MediaPortal: TvDatabase.ScheduleRecordingType
enum ScheduleRecordingType
{
  Once = 0,
  Daily = 1,
  Weekly = 2,
  EveryTimeOnThisChannel = 3,
  EveryTimeOnEveryChannel = 4,
  Weekends = 5,
  WorkingDays = 6
};

enum KeepMethodType
{
  UntilSpaceNeeded = 0,
  UntilWatched = 1,
  UntilKeepDate = 2,
  Forever = 3
};

class cTimer
{
  public:
    cTimer();
    cTimer(const PVR_TIMER &timerinfo);
    virtual ~cTimer();

    void GetPVRtimerinfo(PVR_TIMER &tag);
    int Index(void) const { return m_index; }
    unsigned int Channel(void) const { return m_channel; }
    int Priority(void) { return Mepo2XBMCPriority(m_priority); }
    const char* Title(void) const { return m_title.c_str(); }
    const char* Dir(void) const { return m_directory.c_str(); }
    time_t StartTime(void) const;
    time_t EndTime(void) const;
    bool ParseLine(const char *s);
    int PreRecordInterval(void) const { return m_prerecordinterval; }
    int PostRecordInterval(void) const { return m_postrecordinterval; }
    int RepeatFlags() { return SchedRecType2RepeatFlags(m_schedtype); };
    bool Repeat() const { return (m_schedtype == Once ? false : true); };
    bool Done() const { return m_done; };
    bool IsManual() const { return m_ismanual; };
    bool IsActive() const { return !m_canceled; };
    bool IsRecording() const { return m_isrecording; };
    ScheduleRecordingType RepeatFlags2SchedRecType(int repeatflags);
    std::string AddScheduleCommand();
    std::string UpdateScheduleCommand();

  private:
    int SchedRecType2RepeatFlags(ScheduleRecordingType schedtype);

    /**
     * @brief Convert a XBMC Lifetime value to MediaPortals keepMethod+keepDate settings
     * @param lifetime the XBMC lifetime value (in days) (following the VDR syntax)
     * Should be called after setting m_starttime !!
     */
    void SetKeepMethod(int lifetime);
    int GetLifetime(void);
    int XBMC2MepoPriority(int xbmcprio);
    int Mepo2XBMCPriority(int mepoprio);

    // MediaPortal database fields:
    int         m_index;               ///> MediaPortal id_Schedule
    int         m_channel;             ///> MediaPortal idChannel
    ScheduleRecordingType m_schedtype; ///> MediaPortal scheduleType
    std::string m_title;               ///> MediaPortal programName
    time_t      m_starttime;           ///> MediaPortal startTime
    time_t      m_endtime;             ///> MediaPortal endTime
    //                                      skipped: maxAirings field
    int         m_priority;            ///> MediaPortal priority (not the XBMC one!!!)
    std::string m_directory;           ///> MediaPortal directory
    //                                      skipped:  quality field
    KeepMethodType m_keepmethod;       ///> MediaPortal keepMethod
    time_t      m_keepdate;            ///> MediaPortal keepDate
    int         m_prerecordinterval;   ///> MediaPortal preRecordInterval
    int         m_postrecordinterval;  ///> MediaPortal postRecordInterval
    time_t      m_canceled;            ///> MediaPortal canceled (date + time)
    //                                      skipped: recommendedCard
    bool        m_series;              ///> MediaPortal series
    //                                      skipped: idParentSchedule: not yet supported in XBMC

    // XBMC asks for these fields:
    bool        m_active;
    bool        m_done;
    bool        m_ismanual;
    bool        m_isrecording;
};

const time_t cUndefinedDate = 946681200;   ///> 01-01-2000 00:00:00 in time_t
const int    cSecsInDay  = 86400;          ///> Amount of seconds in one day

#endif //__TIMERS_H
