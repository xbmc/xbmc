#pragma once

class CPoint
{
public:
  CPoint()
  {
    x = 0; y = 0;
  };

  CPoint(float a, float b)
  {
    x = a;
    y = b;
  };

  CPoint operator+(const CPoint &point) const
  {
    CPoint ans;
    ans.x = x + point.x;
    ans.y = y + point.y;
    return ans;
  };

  const CPoint &operator+=(const CPoint &point)
  {
    x += point.x;
    y += point.y;
    return *this;
  };

  CPoint operator-(const CPoint &point) const
  {
    CPoint ans;
    ans.x = x - point.x;
    ans.y = y - point.y;
    return ans;
  };

  const CPoint &operator-=(const CPoint &point)
  {
    x -= point.x;
    y -= point.y;
    return *this;
  };

  float x, y;
};

class CRect
{
public:
  CRect() { x1 = y1 = x2 = y2 = 0;};
  CRect(float left, float top, float right, float bottom) { x1 = left; y1 = top; x2 = right; y2 = bottom; };

  void SetRect(float left, float top, float right, float bottom) { x1 = left; y1 = top; x2 = right; y2 = bottom; };

  bool PtInRect(const CPoint &point) const
  {
    if (x1 <= point.x && point.x <= x2 && y1 <= point.y && point.y <= y2)
      return true;
    return false;
  };

  inline const CRect &operator -=(const CPoint &point)
  {
    x1 -= point.x;
    y1 -= point.y;
    x2 -= point.x;
    y2 -= point.y;
    return *this;
  };

  inline const CRect &operator +=(const CPoint &point)
  {
    x1 += point.x;
    y1 += point.y;
    x2 += point.x;
    y2 += point.y;
    return *this;
  };

  const CRect &Intersect(const CRect &rect)
  { 
    if (rect.x2 < x2) x2 = rect.x2;
    if (rect.y2 < y2) y2 = rect.y2;
    if (rect.x1 > x1) x1 = rect.x1;
    if (rect.y1 > y1) y1 = rect.y1;
    if (x1 > x2) x1 = x2;
    if (y1 > y2) y1 = y2;
    return *this;
  };

  inline bool IsEmpty() const
  {
    return (x2 - x1) * (y2 - y1) == 0;
  };

  inline float Width() const
  {
    return x2 - x1;
  };

  inline float Height() const
  {
    return y2 - y1;
  };

  bool operator !=(const CRect &rect) const
  {
    if (x1 != rect.x1) return true;
    if (x2 != rect.x2) return true;
    if (y1 != rect.y1) return true;
    if (y2 != rect.y2) return true;
    return false;
  };

  float x1, y1, x2, y2;
};
