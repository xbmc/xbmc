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

#include "WakeLock.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

void CJNIWakeLock::acquire()
{
  call_method<void>(m_object,
    "acquire", "()V");
}

void CJNIWakeLock::release()
{
  call_method<void>(m_object,
                    "release", "()V");
}

bool CJNIWakeLock::isHeld()
{
  return call_method<jboolean>(m_object,
                        "isHeld", "()Z");
}

void CJNIWakeLock::setReferenceCounted(bool val)
{
  call_method<void>(m_object,
                        "setReferenceCounted", "(Z)V", val);
}

