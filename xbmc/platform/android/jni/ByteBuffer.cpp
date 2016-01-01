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

#include "ByteBuffer.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

const char* CJNIByteBuffer::m_classname = "java/nio/ByteBuffer";

CJNIByteBuffer CJNIByteBuffer::CJNIByteBuffer::allocateDirect(int capacity)
{
  return CJNIByteBuffer(call_static_method<jhobject>(m_classname,
    "allocateDirect", "(I)Ljava/nio/ByteBuffer;",
    capacity));
}

CJNIByteBuffer CJNIByteBuffer::allocate(int capacity)
{
  return CJNIByteBuffer(call_static_method<jhobject>(m_classname,
    "allocate", "(I)Ljava/nio/ByteBuffer;",
    capacity));
}

CJNIByteBuffer CJNIByteBuffer::wrap(const std::vector<char> &array, int start, int byteCount)
{
  JNIEnv *env = xbmc_jnienv();
  jsize size  = array.size();
  jbyteArray bytearray = env->NewByteArray(size);
  env->SetByteArrayRegion(bytearray, 0, size, (jbyte*)&array[0]);

  return CJNIByteBuffer(call_static_method<jhobject>(m_classname,
    "wrap","([BII)Ljava/nio/ByteBuffer;",
    bytearray, start, byteCount));
}

CJNIByteBuffer CJNIByteBuffer::wrap(const std::vector<char> &array)
{
  JNIEnv *env = xbmc_jnienv();
  jsize size  = array.size();
  jbyteArray bytearray = env->NewByteArray(size);
  env->SetByteArrayRegion(bytearray, 0, size, (jbyte*)&array[0]);

  return CJNIByteBuffer(call_static_method<jhobject>(m_classname,
    "wrap","([B)Ljava/nio/ByteBuffer;",
    bytearray));
}

CJNIByteBuffer CJNIByteBuffer::duplicate()
{
  return CJNIByteBuffer(call_method<jhobject>(m_object,
    "duplicate","()Ljava/nio/ByteBuffer;"));
}

CJNIByteBuffer CJNIByteBuffer::get(const std::vector<char> &dst, int dstOffset, int byteCount)
{
  JNIEnv *env = xbmc_jnienv();
  jsize size  = dst.size();
  jbyteArray bytearray = env->NewByteArray(size);
  env->SetByteArrayRegion(bytearray, 0, size, (jbyte*)&dst[0]);

  return CJNIByteBuffer(call_method<jhobject>(m_object,
    "get","([BII)Ljava/nio/ByteBuffer;",
    bytearray, dstOffset, byteCount));
}

CJNIByteBuffer CJNIByteBuffer::get(const std::vector<char> &dst)
{
  JNIEnv *env = xbmc_jnienv();
  jsize size  = dst.size();
  jbyteArray bytearray = env->NewByteArray(size);
  env->SetByteArrayRegion(bytearray, 0, size, (jbyte*)&dst[0]);

  return CJNIByteBuffer(call_method<jhobject>(m_object,
    "get","([B)Ljava/nio/ByteBuffer;",
    bytearray));
}

CJNIByteBuffer CJNIByteBuffer::put(const CJNIByteBuffer &src)
{
  return CJNIByteBuffer(call_method<jhobject>(m_object,
    "put","(Ljava/nio/ByteBuffer;)Ljava/nio/ByteBuffer;",
    src.get_raw()));
}

CJNIByteBuffer CJNIByteBuffer::put(const std::vector<char> &src, int srcOffset, int byteCount)
{
  JNIEnv *env = xbmc_jnienv();
  jsize size  = src.size();
  jbyteArray bytearray = env->NewByteArray(size);
  env->SetByteArrayRegion(bytearray, 0, size, (jbyte*)src.data());

  return CJNIByteBuffer(call_method<jhobject>(m_object,
    "put","([BII)Ljava/nio/ByteBuffer;",
    bytearray, srcOffset, byteCount));
}

CJNIByteBuffer CJNIByteBuffer::put(const std::vector<char> &src)
{
  JNIEnv *env = xbmc_jnienv();
  jsize size  = src.size();
  jbyteArray bytearray = env->NewByteArray(size);
  env->SetByteArrayRegion(bytearray, 0, size, (jbyte*)src.data());

  return CJNIByteBuffer(call_method<jhobject>(m_object,
    "put","([B)Ljava/nio/ByteBuffer;",
    bytearray));
}

bool CJNIByteBuffer::hasArray()
{
  return call_method<jboolean>(m_object,
    "hasArray", "()Z");
}

std::vector<char> CJNIByteBuffer::array()
{
  JNIEnv *env = xbmc_jnienv();
  jhbyteArray array = call_method<jhbyteArray>(m_object,
    "array", "()[B");

  jsize size = env->GetArrayLength(array.get());

  std::vector<char> result;
  result.resize(size);
  env->GetByteArrayRegion(array.get(), 0, size, (jbyte*)result.data());

  return result;
}

int CJNIByteBuffer::arrayOffset()
{
  return call_method<int>(m_object,
    "arrayOffset", "()I");
}

std::string CJNIByteBuffer::toString()
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "toString", "()Ljava/lang/String;"));
}

int CJNIByteBuffer::hashCode()
{
  return call_method<int>(m_object,
    "hashCode", "()I");
}

/*
bool CJNIByteBuffer::equals(CJNIObject other)
{
}
*/

int CJNIByteBuffer::compareTo(const CJNIByteBuffer &otherBuffer)
{
  return call_method<int>(m_object,
    "compareTo","(Ljava/nio/ByteBuffer;)I",
    otherBuffer.get_raw());
}

/*
CJNIByteOrder CJNIByteBuffer::order()
{
}

CJNIByteBuffer CJNIByteBuffer::order(CJNIByteOrder byteOrder)
{
}

CJNIObject CJNIByteBuffer::array()
{
}

int CJNIByteBuffer::compareTo(const CJNIObject &otherBuffer)
{
}
*/
