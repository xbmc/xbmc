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

#include <assert.h>

#include "Speed.h"
#include "utils/Archive.h"
#include "utils/StringUtils.h"

CSpeed::CSpeed()
{
  m_value = 0.0;
  m_valid = false;
}

CSpeed::CSpeed(const CSpeed& speed)
{
  m_value = speed.m_value;
  m_valid = speed.m_valid;
}

CSpeed::CSpeed(double value)
{
  m_value = value;
  m_valid = true;
}

bool CSpeed::operator >(const CSpeed& right) const
{
  assert(IsValid());
  assert(right.IsValid());

  if (!IsValid() || !right.IsValid())
    return false;

  if (this == &right)
    return false;

  return (m_value > right.m_value);
}

bool CSpeed::operator >=(const CSpeed& right) const
{
  return operator >(right) || operator ==(right);
}

bool CSpeed::operator <(const CSpeed& right) const
{
  assert(IsValid());
  assert(right.IsValid());

  if (!IsValid() || !right.IsValid())
    return false;

  if (this == &right)
    return false;

  return (m_value < right.m_value);
}

bool CSpeed::operator <=(const CSpeed& right) const
{
  return operator <(right) || operator ==(right);
}

bool CSpeed::operator ==(const CSpeed& right) const
{
  assert(IsValid());
  assert(right.IsValid());

  if (!IsValid() || !right.IsValid())
    return false;

  if (this == &right)
    return true;

  return (m_value == right.m_value);
}

bool CSpeed::operator !=(const CSpeed& right) const
{
  return !operator ==(right.m_value);
}

const CSpeed& CSpeed::operator =(const CSpeed& right)
{
  m_valid = right.m_valid;
  m_value = right.m_value;
  return *this;
}

const CSpeed& CSpeed::operator +=(const CSpeed& right)
{
  assert(IsValid());
  assert(right.IsValid());

  m_value += right.m_value;
  return *this;
}

const CSpeed& CSpeed::operator -=(const CSpeed& right)
{
  assert(IsValid());
  assert(right.IsValid());

  m_value -= right.m_value;
  return *this;
}

const CSpeed& CSpeed::operator *=(const CSpeed& right)
{
  assert(IsValid());
  assert(right.IsValid());

  m_value *= right.m_value;
  return *this;
}

const CSpeed& CSpeed::operator /=(const CSpeed& right)
{
  assert(IsValid());
  assert(right.IsValid());

  m_value /= right.m_value;
  return *this;
}

CSpeed CSpeed::operator +(const CSpeed& right) const
{
  assert(IsValid());
  assert(right.IsValid());

  CSpeed temp(*this);

  if (!IsValid() || !right.IsValid())
    temp.SetValid(false);
  else
    temp.m_value += right.m_value;

  return temp;
}

CSpeed CSpeed::operator -(const CSpeed& right) const
{
  assert(IsValid());
  assert(right.IsValid());

  CSpeed temp(*this);
  if (!IsValid() || !right.IsValid())
    temp.SetValid(false);
  else
    temp.m_value -= right.m_value;

  return temp;
}

CSpeed CSpeed::operator *(const CSpeed& right) const
{
  assert(IsValid());
  assert(right.IsValid());

  CSpeed temp(*this);
  if (!IsValid() || !right.IsValid())
    temp.SetValid(false);
  else
    temp.m_value *= right.m_value;
  return temp;
}

CSpeed CSpeed::operator /(const CSpeed& right) const
{
  assert(IsValid());
  assert(right.IsValid());

  CSpeed temp(*this);
  if (!IsValid() || !right.IsValid())
    temp.SetValid(false);
  else
    temp.m_value /= right.m_value;
  return temp;
}

CSpeed& CSpeed::operator ++()
{
  assert(IsValid());

  m_value++;
  return *this;
}

CSpeed& CSpeed::operator --()
{
  assert(IsValid());

  m_value--;
  return *this;
}

CSpeed CSpeed::operator ++(int)
{
  assert(IsValid());

  CSpeed temp(*this);
  m_value++;
  return temp;
}

CSpeed CSpeed::operator --(int)
{
  assert(IsValid());

  CSpeed temp(*this);
  m_value--;
  return temp;
}

bool CSpeed::operator >(double right) const
{
  assert(IsValid());

  if (!IsValid())
    return false;

  return (m_value > right);
}

bool CSpeed::operator >=(double right) const
{
  return operator >(right) || operator ==(right);
}

bool CSpeed::operator <(double right) const
{
  assert(IsValid());

  if (!IsValid())
    return false;

  return (m_value < right);
}

bool CSpeed::operator <=(double right) const
{
  return operator <(right) || operator ==(right);
}

bool CSpeed::operator ==(double right) const
{
  if (!IsValid())
    return false;

  return (m_value == right);
}

bool CSpeed::operator !=(double right) const
{
  return !operator ==(right);
}

const CSpeed& CSpeed::operator +=(double right)
{
  assert(IsValid());

  m_value += right;
  return *this;
}

const CSpeed& CSpeed::operator -=(double right)
{
  assert(IsValid());

  m_value -= right;
  return *this;
}

const CSpeed& CSpeed::operator *=(double right)
{
  assert(IsValid());

  m_value *= right;
  return *this;
}

const CSpeed& CSpeed::operator /=(double right)
{
  assert(IsValid());

  m_value /= right;
  return *this;
}

CSpeed CSpeed::operator +(double right) const
{
  assert(IsValid());

  CSpeed temp(*this);
  temp.m_value += right;
  return temp;
}

CSpeed CSpeed::operator -(double right) const
{
  assert(IsValid());

  CSpeed temp(*this);
  temp.m_value -= right;
  return temp;
}

CSpeed CSpeed::operator *(double right) const
{
  assert(IsValid());

  CSpeed temp(*this);
  temp.m_value *= right;
  return temp;
}

CSpeed CSpeed::operator /(double right) const
{
  assert(IsValid());

  CSpeed temp(*this);
  temp.m_value /= right;
  return temp;
}

CSpeed CSpeed::CreateFromKilometresPerHour(double value)
{
  return CSpeed(value / 3.6);
}

CSpeed CSpeed::CreateFromMetresPerMinute(double value)
{
  return CSpeed(value / 60.0);
}

CSpeed CSpeed::CreateFromMetresPerSecond(double value)
{
  return CSpeed(value);
}

CSpeed CSpeed::CreateFromFeetPerHour(double value)
{
  return CreateFromFeetPerMinute(value / 60.0);
}

CSpeed CSpeed::CreateFromFeetPerMinute(double value)
{
  return CreateFromFeetPerSecond(value / 60.0);
}

CSpeed CSpeed::CreateFromFeetPerSecond(double value)
{
  return CSpeed(value / 3.280839895);
}

CSpeed CSpeed::CreateFromMilesPerHour(double value)
{
  return CSpeed(value / 2.236936292);
}

CSpeed CSpeed::CreateFromKnots(double value)
{
  return CSpeed(value / 1.943846172);
}

CSpeed CSpeed::CreateFromBeaufort(unsigned int value)
{
  if (value == 0)
    return CSpeed(0.15);
  if (value == 1)
    return CSpeed(0.9);
  if (value == 2)
    return CSpeed(2.4);
  if (value == 3)
    return CSpeed(4.4);
  if (value == 4)
    return CSpeed(6.75);
  if (value == 5)
    return CSpeed(9.4);
  if (value == 6)
    return CSpeed(12.35);
  if (value == 7)
    return CSpeed(15.55);
  if (value == 8)
    return CSpeed(18.95);
  if (value == 9)
    return CSpeed(22.6);
  if (value == 10)
    return CSpeed(26.45);
  if (value == 11)
    return CSpeed(30.5);

  return CSpeed(32.6);
}

CSpeed CSpeed::CreateFromInchPerSecond(double value)
{
  return CSpeed(value / 39.37007874);
}

CSpeed CSpeed::CreateFromYardPerSecond(double value)
{
  return CSpeed(value / 1.093613298);
}

CSpeed CSpeed::CreateFromFurlongPerFortnight(double value)
{
  return CSpeed(value / 6012.885613871);
}

void CSpeed::Archive(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << m_value;
    ar << m_valid;
  }
  else
  {
    ar >> m_value;
    ar >> m_valid;
  }
}

bool CSpeed::IsValid() const
{
  return m_valid;
}

double CSpeed::ToKilometresPerHour() const
{
  return m_value * 3.6;
}

double CSpeed::ToMetresPerMinute() const
{
  return m_value * 60.0;
}

double CSpeed::ToMetresPerSecond() const
{
  return m_value;
}

double CSpeed::ToFeetPerHour() const
{
  return ToFeetPerMinute() * 60.0;
}

double CSpeed::ToFeetPerMinute() const
{
  return ToFeetPerSecond() * 60.0;
}

double CSpeed::ToFeetPerSecond() const
{
  return m_value * 3.280839895;
}

double CSpeed::ToMilesPerHour() const
{
  return m_value * 2.236936292;
}

double CSpeed::ToKnots() const
{
  return m_value * 1.943846172;
}

double CSpeed::ToBeaufort() const
{
  if (m_value < 0.3)
    return 0;
  if (m_value >= 0.3 && m_value < 1.5)
    return 1;
  if (m_value >= 1.5 && m_value < 3.3)
    return 2;
  if (m_value >= 3.3 && m_value < 5.5)
    return 3;
  if (m_value >= 5.5 && m_value < 8.0)
    return 4;
  if (m_value >= 8.0 && m_value < 10.8)
    return 5;
  if (m_value >= 10.8 && m_value < 13.9)
    return 6;
  if (m_value >= 13.9 && m_value < 17.2)
    return 7;
  if (m_value >= 17.2 && m_value < 20.7)
    return 8;
  if (m_value >= 20.7 && m_value < 24.5)
    return 9;
  if (m_value >= 24.5 && m_value < 28.4)
    return 10;
  if (m_value >= 28.4 && m_value < 32.6)
    return 11;

  return 12;
}

double CSpeed::ToInchPerSecond() const
{
  return m_value * 39.37007874;
}

double CSpeed::ToYardPerSecond() const
{
  return m_value * 1.093613298;
}

double CSpeed::ToFurlongPerFortnight() const
{
  return m_value * 6012.885613871;
}

double CSpeed::To(Unit speedUnit) const
{
  if (!IsValid())
    return 0;

  double value = 0.0;

  switch (speedUnit)
  {
  case UnitKilometresPerHour:
    value = ToKilometresPerHour();
    break;
  case UnitMetresPerMinute:
    value = ToMetresPerMinute();
    break;
  case UnitMetresPerSecond:
    value = ToMetresPerSecond();
    break;
  case UnitFeetPerHour:
    value = ToFeetPerHour();
    break;
  case UnitFeetPerMinute:
    value = ToFeetPerMinute();
    break;
  case UnitFeetPerSecond:
    value = ToFeetPerSecond();
    break;
  case UnitMilesPerHour:
    value = ToMilesPerHour();
    break;
  case UnitKnots:
    value = ToKnots();
    break;
  case UnitBeaufort:
    value = ToBeaufort();
    break;
  case UnitInchPerSecond:
    value = ToInchPerSecond();
    break;
  case UnitYardPerSecond:
    value = ToYardPerSecond();
    break;
  case UnitFurlongPerFortnight:
    value = ToFurlongPerFortnight();
    break;
  default:
    assert(false);
    break;
  }
  return value;
}

// Returns temperature as localized string
std::string CSpeed::ToString(Unit speedUnit) const
{
  if (!IsValid())
    return "";

  return StringUtils::Format("%2.0f", To(speedUnit));
}
