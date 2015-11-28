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

class CJNIBitSet : public CJNIBase
{
public:
  CJNIBitSet();
  CJNIBitSet(int);
  CJNIBitSet(const jni::jhobject &object) : CJNIBase(object) {};
  ~CJNIBitSet() {};

  void flip(int);
  void flip(int, int);
  void set(int);
  void set(int, bool);
  void set(int, int);
  void set(int, int, bool);
  void clear(int);
  void clear(int, int);
  void clear();
  bool get(int);
  CJNIBitSet get(int, int);
  int  nextSetBit(int);
  int  nextClearBit(int);
  int  length();
  bool isEmpty();
  bool intersects(const CJNIBitSet &);
  int  cardinality();
  void jand(const CJNIBitSet &);
  void jor(const CJNIBitSet &);
  void jxor(const CJNIBitSet &);
  void jandNot(const CJNIBitSet &);
  int  hashCode();
  int  size();
  std::string toString();
};
