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

#include "ByteBuffer.h"

namespace jni
{

class CJNIImagePlane : public CJNIBase
{
public:
  CJNIImagePlane(const jni::jhobject &object) : CJNIBase(object) {}

  const CJNIByteBuffer getBuffer();
  int getPixelStride();
  int getRowStride();

protected:
  CJNIImagePlane();
};

class CJNIImage : public CJNIBase
{
public:
  CJNIImage() : CJNIBase() {}
  CJNIImage(const jhobject &object) : CJNIBase(object) {}
  ~CJNIImage() {}

  void close();
  int getFormat();
  std::vector<CJNIImagePlane> getPlanes();

  int getHeight();
  int getWidth();

};

}

