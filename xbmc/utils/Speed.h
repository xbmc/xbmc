#pragma once
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

#include <string>

#include "utils/IArchivable.h"

class CSpeed : public IArchivable
{
public:
  CSpeed();
  CSpeed(const CSpeed& speed);

  typedef enum Unit
  {
    UnitKilometresPerHour = 0,
    UnitMetresPerMinute,
    UnitMetresPerSecond,
    UnitFeetPerHour,
    UnitFeetPerMinute,
    UnitFeetPerSecond,
    UnitMilesPerHour,
    UnitKnots,
    UnitBeaufort,
    UnitInchPerSecond,
    UnitYardPerSecond,
    UnitFurlongPerFortnight
  } Unit;

  static CSpeed CreateFromKilometresPerHour(double value);
  static CSpeed CreateFromMetresPerMinute(double value);
  static CSpeed CreateFromMetresPerSecond(double value);
  static CSpeed CreateFromFeetPerHour(double value);
  static CSpeed CreateFromFeetPerMinute(double value);
  static CSpeed CreateFromFeetPerSecond(double value);
  static CSpeed CreateFromMilesPerHour(double value);
  static CSpeed CreateFromKnots(double value);
  static CSpeed CreateFromBeaufort(unsigned int value);
  static CSpeed CreateFromInchPerSecond(double value);
  static CSpeed CreateFromYardPerSecond(double value);
  static CSpeed CreateFromFurlongPerFortnight(double value);

  bool operator >(const CSpeed& right) const;
  bool operator >=(const CSpeed& right) const;
  bool operator <(const CSpeed& right) const;
  bool operator <=(const CSpeed& right) const;
  bool operator ==(const CSpeed& right) const;
  bool operator !=(const CSpeed& right) const;

  const CSpeed& operator =(const CSpeed& right);
  const CSpeed& operator +=(const CSpeed& right);
  const CSpeed& operator -=(const CSpeed& right);
  const CSpeed& operator *=(const CSpeed& right);
  const CSpeed& operator /=(const CSpeed& right);
  CSpeed operator +(const CSpeed& right) const;
  CSpeed operator -(const CSpeed& right) const;
  CSpeed operator *(const CSpeed& right) const;
  CSpeed operator /(const CSpeed& right) const;

  bool operator >(double right) const;
  bool operator >=(double right) const;
  bool operator <(double right) const;
  bool operator <=(double right) const;
  bool operator ==(double right) const;
  bool operator !=(double right) const;

  const CSpeed& operator +=(double right);
  const CSpeed& operator -=(double right);
  const CSpeed& operator *=(double right);
  const CSpeed& operator /=(double right);
  CSpeed operator +(double right) const;
  CSpeed operator -(double right) const;
  CSpeed operator *(double right) const;
  CSpeed operator /(double right) const;

  CSpeed& operator ++();
  CSpeed& operator --();
  CSpeed operator ++(int);
  CSpeed operator --(int);

  virtual void Archive(CArchive& ar);

  bool IsValid() const;

  double ToKilometresPerHour() const;
  double ToMetresPerMinute() const;
  double ToMetresPerSecond() const;
  double ToFeetPerHour() const;
  double ToFeetPerMinute() const;
  double ToFeetPerSecond() const;
  double ToMilesPerHour() const;
  double ToKnots() const;
  double ToBeaufort() const;
  double ToInchPerSecond() const;
  double ToYardPerSecond() const;
  double ToFurlongPerFortnight() const;

  double To(Unit speedUnit) const;
  std::string ToString(Unit speedUnit) const;

protected:
  CSpeed(double value);

  void SetValid(bool valid) { m_valid = valid; }

  double m_value; // we store in m/s
  bool m_valid;
};

