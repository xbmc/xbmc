/*
 *      Copyright (C) 2005-2018 Team Kodi
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "System.h"
#include "CompileInfo.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-addon-dev-kit/include/kodi/platform/android/System.h"

#include "platform/android/activity/XBMCApp.h"

static AddonToKodiFuncTable_android_system function_table;

namespace ADDON
{

void Interface_Android::Register()
{
  function_table.get_jni_env = get_jni_env;
  function_table.get_sdk_version = get_sdk_version;
  function_table.get_class_name = get_class_name;
  CAddonDll::RegisterInterface(Get);
}

void* Interface_Android::Get(const std::string &name, const std::string &version)
{
  if (name == INTERFACE_ANDROID_SYSTEM_NAME
  && version >= INTERFACE_ANDROID_SYSTEM_VERSION_MIN
  && version <= INTERFACE_ANDROID_SYSTEM_VERSION)
    return &function_table;

  return nullptr;
};

void* Interface_Android::get_jni_env()
{
  return xbmc_jnienv();
}

int Interface_Android::get_sdk_version()
{
  return CXBMCApp::get()->getActivity()->sdkVersion;
}

const char *Interface_Android::get_class_name()
{
  return CCompileInfo::GetClass();
}


} //namespace ADDON
