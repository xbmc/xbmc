/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <vector>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

#include "os-dependent.h" //needed for snprintf
#include "client.h"
#include "timers.h"
#include "utils.h"

using namespace ADDON;

cTimer::cTimer()
{
  m_index              = -1;
  m_active             = true;
  m_channel            = 0;
  m_schedtype          = Once;
  m_starttime          = 0;
  m_endtime            = 0;
  m_priority           = 0;
  m_keepmethod         = UntilSpaceNeeded;
  m_keepdate           = cUndefinedDate;
  m_prerecordinterval  = -1; // Use MediaPortal setting instead
  m_postrecordinterval = -1; // Use MediaPortal setting instead
  m_canceled           = cUndefinedDate;
  m_series             = false;
}


cTimer::cTimer(const PVR_TIMER& timerinfo)
{
  m_index = timerinfo.iClientIndex;
  m_active = (timerinfo.state == PVR_TIMER_STATE_SCHEDULED || timerinfo.state == PVR_TIMER_STATE_RECORDING);
  if(!m_active)
  {
    time(&m_canceled);
  }
  else
  {
    // Don't know when it was cancelled, so assume that it was canceled now...
    // backend (TVServerXBMC) will only update the canceled date time when
    // this schedule was just canceled
    m_canceled = cUndefinedDate;
  }
 
  m_title = timerinfo.strTitle;
  m_directory = timerinfo.strDirectory;
  m_channel = timerinfo.iClientChannelUid;
  m_starttime = timerinfo.startTime;
  m_endtime = timerinfo.endTime;
  //m_firstday = timerinfo.firstday;
  m_isrecording = timerinfo.state == PVR_TIMER_STATE_RECORDING;
  m_priority = XBMC2MepoPriority(timerinfo.iPriority);

  SetKeepMethod(timerinfo.iLifetime);
  if(timerinfo.bIsRepeating)
  {
    m_schedtype = RepeatFlags2SchedRecType(timerinfo.iWeekdays);
  } else {
    m_schedtype = Once;
  }

  m_prerecordinterval = timerinfo.iMarginStart;
  m_postrecordinterval = timerinfo.iMarginEnd;
}


cTimer::~cTimer()
{
}

/**
 * @brief Fills the PVR_TIMER struct with information from this timer
 * @param tag A reference to the PVR_TIMER struct
 */
void cTimer::GetPVRtimerinfo(PVR_TIMER &tag)
{
  tag.iClientIndex      = m_index;
  if (m_active)
    tag.state           = PVR_TIMER_STATE_SCHEDULED;
  else if (IsRecording())
    tag.state           = PVR_TIMER_STATE_RECORDING;
  else
    tag.state           = PVR_TIMER_STATE_CANCELLED;
  tag.iClientChannelUid = m_channel;
  tag.strTitle          = m_title.c_str();
  tag.strDirectory      = m_directory.c_str();
  tag.startTime         = m_starttime ;
  tag.endTime           = m_endtime ;
  // From the VDR manual
  // firstday: The date of the first day when this timer shall start recording
  //           (only available for repeating timers).
  if(Repeat())
  {
    tag.firstDay        = m_starttime;
  } else {
    tag.firstDay        = 0;
  }
  tag.iPriority         = Priority();
  tag.iLifetime         = GetLifetime();
  tag.bIsRepeating      = Repeat();
  tag.iWeekdays         = RepeatFlags();
  tag.iMarginStart      = m_prerecordinterval * 60;
  tag.iMarginEnd        = m_postrecordinterval * 60;
  tag.iGenreType        = 0;
  tag.iGenreSubType     = 0;
}

time_t cTimer::StartTime(void) const
{
  return m_starttime;
}

time_t cTimer::EndTime(void) const
{
  return m_endtime;
}

bool cTimer::ParseLine(const char *s)
{
  vector<string> schedulefields;
  string data = s;
  uri::decode(data);

  Tokenize(data, schedulefields, "|");

  if(schedulefields.size() >= 10)
  {
    // field 0 = index
    // field 1 = start date + time
    // field 2 = end   date + time
    // field 3 = channel nr
    // field 4 = channel name
    // field 5 = program name
    // field 6 = schedule recording type
    // field 7 = priority
    // field 8 = isdone (finished)
    // field 9 = ismanual
    // field 10 = directory
    // field 11 = keepmethod (0=until space needed, 1=until watched, 2=until keepdate, 3=forever) (TVServerXBMC build >= 100)
    // field 12 = keepdate (2000-01-01 00:00:00 = infinite)  (TVServerXBMC build >= 100)
    // field 13 = preRecordInterval  (TVServerXBMC build >= 100)
    // field 14 = postRecordInterval (TVServerXBMC build >= 100)
    // field 15 = canceled (TVServerXBMC build >= 100)
    // field 16 = series (True/False) (TVServerXBMC build >= 100)
    // field 17 = isrecording (True/False)

    m_index = atoi(schedulefields[0].c_str());
    m_starttime = DateTimeToTimeT(schedulefields[1]);

    if( m_starttime < 0)
      return false;

    m_endtime = DateTimeToTimeT(schedulefields[2]);

    if( m_endtime < 0)
      return false;

    m_channel = atoi(schedulefields[3].c_str());
    m_title = schedulefields[5];

    m_schedtype = (ScheduleRecordingType) atoi(schedulefields[6].c_str());

    m_priority = atoi(schedulefields[7].c_str());
    m_done = stringtobool(schedulefields[8]);
    m_ismanual = stringtobool(schedulefields[9]);
    m_directory = schedulefields[10];
    
    if(schedulefields.size() >= 18)
    {
      //TVServerXBMC build >= 100
      m_keepmethod = (KeepMethodType) atoi(schedulefields[11].c_str());
      m_keepdate = DateTimeToTimeT(schedulefields[12]);

      if( m_keepdate < 0)
        return false;

      m_prerecordinterval = atoi(schedulefields[13].c_str());
      m_postrecordinterval = atoi(schedulefields[14].c_str());

      // The DateTime value 2000-01-01 00:00:00 means: active in MediaPortal
      if(schedulefields[15].compare("2000-01-01 00:00:00Z")==0)
      {
        m_canceled = cUndefinedDate;
        m_active = true;
      }
      else
      {
        m_canceled = DateTimeToTimeT(schedulefields[15]);
        m_active = false;
      }

      m_series = stringtobool(schedulefields[16]);
      m_isrecording = stringtobool(schedulefields[17]);

    }
    else
    {
      m_keepmethod = UntilSpaceNeeded;
      m_keepdate = cUndefinedDate;
      m_prerecordinterval = -1;
      m_postrecordinterval = -1;
      m_canceled = cUndefinedDate;
      m_active = true;
      m_series = false;
      m_isrecording = false;
    }

    return true;
  }
  return false;
}

int cTimer::SchedRecType2RepeatFlags(ScheduleRecordingType schedtype)
{
  // margro: the meaning of the XBMC-PVR Weekdays field is undocumented.
  // Assuming that VDR is the source for this field:
  //   This field contains a bitmask that correcsponds to the days of the week at which this timer runs
  //   It is based on the VDR Day field format "MTWTF--"
  //   The format is a 1 bit for every enabled day and a 0 bit for a disabled day
  //   Thus: WeekDays = "0000 0001" = "M------" (monday only)
  //                    "0110 0000" = "-----SS" (saturday and sunday)
  //                    "0001 1111" = "MTWTF--" (all weekdays)

  int weekdays = 0;

  switch (schedtype)
  {
    case Once:
      weekdays = 0;
      break;
    case Daily:
      weekdays = 127; // 0111 1111
      break;
    case Weekly:
      {
        // Not sure what to do with this MediaPortal option...
        // Assumption: record once a week, on the same day and time
        // => determine weekday and set the corresponding bit
        struct tm timeinfo;

        timeinfo = *localtime( &m_starttime );

        int weekday = timeinfo.tm_wday; //days since Sunday [0-6]
        // bit 0 = monday, need to convert weekday value to bitnumber:
        if (weekday == 0)
          weekday = 6; //sunday
        else
          weekday--;

        weekdays = 2 << weekday;
        break;
      }
    case EveryTimeOnThisChannel:
      // Don't know what to do with this MediaPortal option?
      break;
    case EveryTimeOnEveryChannel:
      // Don't know what to do with this MediaPortal option?
      break;
    case Weekends:
      weekdays = 96; // 0110 0000
      break;
    case WorkingDays:
      weekdays = 31; // 0001 1111
    default:
      weekdays=0;
  }

  return weekdays;
}

ScheduleRecordingType cTimer::RepeatFlags2SchedRecType(int repeatflags)
{
  // margro: the meaning of the XBMC-PVR Weekdays field is undocumented.
  // Assuming that VDR is the source for this field:
  //   This field contains a bitmask that correcsponds to the days of the week at which this timer runs
  //   It is based on the VDR Day field format "MTWTF--"
  //   The format is a 1 bit for every enabled day and a 0 bit for a disabled day
  //   Thus: WeekDays = "0000 0001" = "M------" (monday only)
  //                    "0110 0000" = "-----SS" (saturday and sunday)
  //                    "0001 1111" = "MTWTF--" (all weekdays)

  switch (repeatflags)
  {
    case 0:
      return Once;
      break;
    case 1: //Monday
    case 2: //Tuesday
    case 4: //Wednesday
    case 8: //Thursday
    case 16: //Friday
    case 32: //Saturday
    case 64: //Sunday
      return Weekly;
      break;
    case 31:  // 0001 1111
      return WorkingDays;
    case 96:  // 0110 0000
      return Weekends;
      break;
    case 127: // 0111 1111
      return Daily;
      break;
    default:
      return Once;
  }

  return Once;
}

std::string cTimer::AddScheduleCommand()
{
  char      command[1024];
  struct tm starttime;
  struct tm endtime;
  struct tm keepdate;

  starttime = *localtime( &m_starttime );
  XBMC->Log(LOG_DEBUG, "Start time: %s, marginstart: %i min earlier", asctime(&starttime), m_prerecordinterval);
  endtime = *localtime( &m_endtime );
  XBMC->Log(LOG_DEBUG, "End time: %s, marginstop: %i min later", asctime(&endtime), m_postrecordinterval);
  keepdate = *localtime( &m_keepdate );

  if ( g_iTVServerXBMCBuild >= 100)
  {
    // Sending separate marginStart, marginStop and schedType is supported
    snprintf(command, 1023, "AddSchedule:%i|%s|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i\n",
            m_channel,                                                         //Channel number [0]
            uri::encode(uri::PATH_TRAITS, m_title).c_str(),                    //Program title  [1]
            starttime.tm_year + 1900, starttime.tm_mon + 1, starttime.tm_mday, //Start date     [2..4]
            starttime.tm_hour, starttime.tm_min, starttime.tm_sec,             //Start time     [5..7]
            endtime.tm_year + 1900, endtime.tm_mon + 1, endtime.tm_mday,       //End date       [8..10]
            endtime.tm_hour, endtime.tm_min, endtime.tm_sec,                   //End time       [11..13]
            (int) m_schedtype, m_priority, (int) m_keepmethod,                 //SchedType, Priority, keepMethod [14..16]
            keepdate.tm_year + 1900, keepdate.tm_mon + 1, keepdate.tm_mday,    //Keepdate       [17..19]
            keepdate.tm_hour, keepdate.tm_min, keepdate.tm_sec,                //Keeptime       [20..22]
            m_prerecordinterval, m_postrecordinterval);                        //Prerecord,postrecord [23,24]
  }
  else
  {
    // Sending a separate marginStart, marginStop and schedType is not yet supported
    snprintf(command, 1023, "AddSchedule:%i|%s|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i\n",
            m_channel,                                                         //Channel number
            uri::encode(uri::PATH_TRAITS, m_title).c_str(),                    //Program title
            starttime.tm_year + 1900, starttime.tm_mon + 1, starttime.tm_mday, //Start date
            starttime.tm_hour, starttime.tm_min, starttime.tm_sec,             //Start time
            endtime.tm_year + 1900, endtime.tm_mon + 1, endtime.tm_mday,       //End date
            endtime.tm_hour, endtime.tm_min, endtime.tm_sec);                  //End time
  }

  return command;
}

std::string cTimer::UpdateScheduleCommand()
{
  char      command[1024];
  struct tm starttime;
  struct tm endtime;
  struct tm keepdate;

  starttime = *localtime( &m_starttime );
  XBMC->Log(LOG_DEBUG, "Start time: %s, marginstart: %i min earlier", asctime(&starttime), m_prerecordinterval);
  endtime = *localtime( &m_endtime );
  XBMC->Log(LOG_DEBUG, "End time: %s, marginstop: %i min later", asctime(&endtime), m_postrecordinterval);
  keepdate = *localtime( &m_keepdate );

  if ( g_iTVServerXBMCBuild >= 100)
  {
    // Sending separate marginStart, marginStop and schedType is supported
    snprintf(command, 1024, "UpdateSchedule:%i|%i|%i|%s|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i\n",
            m_index,                                                           //Schedule index [0]
            m_active,                                                          //Active         [1]
            m_channel,                                                         //Channel number [2]
            uri::encode(uri::PATH_TRAITS,m_title).c_str(),                     //Program title  [3]
            starttime.tm_year + 1900, starttime.tm_mon + 1, starttime.tm_mday, //Start date     [4..6]
            starttime.tm_hour, starttime.tm_min, starttime.tm_sec,             //Start time     [7..9]
            endtime.tm_year + 1900, endtime.tm_mon + 1, endtime.tm_mday,       //End date       [10..12]
            endtime.tm_hour, endtime.tm_min, endtime.tm_sec,                   //End time       [13..15]
            (int) m_schedtype, m_priority, (int) m_keepmethod,                 //SchedType, Priority, keepMethod [16..18]
            keepdate.tm_year + 1900, keepdate.tm_mon + 1, keepdate.tm_mday,    //Keepdate       [19..21]
            keepdate.tm_hour, keepdate.tm_min, keepdate.tm_sec,                //Keeptime       [22..24]
            m_prerecordinterval, m_postrecordinterval);                        //Prerecord,postrecord [25,26]
  }
  else
  {
    snprintf(command, 1024, "UpdateSchedule:%i|%i|%i|%s|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i\n",
            m_index,                                                           //Schedule index
            m_active,                                                          //Active
            m_channel,                                                         //Channel number
            uri::encode(uri::PATH_TRAITS,m_title).c_str(),                       //Program title
            starttime.tm_year + 1900, starttime.tm_mon + 1, starttime.tm_mday, //Start date
            starttime.tm_hour, starttime.tm_min, starttime.tm_sec,             //Start time
            endtime.tm_year + 1900, endtime.tm_mon + 1, endtime.tm_mday,       //End date
            endtime.tm_hour, endtime.tm_min, endtime.tm_sec);                  //End time
  }

  return command;
}


int cTimer::XBMC2MepoPriority(int xbmcprio)
{
  // From XBMC side: 0.99 where 0=lowest and 99=highest priority (like VDR). Default value: 50
  // Meaning of the MediaPortal field is unknown to me. Default seems to be 0.
  // TODO: figure out the mapping
  return 0;
}

int cTimer::Mepo2XBMCPriority(int mepoprio)
{
  return 50; //Default value
}


/*
 * @brief Convert a XBMC Lifetime value to MediaPortals keepMethod+keepDate settings
 * @param lifetime the XBMC lifetime value (in days) (following the VDR syntax)
 * Should be called after setting m_starttime !!
 */
void cTimer::SetKeepMethod(int lifetime)
{
  // XBMC follows the VDR definition of lifetime
  // XBMC: 0 means that this recording may be automatically deleted
  //         at  any  time  by a new recording with higher priority
  //    1-98 means that this recording may not be automatically deleted
  //         in favour of a new recording, until the given number of days
  //         since the start time of the recording has passed by
  //      99 means that this recording will never be automatically deleted
  if (lifetime == 0)
  {
    m_keepmethod = UntilSpaceNeeded;
    m_keepdate = cUndefinedDate;
  }
  else if (lifetime == 99)
  {
    m_keepmethod = Forever;
    m_keepdate = cUndefinedDate;
  }
  else
  {
    m_keepmethod = UntilKeepDate;
    m_keepdate = m_starttime + (lifetime * cSecsInDay);
  }
}

int cTimer::GetLifetime(void)
{
  // margro: the meaning of the XBMC-PVR Lifetime field is undocumented.
  // Assuming that VDR is the source for this field:
  //  The guaranteed lifetime (in days) of a recording created by this
  //  timer.  0 means that this recording may be automatically deleted
  //  at  any  time  by a new recording with higher priority. 99 means
  //  that this recording will never  be  automatically  deleted.  Any
  //  number  in the range 1...98 means that this recording may not be
  //  automatically deleted in favour of a new  recording,  until  the
  //  given  number  of days since the start time of the recording has
  //  passed by
  switch (m_keepmethod)
  {
    case UntilSpaceNeeded: //until space needed
    case UntilWatched: //until watched
      return 0;
      break;
    case UntilKeepDate: //until keepdate
      {
        double diffseconds = difftime(m_keepdate, m_starttime);
        int daysremaining = (int)(diffseconds / cSecsInDay);
        // Calculate value in the range 1...98, based on m_keepdate
        if (daysremaining < 99)
        {
          return daysremaining;
        }
        else
        {
          // > 98 days => return forever
          return 99;
        }
      }
      break;
    case Forever: //forever
      return 99;
    default:
      return 0;
  }
}
