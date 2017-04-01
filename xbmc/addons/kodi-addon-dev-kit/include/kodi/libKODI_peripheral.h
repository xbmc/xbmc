/*
 *      Copyright (C) 2014-2017 Team Kodi
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
#include "kodi_peripheral_types.h"

#include <string>
#include <stdio.h>

#if defined(ANDROID)
  #include <sys/stat.h>
#endif

extern "C"
{
namespace KodiAPI
{
namespace Peripheral
{

typedef struct CB_PeripheralLib
{
  void (*TriggerScan)(void* addonData);
  void (*RefreshButtonMaps)(void* addonData, const char* deviceName, const char* controllerId);
  unsigned int (*FeatureCount)(void* addonData, const char* controllerId, JOYSTICK_FEATURE_TYPE type);
} CB_PeripheralLib;

} /* namespace Peripheral */
} /* namespace KodiAPI */
} /* extern "C" */

namespace ADDON
{

class CHelper_libKODI_peripheral
{
public:
  CHelper_libKODI_peripheral(void)
  {
    m_Handle = nullptr;
    m_Callbacks = nullptr;
  }

  ~CHelper_libKODI_peripheral(void)
  {
    if (m_Handle && m_Callbacks)
    {
      m_Handle->PeripheralLib_UnRegisterMe(m_Handle->addonData, m_Callbacks);
    }
  }

  /*!
    * @brief Resolve all callback methods
    * @param handle Pointer to the add-on
    * @return True when all methods were resolved, false otherwise.
    */
  bool RegisterMe(void* handle)
  {
    m_Handle = static_cast<AddonCB*>(handle);
    if (m_Handle)
      m_Callbacks = (KodiAPI::Peripheral::CB_PeripheralLib*)m_Handle->PeripheralLib_RegisterMe(m_Handle->addonData);
    if (!m_Callbacks)
      fprintf(stderr, "libXBMC_peripheral-ERROR: PeripheralLib_register_me can't get callback table from Kodi !!!\n");

    return m_Callbacks != nullptr;
  }

  /*!
   * @brief Trigger a scan for peripherals
   *
   * The add-on calls this if a change in hardware is detected.
   */
  void TriggerScan(void)
  {
    return m_Callbacks->TriggerScan(m_Handle->addonData);
  }

  /*!
   * @brief Notify the frontend that button maps have changed
   *
   * @param[optional] deviceName The name of the device to refresh, or empty/null for all devices
   * @param[optional] controllerId The controller ID to refresh, or empty/null for all controllers
   */
  void RefreshButtonMaps(const std::string& strDeviceName = "", const std::string& strControllerId = "")
  {
    return m_Callbacks->RefreshButtonMaps(m_Handle->addonData, strDeviceName.c_str(), strControllerId.c_str());
  }

  /*!
   * @brief Return the number of features belonging to the specified controller
   *
   * @param controllerId    The controller ID to enumerate
   * @param type[optional]  Type to filter by, or JOYSTICK_FEATURE_TYPE_UNKNOWN for all features
   *
   * @return The number of features matching the request parameters
   */
  unsigned int FeatureCount(const std::string& strControllerId, JOYSTICK_FEATURE_TYPE type = JOYSTICK_FEATURE_TYPE_UNKNOWN)
  {
    return m_Callbacks->FeatureCount(m_Handle->addonData, strControllerId.c_str(), type);
  }

private:
  AddonCB* m_Handle;
  KodiAPI::Peripheral::CB_PeripheralLib* m_Callbacks;
};

} /* namespace ADDON */
