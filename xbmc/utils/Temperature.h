/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

#include "utils/IArchivable.h"

class CTemperature : public IArchivable
{
public:
  CTemperature();
  CTemperature(const CTemperature& temperature);

  typedef enum Unit
  {
    UnitFahrenheit = 0,
    UnitKelvin,
    UnitCelsius,
    UnitReaumur,
    UnitRankine,
    UnitRomer,
    UnitDelisle,
    UnitNewton
  } Unit;

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

  CTemperature& operator =(const CTemperature& right);
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

  void Archive(CArchive& ar) override;

  bool IsValid() const;
  void SetValid(bool valid) { m_valid = valid; }

  double ToFahrenheit() const;
  double ToKelvin() const;
  double ToCelsius() const;
  double ToReaumur() const;
  double ToRankine() const;
  double ToRomer() const;
  double ToDelisle() const;
  double ToNewton() const;

  double To(Unit temperatureUnit) const;
  std::string ToString(Unit temperatureUnit) const;

protected:
  explicit CTemperature(double value);

  double m_value; // we store as fahrenheit
  bool m_valid;
};

