/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Temperature.h"

#include "utils/Archive.h"
#include "utils/StringUtils.h"

#include <assert.h>

CTemperature::CTemperature()
{
  m_value = 0.0;
  m_valid=false;
}

CTemperature::CTemperature(const CTemperature& temperature)
{
  m_value=temperature.m_value;
  m_valid=temperature.m_valid;
}

CTemperature::CTemperature(double value)
{
  m_value=value;
  m_valid=true;
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

CTemperature& CTemperature::operator =(const CTemperature& right)
{
  m_valid=right.m_valid;
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
    temp.SetValid(false);
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
    temp.SetValid(false);
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
    temp.SetValid(false);
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
    temp.SetValid(false);
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
  return CTemperature(value * 2.25 + 32.0);
}

CTemperature CTemperature::CreateFromRankine(double value)
{
  return CTemperature(value - 459.67);
}

CTemperature CTemperature::CreateFromRomer(double value)
{
  return CTemperature((value - 7.5) * 24.0 / 7.0 + 32.0);
}

CTemperature CTemperature::CreateFromDelisle(double value)
{
  CTemperature temp(212.0 - value * 1.2);
  return temp;
}

CTemperature CTemperature::CreateFromNewton(double value)
{
  return CTemperature(value * 60.0 / 11.0 + 32.0);
}

CTemperature CTemperature::CreateFromCelsius(double value)
{
  return CTemperature(value * 1.8 + 32.0);
}

CTemperature CTemperature::CreateFromKelvin(double value)
{
  return CTemperature((value - 273.15) * 1.8 + 32.0);
}

void CTemperature::Archive(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar<<m_value;
    ar<<m_valid;
  }
  else
  {
    ar>>m_value;
    ar>>m_valid;
  }
}

bool CTemperature::IsValid() const
{
  return m_valid;
}

double CTemperature::ToFahrenheit() const
{
  return m_value;
}

double CTemperature::ToKelvin() const
{
  return (m_value + 459.67) / 1.8;
}

double CTemperature::ToCelsius() const
{
  return (m_value - 32.0) / 1.8;
}

double CTemperature::ToReaumur() const
{
  return (m_value - 32.0) / 2.25;
}

double CTemperature::ToRankine() const
{
  return m_value + 459.67;
}

double CTemperature::ToRomer() const
{
  return (m_value - 32.0) * 7.0 / 24.0 + 7.5;
}

double CTemperature::ToDelisle() const
{
  return (212.0 - m_value) * 5.0 / 6.0;
}

double CTemperature::ToNewton() const
{
  return (m_value - 32.0) * 11.0 / 60.0;
}

double CTemperature::To(Unit temperatureUnit) const
{
  if (!IsValid())
    return 0;

  double value = 0.0;

  switch (temperatureUnit)
  {
  case UnitFahrenheit:
    value=ToFahrenheit();
    break;
  case UnitKelvin:
    value=ToKelvin();
    break;
  case UnitCelsius:
    value=ToCelsius();
    break;
  case UnitReaumur:
    value=ToReaumur();
    break;
  case UnitRankine:
    value=ToRankine();
    break;
  case UnitRomer:
    value=ToRomer();
    break;
  case UnitDelisle:
    value=ToDelisle();
    break;
  case UnitNewton:
    value=ToNewton();
    break;
  default:
    assert(false);
    break;
  }
  return value;
}

// Returns temperature as localized string
std::string CTemperature::ToString(Unit temperatureUnit) const
{
  if (!IsValid())
    return "";

  return StringUtils::Format("{:2.0f}", To(temperatureUnit));
}
