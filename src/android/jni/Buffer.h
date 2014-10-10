#pragma once
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

#include "JNIBase.h"

class CJNIBuffer : public CJNIBase
{
protected:
  CJNIBuffer(const jni::jhobject &object) : CJNIBase(object) {};
  ~CJNIBuffer() {};

public:
  int           capacity();
  int           position();
  CJNIBuffer    position(int newPosition);
  int           limit();
  CJNIBuffer    limit(int newLimit);
  CJNIBuffer    mark();
  CJNIBuffer    reset();
  CJNIBuffer    clear();
  CJNIBuffer    flip();
  CJNIBuffer    rewind();
  int           remaining();
  bool          hasRemaining();

  virtual bool  isReadOnly();
  virtual bool  hasArray();
//virtual CJNIObject array();
  virtual int   arrayOffset();
  virtual bool  isDirect();
};
