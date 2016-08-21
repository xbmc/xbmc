/*
 *      Copyright (C) 2014-2016 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "libXBMC_addon.h"
#include "kodi_peripheral_callbacks.h"

#include <string>
#include <stdio.h>

#if defined(ANDROID)
  #include <sys/stat.h>
#endif

#ifdef _WIN32
  #define PERIPHERAL_HELPER_DLL "\\library.kodi.peripheral\\libKODI_peripheral" ADDON_HELPER_EXT
#else
  #define PERIPHERAL_HELPER_DLL_NAME "libKODI_peripheral-" ADDON_HELPER_ARCH ADDON_HELPER_EXT
  #define PERIPHERAL_HELPER_DLL "/library.kodi.peripheral/" PERIPHERAL_HELPER_DLL_NAME
#endif

#define PERIPHERAL_REGISTER_SYMBOL(dll, functionPtr) \
  CHelper_libKODI_peripheral::RegisterSymbol(dll, functionPtr, #functionPtr)

namespace ADDON
{

class CHelper_libKODI_peripheral
{
public:
  CHelper_libKODI_peripheral(void)
  {
    m_handle       = NULL;
    m_callbacks    = NULL;
    m_libKODI_peripheral = NULL;
  }

  ~CHelper_libKODI_peripheral(void)
  {
    if (m_libKODI_peripheral)
    {
      PERIPHERAL_unregister_me(m_handle, m_callbacks);
      dlclose(m_libKODI_peripheral);
    }
  }

  template <typename T>
  static bool RegisterSymbol(void* dll, T& functionPtr, const char* strFunctionPtr)
  {
    if ((functionPtr = (T)dlsym(dll, strFunctionPtr)) == NULL)
    {
      fprintf(stderr, "ERROR: Unable to assign function %s: %s\n", strFunctionPtr, dlerror());
      return false;
    }
    return true;
  }

  /*!
    * @brief Resolve all callback methods
    * @param handle Pointer to the add-on
    * @return True when all methods were resolved, false otherwise.
    */
  bool RegisterMe(void* handle)
  {
    m_handle = handle;

    std::string libBasePath;
    libBasePath  = ((cb_array*)m_handle)->libPath;
    libBasePath += PERIPHERAL_HELPER_DLL;

#if defined(ANDROID)
      struct stat st;
      if (stat(libBasePath.c_str(),&st) != 0)
      {
        std::string tempbin = getenv("XBMC_ANDROID_LIBS");
        libBasePath = tempbin + "/" + PERIPHERAL_HELPER_DLL_NAME;
      }
#endif

    m_libKODI_peripheral = dlopen(libBasePath.c_str(), RTLD_LAZY);
    if (m_libKODI_peripheral == NULL)
    {
      fprintf(stderr, "Unable to load %s\n", dlerror());
      return false;
    }

    if (!PERIPHERAL_REGISTER_SYMBOL(m_libKODI_peripheral, PERIPHERAL_register_me)) return false;
    if (!PERIPHERAL_REGISTER_SYMBOL(m_libKODI_peripheral, PERIPHERAL_unregister_me)) return false;
    if (!PERIPHERAL_REGISTER_SYMBOL(m_libKODI_peripheral, PERIPHERAL_trigger_scan)) return false;
    if (!PERIPHERAL_REGISTER_SYMBOL(m_libKODI_peripheral, PERIPHERAL_refresh_button_maps)) return false;
    if (!PERIPHERAL_REGISTER_SYMBOL(m_libKODI_peripheral, PERIPHERAL_feature_count)) return false;

    m_callbacks = PERIPHERAL_register_me(m_handle);
    return m_callbacks != NULL;
  }

  void TriggerScan(void)
  {
    return PERIPHERAL_trigger_scan(m_handle, m_callbacks);
  }

  void RefreshButtonMaps(const std::string& strDeviceName = "", const std::string& strControllerId = "")
  {
    return PERIPHERAL_refresh_button_maps(m_handle, m_callbacks, strDeviceName.c_str(), strControllerId.c_str());
  }

  unsigned int FeatureCount(const std::string& strControllerId, JOYSTICK_FEATURE_TYPE type = JOYSTICK_FEATURE_TYPE_UNKNOWN)
  {
    return PERIPHERAL_feature_count(m_handle, m_callbacks, strControllerId.c_str(), type);
  }

protected:
    CB_PeripheralLib* (*PERIPHERAL_register_me)(void* handle);
    void (*PERIPHERAL_unregister_me)(void* handle, CB_PeripheralLib* cb);
    void (*PERIPHERAL_trigger_scan)(void* handle, CB_PeripheralLib* cb);
    void (*PERIPHERAL_refresh_button_maps)(void* handle, CB_PeripheralLib* cb, const char* deviceName, const char* controllerId);
    unsigned int (*PERIPHERAL_feature_count)(void* handle, CB_PeripheralLib* cb, const char* controllerId, JOYSTICK_FEATURE_TYPE type);

private:
  void*             m_handle;
  CB_PeripheralLib* m_callbacks;
  void*             m_libKODI_peripheral;

  struct cb_array
  {
    const char* libPath;
  };
};

}
