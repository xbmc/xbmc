#pragma once

class CTemperature : public ISerializable
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

  virtual void Serialize(CArchive& ar);

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

  CStdString ToString() const;

protected:
  CTemperature(double value);

protected:
  double m_value; // we store as fahrenheit
  STATE m_state;
};

