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

#pragma once

#ifdef __GNUC__
// under gcc, inline will only take place if optimizations are applied (-O). this will force inline even whith optimizations.
#define XBMC_FORCE_INLINE __attribute__((always_inline))
#else
#define XBMC_FORCE_INLINE
#endif

#include <vector>
#include <algorithm>

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

  inline const CRect &operator -=(const CPoint &point) XBMC_FORCE_INLINE
  {
    x1 -= point.x;
    y1 -= point.y;
    x2 -= point.x;
    y2 -= point.y;
    return *this;
  };

  inline const CRect &operator +=(const CPoint &point) XBMC_FORCE_INLINE
  {
    x1 += point.x;
    y1 += point.y;
    x2 += point.x;
    y2 += point.y;
    return *this;
  };

  const CRect &Intersect(const CRect &rect)
  {
    x1 = clamp_range(x1, rect.x1, rect.x2);
    x2 = clamp_range(x2, rect.x1, rect.x2);
    y1 = clamp_range(y1, rect.y1, rect.y2);
    y2 = clamp_range(y2, rect.y1, rect.y2);
    return *this;
  };

  const CRect &Union(const CRect &rect)
  {
    if (IsEmpty())
      *this = rect;
    else if (!rect.IsEmpty())
    {
      x1 = std::min(x1,rect.x1);
      y1 = std::min(y1,rect.y1);

      x2 = std::max(x2,rect.x2);
      y2 = std::max(y2,rect.y2);
    }

    return *this;
  };

  inline bool IsEmpty() const XBMC_FORCE_INLINE
  {
    return (x2 - x1) * (y2 - y1) == 0;
  };

  inline float Width() const XBMC_FORCE_INLINE
  {
    return x2 - x1;
  };

  inline float Height() const XBMC_FORCE_INLINE
  {
    return y2 - y1;
  };

  inline float Area() const XBMC_FORCE_INLINE
  {
    return Width() * Height();
  };

  std::vector<CRect> SubtractRect(CRect splitterRect)
  {
    std::vector<CRect> newRectaglesList;
    CRect intersection = splitterRect.Intersect(*this);

    if (!intersection.IsEmpty())
    {
      CRect add;

      // add rect above intersection if not empty
      add = CRect(x1, y1, x2, intersection.y1);
      if (!add.IsEmpty())
        newRectaglesList.push_back(add);

      // add rect below intersection if not empty
      add = CRect(x1, intersection.y2, x2, y2);
      if (!add.IsEmpty())
        newRectaglesList.push_back(add);

      // add rect left intersection if not empty
      add = CRect(x1, intersection.y1, intersection.x1, intersection.y2);
      if (!add.IsEmpty())
        newRectaglesList.push_back(add);

      // add rect right intersection if not empty
      add = CRect(intersection.x2, intersection.y1, x2, intersection.y2);
      if (!add.IsEmpty())
        newRectaglesList.push_back(add);
    }
    else
    {
      newRectaglesList.push_back(*this);
    }

    return newRectaglesList;
  }

  std::vector<CRect> SubtractRects(std::vector<CRect> intersectionList)
  {
    std::vector<CRect> fragmentsList;
    fragmentsList.push_back(*this);

    for (std::vector<CRect>::iterator splitter = intersectionList.begin(); splitter != intersectionList.end(); ++splitter)
    {
      std::vector<CRect> toAddList;

      for (std::vector<CRect>::iterator fragment = fragmentsList.begin(); fragment != fragmentsList.end(); ++fragment)
      {
        std::vector<CRect> newFragmentsList = fragment->SubtractRect(*splitter);
        toAddList.insert(toAddList.end(), newFragmentsList.begin(), newFragmentsList.end());
      }

      fragmentsList.clear();
      fragmentsList.insert(fragmentsList.end(), toAddList.begin(), toAddList.end());
    }

    return fragmentsList;
  }

  bool operator !=(const CRect &rect) const
  {
    if (x1 != rect.x1) return true;
    if (x2 != rect.x2) return true;
    if (y1 != rect.y1) return true;
    if (y2 != rect.y2) return true;
    return false;
  };

  float x1, y1, x2, y2;
private:
  inline static float clamp_range(float x, float l, float h) XBMC_FORCE_INLINE
  {
    return (x > h) ? h : ((x < l) ? l : x);
  }
};

