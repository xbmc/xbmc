/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AndroidFeatures.h"

#include "utils/log.h"

#include <androidjni/JNIThreading.h>
#include <cpu-features.h>

bool CAndroidFeatures::HasNeon()
{
  // All ARMv8-based devices support Neon - https://developer.android.com/ndk/guides/cpu-arm-neon
  if (android_getCpuFamily() == ANDROID_CPU_FAMILY_ARM64)
    return true;

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

    JNIEnv *jenv = xbmc_jnienv();

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
    // <= 19 KitKat
    version = iSdkVersion;

    jenv->DeleteLocalRef(jcOsBuild);
  }
  return version;
}

int CAndroidFeatures::GetCPUCount()
{
  static int count = -1;

  if (count == -1)
  {
    count = android_getCpuCount();
  }
  return count;
}

