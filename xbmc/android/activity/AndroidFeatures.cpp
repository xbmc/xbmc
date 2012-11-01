/*
 *      Copyright (C) 2012 Team XBMC
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

#include "AndroidFeatures.h"
#include "XBMCApp.h"
#include "utils/log.h"

#include <cpu-features.h>

bool CAndroidFeatures::HasNeon()
{
  if (android_getCpuFamily() == ANDROID_CPU_FAMILY_ARM) 
    return ((android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) != 0);
  return false;
}

int CAndroidFeatures::GetVersion()
{
  static int version = -1;

  if (version == -1)
  {
    version = 0;

    JNIEnv *jenv = NULL;
    CXBMCApp::AttachCurrentThread(&jenv, NULL);

    jclass jcOsBuild = jenv->FindClass("android/os/Build$VERSION");
    if (jcOsBuild == NULL) 
    {
      CLog::Log(LOGERROR, "%s: Error getting class android.os.Build.VERSION", __PRETTY_FUNCTION__);
      return version;
    }

    jint iSdkVersion = jenv->GetStaticIntField(jcOsBuild, jenv->GetStaticFieldID(jcOsBuild, "SDK_INT", "I"));
    CLog::Log(LOGDEBUG, "%s: android.os.Build.VERSION %d", __PRETTY_FUNCTION__, (int)iSdkVersion);

    // <= 10 Gingerbread
    // <= 13 Honeycomb
    // <= 15 IceCreamSandwich
    //       JellyBean
    version = iSdkVersion;

    jenv->DeleteLocalRef(jcOsBuild);
    CXBMCApp::DetachCurrentThread();
  }
  return version;
}

std::string CAndroidFeatures::GetLibiomxName()
{
  std::string strOMXLibName;
  int version = GetVersion();

  // Gingerbread
  if (version <= 10)
    strOMXLibName = "libiomx-10.so";
  // Honeycomb
  else if (version <= 13)
    strOMXLibName = "libiomx-13.so";
  // IceCreamSandwich
  else if (version <= 15)
    strOMXLibName = "libiomx-14.so";
  else
    strOMXLibName = "unknown";

  return strOMXLibName;
}

