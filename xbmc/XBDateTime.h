/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/IArchivable.h"
#include "utils/TimeFormat.h"
#include "utils/XTimeUtils.h"

#include <string>

#include "PlatformDefs.h"

class CDateTime;

class CDateTimeSpan
{
public:
  CDateTimeSpan();
  CDateTimeSpan(const CDateTimeSpan& span);
  CDateTimeSpan& operator=(const CDateTimeSpan&) = default;
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
  void SetFromPeriod(const std::string &period);
  void SetFromTimeString(const std::string& time);

  int GetDays() const;
  int GetHours() const;
  int GetMinutes() const;
  int GetSeconds() const;
  int GetSecondsTotal() const;

private:
  void ToLargeInt(LARGE_INTEGER& time) const;
  void FromLargeInt(const LARGE_INTEGER& time);

private:
  KODI::TIME::FileTime m_timeSpan;

  friend class CDateTime;
};

/// \brief DateTime class, which uses FileTime as it's base.
class CDateTime final : public IArchivable
{
public:
  CDateTime();
  CDateTime(const CDateTime& time);
  CDateTime& operator=(const CDateTime&) = default;
  explicit CDateTime(const KODI::TIME::SystemTime& time);
  explicit CDateTime(const KODI::TIME::FileTime& time);
  explicit CDateTime(const time_t& time);
  explicit CDateTime(const tm& time);
  CDateTime(int year, int month, int day, int hour, int minute, int second);

  static CDateTime GetCurrentDateTime();
  static CDateTime GetUTCDateTime();
  static int MonthStringToMonthNum(const std::string& month);

  static CDateTime FromDBDateTime(const std::string &dateTime);
  static CDateTime FromDateString(const std::string &date);
  static CDateTime FromDBDate(const std::string &date);
  static CDateTime FromDBTime(const std::string &time);
  static CDateTime FromW3CDate(const std::string &date);
  static CDateTime FromW3CDateTime(const std::string &date, bool ignoreTimezone = false);
  static CDateTime FromUTCDateTime(const CDateTime &dateTime);
  static CDateTime FromUTCDateTime(const time_t &dateTime);
  static CDateTime FromRFC1123DateTime(const std::string &dateTime);

  const CDateTime& operator=(const KODI::TIME::SystemTime& right);
  const CDateTime& operator=(const KODI::TIME::FileTime& right);
  const CDateTime& operator =(const time_t& right);
  const CDateTime& operator =(const tm& right);

  bool operator >(const CDateTime& right) const;
  bool operator >=(const CDateTime& right) const;
  bool operator <(const CDateTime& right) const;
  bool operator <=(const CDateTime& right) const;
  bool operator ==(const CDateTime& right) const;
  bool operator !=(const CDateTime& right) const;

  bool operator>(const KODI::TIME::FileTime& right) const;
  bool operator>=(const KODI::TIME::FileTime& right) const;
  bool operator<(const KODI::TIME::FileTime& right) const;
  bool operator<=(const KODI::TIME::FileTime& right) const;
  bool operator==(const KODI::TIME::FileTime& right) const;
  bool operator!=(const KODI::TIME::FileTime& right) const;

  bool operator>(const KODI::TIME::SystemTime& right) const;
  bool operator>=(const KODI::TIME::SystemTime& right) const;
  bool operator<(const KODI::TIME::SystemTime& right) const;
  bool operator<=(const KODI::TIME::SystemTime& right) const;
  bool operator==(const KODI::TIME::SystemTime& right) const;
  bool operator!=(const KODI::TIME::SystemTime& right) const;

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

  operator KODI::TIME::FileTime() const;

  void Archive(CArchive& ar) override;

  void Reset();

  int GetDay() const;
  int GetMonth() const;
  int GetYear() const;
  int GetHour() const;
  int GetMinute() const;
  int GetSecond() const;
  int GetDayOfWeek() const;
  int GetMinuteOfDay() const;

  bool SetDateTime(int year, int month, int day, int hour, int minute, int second);
  bool SetDate(int year, int month, int day);
  bool SetTime(int hour, int minute, int second);

  bool SetFromDateString(const std::string &date);
  bool SetFromDBDate(const std::string &date);
  bool SetFromDBTime(const std::string &time);
  bool SetFromW3CDate(const std::string &date);
  bool SetFromW3CDateTime(const std::string &date, bool ignoreTimezone = false);
  bool SetFromUTCDateTime(const CDateTime &dateTime);
  bool SetFromUTCDateTime(const time_t &dateTime);
  bool SetFromRFC1123DateTime(const std::string &dateTime);

  /*! \brief set from a database datetime format YYYY-MM-DD HH:MM:SS
   \sa GetAsDBDateTime()
   */
  bool SetFromDBDateTime(const std::string &dateTime);

  void GetAsSystemTime(KODI::TIME::SystemTime& time) const;
  void GetAsTime(time_t& time) const;
  void GetAsTm(tm& time) const;
  void GetAsTimeStamp(KODI::TIME::FileTime& time) const;

  enum class ReturnFormat : bool
  {
    CHOICE_YES = true,
    CHOICE_NO = false,
  };

  CDateTime GetAsUTCDateTime() const;
  std::string GetAsSaveString() const;
  std::string GetAsDBDateTime() const;
  std::string GetAsDBDate() const;
  std::string GetAsDBTime() const;
  std::string GetAsLocalizedDate(bool longDate=false) const;
  std::string GetAsLocalizedDate(const std::string &strFormat) const;
  std::string GetAsLocalizedDate(const std::string& strFormat, ReturnFormat returnFormat) const;
  std::string GetAsLocalizedTime(const std::string &format, bool withSeconds=true) const;
  std::string GetAsLocalizedDateTime(bool longDate=false, bool withSeconds=true) const;
  std::string GetAsLocalizedTime(TIME_FORMAT format, bool withSeconds = false) const;
  std::string GetAsRFC1123DateTime() const;
  std::string GetAsW3CDate() const;
  std::string GetAsW3CDateTime(bool asUtc = false) const;

  void SetValid(bool yesNo);
  bool IsValid() const;

  static void ResetTimezoneBias(void);
  static CDateTimeSpan GetTimezoneBias(void);

private:
  bool ToFileTime(const KODI::TIME::SystemTime& time, KODI::TIME::FileTime& fileTime) const;
  bool ToFileTime(const time_t& time, KODI::TIME::FileTime& fileTime) const;
  bool ToFileTime(const tm& time, KODI::TIME::FileTime& fileTime) const;

  void ToLargeInt(LARGE_INTEGER& time) const;
  void FromLargeInt(const LARGE_INTEGER& time);

private:
  KODI::TIME::FileTime m_time;

  typedef enum _STATE
  {
    invalid=0,
    valid
  } STATE;

  STATE m_state;
};
