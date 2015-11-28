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

#include "PowerManager.h"
#include "WakeLock.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

int CJNIPowerManager::FULL_WAKE_LOCK(0);
int CJNIPowerManager::SCREEN_BRIGHT_WAKE_LOCK(0xa);

void CJNIPowerManager::PopulateStaticFields()
{
  jhclass clazz  = find_class("platform/android/os/PowerManager");
  FULL_WAKE_LOCK = (get_static_field<int>(clazz, "FULL_WAKE_LOCK"));
  SCREEN_BRIGHT_WAKE_LOCK = (get_static_field<int>(clazz, "SCREEN_BRIGHT_WAKE_LOCK"));
}

CJNIWakeLock CJNIPowerManager::newWakeLock(int levelAndFlags, const std::string &tag)
{
  return call_method<jhobject>(m_object,
    "newWakeLock", "(ILjava/lang/String;)Landroid/os/PowerManager$WakeLock;",
    levelAndFlags, jcast<jhstring>(tag));
}

void CJNIPowerManager::goToSleep(int64_t timestamp)
{
  call_method<void>(m_object,
    "goToSleep", "(J)V",
    (jlong)timestamp);
}

void CJNIPowerManager::reboot(const std::string &reason)
{
  call_method<void>(m_object,
    "reboot", "(Ljava/lang/String;)V",
    jcast<jhstring>(reason));
}
