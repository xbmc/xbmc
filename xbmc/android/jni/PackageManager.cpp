/*
 *      Copyright (C) 2013 Team XBMC
 *      http://www.xbmc.org
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
#include "PackageManager.h"
#include "Intent.h"
#include "Drawable.h"
#include "List.h"
#include "CharSequence.h"
#include "ApplicationInfo.h"
#include "jutils/jutils-details.hpp"

using namespace jni;
int CJNIPackageManager::GET_ACTIVITIES(0);

CJNIPackageManager::CJNIPackageManager(const jni::jhobject &object) : CJNIBase(object)
{
  GET_ACTIVITIES = (get_static_field<int>(m_object, "GET_ACTIVITIES"));
}

CJNIIntent CJNIPackageManager::getLaunchIntentForPackage(const std::string &package)
{
  return (CJNIIntent)call_method<jhobject>(m_object, "getLaunchIntentForPackage", "(Ljava/lang/String;)Landroid/content/Intent;", jcast<jhstring>(package));
}

CJNIDrawable CJNIPackageManager::getApplicationIcon(const std::string &package)
{
  return (CJNIDrawable)call_method<jhobject>(m_object, "getApplicationIcon", "(Ljava/lang/String;)Landroid/graphics/drawable/Drawable;", jcast<jhstring>(package));
}

CJNICharSequence CJNIPackageManager::getApplicationLabel(const CJNIApplicationInfo &info)
{
  return (CJNICharSequence)call_method<jhobject>(m_object, "getApplicationLabel", "(Landroid/content/pm/ApplicationInfo;)Ljava/lang/CharSequence;", info.get());
}

CJNIList<CJNIApplicationInfo> CJNIPackageManager::getInstalledApplications(int flags)
{
  return (CJNIList<CJNIApplicationInfo>)call_method<jhobject>(m_object, "getInstalledApplications", "(I)Ljava/util/List;", flags);
}

