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

#include "ApplicationInfo.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

CJNIApplicationInfo::CJNIApplicationInfo(const jhobject &object) : CJNIPackageItemInfo(object)
  ,sourceDir(       jcast<std::string>(get_field<jhstring>(m_object, "sourceDir")))
  ,publicSourceDir( jcast<std::string>(get_field<jhstring>(m_object, "publicSourceDir")))
  ,dataDir(         jcast<std::string>(get_field<jhstring>(m_object, "dataDir")))
  ,nativeLibraryDir(jcast<std::string>(get_field<jhstring>(m_object, "nativeLibraryDir")))
  ,packageName(     jcast<std::string>(get_field<jhstring>(m_object, "packageName")))
  ,uid(             get_field<int>(m_object, "uid"))
  ,targetSdkVersion(get_field<int>(m_object, "targetSdkVersion"))
  ,enabled(         get_field<jboolean>(m_object, "enabled"))
{
}
