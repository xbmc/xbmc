/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../AddonBase.h"
#include "../../c-api/platform/android/system.h"

//==============================================================================
///
/// \defgroup cpp_kodi_platform  Interface - kodi::platform
/// \ingroup cpp
/// @brief **Android platform specific functions**
///
/// #include <kodi/platform/android/System.h>"
///
//------------------------------------------------------------------------------

#ifdef __cplusplus
namespace kodi
{
namespace platform
{

class ATTRIBUTE_HIDDEN CInterfaceAndroidSystem
{
public:
  CInterfaceAndroidSystem()
    : m_interface(static_cast<AddonToKodiFuncTable_android_system*>(
          GetInterface(INTERFACE_ANDROID_SYSTEM_NAME, INTERFACE_ANDROID_SYSTEM_VERSION))){};

  //============================================================================
  ///
  /// \ingroup cpp_kodi_platform
  /// @brief request an JNI env pointer for the calling thread.
  /// JNI env has to be controlled by kodi because of the underlying
  /// threading concep.
  ///
  /// @param[in]:
  /// @return JNI env pointer for the calling thread
  ///
  inline void* GetJNIEnv()
  {
    if (m_interface)
      return m_interface->get_jni_env();

    return nullptr;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// \ingroup cpp_kodi_platform
  /// @brief request the android sdk version to e.g. initialize JNIBase.
  ///
  /// @param[in]:
  /// @return Android SDK version
  ///
  inline int GetSDKVersion()
  {
    if (m_interface)
      return m_interface->get_sdk_version();

    return 0;
  }

  //============================================================================
  ///
  /// \ingroup cpp_kodi_platform
  /// @brief request the android main class name e.g. org.xbmc.kodi.
  ///
  /// @param[in]:
  /// @return package class name
  ///
  inline std::string GetClassName()
  {
    if (m_interface)
      return m_interface->get_class_name();

    return std::string();
  }

private:
  AddonToKodiFuncTable_android_system* m_interface;
};
//----------------------------------------------------------------------------

} /* namespace platform */
} /* namespace kodi */
#endif /* __cplusplus */
