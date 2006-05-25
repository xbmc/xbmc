#include "stdafx.h"
#include "Temperature.h"


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
  if (this==&right)
    return false;

  return (m_value>right.m_value);
}

bool CTemperature::operator >=(const CTemperature& right) const
{
  if (this==&right)
    return true;

  return (m_value>=right.m_value);
}

bool CTemperature::operator <(const CTemperature& right) const
{
  if (this==&right)
    return false;

  return (m_value<right.m_value);
}

bool CTemperature::operator <=(const CTemperature& right) const
{
  if (this==&right)
    return true;

  return (m_value<=right.m_value);
}

bool CTemperature::operator ==(const CTemperature& right) const
{
  if (this==&right)
    return true;

  return (m_value==right.m_value);
}

bool CTemperature::operator !=(const CTemperature& right) const
{
  if (this==&right)
    return false;

  return (m_value!=right.m_value);
}

CTemperature& CTemperature::operator =(const CTemperature& right)
{
  m_state=right.m_state;
  m_value=right.m_value;
  return *this;
}

CTemperature& CTemperature::operator +=(const CTemperature& right)
{
  m_value+=right.m_value;
  return *this;
}

CTemperature& CTemperature::operator -=(const CTemperature& right)
{
  m_value-=right.m_value;
  return *this;
}

CTemperature& CTemperature::operator *=(const CTemperature& right)
{
  m_value*=right.m_value;
  return *this;
}

CTemperature& CTemperature::operator /=(const CTemperature& right)
{
  m_value/=right.m_value;
  return *this;
}

CTemperature& CTemperature::operator +(const CTemperature& right)
{
  m_value+=right.m_value;
  return *this;
}

CTemperature& CTemperature::operator -(const CTemperature& right)
{
  m_value-=right.m_value;
  return *this;
}

CTemperature& CTemperature::operator *(const CTemperature& right)
{
  m_value*=right.m_value;
  return *this;
}

CTemperature& CTemperature::operator /(const CTemperature& right)
{
  m_value/=right.m_value;
  return *this;
}

CTemperature& CTemperature::operator ++()
{
  m_value++;
  return *this;
}

CTemperature& CTemperature::operator --()
{
  m_value--;
  return *this;
}

bool CTemperature::operator >(double right) const
{
  return (m_value>right);
}

bool CTemperature::operator >=(double right) const
{
  return (m_value>=right);
}

bool CTemperature::operator <(double right) const
{
  return (m_value<right);
}

bool CTemperature::operator <=(double right) const
{
  return (m_value<=right);
}

bool CTemperature::operator ==(double right) const
{
  return (m_value==right);
}

bool CTemperature::operator !=(double right) const
{
  return (m_value!=right);
}

CTemperature& CTemperature::operator +=(double right)
{
  m_value+=right;
  return *this;
}

CTemperature& CTemperature::operator -=(double right)
{
  m_value-=right;
  return *this;
}

CTemperature& CTemperature::operator *=(double right)
{
  m_value*=right;
  return *this;
}

CTemperature& CTemperature::operator /=(double right)
{
  m_value/=right;
  return *this;
}

CTemperature& CTemperature::operator +(double right)
{
  m_value+=right;
  return *this;
}

CTemperature& CTemperature::operator -(double right)
{
  m_value-=right;
  return *this;
}

CTemperature& CTemperature::operator *(double right)
{
  m_value*=right;
  return *this;
}

CTemperature& CTemperature::operator /(double right)
{
  m_value/=right;
  return *this;
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

void CTemperature::Serialize(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar<<m_value;
    ar<<(int)m_state;
  }
  else
  {
    ar>>m_value;
    ar>>(int&)m_state;
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

// Returns temperature as localized string
CStdString CTemperature::ToString() const
{
  if (!IsValid())
    return g_localizeStrings.Get(13205); // "Unknown"

  double value=0.0;

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

  CStdString str;
  str.Format("%2.0f%s", value, g_langInfo.GetTempUnitString());

  return str;
}
