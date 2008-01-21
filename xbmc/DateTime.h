#pragma once
#include "utils/Archive.h"

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
class CDateTime : public ISerializable
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

  virtual void Serialize(CArchive& ar);

  void Reset();

  int GetDay() const;
  int GetMonth() const;
  int GetYear() const;
  int GetHour() const;
  int GetMinute() const;
  int GetSecond() const;
  int GetDayOfWeek() const;

  void SetDateTime(int year, int month, int day, int hour, int minute, int second);
  void SetDate(int year, int month, int day);
  void SetTime(int hour, int minute, int second);
  void SetFromDBDate(const CStdString &date);

  void GetAsSystemTime(SYSTEMTIME& time) const;
  void GetAsTime(time_t& time) const;
  void GetAsTm(tm& time) const;
  void GetAsTimeStamp(FILETIME& time) const;

  CStdString GetAsDBDate() const;
  CStdString GetAsLocalizedDate(bool longDate=false) const;
  CStdString GetAsLocalizedTime(const CStdString &format, bool withSeconds=true) const;
  CStdString GetAsLocalizedDateTime(bool longDate=false, bool withSeconds=true) const;

  void SetValid(bool yesNo);
  bool IsValid() const;

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
