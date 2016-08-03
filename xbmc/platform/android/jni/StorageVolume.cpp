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

#include "StorageVolume.h"
#include "ClassLoader.h"

#include "jutils/jutils-details.hpp"

using namespace jni;

// Not in public API, be extra careful

std::string CJNIStorageVolume::getPath()
{
  jmethodID mid = get_method_id(m_object, "getPath", "()Ljava/lang/String;");
  if (mid != NULL)
    return jcast<std::string>(call_method<jhstring>(m_object, mid));
  else
    return "";
}

std::string CJNIStorageVolume::getDescription(const CJNIContext& context)
{
  jmethodID mid = get_method_id(m_object, "getDescription", "(Landroid/content/Context;)Ljava/lang/String;");
  if (mid != NULL)
    return jcast<std::string>(call_method<jhstring>(m_object, mid, context.get_raw()));
  else
    return "";
}

int CJNIStorageVolume::getDescriptionId()
{
  jmethodID mid = get_method_id(m_object, "getDescriptionId", "()I");
  if (mid != NULL)
    return call_method<int>(m_object, mid);
  else
  {
    return false;
  }
}

bool CJNIStorageVolume::isPrimary()
{
  jmethodID mid = get_method_id(m_object, "isPrimary", "()Z");
  if (mid != NULL)
    return call_method<jboolean>(m_object, mid);
  else
  {
    return false;
  }
}

bool CJNIStorageVolume::isRemovable()
{
  jmethodID mid = get_method_id(m_object, "isRemovable", "()Z");
  if (mid != NULL)
    return call_method<jboolean>(m_object, mid);
  else
  {
    return false;
  }
}

bool CJNIStorageVolume::isEmulated()
{
  jmethodID mid = get_method_id(m_object, "isEmulated", "()Z");
  if (mid != NULL)
    return call_method<jboolean>(m_object, mid);
  else
    return false;
}

int64_t CJNIStorageVolume::getMaxFileSize()
{
  jmethodID mid = get_method_id(m_object, "getMaxFileSize", "()J");
  if (mid != NULL)
    return call_method<jlong>(m_object, mid);
  else
    return -1;
}

std::string CJNIStorageVolume::getUuid()
{
  jmethodID mid = get_method_id(m_object, "getUuid", "()Ljava/lang/String;");
  if (mid != NULL)
    return jcast<std::string>(call_method<jhstring>(m_object, mid));
  else
    return "";
}

int CJNIStorageVolume::getFatVolumeId()
{
  jmethodID mid = get_method_id(m_object, "getFatVolumeId", "()I");
  if (mid != NULL)
    return call_method<int>(m_object, mid);
  else
    return -1;
}

std::string CJNIStorageVolume::getUserLabel()
{
  jmethodID mid = get_method_id(m_object, "getUserLabel", "()Ljava/lang/String;");
  if (mid != NULL)
    return jcast<std::string>(call_method<jhstring>(m_object, mid));
  else
    return "";
}

std::string CJNIStorageVolume::getState()
{
  jmethodID mid = get_method_id(m_object, "getState", "()Ljava/lang/String;");
  if (mid != NULL)
    return jcast<std::string>(call_method<jhstring>(m_object, mid));
  else
    return "";
}
