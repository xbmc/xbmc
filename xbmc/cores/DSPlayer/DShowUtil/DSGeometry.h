#pragma once
/////////////////////////////////////////////////////////////////////////////
class GeometryHelper
{
public:
  static tagRECT CreateRect(int left,int top, int right, int bottom)
  {
    tagRECT p_rect;
	p_rect.left = left;
	p_rect.top = top;
	p_rect.bottom = bottom;
    p_rect.right = right;
	return p_rect;
  }

  static tagRECT CreateRect(tagPOINT p_point, tagSIZE p_size)
  {
    tagRECT p_rect;
	p_rect.left = p_point.x;
	p_rect.right = p_size.cx + p_rect.left;
    p_rect.top = p_point.y;
	p_rect.bottom = p_point.y + p_size.cy;
	return p_rect;
  }

  static int GetWidth(tagRECT p_rect)
  {
	return p_rect.right-p_rect.left;
  }
  static int GetHeight(tagRECT p_rect)
  {
	return p_rect.bottom-p_rect.top;
  }

  static tagPOINT CreatePoint(int x, int y)
  {
    tagPOINT p_point;
	p_point.x = x; p_point.y = y;
    return p_point;
  }

  static tagSIZE CreateSize(int width,int height)
  {
    tagSIZE p_size;
	p_size.cx = width;
    p_size.cy = height;
    return p_size;
  }
  
  static tagSIZE GetSize(tagRECT p_rect)
  {
    tagSIZE p_size;
	p_size.cx = p_rect.right-p_rect.left;
	p_size.cy = p_rect.bottom-p_rect.top;
	return p_size;
  }

  static bool SizeChanged(tagRECT p_rect1,tagRECT p_rect2)
  {
    tagSIZE p_size1,p_size2;
	p_size1 = GetSize(p_rect1); p_size2 = GetSize(p_rect2);
    if ((p_size1.cx != p_size2.cx) || (p_size2.cy != p_size2.cy))
      return false;
    return true;
  }

  static bool PosChanged(tagRECT p_rect1,RECT p_rect2)
  {
    if ((p_rect1.top != p_rect2.top) || (p_rect1.left != p_rect2.left) || (p_rect1.right != p_rect2.right) || (p_rect1.bottom != p_rect2.bottom))
      return true;
    return false;
  }
  
  static bool RectChanged(tagRECT p_rect1,tagRECT p_rect2)
  {
    if ( ( GetWidth(p_rect1) != GetWidth(p_rect2) ) || ( GetHeight(p_rect1) != GetHeight(p_rect2) ) )
      return true;
    return false;
  }

  static tagPOINT GetCenterPoint(tagRECT p_rect)
  {
    int width,height;
    width = GetWidth(p_rect); height = GetHeight(p_rect);
    if (width > 0)
      width = width / 2;
    if (height > 0)
      height = height / 2;
    tagPOINT p_point;
	p_point.x = width;
	p_point.y = height;
    return p_point;
  
  }
};