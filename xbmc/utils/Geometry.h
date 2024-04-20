/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#ifdef __GNUC__
// under gcc, inline will only take place if optimizations are applied (-O). this will force inline even with optimizations.
#define XBMC_FORCE_INLINE __attribute__((always_inline))
#else
#define XBMC_FORCE_INLINE
#endif

#include <algorithm>
#include <stdexcept>
#include <vector>

template <typename T> class CPointGen
{
public:
  typedef CPointGen<T> this_type;

  CPointGen() noexcept = default;

  constexpr CPointGen(T a, T b)
  : x{a}, y{b}
  {}

  template<class U> explicit constexpr CPointGen(const CPointGen<U>& rhs)
  : x{static_cast<T> (rhs.x)}, y{static_cast<T> (rhs.y)}
  {}

  constexpr this_type operator+(const this_type &point) const
  {
    return {x + point.x, y + point.y};
  };

  this_type& operator+=(const this_type &point)
  {
    x += point.x;
    y += point.y;
    return *this;
  };

  constexpr this_type operator-(const this_type &point) const
  {
    return {x - point.x, y - point.y};
  };

  this_type& operator-=(const this_type &point)
  {
    x -= point.x;
    y -= point.y;
    return *this;
  };

  constexpr this_type operator*(T factor) const
  {
    return {x * factor, y * factor};
  }

  this_type& operator*=(T factor)
  {
    x *= factor;
    y *= factor;
    return *this;
  }

  constexpr this_type operator/(T factor) const
  {
    return {x / factor, y / factor};
  }

  this_type& operator/=(T factor)
  {
    x /= factor;
    y /= factor;
    return *this;
  }

  T x{}, y{};
};

template<typename T>
constexpr bool operator==(const CPointGen<T> &point1, const CPointGen<T> &point2) noexcept
{
  return (point1.x == point2.x && point1.y == point2.y);
}

template<typename T>
constexpr bool operator!=(const CPointGen<T> &point1, const CPointGen<T> &point2) noexcept
{
  return !(point1 == point2);
}

using CPoint = CPointGen<float>;
using CPointInt = CPointGen<int>;


/**
 * Generic two-dimensional size representation
 *
 * Class invariant: width and height are both non-negative
 * Throws std::out_of_range if invariant would be violated. The class
 * is exception-safe. If modification would violate the invariant, the size
 * is not changed.
 */
template <typename T> class CSizeGen
{
  T m_w{}, m_h{};

  void CheckSet(T width, T height)
  {
    if (width < 0)
    {
      throw std::out_of_range("Size may not have negative width");
    }
    if (height < 0)
    {
      throw std::out_of_range("Size may not have negative height");
    }
    m_w = width;
    m_h = height;
  }

public:
  typedef CSizeGen<T> this_type;

  CSizeGen() noexcept = default;

  CSizeGen(T width, T height)
  {
    CheckSet(width, height);
  }

  T Width() const
  {
    return m_w;
  }

  T Height() const
  {
    return m_h;
  }

  void SetWidth(T width)
  {
    CheckSet(width, m_h);
  }

  void SetHeight(T height)
  {
    CheckSet(m_w, height);
  }

  void Set(T width, T height)
  {
    CheckSet(width, height);
  }

  bool IsZero() const
  {
    return (m_w == static_cast<T> (0) && m_h == static_cast<T> (0));
  }

  T Area() const
  {
    return m_w * m_h;
  }

  CPointGen<T> ToPoint() const
  {
    return {m_w, m_h};
  }

  template<class U> explicit CSizeGen<T>(const CSizeGen<U>& rhs)
  {
    CheckSet(static_cast<T> (rhs.m_w), static_cast<T> (rhs.m_h));
  }

  this_type operator+(const this_type& size) const
  {
    return {m_w + size.m_w, m_h + size.m_h};
  };

  this_type& operator+=(const this_type& size)
  {
    CheckSet(m_w + size.m_w, m_h + size.m_h);
    return *this;
  };

  this_type operator-(const this_type& size) const
  {
    return {m_w - size.m_w, m_h - size.m_h};
  };

  this_type& operator-=(const this_type& size)
  {
    CheckSet(m_w - size.m_w, m_h - size.m_h);
    return *this;
  };

  this_type operator*(T factor) const
  {
    return {m_w * factor, m_h * factor};
  }

  this_type& operator*=(T factor)
  {
    CheckSet(m_w * factor, m_h * factor);
    return *this;
  }

  this_type operator/(T factor) const
  {
    return {m_w / factor, m_h / factor};
  }

  this_type& operator/=(T factor)
  {
    CheckSet(m_w / factor, m_h / factor);
    return *this;
  }
};

template<typename T>
inline bool operator==(const CSizeGen<T>& size1, const CSizeGen<T>& size2) noexcept
{
  return (size1.Width() == size2.Width() && size1.Height() == size2.Height());
}

template<typename T>
inline bool operator!=(const CSizeGen<T>& size1, const CSizeGen<T>& size2) noexcept
{
  return !(size1 == size2);
}

using CSize = CSizeGen<float>;
using CSizeInt = CSizeGen<int>;


template <typename T> class CRectGen
{
public:
  typedef CRectGen<T> this_type;
  typedef CPointGen<T> point_type;
  typedef CSizeGen<T> size_type;

  CRectGen() noexcept = default;

  constexpr CRectGen(T left, T top, T right, T bottom)
  : x1{left}, y1{top}, x2{right}, y2{bottom}
  {}

  constexpr CRectGen(const point_type &p1, const point_type &p2)
  : x1{p1.x}, y1{p1.y}, x2{p2.x}, y2{p2.y}
  {}

  constexpr CRectGen(const point_type &origin, const size_type &size)
  : x1{origin.x}, y1{origin.y}, x2{x1 + size.Width()}, y2{y1 + size.Height()}
  {}

  template<class U> explicit constexpr CRectGen(const CRectGen<U>& rhs)
  : x1{static_cast<T> (rhs.x1)}, y1{static_cast<T> (rhs.y1)}, x2{static_cast<T> (rhs.x2)}, y2{static_cast<T> (rhs.y2)}
  {}

  void SetRect(T left, T top, T right, T bottom)
  {
    x1 = left;
    y1 = top;
    x2 = right;
    y2 = bottom;
  }

  constexpr bool PtInRect(const point_type &point) const
  {
    return (x1 <= point.x && point.x <= x2 && y1 <= point.y && point.y <= y2);
  };

  constexpr bool Intersects(const this_type& rect) const
  {
    return (x1 < rect.x2 && x2 > rect.x1 && y1 < rect.y2 && y2 > rect.y1);
  };

  this_type& operator-=(const point_type &point) XBMC_FORCE_INLINE
  {
    x1 -= point.x;
    y1 -= point.y;
    x2 -= point.x;
    y2 -= point.y;
    return *this;
  };

  constexpr this_type operator-(const point_type &point) const
  {
    return {x1 - point.x, y1 - point.y, x2 - point.x, y2 - point.y};
  }

  this_type& operator+=(const point_type &point) XBMC_FORCE_INLINE
  {
    x1 += point.x;
    y1 += point.y;
    x2 += point.x;
    y2 += point.y;
    return *this;
  };

  constexpr this_type operator+(const point_type &point) const
  {
    return {x1 + point.x, y1 + point.y, x2 + point.x, y2 + point.y};
  }

  this_type& operator-=(const size_type &size)
  {
    x2 -= size.Width();
    y2 -= size.Height();
    return *this;
  };

  constexpr this_type operator-(const size_type &size) const
  {
    return {x1, y1, x2 - size.Width(), y2 - size.Height()};
  }

  this_type& operator+=(const size_type &size)
  {
    x2 += size.Width();
    y2 += size.Height();
    return *this;
  };

  constexpr this_type operator+(const size_type &size) const
  {
    return {x1, y1, x2 + size.Width(), y2 + size.Height()};
  }

  this_type& Intersect(const this_type &rect)
  {
    x1 = clamp_range(x1, rect.x1, rect.x2);
    x2 = clamp_range(x2, rect.x1, rect.x2);
    y1 = clamp_range(y1, rect.y1, rect.y2);
    y2 = clamp_range(y2, rect.y1, rect.y2);
    return *this;
  };

  this_type& Union(const this_type &rect)
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

  constexpr bool IsEmpty() const XBMC_FORCE_INLINE
  {
    return (x2 - x1) * (y2 - y1) == 0;
  };

  constexpr point_type P1() const XBMC_FORCE_INLINE
  {
    return {x1, y1};
  }

  constexpr point_type P2() const XBMC_FORCE_INLINE
  {
    return {x2, y2};
  }

  constexpr T Width() const XBMC_FORCE_INLINE
  {
    return x2 - x1;
  };

  constexpr T Height() const XBMC_FORCE_INLINE
  {
    return y2 - y1;
  };

  constexpr T Area() const XBMC_FORCE_INLINE
  {
    return Width() * Height();
  };

  size_type ToSize() const
  {
    return {Width(), Height()};
  };

  std::vector<this_type> SubtractRect(this_type splitterRect)
  {
    std::vector<this_type> newRectanglesList;
    this_type intersection = splitterRect.Intersect(*this);

    if (!intersection.IsEmpty())
    {
      this_type add;

      // add rect above intersection if not empty
      add = this_type(x1, y1, x2, intersection.y1);
      if (!add.IsEmpty())
        newRectanglesList.push_back(add);

      // add rect below intersection if not empty
      add = this_type(x1, intersection.y2, x2, y2);
      if (!add.IsEmpty())
        newRectanglesList.push_back(add);

      // add rect left intersection if not empty
      add = this_type(x1, intersection.y1, intersection.x1, intersection.y2);
      if (!add.IsEmpty())
        newRectanglesList.push_back(add);

      // add rect right intersection if not empty
      add = this_type(intersection.x2, intersection.y1, x2, intersection.y2);
      if (!add.IsEmpty())
        newRectanglesList.push_back(add);
    }
    else
    {
      newRectanglesList.push_back(*this);
    }

    return newRectanglesList;
  }

  std::vector<this_type> SubtractRects(const std::vector<this_type>& intersectionList)
  {
    std::vector<this_type> fragmentsList;
    fragmentsList.push_back(*this);

    for (typename std::vector<this_type>::iterator splitter = intersectionList.begin(); splitter != intersectionList.end(); ++splitter)
    {
      typename std::vector<this_type> toAddList;

      for (typename std::vector<this_type>::iterator fragment = fragmentsList.begin(); fragment != fragmentsList.end(); ++fragment)
      {
        std::vector<this_type> newFragmentsList = fragment->SubtractRect(*splitter);
        toAddList.insert(toAddList.end(), newFragmentsList.begin(), newFragmentsList.end());
      }

      fragmentsList.clear();
      fragmentsList.insert(fragmentsList.end(), toAddList.begin(), toAddList.end());
    }

    return fragmentsList;
  }

  void GetQuad(point_type (&points)[4])
  {
    points[0] = { x1, y1 };
    points[1] = { x2, y1 };
    points[2] = { x2, y2 };
    points[3] = { x1, y2 };
  }

  T x1{}, y1{}, x2{}, y2{};
private:
  static constexpr T clamp_range(T x, T l, T h) XBMC_FORCE_INLINE
  {
    return (x > h) ? h : ((x < l) ? l : x);
  }
};

template<typename T>
constexpr bool operator==(const CRectGen<T> &rect1, const CRectGen<T> &rect2) noexcept
{
  return (rect1.x1 == rect2.x1 && rect1.y1 == rect2.y1 && rect1.x2 == rect2.x2 && rect1.y2 == rect2.y2);
}

template<typename T>
constexpr bool operator!=(const CRectGen<T> &rect1, const CRectGen<T> &rect2) noexcept
{
  return !(rect1 == rect2);
}

using CRect = CRectGen<float>;
using CRectInt = CRectGen<int>;
