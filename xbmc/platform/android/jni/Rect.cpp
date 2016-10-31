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

#include "Rect.h"

#include "jutils/jutils-details.hpp"

using namespace jni;

CJNIRect::CJNIRect() : CJNIBase("android/graphics/Rect")
{
  m_object = new_object(GetClassName(),
    "<init>", "()V");
}

CJNIRect::CJNIRect(int l, int t, int r, int b) : CJNIBase("android/graphics/Rect")
{
  m_object = new_object(GetClassName(),
    "<init>", "(IIII)V",
                        l, t, r, b);
}

int CJNIRect::getLeft()
{
  return get_field<int>(m_object, "left");
}

int CJNIRect::getTop()
{
  return get_field<int>(m_object, "top");
}

int CJNIRect::getRight()
{
  return get_field<int>(m_object, "right");
}

int CJNIRect::getBottom()
{
  return get_field<int>(m_object, "bottom");
}

int CJNIRect::width()
{
  return call_method<jint>(m_object,
    "width", "()I");
}

int CJNIRect::height()
{
  return call_method<jint>(m_object,
    "height", "()I");
}

bool CJNIRect::equals(const CJNIRect& other)
{
  return call_method<jboolean>(m_object,
    "equals", "(Ljava/lang/Object;)Z", other.get_raw());
}

std::string CJNIRect::toString() const
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "toString", "()Ljava/lang/String;"));
}

int CJNIRect::describeContents() const
{
  return call_method<jint>(m_object,
    "describeContents", "()I");
}


