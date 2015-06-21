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

#include "Buffer.h"

class CJNIByteBuffer : public CJNIBuffer
{
public:
  CJNIByteBuffer(const jni::jhobject &object) : CJNIBuffer(object) {};
  ~CJNIByteBuffer(){};

  static CJNIByteBuffer allocateDirect(int capacity);
  static CJNIByteBuffer allocate(int capacity);
  static CJNIByteBuffer wrap(const std::vector<char> &array, int start, int byteCount);
  static CJNIByteBuffer wrap(const std::vector<char> &array);

  CJNIByteBuffer    duplicate();

  CJNIByteBuffer    get(const std::vector<char> &dst, int dstOffset, int byteCount);
  CJNIByteBuffer    get(const std::vector<char> &dst);
  CJNIByteBuffer    put(const CJNIByteBuffer &src);
  CJNIByteBuffer    put(const std::vector<char> &src, int srcOffset, int byteCount);
  CJNIByteBuffer    put(const std::vector<char> &src);

  bool              hasArray();
  std::vector<char> array();
  int               arrayOffset();
  std::string       toString();
  int               hashCode();
  //bool            equals(const CJNIObject &other);
  int               compareTo(const CJNIByteBuffer &otherBuffer);
  //CJNIByteOrder   order();
  //CJNIByteBuffer  order(const CJNIByteOrder &byteOrder);
  //CJNIObject      array();
  //int             compareTo(const CJNIObject &otherBuffer);

private:
  static const char *m_classname;
};

typedef std::vector<CJNIByteBuffer> CJNIByteBuffers;
