/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/IArchivable.h"
#include "utils/XTimeUtils.h"

#include <string>

#include "PlatformDefs.h"

/*! \brief TIME_FORMAT enum/bitmask used for formatting time strings
 Note the use of bitmasking, e.g.
  TIME_FORMAT_HH_MM_SS = TIME_FORMAT_HH | TIME_FORMAT_MM | TIME_FORMAT_SS
 \sa StringUtils::SecondsToTimeString
 \note For InfoLabels use the equivalent value listed (bold)
  on the description of each enum value.
 \note<b>Example:</b> 3661 seconds => h=1, hh=01, m=1, mm=01, ss=01, hours=1, mins=61, secs=3661
 <p><hr>
 @skinning_v18 **[Infolabels Updated]** Added <b>secs</b>, <b>mins</b>, <b>hours</b> (total time) and **m** as possible formats for
 InfoLabels that support the definition of a time format. Examples are:
   - \link Player_SeekOffset_format `Player.SeekOffset(format)`\endlink
   - \link Player_TimeRemaining_format `Player.TimeRemaining(format)`\endlink
   - \link Player_Time_format `Player.Time(format)`\endlink
   - \link Player_Duration_format `Player.Duration(format)`\endlink
   - \link Player_FinishTime_format `Player.FinishTime(format)`\endlink
   - \link Player_StartTime_format `Player.StartTime(format)` \endlink
   - \link Player_SeekNumeric_format `Player.SeekNumeric(format)`\endlink
   - \link ListItem_Duration_format `ListItem.Duration(format)`\endlink
   - \link PVR_EpgEventDuration_format `PVR.EpgEventDuration(format)`\endlink
   - \link PVR_EpgEventElapsedTime_format `PVR.EpgEventElapsedTime(format)`\endlink
   - \link PVR_EpgEventRemainingTime_format `PVR.EpgEventRemainingTime(format)`\endlink
   - \link PVR_EpgEventSeekTime_format `PVR.EpgEventSeekTime(format)`\endlink
   - \link PVR_EpgEventFinishTime_format `PVR.EpgEventFinishTime(format)`\endlink
   - \link PVR_TimeShiftStart_format `PVR.TimeShiftStart(format)`\endlink
   - \link PVR_TimeShiftEnd_format `PVR.TimeShiftEnd(format)`\endlink
   - \link PVR_TimeShiftCur_format `PVR.TimeShiftCur(format)`\endlink
   - \link PVR_TimeShiftOffset_format `PVR.TimeShiftOffset(format)`\endlink
   - \link PVR_TimeshiftProgressDuration_format `PVR.TimeshiftProgressDuration(format)`\endlink
   - \link PVR_TimeshiftProgressEndTime `PVR.TimeshiftProgressEndTime`\endlink
   - \link PVR_TimeshiftProgressEndTime_format `PVR.TimeshiftProgressEndTime(format)`\endlink
   - \link ListItem_NextDuration_format `ListItem.NextDuration(format)` \endlink
  <p>
 */
enum TIME_FORMAT
{
  TIME_FORMAT_GUESS = 0, ///< usually used as the fallback value if the format value is empty
  TIME_FORMAT_SS = 1, ///< <b>ss</b> - seconds only
  TIME_FORMAT_MM = 2, ///< <b>mm</b> - minutes only (2-digit)
  TIME_FORMAT_MM_SS = 3, ///< <b>mm:ss</b> - minutes and seconds
  TIME_FORMAT_HH = 4, ///< <b>hh</b> - hours only (2-digit)
  TIME_FORMAT_HH_SS = 5, ///< <b>hh:ss</b> - hours and seconds (this is not particularly useful)
  TIME_FORMAT_HH_MM = 6, ///< <b>hh:mm</b> - hours and minutes
  TIME_FORMAT_HH_MM_SS = 7, ///< <b>hh:mm:ss</b> - hours, minutes and seconds
  TIME_FORMAT_XX = 8, ///<  <b>xx</b> - returns AM/PM for a 12-hour clock
  TIME_FORMAT_HH_MM_XX =
      14, ///< <b>hh:mm xx</b> - returns hours and minutes in a 12-hour clock format (AM/PM)
  TIME_FORMAT_HH_MM_SS_XX =
      15, ///< <b>hh:mm:ss xx</b> - returns hours (2-digit), minutes and seconds in a 12-hour clock format (AM/PM)
  TIME_FORMAT_H = 16, ///< <b>h</b> - hours only (1-digit)
  TIME_FORMAT_H_MM_SS = 19, ///< <b>hh:mm:ss</b> - hours, minutes and seconds
  TIME_FORMAT_H_MM_SS_XX =
      27, ///< <b>hh:mm:ss xx</b> - returns hours (1-digit), minutes and seconds in a 12-hour clock format (AM/PM)
  TIME_FORMAT_SECS = 32, ///<  <b>secs</b> - total time in seconds
  TIME_FORMAT_MINS = 64, ///<  <b>mins</b> - total time in minutes
  TIME_FORMAT_HOURS = 128, ///< <b>hours</b> - total time in hours
  TIME_FORMAT_M = 256 ///< <b>m</b> - minutes only (1-digit)
};

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
  void SetFromPeriod(const std::string &period);
  void SetFromTimeString(const std::string& time);

  int GetDays() const;
  int GetHours() const;
  int GetMinutes() const;
  int GetSeconds() const;
  int GetSecondsTotal() const;

private:
  void ToULargeInt(ULARGE_INTEGER& time) const;
  void FromULargeInt(const ULARGE_INTEGER& time);

private:
  FILETIME m_timeSpan;

  friend class CDateTime;
};

/// \brief DateTime class, which uses FILETIME as it's base.
class CDateTime final : public IArchivable
{
public:
  CDateTime();
  CDateTime(const CDateTime& time);
  explicit CDateTime(const KODI::TIME::SystemTime& time);
  explicit CDateTime(const FILETIME& time);
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

  operator FILETIME() const;

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
  void GetAsTimeStamp(FILETIME& time) const;

  CDateTime GetAsUTCDateTime() const;
  std::string GetAsSaveString() const;
  std::string GetAsDBDateTime() const;
  std::string GetAsDBDate() const;
  std::string GetAsDBTime() const;
  std::string GetAsLocalizedDate(bool longDate=false) const;
  std::string GetAsLocalizedDate(const std::string &strFormat) const;
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
  bool ToFileTime(const KODI::TIME::SystemTime& time, FILETIME& fileTime) const;
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
