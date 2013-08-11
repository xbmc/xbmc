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
#include "jutils/jutils-details.hpp"

using namespace jni;

int CJNIBuffer::capacity()
{
  return call_method<int>(m_object,
    "capacity", "()I");
}

int CJNIBuffer::position()
{
  return call_method<int>(m_object,
    "position", "()I");
}

CJNIBuffer CJNIBuffer::position(int newPosition)
{
  return call_method<jhobject>(m_object,
    "position", "(I)Ljava/nio/Buffer;",
    newPosition);
}

int CJNIBuffer::limit()
{
  return call_method<int>(m_object,
    "limit", "()I");
}

CJNIBuffer CJNIBuffer::limit(int newLimit)
{
  return call_method<jhobject>(m_object,
    "limit", "(I)Ljava/nio/Buffer;",
    newLimit);
}

CJNIBuffer CJNIBuffer::mark()
{
  return call_method<jhobject>(m_object,
    "mark", "()Ljava/nio/Buffer;");
}

CJNIBuffer CJNIBuffer::reset()
{
  return call_method<jhobject>(m_object,
    "reset", "()Ljava/nio/Buffer;");
}

CJNIBuffer CJNIBuffer::clear()
{
  return call_method<jhobject>(m_object,
    "clear", "()Ljava/nio/Buffer;");
}

CJNIBuffer CJNIBuffer::flip()
{
  return call_method<jhobject>(m_object,
    "flip", "()Ljava/nio/Buffer;");
}

CJNIBuffer CJNIBuffer::rewind()
{
  return call_method<jhobject>(m_object,
    "rewind", "()Ljava/nio/Buffer;");
}

int CJNIBuffer::remaining()
{
  return call_method<int>(m_object,
    "remaining", "()I");
}

bool CJNIBuffer::hasRemaining()
{
  return call_method<jboolean>(m_object,
    "hasRemaining", "()Z");
}

bool CJNIBuffer::isReadOnly()
{
  return call_method<jboolean>(m_object,
    "isReadOnly", "()Z");
}

bool CJNIBuffer::hasArray()
{
  return call_method<jboolean>(m_object,
    "hasArray", "()Z");
}

/*
CJNIObject CJNIBuffer::array()
{
}
*/

int CJNIBuffer::arrayOffset()
{
  return call_method<int>(m_object,
    "arrayOffset", "()I");
}

bool CJNIBuffer::isDirect()
{
  return call_method<jboolean>(m_object,
    "isDirect", "()Z");
}

