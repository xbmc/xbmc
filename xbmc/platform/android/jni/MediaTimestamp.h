#pragma once
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

#include "JNIBase.h"

namespace jni
{

class CJNIMediaTimestamp : public CJNIBase
{
public:
  CJNIMediaTimestamp();
  CJNIMediaTimestamp(const jni::jhobject &object);

  int64_t getAnchorMediaTimeUs();
  int64_t getAnchorSytemNanoTime();
  float getMediaClockRate();
};

}

