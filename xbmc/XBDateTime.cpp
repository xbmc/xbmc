/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "XBDateTime.h"

#include "LangInfo.h"
#include "guilib/LocalizeStrings.h"
#include "utils/Archive.h"
#include "utils/StringUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#include <cstdlib>

#define SECONDS_PER_DAY 86400L
#define SECONDS_PER_HOUR 3600L
#define SECONDS_PER_MINUTE 60L
#define SECONDS_TO_FILETIME 10000000L

static const char *DAY_NAMES[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static const char *MONTH_NAMES[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

/////////////////////////////////////////////////
//
// CDateTimeSpan
//

CDateTimeSpan::CDateTimeSpan()
{
  m_timeSpan.highDateTime = 0;
  m_timeSpan.lowDateTime = 0;
}

CDateTimeSpan::CDateTimeSpan(const CDateTimeSpan& span)
{
  m_timeSpan.highDateTime = span.m_timeSpan.highDateTime;
  m_timeSpan.lowDateTime = span.m_timeSpan.lowDateTime;
}

CDateTimeSpan::CDateTimeSpan(int day, int hour, int minute, int second)
{
  SetDateTimeSpan(day, hour, minute, second);
}

bool CDateTimeSpan::operator >(const CDateTimeSpan& right) const
{
  return KODI::TIME::CompareFileTime(&m_timeSpan, &right.m_timeSpan) > 0;
}

bool CDateTimeSpan::operator >=(const CDateTimeSpan& right) const
{
  return operator >(right) || operator ==(right);
}

bool CDateTimeSpan::operator <(const CDateTimeSpan& right) const
{
  return KODI::TIME::CompareFileTime(&m_timeSpan, &right.m_timeSpan) < 0;
}

bool CDateTimeSpan::operator <=(const CDateTimeSpan& right) const
{
  return operator <(right) || operator ==(right);
}

bool CDateTimeSpan::operator ==(const CDateTimeSpan& right) const
{
  return KODI::TIME::CompareFileTime(&m_timeSpan, &right.m_timeSpan) == 0;
}

bool CDateTimeSpan::operator !=(const CDateTimeSpan& right) const
{
  return !operator ==(right);
}

CDateTimeSpan CDateTimeSpan::operator +(const CDateTimeSpan& right) const
{
  CDateTimeSpan left(*this);

  LARGE_INTEGER timeLeft;
  left.ToLargeInt(timeLeft);

  LARGE_INTEGER timeRight;
  right.ToLargeInt(timeRight);

  timeLeft.QuadPart+=timeRight.QuadPart;

  left.FromLargeInt(timeLeft);

  return left;
}

CDateTimeSpan CDateTimeSpan::operator -(const CDateTimeSpan& right) const
{
  CDateTimeSpan left(*this);

  LARGE_INTEGER timeLeft;
  left.ToLargeInt(timeLeft);

  LARGE_INTEGER timeRight;
  right.ToLargeInt(timeRight);

  timeLeft.QuadPart-=timeRight.QuadPart;

  left.FromLargeInt(timeLeft);

  return left;
}

const CDateTimeSpan& CDateTimeSpan::operator +=(const CDateTimeSpan& right)
{
  LARGE_INTEGER timeThis;
  ToLargeInt(timeThis);

  LARGE_INTEGER timeRight;
  right.ToLargeInt(timeRight);

  timeThis.QuadPart+=timeRight.QuadPart;

  FromLargeInt(timeThis);

  return *this;
}

const CDateTimeSpan& CDateTimeSpan::operator -=(const CDateTimeSpan& right)
{
  LARGE_INTEGER timeThis;
  ToLargeInt(timeThis);

  LARGE_INTEGER timeRight;
  right.ToLargeInt(timeRight);

  timeThis.QuadPart-=timeRight.QuadPart;

  FromLargeInt(timeThis);

  return *this;
}

void CDateTimeSpan::ToLargeInt(LARGE_INTEGER& time) const
{
  time.u.HighPart = m_timeSpan.highDateTime;
  time.u.LowPart = m_timeSpan.lowDateTime;
}

void CDateTimeSpan::FromLargeInt(const LARGE_INTEGER& time)
{
  m_timeSpan.highDateTime = time.u.HighPart;
  m_timeSpan.lowDateTime = time.u.LowPart;
}

void CDateTimeSpan::SetDateTimeSpan(int day, int hour, int minute, int second)
{
  LARGE_INTEGER time;
  ToLargeInt(time);

  time.QuadPart= static_cast<long long>(day) *SECONDS_PER_DAY*SECONDS_TO_FILETIME;
  time.QuadPart+= static_cast<long long>(hour) *SECONDS_PER_HOUR*SECONDS_TO_FILETIME;
  time.QuadPart+= static_cast<long long>(minute) *SECONDS_PER_MINUTE*SECONDS_TO_FILETIME;
  time.QuadPart+= static_cast<long long>(second) *SECONDS_TO_FILETIME;

  FromLargeInt(time);
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
  LARGE_INTEGER time;
  ToLargeInt(time);

  return (int)(time.QuadPart/SECONDS_TO_FILETIME)/SECONDS_PER_DAY;
}

int CDateTimeSpan::GetHours() const
{
  LARGE_INTEGER time;
  ToLargeInt(time);

  return (int)((time.QuadPart/SECONDS_TO_FILETIME)%SECONDS_PER_DAY)/SECONDS_PER_HOUR;
}

int CDateTimeSpan::GetMinutes() const
{
  LARGE_INTEGER time;
  ToLargeInt(time);

  return (int)((time.QuadPart/SECONDS_TO_FILETIME%SECONDS_PER_DAY)%SECONDS_PER_HOUR)/SECONDS_PER_MINUTE;
}

int CDateTimeSpan::GetSeconds() const
{
  LARGE_INTEGER time;
  ToLargeInt(time);

  return (int)(((time.QuadPart/SECONDS_TO_FILETIME%SECONDS_PER_DAY)%SECONDS_PER_HOUR)%SECONDS_PER_MINUTE)%SECONDS_PER_MINUTE;
}

int CDateTimeSpan::GetSecondsTotal() const
{
  LARGE_INTEGER time;
  ToLargeInt(time);

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

CDateTime::CDateTime(const KODI::TIME::SystemTime& time)
{
  // we store internally as a FileTime
  m_state = ToFileTime(time, m_time) ? valid : invalid;
}

CDateTime::CDateTime(const KODI::TIME::FileTime& time) : m_time(time)
{
  SetValid(true);
}

CDateTime::CDateTime(const CDateTime& time) : m_time(time.m_time)
{
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
  KODI::TIME::SystemTime time;
  KODI::TIME::GetLocalTime(&time);

  return CDateTime(time);
}

CDateTime CDateTime::GetUTCDateTime()
{
  CDateTime time(GetCurrentDateTime());
  time += GetTimezoneBias();
  return time;
}

const CDateTime& CDateTime::operator=(const KODI::TIME::SystemTime& right)
{
  m_state = ToFileTime(right, m_time) ? valid : invalid;

  return *this;
}

const CDateTime& CDateTime::operator=(const KODI::TIME::FileTime& right)
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

bool CDateTime::operator>(const KODI::TIME::FileTime& right) const
{
  return KODI::TIME::CompareFileTime(&m_time, &right) > 0;
}

bool CDateTime::operator>=(const KODI::TIME::FileTime& right) const
{
  return operator >(right) || operator ==(right);
}

bool CDateTime::operator<(const KODI::TIME::FileTime& right) const
{
  return KODI::TIME::CompareFileTime(&m_time, &right) < 0;
}

bool CDateTime::operator<=(const KODI::TIME::FileTime& right) const
{
  return operator <(right) || operator ==(right);
}

bool CDateTime::operator==(const KODI::TIME::FileTime& right) const
{
  return KODI::TIME::CompareFileTime(&m_time, &right) == 0;
}

bool CDateTime::operator!=(const KODI::TIME::FileTime& right) const
{
  return !operator ==(right);
}

bool CDateTime::operator>(const KODI::TIME::SystemTime& right) const
{
  KODI::TIME::FileTime time;
  ToFileTime(right, time);

  return operator >(time);
}

bool CDateTime::operator>=(const KODI::TIME::SystemTime& right) const
{
  return operator >(right) || operator ==(right);
}

bool CDateTime::operator<(const KODI::TIME::SystemTime& right) const
{
  KODI::TIME::FileTime time;
  ToFileTime(right, time);

  return operator <(time);
}

bool CDateTime::operator<=(const KODI::TIME::SystemTime& right) const
{
  return operator <(right) || operator ==(right);
}

bool CDateTime::operator==(const KODI::TIME::SystemTime& right) const
{
  KODI::TIME::FileTime time;
  ToFileTime(right, time);

  return operator ==(time);
}

bool CDateTime::operator!=(const KODI::TIME::SystemTime& right) const
{
  return !operator ==(right);
}

bool CDateTime::operator >(const time_t& right) const
{
  KODI::TIME::FileTime time;
  ToFileTime(right, time);

  return operator >(time);
}

bool CDateTime::operator >=(const time_t& right) const
{
  return operator >(right) || operator ==(right);
}

bool CDateTime::operator <(const time_t& right) const
{
  KODI::TIME::FileTime time;
  ToFileTime(right, time);

  return operator <(time);
}

bool CDateTime::operator <=(const time_t& right) const
{
  return operator <(right) || operator ==(right);
}

bool CDateTime::operator ==(const time_t& right) const
{
  KODI::TIME::FileTime time;
  ToFileTime(right, time);

  return operator ==(time);
}

bool CDateTime::operator !=(const time_t& right) const
{
  return !operator ==(right);
}

bool CDateTime::operator >(const tm& right) const
{
  KODI::TIME::FileTime time;
  ToFileTime(right, time);

  return operator >(time);
}

bool CDateTime::operator >=(const tm& right) const
{
  return operator >(right) || operator ==(right);
}

bool CDateTime::operator <(const tm& right) const
{
  KODI::TIME::FileTime time;
  ToFileTime(right, time);

  return operator <(time);
}

bool CDateTime::operator <=(const tm& right) const
{
  return operator <(right) || operator ==(right);
}

bool CDateTime::operator ==(const tm& right) const
{
  KODI::TIME::FileTime time;
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

  LARGE_INTEGER timeLeft;
  left.ToLargeInt(timeLeft);

  LARGE_INTEGER timeRight;
  right.ToLargeInt(timeRight);

  timeLeft.QuadPart+=timeRight.QuadPart;

  left.FromLargeInt(timeLeft);

  return left;
}

CDateTime CDateTime::operator -(const CDateTimeSpan& right) const
{
  CDateTime left(*this);

  LARGE_INTEGER timeLeft;
  left.ToLargeInt(timeLeft);

  LARGE_INTEGER timeRight;
  right.ToLargeInt(timeRight);

  timeLeft.QuadPart-=timeRight.QuadPart;

  left.FromLargeInt(timeLeft);

  return left;
}

const CDateTime& CDateTime::operator +=(const CDateTimeSpan& right)
{
  LARGE_INTEGER timeThis;
  ToLargeInt(timeThis);

  LARGE_INTEGER timeRight;
  right.ToLargeInt(timeRight);

  timeThis.QuadPart+=timeRight.QuadPart;

  FromLargeInt(timeThis);

  return *this;
}

const CDateTime& CDateTime::operator -=(const CDateTimeSpan& right)
{
  LARGE_INTEGER timeThis;
  ToLargeInt(timeThis);

  LARGE_INTEGER timeRight;
  right.ToLargeInt(timeRight);

  timeThis.QuadPart-=timeRight.QuadPart;

  FromLargeInt(timeThis);

  return *this;
}

CDateTimeSpan CDateTime::operator -(const CDateTime& right) const
{
  CDateTimeSpan left;

  LARGE_INTEGER timeLeft;
  left.ToLargeInt(timeLeft);

  LARGE_INTEGER timeThis;
  ToLargeInt(timeThis);

  LARGE_INTEGER timeRight;
  right.ToLargeInt(timeRight);

  timeLeft.QuadPart=timeThis.QuadPart-timeRight.QuadPart;

  left.FromLargeInt(timeLeft);

  return left;
}

CDateTime::operator KODI::TIME::FileTime() const
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
      KODI::TIME::SystemTime st;
      GetAsSystemTime(st);
      ar<<st;
    }
  }
  else
  {
    Reset();
    int state;
    ar >> state;
    m_state = CDateTime::STATE(state);
    if (m_state==valid)
    {
      KODI::TIME::SystemTime st;
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

bool CDateTime::ToFileTime(const KODI::TIME::SystemTime& time, KODI::TIME::FileTime& fileTime) const
{
  return KODI::TIME::SystemTimeToFileTime(&time, &fileTime) == 1 &&
         (fileTime.lowDateTime > 0 || fileTime.highDateTime > 0);
}

bool CDateTime::ToFileTime(const time_t& time, KODI::TIME::FileTime& fileTime) const
{
  long long ll = time;
  ll *= 10000000ll;
  ll += 0x19DB1DED53E8000LL;

  fileTime.lowDateTime = (DWORD)(ll & 0xFFFFFFFF);
  fileTime.highDateTime = (DWORD)(ll >> 32);

  return true;
}

bool CDateTime::ToFileTime(const tm& time, KODI::TIME::FileTime& fileTime) const
{
  KODI::TIME::SystemTime st = {};

  st.year = time.tm_year + 1900;
  st.month = time.tm_mon + 1;
  st.dayOfWeek = time.tm_wday;
  st.day = time.tm_mday;
  st.hour = time.tm_hour;
  st.minute = time.tm_min;
  st.second = time.tm_sec;

  return SystemTimeToFileTime(&st, &fileTime) == 1;
}

void CDateTime::ToLargeInt(LARGE_INTEGER& time) const
{
  time.u.HighPart = m_time.highDateTime;
  time.u.LowPart = m_time.lowDateTime;
}

void CDateTime::FromLargeInt(const LARGE_INTEGER& time)
{
  m_time.highDateTime = time.u.HighPart;
  m_time.lowDateTime = time.u.LowPart;
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

  size_t iPos2 = date.find(',');
  std::string strDay = (date.size() >= iPos) ? date.substr(iPos, iPos2-iPos) : "";
  std::string strYear = date.substr(date.find(' ', iPos2) + 1);
  while (months[j] && StringUtils::CompareNoCase(strMonth, months[j]) != 0)
    j++;
  if (!months[j])
    return false;

  return SetDateTime(atol(strYear.c_str()),j+1,atol(strDay.c_str()),0,0,0);
}

int CDateTime::GetDay() const
{
  KODI::TIME::SystemTime st;
  GetAsSystemTime(st);

  return st.day;
}

int CDateTime::GetMonth() const
{
  KODI::TIME::SystemTime st;
  GetAsSystemTime(st);

  return st.month;
}

int CDateTime::GetYear() const
{
  KODI::TIME::SystemTime st;
  GetAsSystemTime(st);

  return st.year;
}

int CDateTime::GetHour() const
{
  KODI::TIME::SystemTime st;
  GetAsSystemTime(st);

  return st.hour;
}

int CDateTime::GetMinute() const
{
  KODI::TIME::SystemTime st;
  GetAsSystemTime(st);

  return st.minute;
}

int CDateTime::GetSecond() const
{
  KODI::TIME::SystemTime st;
  GetAsSystemTime(st);

  return st.second;
}

int CDateTime::GetDayOfWeek() const
{
  KODI::TIME::SystemTime st;
  GetAsSystemTime(st);

  return st.dayOfWeek;
}

int CDateTime::GetMinuteOfDay() const
{
  KODI::TIME::SystemTime st;
  GetAsSystemTime(st);
  return st.hour * 60 + st.minute;
}

bool CDateTime::SetDateTime(int year, int month, int day, int hour, int minute, int second)
{
  KODI::TIME::SystemTime st = {};

  st.year = year;
  st.month = month;
  st.day = day;
  st.hour = hour;
  st.minute = minute;
  st.second = second;

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

void CDateTime::GetAsSystemTime(KODI::TIME::SystemTime& time) const
{
  FileTimeToSystemTime(&m_time, &time);
}

#define UNIX_BASE_TIME 116444736000000000LL /* nanoseconds since epoch */
void CDateTime::GetAsTime(time_t& time) const
{
  long long ll = (static_cast<long long>(m_time.highDateTime) << 32) + m_time.lowDateTime;
  time=(time_t)((ll - UNIX_BASE_TIME) / 10000000);
}

void CDateTime::GetAsTm(tm& time) const
{
  KODI::TIME::SystemTime st;
  GetAsSystemTime(st);

  time = {};
  time.tm_year = st.year - 1900;
  time.tm_mon = st.month - 1;
  time.tm_wday = st.dayOfWeek;
  time.tm_mday = st.day;
  time.tm_hour = st.hour;
  time.tm_min = st.minute;
  time.tm_sec = st.second;
  time.tm_isdst = -1;

  mktime(&time);
}

void CDateTime::GetAsTimeStamp(KODI::TIME::FileTime& time) const
{
  KODI::TIME::LocalFileTimeToFileTime(&m_time, &time);
}

std::string CDateTime::GetAsDBDate() const
{
  KODI::TIME::SystemTime st;
  GetAsSystemTime(st);

  return StringUtils::Format("{:04}-{:02}-{:02}", st.year, st.month, st.day);
}

std::string CDateTime::GetAsDBTime() const
{
  KODI::TIME::SystemTime st;
  GetAsSystemTime(st);

  return StringUtils::Format("{:02}:{:02}:{:02}", st.hour, st.minute, st.second);
}

std::string CDateTime::GetAsDBDateTime() const
{
  KODI::TIME::SystemTime st;
  GetAsSystemTime(st);

  return StringUtils::Format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}", st.year, st.month, st.day,
                             st.hour, st.minute, st.second);
}

std::string CDateTime::GetAsSaveString() const
{
  KODI::TIME::SystemTime st;
  GetAsSystemTime(st);

  return StringUtils::Format("{:04}{:02}{:02}_{:02}{:02}{:02}", st.year, st.month, st.day, st.hour,
                             st.minute, st.second);
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
    KODI::TIME::TimeZoneInformation tz;
    switch (KODI::TIME::GetTimeZoneInformation(&tz))
    {
      case KODI::TIME::KODI_TIME_ZONE_ID_DAYLIGHT:
        timezoneBias = CDateTimeSpan(0, 0, tz.bias + tz.daylightBias, 0);
        break;
      case KODI::TIME::KODI_TIME_ZONE_ID_STANDARD:
        timezoneBias = CDateTimeSpan(0, 0, tz.bias + tz.standardBias, 0);
        break;
      case KODI::TIME::KODI_TIME_ZONE_ID_UNKNOWN:
        timezoneBias = CDateTimeSpan(0, 0, tz.bias, 0);
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

  size_t posT = dateTime.find('T');
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

  size_t posT = dateTime.find('T');
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
          tmpDateTime -= zoneSpan;
        else if (StringUtils::StartsWith(zone, "-"))
          tmpDateTime += zoneSpan;
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
  if (time.size() < 5)
    return false;

  int hour;
  int minute;

  int second = 0;
  // HH:MM or HH:MM:SS
  hour   = atoi(time.substr(0, 2).c_str());
  minute = atoi(time.substr(3, 2).c_str());
  // HH:MM:SS
  if (time.size() == 8)
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

  KODI::TIME::SystemTime dateTime;
  GetAsSystemTime(dateTime);

  // Prefetch meridiem symbol
  const std::string& strMeridiem =
      CLangInfo::MeridiemSymbolToString(dateTime.hour > 11 ? MeridiemSymbolPM : MeridiemSymbolAM);

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

      int hour = dateTime.hour;
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
        str = std::to_string(hour);
      else
        str = StringUtils::Format("{:02}", hour);

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
        str = std::to_string(dateTime.minute);
      else
        str = StringUtils::Format("{:02}", dateTime.minute);

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
          str = std::to_string(dateTime.second);
        else
          str = StringUtils::Format("{:02}", dateTime.second);

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
  return GetAsLocalizedDate(strFormat, ReturnFormat::CHOICE_NO);
}

std::string CDateTime::GetAsLocalizedDate(const std::string& strFormat,
                                          ReturnFormat returnFormat) const
{
  std::string strOut;
  std::string fmtOut;

  KODI::TIME::SystemTime dateTime;
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
      fmtOut += strPart;
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
      {
        str = std::to_string(dateTime.day);
        fmtOut += "%-d";
      }
      else if (partLength==2) // two-digit number
      {
        str = StringUtils::Format("{:02}", dateTime.day);
        fmtOut += "%d";
      }
      else // Day of week string
      {
        int wday = dateTime.dayOfWeek;
        if (wday < 1 || wday > 7) wday = 7;
        {
          str = g_localizeStrings.Get((c == 'd' ? 40 : 10) + wday);
          fmtOut += (c == 'd' ? "%a" : "%A");
        }
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
      {
        str = std::to_string(dateTime.month);
        fmtOut += "%-m";
      }
      else if (partLength==2) // two-digit number
      {
        str = StringUtils::Format("{:02}", dateTime.month);
        fmtOut += "%m";
      }
      else // Month string
      {
        int wmonth = dateTime.month;
        if (wmonth < 1 || wmonth > 12) wmonth = 12;
        {
          str = g_localizeStrings.Get((c == 'm' ? 50 : 20) + wmonth);
          fmtOut += (c == 'm' ? "%b" : "%B");
        }
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
      std::string str = std::to_string(dateTime.year); // four-digit number
      if (partLength <= 2)
      {
        str.erase(0, 2); // two-digit number
        fmtOut += "%y";
      }
      else
      {
        fmtOut += "%Y";
      }

      strOut += str;
    }
    else // everything else pass to output
    {
      strOut+=c;
      fmtOut += c;
    }
  }

  return (returnFormat == ReturnFormat::CHOICE_YES ? fmtOut : strOut);
}

std::string CDateTime::GetAsLocalizedDateTime(bool longDate/*=false*/, bool withSeconds/*=true*/) const
{
  return GetAsLocalizedDate(longDate) + ' ' + GetAsLocalizedTime("", withSeconds);
}

std::string CDateTime::GetAsLocalizedTime(TIME_FORMAT format, bool withSeconds /* = false */) const
{
  const std::string timeFormat = g_langInfo.GetTimeFormat();
  bool use12hourclock = timeFormat.find('h') != std::string::npos;
  switch (format)
  {
    case TIME_FORMAT_GUESS:
      return GetAsLocalizedTime("", withSeconds);
    case TIME_FORMAT_SS:
      return GetAsLocalizedTime("ss", true);
    case TIME_FORMAT_MM:
      return GetAsLocalizedTime("mm", true);
    case TIME_FORMAT_MM_SS:
      return GetAsLocalizedTime("mm:ss", true);
    case TIME_FORMAT_HH:  // this forces it to a 12 hour clock
      return GetAsLocalizedTime(use12hourclock ? "h" : "HH", false);
    case TIME_FORMAT_HH_SS:
      return GetAsLocalizedTime(use12hourclock ? "h:ss" : "HH:ss", true);
    case TIME_FORMAT_HH_MM:
      return GetAsLocalizedTime(use12hourclock ? "h:mm" : "HH:mm", false);
    case TIME_FORMAT_HH_MM_XX:
      return GetAsLocalizedTime(use12hourclock ? "h:mm xx" : "HH:mm", false);
    case TIME_FORMAT_HH_MM_SS:
      return GetAsLocalizedTime(use12hourclock ? "hh:mm:ss" : "HH:mm:ss", true);
    case TIME_FORMAT_HH_MM_SS_XX:
      return GetAsLocalizedTime(use12hourclock ? "hh:mm:ss xx" : "HH:mm:ss", true);
    case TIME_FORMAT_H:
      return GetAsLocalizedTime("h", false);
    case TIME_FORMAT_M:
      return GetAsLocalizedTime("m", false);
    case TIME_FORMAT_H_MM_SS:
      return GetAsLocalizedTime("h:mm:ss", true);
    case TIME_FORMAT_H_MM_SS_XX:
      return GetAsLocalizedTime("h:mm:ss xx", true);
    case TIME_FORMAT_XX:
      return use12hourclock ? GetAsLocalizedTime("xx", false) : "";
    default:
      break;
  }
  return GetAsLocalizedTime("", false);
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
    CLog::Log(LOGWARNING, "Invalid day of week {} in {}", time.GetDayOfWeek(),
              time.GetAsDBDateTime());

  int month = time.GetMonth();
  if (month < 1)
    month = 1;
  else if (month > 12)
    month = 12;
  if (month != time.GetMonth())
    CLog::Log(LOGWARNING, "Invalid month {} in {}", time.GetMonth(), time.GetAsDBDateTime());

  return StringUtils::Format("{}, {:02} {} {:04} {:02}:{:02}:{:02} GMT", DAY_NAMES[weekDay],
                             time.GetDay(), MONTH_NAMES[month - 1], time.GetYear(), time.GetHour(),
                             time.GetMinute(), time.GetSecond());
}

std::string CDateTime::GetAsW3CDate() const
{
  KODI::TIME::SystemTime st;
  GetAsSystemTime(st);

  return StringUtils::Format("{:04}-{:02}-{:02}", st.year, st.month, st.day);
}

std::string CDateTime::GetAsW3CDateTime(bool asUtc /* = false */) const
{
  CDateTime w3cDate = *this;
  if (asUtc)
    w3cDate = GetAsUTCDateTime();
  KODI::TIME::SystemTime st;
  w3cDate.GetAsSystemTime(st);

  std::string result = StringUtils::Format("{:04}-{:02}-{:02}T{:02}:{:02}:{:02}", st.year, st.month,
                                           st.day, st.hour, st.minute, st.second);
  if (asUtc)
    return result + "Z";

  CDateTimeSpan bias = GetTimezoneBias();
  return result + StringUtils::Format("{}{:02}:{:02}", (bias.GetSecondsTotal() >= 0 ? '+' : '-'),
                                      abs(bias.GetHours()), abs(bias.GetMinutes()));
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
