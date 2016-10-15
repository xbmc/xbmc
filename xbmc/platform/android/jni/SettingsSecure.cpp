/*
 *      Copyright (C) 2016 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "SettingsSecure.h"

using namespace jni;

std::string CJNISettingsSecure::ANDROID_ID;

void CJNISettingsSecure::PopulateStaticFields()
{
  jhclass clazz = find_class("android/provider/Settings$Secure");
  ANDROID_ID = (jcast<std::string>(get_static_field<jhstring>(clazz, "ANDROID_ID")));
}
                                  
std::string CJNISettingsSecure::getString(CJNIContentResolver resolver, std::string name)
{
  return jcast<std::string>(call_static_method<jhstring>("android/provider/Settings$Secure", "getString", "(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;", resolver.get_raw(), jcast<jhstring>(name)));
}
