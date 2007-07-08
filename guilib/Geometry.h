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

  CPoint operator-(const CPoint &point) const
  {
    CPoint ans;
    ans.x = x - point.x;
    ans.y = y - point.y;
    return ans;
  };

  float x, y;
};

class CRect
{
public:
  CRect() { x = y = w = h = 0;};
  CRect(float left, float top, float width, float height) { x = left; y = top; w = width; h = height; };

  void SetRect(float left, float top, float width, float height) { x = left; y = top; w = width; h = height; };

  bool PtInRect(const CPoint &point) const
  {
    if (x <= point.x && point.x <= x + w && y <= point.y && point.y <= y + h)
      return true;
    return false;
  };

  const CRect &operator -=(const CPoint &point)
  {
    x -= point.x;
    y -= point.y;
    return *this;
  };

  void Intersect(const CRect &rect)
  { 
    float x2 = min(x + w, rect.x + rect.w);
    float y2 = min(y + h, rect.y + rect.h);
    if (rect.x > x) x = rect.x;
    if (rect.y > y) y = rect.y;
    w = max(x2 - x, 0);
    h = max(y2 - y, 0);
  };

  bool IsEmpty() const
  {
    return w * h == 0;
  };

  float x, y, w, h;
};
