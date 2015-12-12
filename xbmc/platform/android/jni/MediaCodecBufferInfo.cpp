/*
 *      Copyright (C) 2013 Team XBMC
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

#include "MediaCodecBufferInfo.h"
#include "Context.h"
#include "ClassLoader.h"

#include "jutils/jutils-details.hpp"

using namespace jni;

CJNIMediaCodecBufferInfo::CJNIMediaCodecBufferInfo() : CJNIBase("platform/android/media/MediaCodec$BufferInfo")
{
  m_object = new_object(GetClassName(), "<init>", "()V");
  m_object.setGlobal();
}

void CJNIMediaCodecBufferInfo::set(int newOffset, int newSize, int64_t newTimeUs, int newFlags)
{
  call_method<void>(m_object,
    "set", "(IIJI)V",
    newOffset, newSize, newTimeUs, newFlags);
}

int CJNIMediaCodecBufferInfo::offset() const
{
  return get_field<int>(m_object, "offset");
}

int CJNIMediaCodecBufferInfo::size() const
{
  return get_field<int>(m_object, "size");
}

int64_t CJNIMediaCodecBufferInfo::presentationTimeUs() const
{
  return get_field<jlong>(m_object, "presentationTimeUs");
}

int CJNIMediaCodecBufferInfo::flags() const
{
  return get_field<int>(m_object, "flags");
}
