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

#include "BitSet.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

CJNIBitSet::CJNIBitSet() : CJNIBase("java/util/BitSet")
{
  m_object = new_object(GetClassName());
  m_object.setGlobal();
}

CJNIBitSet::CJNIBitSet(int bitCount) : CJNIBase("java/util/BitSet")
{
  m_object = new_object(GetClassName(),
    "<init>", "(I)V",
    (jint)bitCount);
}

void CJNIBitSet::flip(int index)
{
  call_method<void>(m_object,
    "flip", "(I)V",
    index);
}

void CJNIBitSet::flip(int fromIndex, int toIndex)
{
  call_method<void>(m_object,
    "flip", "(II)V",
    fromIndex, toIndex);
}

void CJNIBitSet::set(int index)
{
  call_method<void>(m_object, "set", "(I)V", index);
}

void CJNIBitSet::set(int fromIndex, bool state)
{
  call_method<void>(m_object,
    "fromIndex", "(IZ)V",
    fromIndex, state);
}

void CJNIBitSet::set(int fromIndex, int toIndex)
{
  call_method<void>(m_object,
    "set", "(II)V",
    fromIndex, toIndex);
}

void CJNIBitSet::set(int fromIndex, int toIndex, bool state)
{
  call_method<void>(m_object,
    "set", "(IIZ)V",
    fromIndex, toIndex, state);
}

void CJNIBitSet::clear(int index)
{
  call_method<void>(m_object,
    "clear", "(I)V",
    index);
}

void CJNIBitSet::clear(int fromIndex, int toIndex)
{
  call_method<void>(m_object,
    "clear", "(II)V",
    fromIndex, toIndex);
}

void CJNIBitSet::clear()
{
  call_method<void>(m_object,
    "clear", "()V");
}

bool CJNIBitSet::get(int index)
{
  return call_method<jboolean>(m_object,
    "get", "(I)V",
    index);
}

CJNIBitSet CJNIBitSet::get(int fromIndex, int toIndex)
{
  return call_method<jhobject>(m_object,
    "get", "(II)V",
    fromIndex, toIndex);
}

int CJNIBitSet::nextSetBit(int index)
{
  return call_method<jint>(m_object,
    "nextSetBit", "(I)I",
    index);
}

int CJNIBitSet::nextClearBit(int index)
{
  return call_method<jint>(m_object,
    "nextClearBit", "(I)I",
    index);
}

int CJNIBitSet::length()
{
  return call_method<jint>(m_object,
    "length", "()I");
}

bool CJNIBitSet::isEmpty()
{
  return call_method<jboolean>(m_object,
    "isEmpty", "()Z");
}

bool CJNIBitSet::intersects(const CJNIBitSet &bs)
{
  return call_method<jboolean>(m_object,
    "intersects", "(Ljava/util/BitSet;)Z",
    bs.get_raw());
}

int CJNIBitSet::cardinality()
{
  return call_method<jint>(m_object,
    "cardinality", "()I");
}

void CJNIBitSet::jand(const CJNIBitSet &bs)
{
  call_method<void>(m_object,
    "jand", "(Ljava/util/BitSet;)V",
    bs.get_raw());
}

void CJNIBitSet::jor(const CJNIBitSet &bs)
{
  call_method<void>(m_object,
    "jor", "(Ljava/util/BitSet;)V",
    bs.get_raw());
}

void CJNIBitSet::jxor(const CJNIBitSet &bs)
{
  call_method<void>(m_object,
    "jxor", "(Ljava/util/BitSet;)V",
    bs.get_raw());
}

void CJNIBitSet::jandNot(const CJNIBitSet &bs)
{
  call_method<void>(m_object,
    "jandNot", "(Ljava/util/BitSet;)V",
    bs.get_raw());
}

int CJNIBitSet::hashCode()
{
  return call_method<jint>(m_object,
    "hashCode", "()I");
}

int CJNIBitSet::size()
{
  return call_method<jint>(m_object,
    "size", "()I");
}

std::string CJNIBitSet::toString()
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "toString", "()Ljava/lang/String;"));
}
