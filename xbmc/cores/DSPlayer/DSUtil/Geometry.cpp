#include "stdafx.h"
#include "Geometry.h"

// SmartSize
using namespace Com;
SmartSize::SmartSize() throw()
{ /* random filled */ }
SmartSize::SmartSize(int initCX, int initCY) throw()
{ cx = initCX; cy = initCY; }
SmartSize::SmartSize(SIZE initSize) throw()
{ *(SIZE*)this = initSize; }
SmartSize::SmartSize(POINT initPt) throw()
{ *(POINT*)this = initPt; }
SmartSize::SmartSize(DWORD dwSize) throw()
{
    cx = (short)LOWORD(dwSize);
    cy = (short)HIWORD(dwSize);
}
BOOL SmartSize::operator==(SIZE size) const throw()
{ return (cx == size.cx && cy == size.cy); }
BOOL SmartSize::operator!=(SIZE size) const throw()
{ return (cx != size.cx || cy != size.cy); }
void SmartSize::operator+=(SIZE size) throw()
{ cx += size.cx; cy += size.cy; }
void SmartSize::operator-=(SIZE size) throw()
{ cx -= size.cx; cy -= size.cy; }
void SmartSize::SetSize(int CX, int CY) throw()
{ cx = CX; cy = CY; }  
SmartSize SmartSize::operator+(SIZE size) const throw()
{ return SmartSize(cx + size.cx, cy + size.cy); }
SmartSize SmartSize::operator-(SIZE size) const throw()
{ return SmartSize(cx - size.cx, cy - size.cy); }
SmartSize SmartSize::operator-() const throw()
{ return SmartSize(-cx, -cy); }
SmartPoint SmartSize::operator+(POINT point) const throw()
{ return SmartPoint(cx + point.x, cy + point.y); }
SmartPoint SmartSize::operator-(POINT point) const throw()
{ return SmartPoint(cx - point.x, cy - point.y); }
SmartRect SmartSize::operator+(const RECT* lpRect) const throw()
{ return SmartRect(lpRect) + *this; }
SmartRect SmartSize::operator-(const RECT* lpRect) const throw()
{ return SmartRect(lpRect) - *this; }

// SmartPoint
SmartPoint::SmartPoint() throw()
{ /* random filled */ }
SmartPoint::SmartPoint(int initX, int initY) throw()
{ x = initX; y = initY; }
SmartPoint::SmartPoint(POINT initPt) throw()
{ *(POINT*)this = initPt; }
SmartPoint::SmartPoint(SIZE initSize) throw()
{ *(SIZE*)this = initSize; }
SmartPoint::SmartPoint(LPARAM dwPoint) throw()
{
    x = (short)GET_X_LPARAM(dwPoint);
    y = (short)GET_Y_LPARAM(dwPoint);
}
void SmartPoint::Offset(int xOffset, int yOffset) throw()
{ x += xOffset; y += yOffset; }
void SmartPoint::Offset(POINT point) throw()
{ x += point.x; y += point.y; }
void SmartPoint::Offset(SIZE size) throw()
{ x += size.cx; y += size.cy; }
void SmartPoint::SetPoint(int X, int Y) throw()
{ x = X; y = Y; }
BOOL SmartPoint::operator==(POINT point) const throw()
{ return (x == point.x && y == point.y); }
BOOL SmartPoint::operator!=(POINT point) const throw()
{ return (x != point.x || y != point.y); }
void SmartPoint::operator+=(SIZE size) throw()
{ x += size.cx; y += size.cy; }
void SmartPoint::operator-=(SIZE size) throw()
{ x -= size.cx; y -= size.cy; }
void SmartPoint::operator+=(POINT point) throw()
{ x += point.x; y += point.y; }
void SmartPoint::operator-=(POINT point) throw()
{ x -= point.x; y -= point.y; }
SmartPoint SmartPoint::operator+(SIZE size) const throw()
{ return SmartPoint(x + size.cx, y + size.cy); }
SmartPoint SmartPoint::operator-(SIZE size) const throw()
{ return SmartPoint(x - size.cx, y - size.cy); }
SmartPoint SmartPoint::operator-() const throw()
{ return SmartPoint(-x, -y); }
SmartPoint SmartPoint::operator+(POINT point) const throw()
{ return SmartPoint(x + point.x, y + point.y); }
SmartSize SmartPoint::operator-(POINT point) const throw()
{ return SmartSize(x - point.x, y - point.y); }
SmartRect SmartPoint::operator+(const RECT* lpRect) const throw()
{ return SmartRect(lpRect) + *this; }
SmartRect SmartPoint::operator-(const RECT* lpRect) const throw()
{ return SmartRect(lpRect) - *this; }

// SmartRect
SmartRect::SmartRect() throw()
{ /* random filled */ }
SmartRect::SmartRect(int l, int t, int r, int b) throw()
{ left = l; top = t; right = r; bottom = b; }
SmartRect::SmartRect(const RECT& srcRect) throw()
{ ::CopyRect(this, &srcRect); }
SmartRect::SmartRect(LPCRECT lpSrcRect) throw()
{ ::CopyRect(this, lpSrcRect); }
SmartRect::SmartRect(POINT point, SIZE size) throw()
{ right = (left = point.x) + size.cx; bottom = (top = point.y) + size.cy; }
SmartRect::SmartRect(POINT topLeft, POINT bottomRight) throw()
{ left = topLeft.x; top = topLeft.y;
    right = bottomRight.x; bottom = bottomRight.y; }
int SmartRect::Width() const throw()
{ return right - left; }
int SmartRect::Height() const throw()
{ return bottom - top; }
SmartSize SmartRect::Size() const throw()
{ return SmartSize(right - left, bottom - top); }
SmartPoint& SmartRect::TopLeft() throw()
{ return *((SmartPoint*)this); }
SmartPoint& SmartRect::BottomRight() throw()
{ return *((SmartPoint*)this+1); }
const SmartPoint& SmartRect::TopLeft() const throw()
{ return *((SmartPoint*)this); }
const SmartPoint& SmartRect::BottomRight() const throw()
{ return *((SmartPoint*)this+1); }
SmartPoint SmartRect::CenterPoint() const throw()
{ return SmartPoint((left+right)/2, (top+bottom)/2); }
void SmartRect::SwapLeftRight() throw()
{ SwapLeftRight(LPRECT(this)); }
void WINAPI SmartRect::SwapLeftRight(LPRECT lpRect) throw()
{ LONG temp = lpRect->left; lpRect->left = lpRect->right; lpRect->right = temp; }
SmartRect::operator LPRECT() throw()
{ return this; }
SmartRect::operator LPCRECT() const throw()
{ return this; }
BOOL SmartRect::IsRectEmpty() const throw()
{ return ::IsRectEmpty(this); }
BOOL SmartRect::IsRectNull() const throw()
{ return (left == 0 && right == 0 && top == 0 && bottom == 0); }
BOOL SmartRect::PtInRect(POINT point) const throw()
{ return ::PtInRect(this, point); }
void SmartRect::SetRect(int x1, int y1, int x2, int y2) throw()
{ ::SetRect(this, x1, y1, x2, y2); }
void SmartRect::SetRect(POINT topLeft, POINT bottomRight) throw()
{ ::SetRect(this, topLeft.x, topLeft.y, bottomRight.x, bottomRight.y); }
void SmartRect::SetRectEmpty() throw()
{ ::SetRectEmpty(this); }
void SmartRect::CopyRect(LPCRECT lpSrcRect) throw()
{ ::CopyRect(this, lpSrcRect); }
BOOL SmartRect::EqualRect(LPCRECT lpRect) const throw()
{ return ::EqualRect(this, lpRect); }
void SmartRect::InflateRect(int x, int y) throw()
{ ::InflateRect(this, x, y); }
void SmartRect::InflateRect(SIZE size) throw()
{ ::InflateRect(this, size.cx, size.cy); }
void SmartRect::DeflateRect(int x, int y) throw()
{ ::InflateRect(this, -x, -y); }
void SmartRect::DeflateRect(SIZE size) throw()
{ ::InflateRect(this, -size.cx, -size.cy); }
void SmartRect::OffsetRect(int x, int y) throw()
{ ::OffsetRect(this, x, y); }
void SmartRect::OffsetRect(POINT point) throw()
{ ::OffsetRect(this, point.x, point.y); }
void SmartRect::OffsetRect(SIZE size) throw()
{ ::OffsetRect(this, size.cx, size.cy); }
void SmartRect::MoveToY(int y) throw()
{ bottom = Height() + y; top = y; }
void SmartRect::MoveToX(int x) throw()
{ right = Width() + x; left = x; }
void SmartRect::MoveToXY(int x, int y) throw()
{ MoveToX(x); MoveToY(y); }
void SmartRect::MoveToXY(POINT pt) throw()
{ MoveToX(pt.x); MoveToY(pt.y); }
BOOL SmartRect::IntersectRect(LPCRECT lpRect1, LPCRECT lpRect2) throw()
{ return ::IntersectRect(this, lpRect1, lpRect2);}
BOOL SmartRect::UnionRect(LPCRECT lpRect1, LPCRECT lpRect2) throw()
{ return ::UnionRect(this, lpRect1, lpRect2); }
void SmartRect::operator=(const RECT& srcRect) throw()
{ ::CopyRect(this, &srcRect); }
BOOL SmartRect::operator==(const RECT& rect) const throw()
{ return ::EqualRect(this, &rect); }
BOOL SmartRect::operator!=(const RECT& rect) const throw()
{ return !::EqualRect(this, &rect); }
void SmartRect::operator+=(POINT point) throw()
{ ::OffsetRect(this, point.x, point.y); }
void SmartRect::operator+=(SIZE size) throw()
{ ::OffsetRect(this, size.cx, size.cy); }
void SmartRect::operator+=(LPCRECT lpRect) throw()
{ InflateRect(lpRect); }
void SmartRect::operator-=(POINT point) throw()
{ ::OffsetRect(this, -point.x, -point.y); }
void SmartRect::operator-=(SIZE size) throw()
{ ::OffsetRect(this, -size.cx, -size.cy); }
void SmartRect::operator-=(LPCRECT lpRect) throw()
{ DeflateRect(lpRect); }
void SmartRect::operator&=(const RECT& rect) throw()
{ ::IntersectRect(this, this, &rect); }
void SmartRect::operator|=(const RECT& rect) throw()
{ ::UnionRect(this, this, &rect); }
SmartRect SmartRect::operator+(POINT pt) const throw()
{ SmartRect rect(*this); ::OffsetRect(&rect, pt.x, pt.y); return rect; }
SmartRect SmartRect::operator-(POINT pt) const throw()
{ SmartRect rect(*this); ::OffsetRect(&rect, -pt.x, -pt.y); return rect; }
SmartRect SmartRect::operator+(SIZE size) const throw()
{ SmartRect rect(*this); ::OffsetRect(&rect, size.cx, size.cy); return rect; }
SmartRect SmartRect::operator-(SIZE size) const throw()
{ SmartRect rect(*this); ::OffsetRect(&rect, -size.cx, -size.cy); return rect; }
SmartRect SmartRect::operator+(LPCRECT lpRect) const throw()
{ SmartRect rect(this); rect.InflateRect(lpRect); return rect; }
SmartRect SmartRect::operator-(LPCRECT lpRect) const throw()
{ SmartRect rect(this); rect.DeflateRect(lpRect); return rect; }
SmartRect SmartRect::operator&(const RECT& rect2) const throw()
{ SmartRect rect; ::IntersectRect(&rect, this, &rect2);
    return rect; }
SmartRect SmartRect::operator|(const RECT& rect2) const throw()
{ SmartRect rect; ::UnionRect(&rect, this, &rect2);
    return rect; }
BOOL SmartRect::SubtractRect(LPCRECT lpRectSrc1, LPCRECT lpRectSrc2) throw()
{ return ::SubtractRect(this, lpRectSrc1, lpRectSrc2); }

void SmartRect::NormalizeRect() throw()
{
    int nTemp;
    if (left > right)
  {
      nTemp = left;
      left = right;
      right = nTemp;
  }
    if (top > bottom)
  {
      nTemp = top;
      top = bottom;
      bottom = nTemp;
  }
}

void SmartRect::InflateRect(LPCRECT lpRect) throw()
{
    left -= lpRect->left;    top -= lpRect->top;
    right += lpRect->right;    bottom += lpRect->bottom;
}

void SmartRect::InflateRect(int l, int t, int r, int b) throw()
{
    left -= l;      top -= t;
    right += r;      bottom += b;
}

void SmartRect::DeflateRect(LPCRECT lpRect) throw()
{
    left += lpRect->left;  top += lpRect->top;
    right -= lpRect->right;  bottom -= lpRect->bottom;
}

void SmartRect::DeflateRect(int l, int t, int r, int b) throw()
{
    left += l;    top += t;
    right -= r;    bottom -= b;
}

SmartRect SmartRect::MulDiv(int nMultiplier, int nDivisor) const throw()
{
    return SmartRect(
      ::MulDiv(left, nMultiplier, nDivisor),
      ::MulDiv(top, nMultiplier, nDivisor),
      ::MulDiv(right, nMultiplier, nDivisor),
      ::MulDiv(bottom, nMultiplier, nDivisor));
}
