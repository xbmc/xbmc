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
      m_Callbacks = (CB_PeripheralLib*)m_Handle->PeripheralLib_RegisterMe(m_Handle->addonData);
    if (!m_Callbacks)
      fprintf(stderr, "libXBMC_pvr-ERROR: PVRLib_register_me can't get callback table from Kodi !!!\n");

    return m_Callbacks != nullptr;
  }

  void TriggerScan(void)
  {
    return m_Callbacks->TriggerScan(m_Handle->addonData);
  }

  void RefreshButtonMaps(const std::string& strDeviceName = "", const std::string& strControllerId = "")
  {
    return m_Callbacks->RefreshButtonMaps(m_Handle->addonData, strDeviceName.c_str(), strControllerId.c_str());
  }

  unsigned int FeatureCount(const std::string& strControllerId, JOYSTICK_FEATURE_TYPE type = JOYSTICK_FEATURE_TYPE_UNKNOWN)
  {
    return m_Callbacks->FeatureCount(m_Handle->addonData, strControllerId.c_str(), type);
  }

private:
  AddonCB* m_Handle;
  CB_PeripheralLib* m_Callbacks;
};

}
