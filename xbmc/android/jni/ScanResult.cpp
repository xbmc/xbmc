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

#include "ScanResult.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

CJNIScanResult::CJNIScanResult(const jhobject &object) : CJNIBase(object)
  ,SSID(        jcast<std::string>(get_field<jhstring>(m_object, "SSID")))
  ,BSSID(       jcast<std::string>(get_field<jhstring>(m_object, "BSSID")))
  ,capabilities(jcast<std::string>(get_field<jhstring>(m_object, "capabilities")))
  ,level(       get_field<int>(m_object, "level"))
  ,frequency(   get_field<int>(m_object, "frequency"))
{
}

std::string CJNIScanResult::toString()
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "toString", "()Ljava/lang/String;"));
}

int CJNIScanResult::describeContents()
{
  return call_method<jint>(m_object,
    "describeContents", "()I");
}
