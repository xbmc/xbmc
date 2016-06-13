/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <cstdlib>

#include "XBDateTime.h"
#include "LangInfo.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/Archive.h"
#ifdef TARGET_POSIX
#include "XTimeUtils.h"
#include "XFileUtils.h"
#else
#include <Windows.h>
#endif

#define SECONDS_PER_DAY 86400UL
#define SECONDS_PER_HOUR 3600UL
#define SECONDS_PER_MINUTE 60UL
#define SECONDS_TO_FILETIME 10000000UL

static const char *DAY_NAMES[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static const char *MONTH_NAMES[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

/////////////////////////////////////////////////
//
// CDateTimeSpan
//

CDateTimeSpan::CDateTimeSpan()
{
  m_timeSpan.dwHighDateTime=0;
  m_timeSpan.dwLowDateTime=0;
}

CDateTimeSpan::CDateTimeSpan(const CDateTimeSpan& span)
{
  m_timeSpan.dwHighDateTime=span.m_timeSpan.dwHighDateTime;
  m_timeSpan.dwLowDateTime=span.m_timeSpan.dwLowDateTime;
}

CDateTimeSpan::CDateTimeSpan(int day, int hour, int minute, int second)
{
  SetDateTimeSpan(day, hour, minute, second);
}

bool CDateTimeSpan::operator >(const CDateTimeSpan& right) const
{
  return CompareFileTime(&m_timeSpan, &right.m_timeSpan)>0;
}

bool CDateTimeSpan::operator >=(const CDateTimeSpan& right) const
{
  return operator >(right) || operator ==(right);
}

bool CDateTimeSpan::operator <(const CDateTimeSpan& right) const
{
  return CompareFileTime(&m_timeSpan, &right.m_timeSpan)<0;
}

bool CDateTimeSpan::operator <=(const CDateTimeSpan& right) const
{
  return operator <(right) || operator ==(right);
}

bool CDateTimeSpan::operator ==(const CDateTimeSpan& right) const
{
  return CompareFileTime(&m_timeSpan, &right.m_timeSpan)==0;
}

bool CDateTimeSpan::operator !=(const CDateTimeSpan& right) const
{
  return !operator ==(right);
}

CDateTimeSpan CDateTimeSpan::operator +(const CDateTimeSpan& right) const
{
  CDateTimeSpan left(*this);

  ULARGE_INTEGER timeLeft;
  left.ToULargeInt(timeLeft);

  ULARGE_INTEGER timeRight;
  right.ToULargeInt(timeRight);

  timeLeft.QuadPart+=timeRight.QuadPart;

  left.FromULargeInt(timeLeft);

  return left;
}

CDateTimeSpan CDateTimeSpan::operator -(const CDateTimeSpan& right) const
{
  CDateTimeSpan left(*this);

  ULARGE_INTEGER timeLeft;
  left.ToULargeInt(timeLeft);

  ULARGE_INTEGER timeRight;
  right.ToULargeInt(timeRight);

  timeLeft.QuadPart-=timeRight.QuadPart;

  left.FromULargeInt(timeLeft);

  return left;
}

const CDateTimeSpan& CDateTimeSpan::operator +=(const CDateTimeSpan& right)
{
  ULARGE_INTEGER timeThis;
  ToULargeInt(timeThis);

  ULARGE_INTEGER timeRight;
  right.ToULargeInt(timeRight);

  timeThis.QuadPart+=timeRight.QuadPart;

  FromULargeInt(timeThis);

  return *this;
}

const CDateTimeSpan& CDateTimeSpan::operator -=(const CDateTimeSpan& right)
{
  ULARGE_INTEGER timeThis;
  ToULargeInt(timeThis);

  ULARGE_INTEGER timeRight;
  right.ToULargeInt(timeRight);

  timeThis.QuadPart-=timeRight.QuadPart;

  FromULargeInt(timeThis);

  return *this;
}

void CDateTimeSpan::ToULargeInt(ULARGE_INTEGER& time) const
{
  time.u.HighPart=m_timeSpan.dwHighDateTime;
  time.u.LowPart=m_timeSpan.dwLowDateTime;
}

void CDateTimeSpan::FromULargeInt(const ULARGE_INTEGER& time)
{
  m_timeSpan.dwHighDateTime=time.u.HighPart;
  m_timeSpan.dwLowDateTime=time.u.LowPart;
}

void CDateTimeSpan::SetDateTimeSpan(int day, int hour, int minute, int second)
{
  ULARGE_INTEGER time;
  ToULargeInt(time);

  time.QuadPart=(LONGLONG)day*SECONDS_PER_DAY*SECONDS_TO_FILETIME;
  time.QuadPart+=(LONGLONG)hour*SECONDS_PER_HOUR*SECONDS_TO_FILETIME;
  time.QuadPart+=(LONGLONG)minute*SECONDS_PER_MINUTE*SECONDS_TO_FILETIME;
  time.QuadPart+=(LONGLONG)second*SECONDS_TO_FILETIME;

  FromULargeInt(time);
}

void CDateTimeSpan::SetFromTimeString(const std::string& time) // hh:mm
{
  if (time.size() >= 5 && time[2] == ':')
  {
    int hour    = atoi(time.substr(0, 2).c_str());
    int minutes = atoi(time.substr(3, 2).c_str());
    SetDateTimeSpan(0,hour,minutes,0);
  }
}

int CDateTimeSpan::GetDays() const
{
  ULARGE_INTEGER time;
  ToULargeInt(time);

  return (int)(time.QuadPart/SECONDS_TO_FILETIME)/SECONDS_PER_DAY;
}

int CDateTimeSpan::GetHours() const
{
  ULARGE_INTEGER time;
  ToULargeInt(time);

  return (int)((time.QuadPart/SECONDS_TO_FILETIME)%SECONDS_PER_DAY)/SECONDS_PER_HOUR;
}

int CDateTimeSpan::GetMinutes() const
{
  ULARGE_INTEGER time;
  ToULargeInt(time);

  return (int)((time.QuadPart/SECONDS_TO_FILETIME%SECONDS_PER_DAY)%SECONDS_PER_HOUR)/SECONDS_PER_MINUTE;
}

int CDateTimeSpan::GetSeconds() const
{
  ULARGE_INTEGER time;
  ToULargeInt(time);

  return (int)(((time.QuadPart/SECONDS_TO_FILETIME%SECONDS_PER_DAY)%SECONDS_PER_HOUR)%SECONDS_PER_MINUTE)%SECONDS_PER_MINUTE;
}

int CDateTimeSpan::GetSecondsTotal() const
{
  ULARGE_INTEGER time;
  ToULargeInt(time);
  
  return (int)(time.QuadPart/SECONDS_TO_FILETIME);
}

void CDateTimeSpan::SetFromPeriod(const std::string &period)
{
  long days = atoi(period.c_str());
  // find the first non-space and non-number
  size_t pos = period.find_first_not_of("0123456789 ", 0);
  if (pos != std::string::npos)
  {
    std::string units = period.substr(pos, 3);
    if (StringUtils::EqualsNoCase(units, "wee"))
      days *= 7;
    else if (StringUtils::EqualsNoCase(units, "mon"))
      days *= 31;
  }

  SetDateTimeSpan(days, 0, 0, 0);
}

/////////////////////////////////////////////////
//
// CDateTime
//

CDateTime::CDateTime()
{
  Reset();
}

CDateTime::CDateTime(const SYSTEMTIME &time)
{
  // we store internally as a FILETIME
  m_state = ToFileTime(time, m_time) ? valid : invalid;
}

CDateTime::CDateTime(const FILETIME &time)
{
  m_time=time;
  SetValid(true);
}

CDateTime::CDateTime(const CDateTime& time)
{
  m_time=time.m_time;
  m_state=time.m_state;
}

CDateTime::CDateTime(const time_t& time)
{
  m_state = ToFileTime(time, m_time) ? valid : invalid;
}

CDateTime::CDateTime(const tm& time)
{
  m_state = ToFileTime(time, m_time) ? valid : invalid;
}

CDateTime::CDateTime(int year, int month, int day, int hour, int minute, int second)
{
  SetDateTime(year, month, day, hour, minute, second);
}

CDateTime CDateTime::GetCurrentDateTime()
{
  // get the current time
  SYSTEMTIME time;
  GetLocalTime(&time);

  return CDateTime(time);
}

CDateTime CDateTime::GetUTCDateTime()
{
  CDateTime time(GetCurrentDateTime());
  time += GetTimezoneBias();
  return time;
}

const CDateTime& CDateTime::operator =(const SYSTEMTIME& right)
{
  m_state = ToFileTime(right, m_time) ? valid : invalid;

  return *this;
}

const CDateTime& CDateTime::operator =(const FILETIME& right)
{
  m_time=right;
  SetValid(true);

  return *this;
}

const CDateTime& CDateTime::operator =(const time_t& right)
{
  m_state = ToFileTime(right, m_time) ? valid : invalid;

  return *this;
}

const CDateTime& CDateTime::operator =(const tm& right)
{
  m_state = ToFileTime(right, m_time) ? valid : invalid;

  return *this;
}

bool CDateTime::operator >(const CDateTime& right) const
{
  return operator >(right.m_time);
}

bool CDateTime::operator >=(const CDateTime& right) const
{
  return operator >(right) || operator ==(right);
}

bool CDateTime::operator <(const CDateTime& right) const
{
  return operator <(right.m_time);
}

bool CDateTime::operator <=(const CDateTime& right) const
{
  return operator <(right) || operator ==(right);
}

bool CDateTime::operator ==(const CDateTime& right) const
{
  return operator ==(right.m_time);
}

bool CDateTime::operator !=(const CDateTime& right) const
{
  return !operator ==(right);
}

bool CDateTime::operator >(const FILETIME& right) const
{
  return CompareFileTime(&m_time, &right)>0;
}

bool CDateTime::operator >=(const FILETIME& right) const
{
  return operator >(right) || operator ==(right);
}

bool CDateTime::operator <(const FILETIME& right) const
{
  return CompareFileTime(&m_time, &right)<0;
}

bool CDateTime::operator <=(const FILETIME& right) const
{
  return operator <(right) || operator ==(right);
}

bool CDateTime::operator ==(const FILETIME& right) const
{
  return CompareFileTime(&m_time, &right)==0;
}

bool CDateTime::operator !=(const FILETIME& right) const
{
  return !operator ==(right);
}

bool CDateTime::operator >(const SYSTEMTIME& right) const
{
  FILETIME time;
  ToFileTime(right, time);

  return operator >(time);
}

bool CDateTime::operator >=(const SYSTEMTIME& right) const
{
  return operator >(right) || operator ==(right);
}

bool CDateTime::operator <(const SYSTEMTIME& right) const
{
  FILETIME time;
  ToFileTime(right, time);

  return operator <(time);
}

bool CDateTime::operator <=(const SYSTEMTIME& right) const
{
  return operator <(right) || operator ==(right);
}

bool CDateTime::operator ==(const SYSTEMTIME& right) const
{
  FILETIME time;
  ToFileTime(right, time);

  return operator ==(time);
}

bool CDateTime::operator !=(const SYSTEMTIME& right) const
{
  return !operator ==(right);
}

bool CDateTime::operator >(const time_t& right) const
{
  FILETIME time;
  ToFileTime(right, time);

  return operator >(time);
}

bool CDateTime::operator >=(const time_t& right) const
{
  return operator >(right) || operator ==(right);
}

bool CDateTime::operator <(const time_t& right) const
{
  FILETIME time;
  ToFileTime(right, time);

  return operator <(time);
}

bool CDateTime::operator <=(const time_t& right) const
{
  return operator <(right) || operator ==(right);
}

bool CDateTime::operator ==(const time_t& right) const
{
  FILETIME time;
  ToFileTime(right, time);

  return operator ==(time);
}

bool CDateTime::operator !=(const time_t& right) const
{
  return !operator ==(right);
}

bool CDateTime::operator >(const tm& right) const
{
  FILETIME time;
  ToFileTime(right, time);

  return operator >(time);
}

bool CDateTime::operator >=(const tm& right) const
{
  return operator >(right) || operator ==(right);
}

bool CDateTime::operator <(const tm& right) const
{
  FILETIME time;
  ToFileTime(right, time);

  return operator <(time);
}

bool CDateTime::operator <=(const tm& right) const
{
  return operator <(right) || operator ==(right);
}

bool CDateTime::operator ==(const tm& right) const
{
  FILETIME time;
  ToFileTime(right, time);

  return operator ==(time);
}

bool CDateTime::operator !=(const tm& right) const
{
  return !operator ==(right);
}

CDateTime CDateTime::operator +(const CDateTimeSpan& right) const
{
  CDateTime left(*this);

  ULARGE_INTEGER timeLeft;
  left.ToULargeInt(timeLeft);

  ULARGE_INTEGER timeRight;
  right.ToULargeInt(timeRight);

  timeLeft.QuadPart+=timeRight.QuadPart;

  left.FromULargeInt(timeLeft);

  return left;
}

CDateTime CDateTime::operator -(const CDateTimeSpan& right) const
{
  CDateTime left(*this);

  ULARGE_INTEGER timeLeft;
  left.ToULargeInt(timeLeft);

  ULARGE_INTEGER timeRight;
  right.ToULargeInt(timeRight);

  timeLeft.QuadPart-=timeRight.QuadPart;

  left.FromULargeInt(timeLeft);

  return left;
}

const CDateTime& CDateTime::operator +=(const CDateTimeSpan& right)
{
  ULARGE_INTEGER timeThis;
  ToULargeInt(timeThis);

  ULARGE_INTEGER timeRight;
  right.ToULargeInt(timeRight);

  timeThis.QuadPart+=timeRight.QuadPart;

  FromULargeInt(timeThis);

  return *this;
}

const CDateTime& CDateTime::operator -=(const CDateTimeSpan& right)
{
  ULARGE_INTEGER timeThis;
  ToULargeInt(timeThis);

  ULARGE_INTEGER timeRight;
  right.ToULargeInt(timeRight);

  timeThis.QuadPart-=timeRight.QuadPart;

  FromULargeInt(timeThis);

  return *this;
}

CDateTimeSpan CDateTime::operator -(const CDateTime& right) const
{
  CDateTimeSpan left;

  ULARGE_INTEGER timeLeft;
  left.ToULargeInt(timeLeft);

  ULARGE_INTEGER timeThis;
  ToULargeInt(timeThis);

  ULARGE_INTEGER timeRight;
  right.ToULargeInt(timeRight);

  timeLeft.QuadPart=timeThis.QuadPart-timeRight.QuadPart;

  left.FromULargeInt(timeLeft);

  return left;
}

CDateTime::operator FILETIME() const
{
  return m_time;
}

void CDateTime::Archive(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar<<(int)m_state;
    if (m_state==valid)
    {
      SYSTEMTIME st;
      GetAsSystemTime(st);
      ar<<st;
    }
  }
  else
  {
    Reset();
    int state;
    ar >> (int &)state;
    m_state = CDateTime::STATE(state);
    if (m_state==valid)
    {
      SYSTEMTIME st;
      ar>>st;
      ToFileTime(st, m_time);
    }
  }
}

void CDateTime::Reset()
{
  SetDateTime(1601, 1, 1, 0, 0, 0);
  SetValid(false);
}

void CDateTime::SetValid(bool yesNo)
{
  m_state=yesNo ? valid : invalid;
}

bool CDateTime::IsValid() const
{
  return m_state==valid;
}

bool CDateTime::ToFileTime(const SYSTEMTIME& time, FILETIME& fileTime) const
{
  return SystemTimeToFileTime(&time, &fileTime) == TRUE &&
         (fileTime.dwLowDateTime > 0 || fileTime.dwHighDateTime > 0);
}

bool CDateTime::ToFileTime(const time_t& time, FILETIME& fileTime) const
{
  LONGLONG ll = Int32x32To64(time, 10000000)+0x19DB1DED53E8000LL;

  fileTime.dwLowDateTime  = (DWORD)(ll & 0xFFFFFFFF);
  fileTime.dwHighDateTime = (DWORD)(ll >> 32);

  return true;
}

bool CDateTime::ToFileTime(const tm& time, FILETIME& fileTime) const
{
  SYSTEMTIME st;
  ZeroMemory(&st, sizeof(SYSTEMTIME));

  st.wYear=time.tm_year+1900;
  st.wMonth=time.tm_mon+1;
  st.wDayOfWeek=time.tm_wday;
  st.wDay=time.tm_mday;
  st.wHour=time.tm_hour;
  st.wMinute=time.tm_min;
  st.wSecond=time.tm_sec;

  return SystemTimeToFileTime(&st, &fileTime)==TRUE;
}

void CDateTime::ToULargeInt(ULARGE_INTEGER& time) const
{
  time.u.HighPart=m_time.dwHighDateTime;
  time.u.LowPart=m_time.dwLowDateTime;
}

void CDateTime::FromULargeInt(const ULARGE_INTEGER& time)
{
  m_time.dwHighDateTime=time.u.HighPart;
  m_time.dwLowDateTime=time.u.LowPart;
}

bool CDateTime::SetFromDateString(const std::string &date)
{
  //! @todo STRING_CLEANUP
  if (date.empty())
  {
    SetValid(false);
    return false;
  }

  if (SetFromDBDate(date))
    return true;

  const char* months[] = {"january","february","march","april","may","june","july","august","september","october","november","december",NULL};
  int j=0;
  size_t iDayPos = date.find("day");
  size_t iPos = date.find(' ');
  if (iDayPos < iPos && iDayPos != std::string::npos)
  {
    iDayPos = iPos + 1;
    iPos = date.find(' ', iPos+1);
  }
  else
    iDayPos = 0;

  std::string strMonth = date.substr(iDayPos, iPos - iDayPos);
  if (strMonth.empty())
    return false;

  size_t iPos2 = date.find(",");
  std::string strDay = (date.size() >= iPos) ? date.substr(iPos, iPos2-iPos) : "";
  std::string strYear = date.substr(date.find(' ', iPos2) + 1);
  while (months[j] && stricmp(strMonth.c_str(),months[j]) != 0)
    j++;
  if (!months[j])
    return false;

  return SetDateTime(atol(strYear.c_str()),j+1,atol(strDay.c_str()),0,0,0);
}

int CDateTime::GetDay() const
{
  SYSTEMTIME st;
  GetAsSystemTime(st);

  return st.wDay;
}

int CDateTime::GetMonth() const
{
  SYSTEMTIME st;
  GetAsSystemTime(st);

  return st.wMonth;
}

int CDateTime::GetYear() const
{
  SYSTEMTIME st;
  GetAsSystemTime(st);

  return st.wYear;
}

int CDateTime::GetHour() const
{
  SYSTEMTIME st;
  GetAsSystemTime(st);

  return st.wHour;
}

int CDateTime::GetMinute() const
{
  SYSTEMTIME st;
  GetAsSystemTime(st);

  return st.wMinute;
}

int CDateTime::GetSecond() const
{
  SYSTEMTIME st;
  GetAsSystemTime(st);

  return st.wSecond;
}

int CDateTime::GetDayOfWeek() const
{
  SYSTEMTIME st;
  GetAsSystemTime(st);

  return st.wDayOfWeek;
}

int CDateTime::GetMinuteOfDay() const
{
  SYSTEMTIME st;
  GetAsSystemTime(st);
  return st.wHour*60+st.wMinute;
}

bool CDateTime::SetDateTime(int year, int month, int day, int hour, int minute, int second)
{
  SYSTEMTIME st;
  ZeroMemory(&st, sizeof(SYSTEMTIME));

  st.wYear=year;
  st.wMonth=month;
  st.wDay=day;
  st.wHour=hour;
  st.wMinute=minute;
  st.wSecond=second;

  m_state = ToFileTime(st, m_time) ? valid : invalid;
  return m_state == valid;
}

bool CDateTime::SetDate(int year, int month, int day)
{
  return SetDateTime(year, month, day, 0, 0, 0);
}

bool CDateTime::SetTime(int hour, int minute, int second)
{
  // 01.01.1601 00:00:00 is 0 as filetime
  return SetDateTime(1601, 1, 1, hour, minute, second);
}

void CDateTime::GetAsSystemTime(SYSTEMTIME& time) const
{
  FileTimeToSystemTime(&m_time, &time);
}

#define UNIX_BASE_TIME 116444736000000000LL /* nanoseconds since epoch */
void CDateTime::GetAsTime(time_t& time) const
{
  LONGLONG ll;
  ll = ((LONGLONG)m_time.dwHighDateTime << 32) + m_time.dwLowDateTime;
  time=(time_t)((ll - UNIX_BASE_TIME) / 10000000);
}

void CDateTime::GetAsTm(tm& time) const
{
  SYSTEMTIME st;
  GetAsSystemTime(st);

  time.tm_year=st.wYear-1900;
  time.tm_mon=st.wMonth-1;
  time.tm_wday=st.wDayOfWeek;
  time.tm_mday=st.wDay;
  time.tm_hour=st.wHour;
  time.tm_min=st.wMinute;
  time.tm_sec=st.wSecond;

  mktime(&time);
}

void CDateTime::GetAsTimeStamp(FILETIME& time) const
{
  ::LocalFileTimeToFileTime(&m_time, &time);
}

std::string CDateTime::GetAsDBDate() const
{
  SYSTEMTIME st;
  GetAsSystemTime(st);

  return StringUtils::Format("%04i-%02i-%02i", st.wYear, st.wMonth, st.wDay);
}

std::string CDateTime::GetAsDBDateTime() const
{
  SYSTEMTIME st;
  GetAsSystemTime(st);

  return StringUtils::Format("%04i-%02i-%02i %02i:%02i:%02i", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
}

std::string CDateTime::GetAsSaveString() const
{
  SYSTEMTIME st;
  GetAsSystemTime(st);

  return StringUtils::Format("%04i%02i%02i_%02i%02i%02i", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
}

bool CDateTime::SetFromUTCDateTime(const CDateTime &dateTime)
{
  CDateTime tmp(dateTime);
  tmp -= GetTimezoneBias();

  m_time = tmp.m_time;
  m_state = tmp.m_state;
  return m_state == valid;
}

static bool bGotTimezoneBias = false;

void CDateTime::ResetTimezoneBias(void)
{
  bGotTimezoneBias = false;
}

CDateTimeSpan CDateTime::GetTimezoneBias(void)
{
  static CDateTimeSpan timezoneBias;

  if (!bGotTimezoneBias)
  {
    bGotTimezoneBias = true;
    TIME_ZONE_INFORMATION tz;
    switch(GetTimeZoneInformation(&tz))
    {
      case TIME_ZONE_ID_DAYLIGHT:
        timezoneBias = CDateTimeSpan(0, 0, tz.Bias + tz.DaylightBias, 0);
        break;
      case TIME_ZONE_ID_STANDARD:
        timezoneBias = CDateTimeSpan(0, 0, tz.Bias + tz.StandardBias, 0);
        break;
      case TIME_ZONE_ID_UNKNOWN:
        timezoneBias = CDateTimeSpan(0, 0, tz.Bias, 0);
        break;
    }
  }

  return timezoneBias;
}

bool CDateTime::SetFromUTCDateTime(const time_t &dateTime)
{
  CDateTime tmp(dateTime);
  return SetFromUTCDateTime(tmp);
}

bool CDateTime::SetFromW3CDate(const std::string &dateTime)
{
  std::string date;

  size_t posT = dateTime.find("T");
  if(posT != std::string::npos)
    date = dateTime.substr(0, posT);
  else
    date = dateTime;

  int year = 0, month = 1, day = 1;

  if (date.size() >= 4)
    year  = atoi(date.substr(0, 4).c_str());

  if (date.size() >= 10)
  {
    month = atoi(date.substr(5, 2).c_str());
    day   = atoi(date.substr(8, 2).c_str());
  }

  CDateTime tmpDateTime(year, month, day, 0, 0, 0);
  if (tmpDateTime.IsValid())
    *this = tmpDateTime;

  return IsValid();
}

bool CDateTime::SetFromW3CDateTime(const std::string &dateTime, bool ignoreTimezone /* = false */)
{
  std::string date, time, zone;

  size_t posT = dateTime.find("T");
  if(posT != std::string::npos)
  {
    date = dateTime.substr(0, posT);
    std::string::size_type posZ = dateTime.find_first_of("+-Z", posT);
    if(posZ == std::string::npos)
      time = dateTime.substr(posT + 1);
    else
    {
      time = dateTime.substr(posT + 1, posZ - posT - 1);
      zone = dateTime.substr(posZ);
    }
  }
  else
    date = dateTime;

  int year = 0, month = 1, day = 1, hour = 0, min = 0, sec = 0;

  if (date.size() >= 4)
    year  = atoi(date.substr(0, 4).c_str());

  if (date.size() >= 10)
  {
    month = atoi(date.substr(5, 2).c_str());
    day   = atoi(date.substr(8, 2).c_str());
  }

  if (time.length() >= 5)
  {
    hour = atoi(time.substr(0, 2).c_str());
    min  = atoi(time.substr(3, 2).c_str());
  }

  if (time.length() >= 8)
    sec  = atoi(time.substr(6, 2).c_str());

  CDateTime tmpDateTime(year, month, day, hour, min, sec);
  if (!tmpDateTime.IsValid())
    return false;

  if (!ignoreTimezone && !zone.empty())
  {
    // check if the timezone is UTC
    if (StringUtils::StartsWith(zone, "Z"))
      return SetFromUTCDateTime(tmpDateTime);
    else
    {
      // retrieve the timezone offset (ignoring the + or -)
      CDateTimeSpan zoneSpan; zoneSpan.SetFromTimeString(zone.substr(1));
      if (zoneSpan.GetSecondsTotal() != 0)
      {
        if (StringUtils::StartsWith(zone, "+"))
          tmpDateTime += zoneSpan;
        else if (StringUtils::StartsWith(zone, "-"))
          tmpDateTime -= zoneSpan;
      }
    }
  }

  *this = tmpDateTime;
  return IsValid();
}

bool CDateTime::SetFromDBDateTime(const std::string &dateTime)
{
  // assumes format YYYY-MM-DD HH:MM:SS
  if (dateTime.size() == 19)
  {
    int year  = atoi(dateTime.substr(0, 4).c_str());
    int month = atoi(dateTime.substr(5, 2).c_str());
    int day   = atoi(dateTime.substr(8, 2).c_str());
    int hour  = atoi(dateTime.substr(11, 2).c_str());
    int min   = atoi(dateTime.substr(14, 2).c_str());
    int sec   = atoi(dateTime.substr(17, 2).c_str());
    return SetDateTime(year, month, day, hour, min, sec);
  }
  return false;
}

bool CDateTime::SetFromDBDate(const std::string &date)
{
  if (date.size() < 10)
    return false;
  // assumes format:
  // YYYY-MM-DD or DD-MM-YYYY
  const static std::string sep_chars = "-./";
  int year = 0, month = 0, day = 0;
  if (sep_chars.find(date[2]) != std::string::npos)
  {
    day = atoi(date.substr(0, 2).c_str());
    month = atoi(date.substr(3, 2).c_str());
    year = atoi(date.substr(6, 4).c_str());
  }
  else if (sep_chars.find(date[4]) != std::string::npos)
  {
    year = atoi(date.substr(0, 4).c_str());
    month = atoi(date.substr(5, 2).c_str());
    day = atoi(date.substr(8, 2).c_str());
  }
  return SetDate(year, month, day);
}

bool CDateTime::SetFromDBTime(const std::string &time)
{
  if (time.size() < 8)
    return false;
  // assumes format:
  // HH:MM:SS
  int hour, minute, second;

  hour   = atoi(time.substr(0, 2).c_str());
  minute = atoi(time.substr(3, 2).c_str());
  second = atoi(time.substr(6, 2).c_str());

  return SetTime(hour, minute, second);
}

bool CDateTime::SetFromRFC1123DateTime(const std::string &dateTime)
{
  std::string date = dateTime;
  StringUtils::Trim(date);

  if (date.size() != 29)
    return false;

  int day  = strtol(date.substr(5, 2).c_str(), NULL, 10);

  std::string strMonth = date.substr(8, 3);
  int month = 0;
  for (unsigned int index = 0; index < 12; index++)
  {
    if (strMonth == MONTH_NAMES[index])
    {
      month = index + 1;
      break;
    }
  }

  if (month < 1)
    return false;

  int year = strtol(date.substr(12, 4).c_str(), NULL, 10);
  int hour = strtol(date.substr(17, 2).c_str(), NULL, 10);
  int min  = strtol(date.substr(20, 2).c_str(), NULL, 10);
  int sec  = strtol(date.substr(23, 2).c_str(), NULL, 10);

  return SetDateTime(year, month, day, hour, min, sec);
}

CDateTime CDateTime::FromDateString(const std::string &date)
{
  CDateTime dt;
  dt.SetFromDateString(date);
  return dt;
}

CDateTime CDateTime::FromDBDateTime(const std::string &dateTime)
{
  CDateTime dt;
  dt.SetFromDBDateTime(dateTime);
  return dt;
}

CDateTime CDateTime::FromDBDate(const std::string &date)
{
  CDateTime dt;
  dt.SetFromDBDate(date);
  return dt;
}

CDateTime CDateTime::FromDBTime(const std::string &time)
{
  CDateTime dt;
  dt.SetFromDBTime(time);
  return dt;
}

CDateTime CDateTime::FromW3CDate(const std::string &date)
{
  CDateTime dt;
  dt.SetFromW3CDate(date);
  return dt;
}

CDateTime CDateTime::FromW3CDateTime(const std::string &date, bool ignoreTimezone /* = false */)
{
  CDateTime dt;
  dt.SetFromW3CDateTime(date, ignoreTimezone);
  return dt;
}

CDateTime CDateTime::FromUTCDateTime(const CDateTime &dateTime)
{
  CDateTime dt;
  dt.SetFromUTCDateTime(dateTime);
  return dt;
}

CDateTime CDateTime::FromUTCDateTime(const time_t &dateTime)
{
  CDateTime dt;
  dt.SetFromUTCDateTime(dateTime);
  return dt;
}

CDateTime CDateTime::FromRFC1123DateTime(const std::string &dateTime)
{
  CDateTime dt;
  dt.SetFromRFC1123DateTime(dateTime);
  return dt;
}

std::string CDateTime::GetAsLocalizedTime(const std::string &format, bool withSeconds) const
{
  std::string strOut;
  const std::string& strFormat = format.empty() ? g_langInfo.GetTimeFormat() : format;

  SYSTEMTIME dateTime;
  GetAsSystemTime(dateTime);

  // Prefetch meridiem symbol
  const std::string& strMeridiem = CLangInfo::MeridiemSymbolToString(dateTime.wHour > 11 ? MeridiemSymbolPM : MeridiemSymbolAM);

  size_t length = strFormat.size();
  for (size_t i=0; i < length; ++i)
  {
    char c=strFormat[i];
    if (c=='\'')
    {
      // To be able to display a "'" in the string,
      // find the last "'" that doesn't follow a "'"
      size_t pos=i + 1;
      while(((pos = strFormat.find(c, pos + 1)) != std::string::npos &&
             pos<strFormat.size()) && strFormat[pos+1]=='\'') {}

      std::string strPart;
      if (pos != std::string::npos)
      {
        // Extract string between ' '
        strPart=strFormat.substr(i + 1, pos - i - 1);
        i=pos;
      }
      else
      {
        strPart=strFormat.substr(i + 1, length - i - 1);
        i=length;
      }

      StringUtils::Replace(strPart, "''", "'");

      strOut+=strPart;
    }
    else if (c=='h' || c=='H') // parse hour (H="24 hour clock")
    {
      int partLength=0;

      int pos=strFormat.find_first_not_of(c,i+1);
      if (pos>-1)
      {
        // Get length of the hour mask, eg. HH
        partLength=pos-i;
        i=pos-1;
      }
      else
      {
        // mask ends at the end of the string, extract it
        partLength=length-i;
        i=length;
      }

      int hour=dateTime.wHour;
      if (c=='h')
      { // recalc to 12 hour clock
        if (hour > 11)
          hour -= (12 * (hour > 12));
        else
          hour += (12 * (hour < 1));
      }

      // Format hour string with the length of the mask
      std::string str;
      if (partLength==1)
        str = StringUtils::Format("%d", hour);
      else
        str = StringUtils::Format("%02d", hour);

      strOut+=str;
    }
    else if (c=='m') // parse minutes
    {
      int partLength=0;

      int pos=strFormat.find_first_not_of(c,i+1);
      if (pos>-1)
      {
        // Get length of the minute mask, eg. mm
        partLength=pos-i;
        i=pos-1;
      }
      else
      {
        // mask ends at the end of the string, extract it
        partLength=length-i;
        i=length;
      }

      // Format minute string with the length of the mask
      std::string str;
      if (partLength==1)
        str = StringUtils::Format("%d", dateTime.wMinute);
      else
        str = StringUtils::Format("%02d", dateTime.wMinute);

      strOut+=str;
    }
    else if (c=='s') // parse seconds
    {
      int partLength=0;

      int pos=strFormat.find_first_not_of(c,i+1);
      if (pos>-1)
      {
        // Get length of the seconds mask, eg. ss
        partLength=pos-i;
        i=pos-1;
      }
      else
      {
        // mask ends at the end of the string, extract it
        partLength=length-i;
        i=length;
      }

      if (withSeconds)
      {
        // Format seconds string with the length of the mask
        std::string str;
        if (partLength==1)
          str = StringUtils::Format("%d", dateTime.wSecond);
        else
          str = StringUtils::Format("%02d", dateTime.wSecond);

        strOut+=str;
      }
      else
        strOut.erase(strOut.size()-1,1);
    }
    else if (c=='x') // add meridiem symbol
    {
      int pos=strFormat.find_first_not_of(c,i+1);
      if (pos>-1)
      {
        // Get length of the meridiem mask
        i=pos-1;
      }
      else
      {
        // mask ends at the end of the string, extract it
        i=length;
      }

      strOut+=strMeridiem;
    }
    else // everything else pass to output
      strOut+=c;
  }

  return strOut;
}

std::string CDateTime::GetAsLocalizedDate(bool longDate/*=false*/) const
{
  return GetAsLocalizedDate(g_langInfo.GetDateFormat(longDate));
}

std::string CDateTime::GetAsLocalizedDate(const std::string &strFormat) const
{
  std::string strOut;

  SYSTEMTIME dateTime;
  GetAsSystemTime(dateTime);

  size_t length = strFormat.size();
  for (size_t i = 0; i < length; ++i)
  {
    char c=strFormat[i];
    if (c=='\'')
    {
      // To be able to display a "'" in the string,
      // find the last "'" that doesn't follow a "'"
      size_t pos = i + 1;
      while(((pos = strFormat.find(c, pos + 1)) != std::string::npos &&
             pos < strFormat.size()) &&
            strFormat[pos + 1] == '\'') {}

      std::string strPart;
      if (pos != std::string::npos)
      {
        // Extract string between ' '
        strPart = strFormat.substr(i + 1, pos - i - 1);
        i = pos;
      }
      else
      {
        strPart = strFormat.substr(i + 1, length - i - 1);
        i = length;
      }
      StringUtils::Replace(strPart, "''", "'");
      strOut+=strPart;
    }
    else if (c=='D' || c=='d') // parse days
    {
      size_t partLength=0;

      size_t pos = strFormat.find_first_not_of(c, i+1);
      if (pos != std::string::npos)
      {
        // Get length of the day mask, eg. DDDD
        partLength=pos-i;
        i=pos-1;
      }
      else
      {
        // mask ends at the end of the string, extract it
        partLength=length-i;
        i=length;
      }

      // Format string with the length of the mask
      std::string str;
      if (partLength==1) // single-digit number
        str = StringUtils::Format("%d", dateTime.wDay);
      else if (partLength==2) // two-digit number
        str = StringUtils::Format("%02d", dateTime.wDay);
      else // Day of week string
      {
        int wday = dateTime.wDayOfWeek;
        if (wday < 1 || wday > 7) wday = 7;
        str = g_localizeStrings.Get((c =='d' ? 40 : 10) + wday);
      }
      strOut+=str;
    }
    else if (c=='M' || c=='m') // parse month
    {
      size_t partLength=0;

      size_t pos=strFormat.find_first_not_of(c,i+1);
      if (pos != std::string::npos)
      {
        // Get length of the month mask, eg. MMMM
        partLength=pos-i;
        i=pos-1;
      }
      else
      {
        // mask ends at the end of the string, extract it
        partLength=length-i;
        i=length;
      }

      // Format string with the length of the mask
      std::string str;
      if (partLength==1) // single-digit number
        str = StringUtils::Format("%d", dateTime.wMonth);
      else if (partLength==2) // two-digit number
        str = StringUtils::Format("%02d", dateTime.wMonth);
      else // Month string
      {
        int wmonth = dateTime.wMonth;
        if (wmonth < 1 || wmonth > 12) wmonth = 12;
        str = g_localizeStrings.Get((c =='m' ? 50 : 20) + wmonth);
      }
      strOut+=str;
    }
    else if (c=='Y' || c =='y') // parse year
    {
      size_t partLength = 0;

      size_t pos = strFormat.find_first_not_of(c,i+1);
      if (pos != std::string::npos)
      {
        // Get length of the year mask, eg. YYYY
        partLength=pos-i;
        i=pos-1;
      }
      else
      {
        // mask ends at the end of the string, extract it
        partLength=length-i;
        i=length;
      }

      // Format string with the length of the mask
      std::string str = StringUtils::Format("%d", dateTime.wYear); // four-digit number
      if (partLength <= 2)
        str.erase(0, 2); // two-digit number

      strOut+=str;
    }
    else // everything else pass to output
      strOut+=c;
  }

  return strOut;
}

std::string CDateTime::GetAsLocalizedDateTime(bool longDate/*=false*/, bool withSeconds/*=true*/) const
{
  return GetAsLocalizedDate(longDate) + ' ' + GetAsLocalizedTime("", withSeconds);
}

CDateTime CDateTime::GetAsUTCDateTime() const
{
  CDateTime time(m_time);
  time += GetTimezoneBias();
  return time;
}

std::string CDateTime::GetAsRFC1123DateTime() const
{
  CDateTime time(GetAsUTCDateTime());

  int weekDay = time.GetDayOfWeek();
  if (weekDay < 0)
    weekDay = 0;
  else if (weekDay > 6)
    weekDay = 6;
  if (weekDay != time.GetDayOfWeek())
    CLog::Log(LOGWARNING, "Invalid day of week %d in %s", time.GetDayOfWeek(), time.GetAsDBDateTime().c_str());

  int month = time.GetMonth();
  if (month < 1)
    month = 1;
  else if (month > 12)
    month = 12;
  if (month != time.GetMonth())
    CLog::Log(LOGWARNING, "Invalid month %d in %s", time.GetMonth(), time.GetAsDBDateTime().c_str());

  return StringUtils::Format("%s, %02i %s %04i %02i:%02i:%02i GMT", DAY_NAMES[weekDay], time.GetDay(), MONTH_NAMES[month - 1], time.GetYear(), time.GetHour(), time.GetMinute(), time.GetSecond());
}

std::string CDateTime::GetAsW3CDate() const
{
  SYSTEMTIME st;
  GetAsSystemTime(st);

  return StringUtils::Format("%04i-%02i-%02i", st.wYear, st.wMonth, st.wDay);
}

std::string CDateTime::GetAsW3CDateTime(bool asUtc /* = false */) const
{
  CDateTime w3cDate = *this;
  if (asUtc)
    w3cDate = GetAsUTCDateTime();
  SYSTEMTIME st;
  w3cDate.GetAsSystemTime(st);

  std::string result = StringUtils::Format("%04i-%02i-%02iT%02i:%02i:%02i", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
  if (asUtc)
    return result + "Z";

  CDateTimeSpan bias = GetTimezoneBias();
  return result + StringUtils::Format("%c%02i:%02i", (bias.GetSecondsTotal() >= 0 ? '+' : '-'), abs(bias.GetHours()), abs(bias.GetMinutes())).c_str();
}

int CDateTime::MonthStringToMonthNum(const std::string& month)
{
  const char* months[] = {"january","february","march","april","may","june","july","august","september","october","november","december"};
  const char* abr_months[] = {"jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec"};

  int i = 0;
  for (; i < 12 && !StringUtils::EqualsNoCase(month, months[i]) && !StringUtils::EqualsNoCase(month, abr_months[i]); i++);
  i++;

  return i;
}
