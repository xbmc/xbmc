/*
 *      Copyright (C) 2016 Chris Browet
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

#include "MediaTimestamp.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

CJNIMediaTimestamp::CJNIMediaTimestamp()
 : CJNIBase("android.media.MediaTimestamp")
{
  m_object = new_object(GetClassName());
  m_object.setGlobal();
}

CJNIMediaTimestamp::CJNIMediaTimestamp(const jni::jhobject& object)
 : CJNIBase(object)
{
}

int64_t CJNIMediaTimestamp::getAnchorMediaTimeUs()
{
  return call_method<jlong>(m_object, "getAnchorMediaTimeUs", "()J");
}

int64_t CJNIMediaTimestamp::getAnchorSytemNanoTime()
{
  return call_method<jlong>(m_object, "getAnchorSytemNanoTime", "()J");
}

float CJNIMediaTimestamp::getMediaClockRate()
{
  return call_method<jfloat>(m_object, "getMediaClockRate", "()F");
}

