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
#include "WifiManagerMulticastLock.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

void CJNIWifiManagerMulticastLock::acquire()
{
  call_method<void>(m_object, "acquire", "()V");
}

void CJNIWifiManagerMulticastLock::release()
{
  call_method<void>(m_object, "release", "()V");
}

void CJNIWifiManagerMulticastLock::setReferenceCounted(bool refCounted)
{
  call_method<void>(m_object, "setReferenceCounted", "(Z)V", refCounted);
}

bool CJNIWifiManagerMulticastLock::isHeld()
{
  return call_method<jboolean>(m_object, "setReferenceCounted", "()Z");
}

std::string CJNIWifiManagerMulticastLock::toString()
{
  return jcast<std::string>(call_method<jhstring>(m_object, "toString", "()Ljava/lang/String;"));
}
