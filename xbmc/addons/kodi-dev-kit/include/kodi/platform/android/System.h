/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

/*---AUTO_GEN_PARSE<$$CORE_SYSTEM_NAME:android>---*/

#pragma once

#include "../../AddonBase.h"
#include "../../c-api/platform/android/system.h"

#ifdef __cplusplus
namespace kodi
{
namespace platform
{

//==============================================================================
/// @defgroup cpp_kodi_platform_CInterfaceAndroidSystem class CInterfaceAndroidSystem
/// @ingroup cpp_kodi_platform
/// @brief **Android platform specific functions**\n
/// C++ class to query Android specific things in Kodi.
///
/// It has the header is @ref System.h "#include <kodi/platform/android/System.h>".
///
/// ----------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/platform/android/System.h>
///
/// #if defined(ANDROID)
/// kodi::platform::CInterfaceAndroidSystem system;
/// if (system.GetSDKVersion() >= 23)
/// {
///   ...
/// }
/// #endif
/// ~~~~~~~~~~~~~
///
class ATTR_DLL_LOCAL CInterfaceAndroidSystem
{
public:
  CInterfaceAndroidSystem()
    : m_interface(static_cast<AddonToKodiFuncTable_android_system*>(kodi::addon::GetInterface(
          INTERFACE_ANDROID_SYSTEM_NAME, INTERFACE_ANDROID_SYSTEM_VERSION)))
  {
  }

  //============================================================================
  /// @ingroup cpp_kodi_platform_CInterfaceAndroidSystem
  /// @brief Request an JNI env pointer for the calling thread.
  ///
  /// JNI env has to be controlled by kodi because of the underlying
  /// threading concep.
  ///
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
  /// @ingroup cpp_kodi_platform_CInterfaceAndroidSystem
  /// @brief Request the android sdk version to e.g. initialize <b>`JNIBase`</b>.
  ///
  /// @return Android SDK version
  ///
  inline int GetSDKVersion()
  {
    if (m_interface)
      return m_interface->get_sdk_version();

    return 0;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_platform_CInterfaceAndroidSystem
  /// @brief Request the android main class name e.g. <b>`org.xbmc.kodi`</b>.
  ///
  /// @return package class name
  ///
  inline std::string GetClassName()
  {
    if (m_interface)
      return m_interface->get_class_name();

    return std::string();
  }
  //----------------------------------------------------------------------------

private:
  AddonToKodiFuncTable_android_system* m_interface;
};
//------------------------------------------------------------------------------

} /* namespace platform */
} /* namespace kodi */
#endif /* __cplusplus */
