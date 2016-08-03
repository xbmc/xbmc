/*
 *      Copyright (C) 2015 Team Kodi
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

#include "StorageManager.h"
#include "ClassLoader.h"

#include "jutils/jutils-details.hpp"

using namespace jni;


// Not in public API

std::vector<CJNIStorageVolume> CJNIStorageManager::getVolumeList()
{
  jmethodID mid = get_method_id(m_object, "getVolumeList", "()[Landroid/os/storage/StorageVolume;");
  if (mid != NULL)
    return jcast<CJNIStorageVolumes>(call_method<jhobjectArray>(m_object, mid));
  else
    return std::vector<CJNIStorageVolume>();
}

std::vector<std::string> CJNIStorageManager::getVolumePaths()
{
  jmethodID mid = get_method_id(m_object, "getVolumePaths", "()[Ljava/lang/String;");
  if (mid != NULL)
    return jcast<std::vector<std::string>>(call_method<jhobjectArray>(m_object, mid));
  else
    return std::vector<std::string>();
}

std::string CJNIStorageManager::getVolumeState(const std::string& mountPoint)
{
  jmethodID mid = get_method_id(m_object, "getVolumeState", "(Ljava/lang/String;)Ljava/lang/String;");
  if (mid != NULL)
    return jcast<std::string>(call_method<jhstring>(m_object, mid, jcast<jhstring>(mountPoint)));
  else
    return "";
}
