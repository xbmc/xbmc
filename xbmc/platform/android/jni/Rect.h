#pragma once
/*
 *      Copyright (C) 2016 Christian Browet
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

#include "JNIBase.h"

class CJNIRect: public CJNIBase
{
public:
  CJNIRect(const jni::jhobject &object) : CJNIBase(object){}
  ~CJNIRect(){}

  CJNIRect();
  CJNIRect(int l, int t, int r, int b);

  // Fields
  int getLeft();
  int getTop();
  int getRight();
  int getBottom();

  int width();
  int height();

  bool        equals(const CJNIRect &other);
  std::string toString()    const;
  int         describeContents() const;

};
