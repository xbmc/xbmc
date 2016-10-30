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

#include "Image.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

const CJNIByteBuffer CJNIImagePlane::getBuffer()
{
  return call_method<jhobject>(m_object,
    "getBuffer", "()Ljava/nio/ByteBuffer;");
}

int CJNIImagePlane::getPixelStride()
{
  return call_method<jint>(m_object,
    "getPixelStride", "()I");
}

int CJNIImagePlane::getRowStride()
{
  return call_method<jint>(m_object,
    "getRowStride", "()I");
}

void CJNIImage::close()
{
   call_method<void>(m_object,
    "close", "()V");
}

int CJNIImage::getFormat()
{
  return call_method<jint>(m_object,
    "getFormat", "()I");
}

std::vector<CJNIImagePlane> CJNIImage::getPlanes()
{
  return jcast<std::vector<CJNIImagePlane>>(call_method<jhobjectArray>(m_object,
    "getPlanes", "()[Landroid/media/Image$Plane;"));
}

int CJNIImage::getHeight()
{
  return call_method<jint>(m_object,
    "getHeight", "()I");
}

int CJNIImage::getWidth()
{
  return call_method<jint>(m_object,
    "getWidth", "()I");
}
