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

#include "NetworkInfo.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

int CJNINetworkInfo::getType() const
{
  return call_method<jint>(m_object,
    "getType", "()I");
}

int CJNINetworkInfo::getSubtype() const
{
  return call_method<jint>(m_object,
    "getSubtype", "()I");
}

std::string CJNINetworkInfo::getTypeName() const
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "getTypeName", "()Ljava/lang/String;"));
}

std::string CJNINetworkInfo::getSubtypeName() const
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "getTypeName", "()Ljava/lang/String;"));
}

bool CJNINetworkInfo::isConnectedOrConnecting() const
{
  return call_method<jboolean>(m_object,
    "isConnectedOrConnecting", "()Z");
}

bool CJNINetworkInfo::isConnected() const
{
  return call_method<jboolean>(m_object,
    "isConnected", "()Z");
}

bool CJNINetworkInfo::isAvailable() const
{
  return call_method<jboolean>(m_object,
    "isAvailable", "()Z");
}

bool CJNINetworkInfo::isFailover() const
{
  return call_method<jboolean>(m_object,
    "isFailover", "()Z");
}

bool CJNINetworkInfo::isRoaming() const
{
  return call_method<jboolean>(m_object,
    "isRoaming", "()Z");
}

CJNINetworkInfoState CJNINetworkInfo::getState() const
{
  return call_method<jhobject>(m_object,
    "getState", "()Landroid/net/NetworkInfo$State;");
}

CJNINetworkInfoDetailedState CJNINetworkInfo::getDetailedState() const
{
  return call_method<jhobject>(m_object,
    "getDetailedState", "()Landroid/net/NetworkInfo$DetailedState;");
}

std::string CJNINetworkInfo::getReason() const
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "getReason", "()Ljava/lang/String;"));
}

std::string CJNINetworkInfo::getExtraInfo() const
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "getExtraInfo", "()Ljava/lang/String;"));
}

std::string CJNINetworkInfo::toString() const
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "toString", "()Ljava/lang/String;"));
}

int CJNINetworkInfo::describeContents() const
{
  return call_method<jint>(m_object,
    "describeContents", "()I");
}

