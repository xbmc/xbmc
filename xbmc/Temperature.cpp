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

#include "LangInfo.h"
#include "guilib/LocalizeStrings.h"
#include "Temperature.h"
#include "utils/StringUtils.h"
#include "utils/Archive.h"
#include <assert.h>

CTemperature::CTemperature()
{
  m_value=0.0f;
  m_state=invalid;
}

CTemperature::CTemperature(const CTemperature& temperature)
{
  m_value=temperature.m_value;
  m_state=temperature.m_state;
}

CTemperature::CTemperature(double value)
{
  m_value=value;
  m_state=valid;
}

bool CTemperature::operator >(const CTemperature& right) const
{
  assert(IsValid());
  assert(right.IsValid());

  if (!IsValid() || !right.IsValid())
    return false;

  if (this==&right)
    return false;

  return (m_value>right.m_value);
}

bool CTemperature::operator >=(const CTemperature& right) const
{
  return operator >(right) || operator ==(right);
}

bool CTemperature::operator <(const CTemperature& right) const
{
  assert(IsValid());
  assert(right.IsValid());

  if (!IsValid() || !right.IsValid())
    return false;

  if (this==&right)
    return false;

  return (m_value<right.m_value);
}

bool CTemperature::operator <=(const CTemperature& right) const
{
  return operator <(right) || operator ==(right);
}

bool CTemperature::operator ==(const CTemperature& right) const
{
  assert(IsValid());
  assert(right.IsValid());

  if (!IsValid() || !right.IsValid())
    return false;

  if (this==&right)
    return true;

  return (m_value==right.m_value);
}

bool CTemperature::operator !=(const CTemperature& right) const
{
  return !operator ==(right.m_value);
}

const CTemperature& CTemperature::operator =(const CTemperature& right)
{
  m_state=right.m_state;
  m_value=right.m_value;
  return *this;
}

const CTemperature& CTemperature::operator +=(const CTemperature& right)
{
  assert(IsValid());
  assert(right.IsValid());

  m_value+=right.m_value;
  return *this;
}

const CTemperature& CTemperature::operator -=(const CTemperature& right)
{
  assert(IsValid());
  assert(right.IsValid());

  m_value-=right.m_value;
  return *this;
}

const CTemperature& CTemperature::operator *=(const CTemperature& right)
{
  assert(IsValid());
  assert(right.IsValid());

  m_value*=right.m_value;
  return *this;
}

const CTemperature& CTemperature::operator /=(const CTemperature& right)
{
  assert(IsValid());
  assert(right.IsValid());

  m_value/=right.m_value;
  return *this;
}

CTemperature CTemperature::operator +(const CTemperature& right) const
{
  assert(IsValid());
  assert(right.IsValid());

  CTemperature temp(*this);

  if (!IsValid() || !right.IsValid())
    temp.SetState(invalid);
  else
    temp.m_value+=right.m_value;

  return temp;
}

CTemperature CTemperature::operator -(const CTemperature& right) const
{
  assert(IsValid());
  assert(right.IsValid());

  CTemperature temp(*this);
  if (!IsValid() || !right.IsValid())
    temp.SetState(invalid);
  else
    temp.m_value-=right.m_value;

  return temp;
}

CTemperature CTemperature::operator *(const CTemperature& right) const
{
  assert(IsValid());
  assert(right.IsValid());

  CTemperature temp(*this);
  if (!IsValid() || !right.IsValid())
    temp.SetState(invalid);
  else
    temp.m_value*=right.m_value;
  return temp;
}

CTemperature CTemperature::operator /(const CTemperature& right) const
{
  assert(IsValid());
  assert(right.IsValid());

  CTemperature temp(*this);
  if (!IsValid() || !right.IsValid())
    temp.SetState(invalid);
  else
    temp.m_value/=right.m_value;
  return temp;
}

CTemperature& CTemperature::operator ++()
{
  assert(IsValid());

  m_value++;
  return *this;
}

CTemperature& CTemperature::operator --()
{
  assert(IsValid());

  m_value--;
  return *this;
}

CTemperature CTemperature::operator ++(int)
{
  assert(IsValid());

  CTemperature temp(*this);
  m_value++;
  return temp;
}

CTemperature CTemperature::operator --(int)
{
  assert(IsValid());

  CTemperature temp(*this);
  m_value--;
  return temp;
}

bool CTemperature::operator >(double right) const
{
  assert(IsValid());

  if (!IsValid())
    return false;

  return (m_value>right);
}

bool CTemperature::operator >=(double right) const
{
  return operator >(right) || operator ==(right);
}

bool CTemperature::operator <(double right) const
{
  assert(IsValid());

  if (!IsValid())
    return false;

  return (m_value<right);
}

bool CTemperature::operator <=(double right) const
{
  return operator <(right) || operator ==(right);
}

bool CTemperature::operator ==(double right) const
{
  if (!IsValid())
    return false;

  return (m_value==right);
}

bool CTemperature::operator !=(double right) const
{
  return !operator ==(right);
}

const CTemperature& CTemperature::operator +=(double right)
{
  assert(IsValid());

  m_value+=right;
  return *this;
}

const CTemperature& CTemperature::operator -=(double right)
{
  assert(IsValid());

  m_value-=right;
  return *this;
}

const CTemperature& CTemperature::operator *=(double right)
{
  assert(IsValid());

  m_value*=right;
  return *this;
}

const CTemperature& CTemperature::operator /=(double right)
{
  assert(IsValid());

  m_value/=right;
  return *this;
}

CTemperature CTemperature::operator +(double right) const
{
  assert(IsValid());

  CTemperature temp(*this);
  temp.m_value+=right;
  return temp;
}

CTemperature CTemperature::operator -(double right) const
{
  assert(IsValid());

  CTemperature temp(*this);
  temp.m_value-=right;
  return temp;
}

CTemperature CTemperature::operator *(double right) const
{
  assert(IsValid());

  CTemperature temp(*this);
  temp.m_value*=right;
  return temp;
}

CTemperature CTemperature::operator /(double right) const
{
  assert(IsValid());

  CTemperature temp(*this);
  temp.m_value/=right;
  return temp;
}

CTemperature CTemperature::CreateFromFahrenheit(double value)
{
  return CTemperature(value);
}

CTemperature CTemperature::CreateFromReaumur(double value)
{
  return CTemperature(value*2.25f+32.0f);
}

CTemperature CTemperature::CreateFromRankine(double value)
{
  return CTemperature(value-459.67f);
}

CTemperature CTemperature::CreateFromRomer(double value)
{
  return CTemperature((value-7.5f)*24.0f/7.0f+32.0f);
}

CTemperature CTemperature::CreateFromDelisle(double value)
{
  CTemperature temp(212.0f - value * 1.2f);
  return temp;
}

CTemperature CTemperature::CreateFromNewton(double value)
{
  return CTemperature(value*60.0f/11.0f+32.0f);
}

CTemperature CTemperature::CreateFromCelsius(double value)
{
  return CTemperature(value*1.8f+32.0f);
}

void CTemperature::Archive(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar<<m_value;
    ar<<(int)m_state;
  }
  else
  {
    ar>>m_value;
    int state;
    ar>>(int&)state;
    m_state = CTemperature::STATE(state);
  }
}

void CTemperature::SetState(CTemperature::STATE state)
{
  m_state=state;
}

bool CTemperature::IsValid() const
{
  return (m_state==valid);
}

double CTemperature::ToFahrenheit() const
{
  return m_value;
}

double CTemperature::ToKelvin() const
{
  return (m_value+459.67F)/1.8f;
}

double CTemperature::ToCelsius() const
{
  return (m_value-32.0f)/1.8f;
}

double CTemperature::ToReaumur() const
{
  return (m_value-32.0f)/2.25f;
}

double CTemperature::ToRankine() const
{
  return m_value+459.67f;
}

double CTemperature::ToRomer() const
{
  return (m_value-32.0f)*7.0f/24.0f+7.5f;
}

double CTemperature::ToDelisle() const
{
  return (212.f-m_value)*5.0f/6.0f;
}

double CTemperature::ToNewton() const
{
  return (m_value-32.0f)*11.0f/60.0f;
}

double CTemperature::ToLocale() const
{
  if (!IsValid())
    return 0;
  double value = 0.0;

  switch(g_langInfo.GetTempUnit())
  {
  case CLangInfo::TEMP_UNIT_FAHRENHEIT:
    value=ToFahrenheit();
    break;
  case CLangInfo::TEMP_UNIT_KELVIN:
    value=ToKelvin();
    break;
  case CLangInfo::TEMP_UNIT_CELSIUS:
    value=ToCelsius();
    break;
  case CLangInfo::TEMP_UNIT_REAUMUR:
    value=ToReaumur();
    break;
  case CLangInfo::TEMP_UNIT_RANKINE:
    value=ToRankine();
    break;
  case CLangInfo::TEMP_UNIT_ROMER:
    value=ToRomer();
    break;
  case CLangInfo::TEMP_UNIT_DELISLE:
    value=ToDelisle();
    break;
  case CLangInfo::TEMP_UNIT_NEWTON:
    value=ToNewton();
    break;
  default:
    assert(false);
    break;
  }
  return value;
}

// Returns temperature as localized string
std::string CTemperature::ToString() const
{
  if (!IsValid())
    return g_localizeStrings.Get(13205); // "Unknown"

  return StringUtils::Format("%2.0f%s", ToLocale(), g_langInfo.GetTempUnitString().c_str());
}
