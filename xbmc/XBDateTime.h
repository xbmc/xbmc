#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/Archive.h"

/*! \brief TIME_FORMAT enum/bitmask used for formatting time strings
 Note the use of bitmasking, e.g.
  TIME_FORMAT_HH_MM_SS = TIME_FORMAT_HH | TIME_FORMAT_MM | TIME_FORMAT_SS
 \sa StringUtils::SecondsToTimeString
 */
enum TIME_FORMAT { TIME_FORMAT_GUESS     =  0,
                   TIME_FORMAT_SS        =  1,
                   TIME_FORMAT_MM        =  2,
                   TIME_FORMAT_MM_SS     =  3,
                   TIME_FORMAT_HH        =  4,
                   TIME_FORMAT_HH_SS     =  5, // not particularly useful
                   TIME_FORMAT_HH_MM     =  6,
                   TIME_FORMAT_HH_MM_SS  =  7,
                   TIME_FORMAT_XX        =  8, // AM/PM
                   TIME_FORMAT_HH_MM_XX  = 14,
                   TIME_FORMAT_H         = 16,
                   TIME_FORMAT_H_MM_SS   = 19};

class CDateTime;

class CDateTimeSpan
{
public:
  CDateTimeSpan();
  CDateTimeSpan(const CDateTimeSpan& span);
  CDateTimeSpan(int day, int hour, int minute, int second);

  bool operator >(const CDateTimeSpan& right) const;
  bool operator >=(const CDateTimeSpan& right) const;
  bool operator <(const CDateTimeSpan& right) const;
  bool operator <=(const CDateTimeSpan& right) const;
  bool operator ==(const CDateTimeSpan& right) const;
  bool operator !=(const CDateTimeSpan& right) const;

  CDateTimeSpan operator +(const CDateTimeSpan& right) const;
  CDateTimeSpan operator -(const CDateTimeSpan& right) const;

  const CDateTimeSpan& operator +=(const CDateTimeSpan& right);
  const CDateTimeSpan& operator -=(const CDateTimeSpan& right);

  void SetDateTimeSpan(int day, int hour, int minute, int second);
  void SetFromPeriod(const CStdString &period);
  void SetFromTimeString(const CStdString& time);

  int GetDays() const;
  int GetHours() const;
  int GetMinutes() const;
  int GetSeconds() const;

private:
  void ToULargeInt(ULARGE_INTEGER& time) const;
  void FromULargeInt(const ULARGE_INTEGER& time);

private:
  FILETIME m_timeSpan;

  friend class CDateTime;
};

/// \brief DateTime class, which uses FILETIME as it's base.
class CDateTime : public IArchivable
{
public:
  CDateTime();
  CDateTime(const CDateTime& time);
  CDateTime(const SYSTEMTIME& time);
  CDateTime(const FILETIME& time);
  CDateTime(const time_t& time);
  CDateTime(const tm& time);
  CDateTime(int year, int month, int day, int hour, int minute, int second);
  virtual ~CDateTime() {}

  void SetFromDateString(const CStdString &date);

  static CDateTime GetCurrentDateTime();
  static CDateTime GetUTCDateTime();
  static int MonthStringToMonthNum(const CStdString& month);

  const CDateTime& operator =(const SYSTEMTIME& right);
  const CDateTime& operator =(const FILETIME& right);
  const CDateTime& operator =(const time_t& right);
  const CDateTime& operator =(const tm& right);

  bool operator >(const CDateTime& right) const;
  bool operator >=(const CDateTime& right) const;
  bool operator <(const CDateTime& right) const;
  bool operator <=(const CDateTime& right) const;
  bool operator ==(const CDateTime& right) const;
  bool operator !=(const CDateTime& right) const;

  bool operator >(const FILETIME& right) const;
  bool operator >=(const FILETIME& right) const;
  bool operator <(const FILETIME& right) const;
  bool operator <=(const FILETIME& right) const;
  bool operator ==(const FILETIME& right) const;
  bool operator !=(const FILETIME& right) const;

  bool operator >(const SYSTEMTIME& right) const;
  bool operator >=(const SYSTEMTIME& right) const;
  bool operator <(const SYSTEMTIME& right) const;
  bool operator <=(const SYSTEMTIME& right) const;
  bool operator ==(const SYSTEMTIME& right) const;
  bool operator !=(const SYSTEMTIME& right) const;

  bool operator >(const time_t& right) const;
  bool operator >=(const time_t& right) const;
  bool operator <(const time_t& right) const;
  bool operator <=(const time_t& right) const;
  bool operator ==(const time_t& right) const;
  bool operator !=(const time_t& right) const;

  bool operator >(const tm& right) const;
  bool operator >=(const tm& right) const;
  bool operator <(const tm& right) const;
  bool operator <=(const tm& right) const;
  bool operator ==(const tm& right) const;
  bool operator !=(const tm& right) const;

  CDateTime operator +(const CDateTimeSpan& right) const;
  CDateTime operator -(const CDateTimeSpan& right) const;

  const CDateTime& operator +=(const CDateTimeSpan& right);
  const CDateTime& operator -=(const CDateTimeSpan& right);

  CDateTimeSpan operator -(const CDateTime& right) const;

  operator FILETIME() const;

  virtual void Archive(CArchive& ar);

  void Reset();

  int GetDay() const;
  int GetMonth() const;
  int GetYear() const;
  int GetHour() const;
  int GetMinute() const;
  int GetSecond() const;
  int GetDayOfWeek() const;
  int GetMinuteOfDay() const;

  void SetDateTime(int year, int month, int day, int hour, int minute, int second);
  void SetDate(int year, int month, int day);
  void SetTime(int hour, int minute, int second);
  void SetFromDBDate(const CStdString &date);
  void SetFromDBTime(const CStdString &time);
  void SetFromW3CDate(const CStdString &date);
  void SetFromUTCDateTime(const CDateTime &dateTime);
  void SetFromUTCDateTime(const time_t &dateTime);
  void SetFromRFC1123DateTime(const CStdString &dateTime);

  /*! \brief set from a database datetime format YYYY-MM-DD HH:MM:SS
   \sa GetAsDBDateTime()
   */
  void SetFromDBDateTime(const CStdString &dateTime);

  void GetAsSystemTime(SYSTEMTIME& time) const;
  void GetAsTime(time_t& time) const;
  void GetAsTm(tm& time) const;
  void GetAsTimeStamp(FILETIME& time) const;

  CDateTime GetAsUTCDateTime() const;
  CStdString GetAsSaveString() const;
  CStdString GetAsDBDateTime() const;
  CStdString GetAsDBDate() const;
  CStdString GetAsLocalizedDate(bool longDate=false, bool withShortNames=true) const;
  CStdString GetAsLocalizedDate(const CStdString &strFormat, bool withShortNames=true) const;
  CStdString GetAsLocalizedTime(const CStdString &format, bool withSeconds=true) const;
  CStdString GetAsLocalizedDateTime(bool longDate=false, bool withSeconds=true) const;
  CStdString GetAsRFC1123DateTime() const;

  void SetValid(bool yesNo);
  bool IsValid() const;

  static void ResetTimezoneBias(void);
  static CDateTimeSpan GetTimezoneBias(void);

private:
  bool ToFileTime(const SYSTEMTIME& time, FILETIME& fileTime) const;
  bool ToFileTime(const time_t& time, FILETIME& fileTime) const;
  bool ToFileTime(const tm& time, FILETIME& fileTime) const;

  void ToULargeInt(ULARGE_INTEGER& time) const;
  void FromULargeInt(const ULARGE_INTEGER& time);

private:
  FILETIME m_time;

  typedef enum _STATE
  {
    invalid=0,
    valid
  } STATE;

  STATE m_state;
};
