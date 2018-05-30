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

#pragma once

#include "../../AddonBase.h"

/*
 * For interface between add-on and kodi.
 *
 * This structure defines the addresses of functions stored inside Kodi which
 * are then available for the add-on to call
 *
 * All function pointers there are used by the C++ interface functions below.
 * You find the set of them on xbmc/addons/interfaces/General.cpp
 *
 * Note: For add-on development itself this is not needed
 */

static const char* INTERFACE_ANDROID_SYSTEM_NAME = "ANDROID_SYSTEM";
static const char* INTERFACE_ANDROID_SYSTEM_VERSION = "1.0.1";
static const char* INTERFACE_ANDROID_SYSTEM_VERSION_MIN = "1.0.1";

struct AddonToKodiFuncTable_android_system
{
  void* (*get_jni_env)();
  int (*get_sdk_version)();
  const char *(*get_class_name)();
};

//==============================================================================
///
/// \defgroup cpp_kodi_platform  Interface - kodi::platform
/// \ingroup cpp
/// @brief **Android platform specific functions**
///
/// #include <kodi/platform/android/System.h>"
///
//------------------------------------------------------------------------------

namespace kodi
{
namespace platform
{
  class CInterfaceAndroidSystem
  {
  public:
    CInterfaceAndroidSystem()
     : m_interface(static_cast<AddonToKodiFuncTable_android_system*>(GetInterface(INTERFACE_ANDROID_SYSTEM_NAME, INTERFACE_ANDROID_SYSTEM_VERSION)))
     {};

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
    inline void * GetJNIEnv()
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
    AddonToKodiFuncTable_android_system *m_interface;
  };
  //----------------------------------------------------------------------------
} /* namespace platform */
} /* namespace kodi */
