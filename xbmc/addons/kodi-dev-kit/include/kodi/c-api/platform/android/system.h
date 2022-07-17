/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

/*---AUTO_GEN_PARSE<$$CORE_SYSTEM_NAME:android>---*/

#ifndef C_API_PLATFORM_ANDROID_H
#define C_API_PLATFORM_ANDROID_H

#define INTERFACE_ANDROID_SYSTEM_NAME "ANDROID_SYSTEM"
#define INTERFACE_ANDROID_SYSTEM_VERSION "1.0.2"
#define INTERFACE_ANDROID_SYSTEM_VERSION_MIN "1.0.1"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  struct AddonToKodiFuncTable_android_system
  {
    void* (*get_jni_env)();
    int (*get_sdk_version)();
    const char* (*get_class_name)();
  };

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !C_API_PLATFORM_ANDROID_H */
