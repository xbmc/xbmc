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

class CTemperature : public IArchivable
{
public:
  CTemperature();
  CTemperature(const CTemperature& temperature);

  static CTemperature CreateFromFahrenheit(double value);
  static CTemperature CreateFromKelvin(double value);
  static CTemperature CreateFromCelsius(double value);
  static CTemperature CreateFromReaumur(double value);
  static CTemperature CreateFromRankine(double value);
  static CTemperature CreateFromRomer(double value);
  static CTemperature CreateFromDelisle(double value);
  static CTemperature CreateFromNewton(double value);

  bool operator >(const CTemperature& right) const;
  bool operator >=(const CTemperature& right) const;
  bool operator <(const CTemperature& right) const;
  bool operator <=(const CTemperature& right) const;
  bool operator ==(const CTemperature& right) const;
  bool operator !=(const CTemperature& right) const;

  const CTemperature& operator =(const CTemperature& right);
  const CTemperature& operator +=(const CTemperature& right);
  const CTemperature& operator -=(const CTemperature& right);
  const CTemperature& operator *=(const CTemperature& right);
  const CTemperature& operator /=(const CTemperature& right);
  CTemperature operator +(const CTemperature& right) const;
  CTemperature operator -(const CTemperature& right) const;
  CTemperature operator *(const CTemperature& right) const;
  CTemperature operator /(const CTemperature& right) const;

  bool operator >(double right) const;
  bool operator >=(double right) const;
  bool operator <(double right) const;
  bool operator <=(double right) const;
  bool operator ==(double right) const;
  bool operator !=(double right) const;

  const CTemperature& operator +=(double right);
  const CTemperature& operator -=(double right);
  const CTemperature& operator *=(double right);
  const CTemperature& operator /=(double right);
  CTemperature operator +(double right) const;
  CTemperature operator -(double right) const;
  CTemperature operator *(double right) const;
  CTemperature operator /(double right) const;

  CTemperature& operator ++();
  CTemperature& operator --();
  CTemperature operator ++(int);
  CTemperature operator --(int);

  virtual void Archive(CArchive& ar);

  typedef enum _STATE
  {
    invalid=0,
    valid
  } STATE;

  void SetState(CTemperature::STATE state);
  bool IsValid() const;

  double ToFahrenheit() const;
  double ToKelvin() const;
  double ToCelsius() const;
  double ToReaumur() const;
  double ToRankine() const;
  double ToRomer() const;
  double ToDelisle() const;
  double ToNewton() const;

  double ToLocale() const;
  CStdString ToString() const;

protected:
  CTemperature(double value);

protected:
  double m_value; // we store as fahrenheit
  STATE m_state;
};

