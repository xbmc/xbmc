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
#ifndef __PERIPHERAL_CALLBACKS_H__
#define __PERIPHERAL_CALLBACKS_H__

#include "kodi_peripheral_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct CB_PeripheralLib
{
  /*!
   * @brief Trigger a scan for peripherals
   *
   * The add-on calls this if a change in hardware is detected.
   */
  void (*TriggerScan)(void* addonData);

  /*!
   * @brief Notify the frontend that button maps have changed
   *
   * @param[optional] deviceName The name of the device to refresh, or empty/null for all devices
   * @param[optional] controllerId The controller ID to refresh, or empty/null for all controllers
   */
  void (*RefreshButtonMaps)(void* addonData, const char* deviceName, const char* controllerId);

} CB_PeripheralLib;

#ifdef __cplusplus
}
#endif

#endif // __PERIPHERAL_CALLBACKS_H__
