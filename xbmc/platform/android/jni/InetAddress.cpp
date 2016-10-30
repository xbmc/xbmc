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

#include "InetAddress.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

const char* CJNIInetAddress::m_classname = "java/net/InetAddress";

CJNIInetAddress CJNIInetAddress::getLocalHost()
{
  return CJNIInetAddress(call_static_method<jhobject>(m_classname,
                                                      "getLocalHost", "()Ljava/net/InetAddress;"));
}

CJNIInetAddress CJNIInetAddress::getLoopbackAddress()
{
  return CJNIInetAddress(call_static_method<jhobject>(m_classname,
                                                      "getLoopbackAddress", "()Ljava/net/InetAddress;"));
}

CJNIInetAddress CJNIInetAddress::getByName(std::string host)
{
  return CJNIInetAddress(call_static_method<jhobject>(m_classname,
    "getByName", "(Ljava/lang/String;)Ljava/net/InetAddress;", jcast<jhstring>(host)));
}

std::vector<char> CJNIInetAddress::getAddress()
{
  JNIEnv *env = xbmc_jnienv();
  jhbyteArray array = call_method<jhbyteArray>(m_object,
    "getAddress", "()[B");

  jsize size = env->GetArrayLength(array.get());

  std::vector<char> result;
  result.resize(size);
  env->GetByteArrayRegion(array.get(), 0, size, (jbyte*)result.data());

  return result;
}

std::string CJNIInetAddress::getHostAddress()
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "getHostAddress", "()Ljava/lang/String;"));
}

std::string CJNIInetAddress::getHostName()
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "getHostName", "()Ljava/lang/String;"));
}

std::string CJNIInetAddress::getCanonicalHostName()
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "getCanonicalHostName", "()Ljava/lang/String;"));
}

bool CJNIInetAddress::equals(const CJNIInetAddress& other)
{
  return call_method<jboolean>(m_object,
    "equals", "(Ljava/lang/Object;)Z", other.get_raw());
}

std::string CJNIInetAddress::toString() const
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "toString", "()Ljava/lang/String;"));
}

int CJNIInetAddress::describeContents() const
{
  return call_method<jint>(m_object,
    "describeContents", "()I");
}

