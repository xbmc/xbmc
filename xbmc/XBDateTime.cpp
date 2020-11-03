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

#define USE_OS_TZDB 1
#define HAS_REMOTE_API 0
#include <date/date.h>
#include <date/iso_week.h>
#include <date/tz.h>

static const char *MONTH_NAMES[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

CDateTimeSpan::CDateTimeSpan(const CDateTimeSpan& span)
{
  m_timeSpan = span.m_timeSpan;
  SetValid(span.m_state);
}

CDateTimeSpan::CDateTimeSpan(int day, int hour, int minute, int second) : CDateTimeSpan()
{
  SetDateTimeSpan(day, hour, minute, second);
}

bool CDateTimeSpan::operator >(const CDateTimeSpan& right) const
{
  return m_timeSpan > right.m_timeSpan;
}

bool CDateTimeSpan::operator >=(const CDateTimeSpan& right) const
{
  return operator >(right) || operator ==(right);
}

bool CDateTimeSpan::operator <(const CDateTimeSpan& right) const
{
  return m_timeSpan < right.m_timeSpan;
}

bool CDateTimeSpan::operator <=(const CDateTimeSpan& right) const
{
  return operator <(right) || operator ==(right);
}

bool CDateTimeSpan::operator ==(const CDateTimeSpan& right) const
{
  return m_timeSpan == right.m_timeSpan;
}

bool CDateTimeSpan::operator !=(const CDateTimeSpan& right) const
{
  return !operator ==(right);
}

CDateTimeSpan CDateTimeSpan::operator +(const CDateTimeSpan& right) const
{
  CDateTimeSpan left(*this);

  left.m_timeSpan += right.m_timeSpan;

  return left;
}

CDateTimeSpan CDateTimeSpan::operator -(const CDateTimeSpan& right) const
{
  CDateTimeSpan left(*this);

  left.m_timeSpan -= right.m_timeSpan;

  return left;
}

const CDateTimeSpan& CDateTimeSpan::operator +=(const CDateTimeSpan& right)
{
  m_timeSpan += right.m_timeSpan;

  return *this;
}

const CDateTimeSpan& CDateTimeSpan::operator -=(const CDateTimeSpan& right)
{
  m_timeSpan -= right.m_timeSpan;

  return *this;
}

void CDateTimeSpan::SetDateTimeSpan(int day, int hour, int minute, int second)
{
  m_timeSpan = std::chrono::duration_cast<std::chrono::seconds>(date::days(day)) +
               std::chrono::duration_cast<std::chrono::seconds>(std::chrono::hours(hour)) +
               std::chrono::duration_cast<std::chrono::seconds>(std::chrono::minutes(minute)) +
               std::chrono::duration_cast<std::chrono::seconds>(std::chrono::seconds(second));

  SetValid(true);
}

void CDateTimeSpan::SetFromTimeString(const std::string& time) // hh:mm
{
  try
  {
    if (time.size() >= 5 && time[2] == ':')
    {
      int hour = std::stoi(time.substr(0, 2));
      int minutes = std::stoi(time.substr(3, 2));
      SetDateTimeSpan(0, hour, minutes, 0);
    }
    else
    {
      SetValid(false);
    }
  }
  catch (std::invalid_argument const& ex)
  {
    SetValid(false);
  }
  catch (std::out_of_range const& ex)
  {
    SetValid(false);
  }
}

int CDateTimeSpan::GetDays() const
{
  return date::floor<date::days>(m_timeSpan).count();
}

int CDateTimeSpan::GetHours() const
{
  auto time = date::floor<date::days>(m_timeSpan);
  auto dp = date::make_time(m_timeSpan - time);

  return dp.hours().count();
}

int CDateTimeSpan::GetMinutes() const
{
  auto time = date::floor<date::days>(m_timeSpan);
  auto dp = date::make_time(m_timeSpan - time);

  return dp.minutes().count();
}

int CDateTimeSpan::GetSeconds() const
{
  auto time = date::floor<date::days>(m_timeSpan);
  auto dp = date::make_time(m_timeSpan - time);

  return dp.seconds().count();
}

int CDateTimeSpan::GetSecondsTotal() const
{
  return std::chrono::duration_cast<std::chrono::seconds>(m_timeSpan).count();
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

void CDateTimeSpan::SetValid(bool yesNo)
{
  m_state = yesNo ? valid : invalid;
}

bool CDateTimeSpan::IsValid() const
{
  return m_state == valid;
}

CDateTime::CDateTime()
{
  Reset();
}

CDateTime::CDateTime(const CDateTime& time) : m_time(time.m_time)
{
  m_state = time.m_state;
}

CDateTime::CDateTime(const time_t& time)
{
  m_time = std::chrono::system_clock::from_time_t(time);
  SetValid(true);
}

CDateTime::CDateTime(const std::chrono::system_clock::time_point& time)
{
  m_time = time;
  SetValid(true);
}

CDateTime::CDateTime(const tm& time)
{
  m_time = std::chrono::system_clock::from_time_t(std::mktime(const_cast<tm*>(&time)));
  SetValid(true);
}

CDateTime::CDateTime(int year, int month, int day, int hour, int minute, int second)
{
  SetDateTime(year, month, day, hour, minute, second);
}

CDateTime CDateTime::GetCurrentDateTime()
{
  auto zone = date::make_zoned(date::current_zone(), std::chrono::system_clock::now());

  return CDateTime(
      std::chrono::duration_cast<std::chrono::seconds>(zone.get_local_time().time_since_epoch())
          .count());
}

CDateTime CDateTime::GetUTCDateTime()
{
  return CDateTime(std::chrono::system_clock::now());
}

const CDateTime& CDateTime::operator=(const time_t& right)
{
  m_time = std::chrono::system_clock::from_time_t(right);
  SetValid(true);

  return *this;
}

const CDateTime& CDateTime::operator=(const tm& right)
{
  m_time = std::chrono::system_clock::from_time_t(std::mktime(const_cast<tm*>(&right)));
  SetValid(true);

  return *this;
}

const CDateTime& CDateTime::operator=(const std::chrono::system_clock::time_point& right)
{
  m_time = right;
  SetValid(true);

  return *this;
}

bool CDateTime::operator >(const CDateTime& right) const
{
  return m_time > right.m_time;
}

bool CDateTime::operator >=(const CDateTime& right) const
{
  return operator >(right) || operator ==(right);
}

bool CDateTime::operator <(const CDateTime& right) const
{
  return m_time < right.m_time;
}

bool CDateTime::operator <=(const CDateTime& right) const
{
  return operator <(right) || operator ==(right);
}

bool CDateTime::operator ==(const CDateTime& right) const
{
  return m_time == right.m_time;
}

bool CDateTime::operator !=(const CDateTime& right) const
{
  return !operator ==(right);
}

bool CDateTime::operator>(const time_t& right) const
{
  return m_time > std::chrono::system_clock::from_time_t(right);
}

bool CDateTime::operator>=(const time_t& right) const
{
  return operator >(right) || operator ==(right);
}

bool CDateTime::operator<(const time_t& right) const
{
  return m_time < std::chrono::system_clock::from_time_t(right);
}

bool CDateTime::operator<=(const time_t& right) const
{
  return operator <(right) || operator ==(right);
}

bool CDateTime::operator==(const time_t& right) const
{
  return m_time == std::chrono::system_clock::from_time_t(right);
}

bool CDateTime::operator!=(const time_t& right) const
{
  return !operator ==(right);
}

bool CDateTime::operator>(const tm& right) const
{
  return m_time > std::chrono::system_clock::from_time_t(std::mktime(const_cast<tm*>(&right)));
}

bool CDateTime::operator>=(const tm& right) const
{
  return operator >(right) || operator ==(right);
}

bool CDateTime::operator<(const tm& right) const
{
  return m_time < std::chrono::system_clock::from_time_t(std::mktime(const_cast<tm*>(&right)));
}

bool CDateTime::operator<=(const tm& right) const
{
  return operator <(right) || operator ==(right);
}

bool CDateTime::operator==(const tm& right) const
{
  return m_time == std::chrono::system_clock::from_time_t(std::mktime(const_cast<tm*>(&right)));
}

bool CDateTime::operator!=(const tm& right) const
{
  return !operator ==(right);
}

bool CDateTime::operator>(const std::chrono::system_clock::time_point& right) const
{
  return m_time > right;
}

bool CDateTime::operator>=(const std::chrono::system_clock::time_point& right) const
{
  return operator>(right) || operator==(right);
}

bool CDateTime::operator<(const std::chrono::system_clock::time_point& right) const
{
  return m_time < right;
}

bool CDateTime::operator<=(const std::chrono::system_clock::time_point& right) const
{
  return operator<(right) || operator==(right);
}

bool CDateTime::operator==(const std::chrono::system_clock::time_point& right) const
{
  return m_time == right;
}

bool CDateTime::operator!=(const std::chrono::system_clock::time_point& right) const
{
  return !operator==(right);
}

CDateTime CDateTime::operator+(const CDateTimeSpan& right) const
{
  CDateTime left(*this);

  left.m_time += right.m_timeSpan;

  return left;
}

CDateTime CDateTime::operator-(const CDateTimeSpan& right) const
{
  CDateTime left(*this);

  left.m_time -= right.m_timeSpan;

  return left;
}

const CDateTime& CDateTime::operator +=(const CDateTimeSpan& right)
{
  m_time += right.m_timeSpan;

  return *this;
}

const CDateTime& CDateTime::operator -=(const CDateTimeSpan& right)
{
  m_time -= right.m_timeSpan;

  return *this;
}

CDateTimeSpan CDateTime::operator -(const CDateTime& right) const
{
  CDateTimeSpan left;

  left.m_timeSpan = std::chrono::duration_cast<std::chrono::seconds>(m_time - right.m_time);
  return left;
}

KODI::TIME::SystemTime CDateTime::GetAsSystemTime()
{
  KODI::TIME::SystemTime st{};

  if (!IsValid())
    return st;

  st.year = GetYear();
  st.month = GetMonth();
  st.day = GetDay();
  st.dayOfWeek = GetDayOfWeek();
  st.hour = GetHour();
  st.minute = GetMinute();
  st.second = GetSecond();

  auto dp = date::floor<std::chrono::seconds>(m_time);
  auto ms = date::floor<std::chrono::milliseconds>(m_time - dp);

  st.milliseconds = ms.count();

  return st;
}

void CDateTime::SetFromSystemTime(const KODI::TIME::SystemTime& right)
{
  Reset();

  auto ymd = date::sys_days(date::year(right.year) / date::month(right.month) / right.day);
  auto dur = ymd + std::chrono::hours(right.hour) + std::chrono::minutes(right.minute) +
             std::chrono::seconds(right.second) + std::chrono::milliseconds(right.milliseconds);

  auto timeT = date::floor<std::chrono::milliseconds>(dur.time_since_epoch()).count();

  std::chrono::system_clock::time_point tp{std::chrono::milliseconds{timeT}};

  m_time = tp;

  SetValid(true);
}

void CDateTime::Archive(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar<<(int)m_state;
    if (m_state==valid)
    {
      ar << GetAsSystemTime();
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
      ar >> st;
      SetFromSystemTime(st);
    }
  }
}

void CDateTime::Reset()
{
  m_time = {};
  SetValid(false);
}

void CDateTime::SetValid(bool yesNo)
{
  m_state = yesNo ? valid : invalid;
}

bool CDateTime::IsValid() const
{
  return m_state == valid;
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
  auto dp = date::floor<date::days>(m_time);
  auto ymd = date::year_month_day{dp};

  return static_cast<unsigned int>(ymd.day());
}

int CDateTime::GetMonth() const
{
  auto dp = date::floor<date::days>(m_time);
  auto ymd = date::year_month_day{dp};

  return static_cast<unsigned int>(ymd.month());
}

int CDateTime::GetYear() const
{
  auto dp = date::floor<date::days>(m_time);
  auto ymd = date::year_month_day{dp};

  return static_cast<int>(ymd.year());
}

int CDateTime::GetHour() const
{
  auto dp = date::floor<date::days>(m_time);
  auto time = date::make_time(m_time - dp);

  return time.hours().count();
}

int CDateTime::GetMinute() const
{
  auto dp = date::floor<date::days>(m_time);
  auto time = date::make_time(m_time - dp);

  return time.minutes().count();
}

int CDateTime::GetSecond() const
{
  auto dp = date::floor<date::days>(m_time);
  auto time = date::make_time(m_time - dp);

  return time.seconds().count();
}

int CDateTime::GetDayOfWeek() const
{
  auto dp = date::floor<date::days>(m_time);
  auto yww = iso_week::year_weeknum_weekday{dp};

  return static_cast<unsigned int>(yww.weekday());
}

int CDateTime::GetMinuteOfDay() const
{
  auto dp = date::floor<date::days>(m_time);
  auto time = date::make_time(m_time - dp);

  return time.hours().count() * 60 + time.minutes().count();
}

bool CDateTime::SetDateTime(int year, int month, int day, int hour, int minute, int second)
{
  auto ymd = date::year(year) / month / day;
  if (!ymd.ok())
  {
    SetValid(false);
    return false;
  }

  m_time = date::sys_days(ymd) + std::chrono::hours(hour) + std::chrono::minutes(minute) +
           std::chrono::seconds(second);

  SetValid(true);
  return true;
}

bool CDateTime::SetDate(int year, int month, int day)
{
  return SetDateTime(year, month, day, 0, 0, 0);
}

bool CDateTime::SetTime(int hour, int minute, int second)
{
  m_time = date::sys_seconds(std::chrono::seconds(0)) + std::chrono::hours(hour) +
           std::chrono::minutes(minute) + std::chrono::seconds(second);

  SetValid(true);
  return true;
}

void CDateTime::GetAsTime(time_t& time) const
{
  time = std::chrono::system_clock::to_time_t(m_time);
}

void CDateTime::GetAsTm(tm& time) const
{
  auto t = std::chrono::system_clock::to_time_t(m_time);

  time = {};
  localtime_r(&t, &time);
}

std::chrono::system_clock::time_point CDateTime::GetAsTimePoint() const
{
  return m_time;
}

std::string CDateTime::GetAsDBDate() const
{
  return date::format("%F", m_time);
}

std::string CDateTime::GetAsDBTime() const
{
  auto sp = date::floor<std::chrono::seconds>(m_time);
  return date::format("%T", sp);
}

std::string CDateTime::GetAsDBDateTime() const
{
  auto sp = date::floor<std::chrono::seconds>(m_time);

  return date::format("%F %T", sp);
}

std::string CDateTime::GetAsSaveString() const
{
  auto sp = date::floor<std::chrono::seconds>(m_time);

  return date::format("%Y%m%d_%H%M%S", sp);
}

bool CDateTime::SetFromUTCDateTime(const CDateTime &dateTime)
{
  m_time = dateTime.m_time;
  m_state = valid;
  return true;
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

  // Prefetch meridiem symbol
  const std::string& strMeridiem =
      CLangInfo::MeridiemSymbolToString(GetHour() > 11 ? MeridiemSymbolPM : MeridiemSymbolAM);

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

      int hour = GetHour();
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
        str = std::to_string(GetMinute());
      else
        str = StringUtils::Format("{:02}", GetMinute());

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
          str = std::to_string(GetSecond());
        else
          str = StringUtils::Format("{:02}", GetSecond());

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
        str = std::to_string(GetDay());
        fmtOut += "%-d";
      }
      else if (partLength==2) // two-digit number
      {
        str = StringUtils::Format("{:02}", GetDay());
        fmtOut += "%d";
      }
      else // Day of week string
      {
        int wday = GetDayOfWeek();
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
        str = std::to_string(GetMonth());
        fmtOut += "%-m";
      }
      else if (partLength==2) // two-digit number
      {
        str = StringUtils::Format("{:02}", GetMonth());
        fmtOut += "%m";
      }
      else // Month string
      {
        int wmonth = GetMonth();
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
      std::string str = std::to_string(GetYear()); // four-digit number
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

CDateTime CDateTime::GetAsLocalDateTime() const
{
  auto zone = date::make_zoned(date::current_zone(), m_time);

  return CDateTime(
      std::chrono::duration_cast<std::chrono::seconds>(zone.get_local_time().time_since_epoch())
          .count());
}

std::string CDateTime::GetAsRFC1123DateTime() const
{
  auto time = date::floor<std::chrono::seconds>(m_time);

  return date::format("%a, %d %b %Y %T GMT", time);
}

std::string CDateTime::GetAsW3CDate() const
{
  return GetAsDBDate();
}

std::string CDateTime::GetAsW3CDateTime(bool asUtc /* = false */) const
{
  auto time = date::floor<std::chrono::seconds>(m_time);

  if (asUtc)
    return date::format("%FT%TZ", time);

  auto zt = date::make_zoned(date::current_zone(), time);

  return date::format("%FT%T%Ez", zt);
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
