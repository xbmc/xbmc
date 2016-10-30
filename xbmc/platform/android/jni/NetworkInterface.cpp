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

#include "NetworkInterface.h"
#include "InetAddress.h"

#include "jutils/jutils-details.hpp"

using namespace jni;

const char* CJNINetworkInterface::m_classname = "java/net/NetworkInterface";

CJNINetworkInterface CJNINetworkInterface::getByName(const std::string& name)
{
  return CJNINetworkInterface(call_static_method<jhobject>(m_classname,
    "getByName", "(Ljava/lang/String;)Ljava/net/NetworkInterface;", jcast<jhstring>(name)));
}

CJNINetworkInterface CJNINetworkInterface::getByIndex(int index)
{
  return CJNINetworkInterface(call_static_method<jhobject>(m_classname,
    "getByIndex", "(I)Ljava/net/NetworkInterface;", index));
}

CJNINetworkInterface CJNINetworkInterface::getByInetAddress(const CJNIInetAddress& addr)
{
  return CJNINetworkInterface(call_static_method<jhobject>(m_classname,
    "getByInetAddress", "(Ljava/net/InetAddress;)Ljava/net/NetworkInterface;", addr.get_raw()));
}

std::string CJNINetworkInterface::getName()
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "getName", "()Ljava/lang/String;"));
}

std::string CJNINetworkInterface::getDisplayName()
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "getDisplayName", "()Ljava/lang/String;"));
}

std::vector<char> CJNINetworkInterface::getHardwareAddress()
{
  JNIEnv *env = xbmc_jnienv();
  jhbyteArray array = call_method<jhbyteArray>(m_object,
    "getHardwareAddress", "()[B");

  jsize size = env->GetArrayLength(array.get());

  std::vector<char> result;
  result.resize(size);
  env->GetByteArrayRegion(array.get(), 0, size, (jbyte*)result.data());

  return result;
}

int CJNINetworkInterface::getIndex()
{
  return call_method<jboolean>(m_object,
    "getIndex", "()I");
}

int CJNINetworkInterface::getMTU()
{
  return call_method<jboolean>(m_object,
    "getMTU", "()I");
}

bool CJNINetworkInterface::isLoopback()
{
  return call_method<jboolean>(m_object,
    "isLoopback", "()Z");
}

bool CJNINetworkInterface::isPointToPoint()
{
  return call_method<jboolean>(m_object,
    "isPointToPoint", "()Z");
}

bool CJNINetworkInterface::isUp()
{
  return call_method<jboolean>(m_object,
    "isUp", "()Z");
}

bool CJNINetworkInterface::isVirtual()
{
  return call_method<jboolean>(m_object,
    "isVirtual", "()Z");
}

bool CJNINetworkInterface::supportsMulticast()
{
  return call_method<jboolean>(m_object,
    "supportsMulticast", "()Z");
}

bool CJNINetworkInterface::equals(const CJNINetworkInterface& other)
{
  return call_method<jboolean>(m_object,
    "equals", "(Ljava/lang/Object;)Z", other.get_raw());
}

std::string CJNINetworkInterface::toString() const
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "toString", "()Ljava/lang/String;"));
}

