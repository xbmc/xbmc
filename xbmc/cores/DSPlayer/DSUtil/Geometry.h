#pragma once

/*
 *      Copyright (C) 2005-2009 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#ifndef _GEOMETRYHELPER_H
#define _GEOMETRYHELPER_H

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))
#endif
namespace Com
{
class SmartSize;
class SmartPoint;
class SmartRect;
#include <windef.h>
class SmartSize : public tagSIZE
{
public:

// Constructors
  // construct an uninitialized size
  SmartSize() throw();
  // create from two integers
  SmartSize(int initCX, int initCY) throw();
  // create from another size
  SmartSize(SIZE initSize) throw();
  // create from a point
  SmartSize(POINT initPt) throw();
  // create from a DWORD: cx = LOWORD(dw) cy = HIWORD(dw)
  SmartSize(DWORD dwSize) throw();

// Operations
  BOOL operator==(SIZE size) const throw();
  BOOL operator!=(SIZE size) const throw();
  void operator+=(SIZE size) throw();
  void operator-=(SIZE size) throw();
  void SetSize(int CX, int CY) throw();

// Operators returning SmartSize values
  SmartSize operator+(SIZE size) const throw();
  SmartSize operator-(SIZE size) const throw();
  SmartSize operator-() const throw();

// Operators returning SmartPoint values
  SmartPoint operator+(POINT point) const throw();
  SmartPoint operator-(POINT point) const throw();

// Operators returning SmartRect values
  SmartRect operator+(const RECT* lpRect) const throw();
  SmartRect operator-(const RECT* lpRect) const throw();
};

/////////////////////////////////////////////////////////////////////////////
// SmartPoint - A 2-D point, similar to Windows POINT structure.

class SmartPoint : public tagPOINT
{
public:
// Constructors

  // create an uninitialized point
  SmartPoint() throw();
  // create from two integers
  SmartPoint(int initX, int initY) throw();
  // create from another point
  SmartPoint(POINT initPt) throw();
  // create from a size
  SmartPoint(SIZE initSize) throw();
  // create from an LPARAM: x = LOWORD(dw) y = HIWORD(dw)
  SmartPoint(LPARAM dwPoint) throw();


// Operations

// translate the point
  void Offset(int xOffset, int yOffset) throw();
  void Offset(POINT point) throw();
  void Offset(SIZE size) throw();
  void SetPoint(int X, int Y) throw();

  BOOL operator==(POINT point) const throw();
  BOOL operator!=(POINT point) const throw();
  void operator+=(SIZE size) throw();
  void operator-=(SIZE size) throw();
  void operator+=(POINT point) throw();
  void operator-=(POINT point) throw();

// Operators returning SmartPoint values
  SmartPoint operator+(SIZE size) const throw();
  SmartPoint operator-(SIZE size) const throw();
  SmartPoint operator-() const throw();
  SmartPoint operator+(POINT point) const throw();

// Operators returning SmartSize values
  SmartSize operator-(POINT point) const throw();

// Operators returning SmartRect values
  SmartRect operator+(const RECT* lpRect) const throw();
  SmartRect operator-(const RECT* lpRect) const throw();
};

/////////////////////////////////////////////////////////////////////////////
// SmartRect - A 2-D rectangle, similar to Windows RECT structure.

class SmartRect : public tagRECT
{
// Constructors
public:
  // uninitialized rectangle
  SmartRect() throw();
  // from left, top, right, and bottom
  SmartRect(int l, int t, int r, int b) throw();
  // copy constructor
  SmartRect(const RECT& srcRect) throw();
  // from a pointer to another rect
  SmartRect(LPCRECT lpSrcRect) throw();
  // from a point and size
  SmartRect(POINT point, SIZE size) throw();
  // from two points
  SmartRect(POINT topLeft, POINT bottomRight) throw();

// Attributes (in addition to RECT members)

  // retrieves the width
  int Width() const throw();
  // returns the height
  int Height() const throw();
  // returns the size
  SmartSize Size() const throw();
  // reference to the top-left point
  SmartPoint& TopLeft() throw();
  // reference to the bottom-right point
  SmartPoint& BottomRight() throw();
  // const reference to the top-left point
  const SmartPoint& TopLeft() const throw();
  // const reference to the bottom-right point
  const SmartPoint& BottomRight() const throw();
  // the geometric center point of the rectangle
  SmartPoint CenterPoint() const throw();
  // swap the left and right
  void SwapLeftRight() throw();
  static void WINAPI SwapLeftRight(LPRECT lpRect) throw();

  // convert between SmartRect and LPRECT/LPCRECT (no need for &)
  operator LPRECT() throw();
  operator LPCRECT() const throw();

  // returns TRUE if rectangle has no area
  BOOL IsRectEmpty() const throw();
  // returns TRUE if rectangle is at (0,0) and has no area
  BOOL IsRectNull() const throw();
  // returns TRUE if point is within rectangle
  BOOL PtInRect(POINT point) const throw();

// Operations

  // set rectangle from left, top, right, and bottom
  void SetRect(int x1, int y1, int x2, int y2) throw();
  void SetRect(POINT topLeft, POINT bottomRight) throw();
  // empty the rectangle
  void SetRectEmpty() throw();
  // copy from another rectangle
  void CopyRect(LPCRECT lpSrcRect) throw();
  // TRUE if exactly the same as another rectangle
  BOOL EqualRect(LPCRECT lpRect) const throw();

  // Inflate rectangle's width and height by
  // x units to the left and right ends of the rectangle
  // and y units to the top and bottom.
  void InflateRect(int x, int y) throw();
  // Inflate rectangle's width and height by
  // size.cx units to the left and right ends of the rectangle
  // and size.cy units to the top and bottom.
  void InflateRect(SIZE size) throw();
  // Inflate rectangle's width and height by moving individual sides.
  // Left side is moved to the left, right side is moved to the right,
  // top is moved up and bottom is moved down.
  void InflateRect(LPCRECT lpRect) throw();
  void InflateRect(int l, int t, int r, int b) throw();

  // deflate the rectangle's width and height without
  // moving its top or left
  void DeflateRect(int x, int y) throw();
  void DeflateRect(SIZE size) throw();
  void DeflateRect(LPCRECT lpRect) throw();
  void DeflateRect(int l, int t, int r, int b) throw();

  // translate the rectangle by moving its top and left
  void OffsetRect(int x, int y) throw();
  void OffsetRect(SIZE size) throw();
  void OffsetRect(POINT point) throw();
  void NormalizeRect() throw();

  // absolute position of rectangle
  void MoveToY(int y) throw();
  void MoveToX(int x) throw();
  void MoveToXY(int x, int y) throw();
  void MoveToXY(POINT point) throw();

  // set this rectangle to intersection of two others
  BOOL IntersectRect(LPCRECT lpRect1, LPCRECT lpRect2) throw();

  // set this rectangle to bounding union of two others
  BOOL UnionRect(LPCRECT lpRect1, LPCRECT lpRect2) throw();

  // set this rectangle to minimum of two others
  BOOL SubtractRect(LPCRECT lpRectSrc1, LPCRECT lpRectSrc2) throw();

// Additional Operations
  void operator=(const RECT& srcRect) throw();
  BOOL operator==(const RECT& rect) const throw();
  BOOL operator!=(const RECT& rect) const throw();
  void operator+=(POINT point) throw();
  void operator+=(SIZE size) throw();
  void operator+=(LPCRECT lpRect) throw();
  void operator-=(POINT point) throw();
  void operator-=(SIZE size) throw();
  void operator-=(LPCRECT lpRect) throw();
  void operator&=(const RECT& rect) throw();
  void operator|=(const RECT& rect) throw();

// Operators returning SmartRect values
  SmartRect operator+(POINT point) const throw();
  SmartRect operator-(POINT point) const throw();
  SmartRect operator+(LPCRECT lpRect) const throw();
  SmartRect operator+(SIZE size) const throw();
  SmartRect operator-(SIZE size) const throw();
  SmartRect operator-(LPCRECT lpRect) const throw();
  SmartRect operator&(const RECT& rect2) const throw();
  SmartRect operator|(const RECT& rect2) const throw();
  SmartRect MulDiv(int nMultiplier, int nDivisor) const throw();
};

}
#endif
