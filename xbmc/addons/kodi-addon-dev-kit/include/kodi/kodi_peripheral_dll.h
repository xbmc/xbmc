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
#ifndef __PERIPHERAL_DLL_H__
#define __PERIPHERAL_DLL_H__

#include "kodi_peripheral_types.h"

#define PERIPHERAL_ADDON_JOYSTICKS //! @todo

#ifdef __cplusplus
extern "C"
{
#endif

  /// @name Peripheral operations
  ///{
  /*!
   * @brief  Get the PERIPHERAL_API_VERSION used to compile this peripheral add-on
   * @return KODI_PERIPHERAL_API_VERSION from kodi_peripheral_types.h
   * @remarks Valid implementation required
   *
   * Used to check if the implementation is compatible with the frontend.
   */
  const char* GetPeripheralAPIVersion(void);

  /*!
   * @brief Get the KODI_PERIPHERAL_MIN_API_VERSION used to compile this peripheral add-on
   * @return KODI_PERIPHERAL_MIN_API_VERSION from kodi_peripheral_types.h
   * @remarks Valid implementation required
   *
   * Used to check if the implementation is compatible with the frontend.
   */
  const char* GetMinimumPeripheralAPIVersion(void);

  /*!
   * @brief Get the list of features that this add-on provides
   * @param pCapabilities The add-on's capabilities.
   * @return PERIPHERAL_NO_ERROR if the properties were fetched successfully.
   * @remarks Valid implementation required.
   *
   * Called by the frontend to query the add-on's capabilities and supported
   * peripherals. All capabilities that the add-on supports should be set to true.
   *
   */
  PERIPHERAL_ERROR GetAddonCapabilities(PERIPHERAL_CAPABILITIES *pCapabilities);

  /*!
   * @brief Perform a scan for joysticks
   * @param peripheral_count  Assigned to the number of peripherals allocated
   * @param scan_results      Assigned to allocated memory
   * @return PERIPHERAL_NO_ERROR if successful; peripherals must be freed using
   * FreeScanResults() in this case
   *
   * The frontend calls this when a hardware change is detected. If an add-on
   * detects a hardware change, it can trigger this function using the
   * TriggerScan() callback.
   */
  PERIPHERAL_ERROR PerformDeviceScan(unsigned int* peripheral_count, PERIPHERAL_INFO** scan_results);

  /*!
   * @brief Free the memory allocated in PerformDeviceScan()
   *
   * Must be called if PerformDeviceScan() returns PERIPHERAL_NO_ERROR.
   *
   * @param peripheral_count  The number of events allocated for the events array
   * @param scan_results      The array of allocated peripherals
   */
  void FreeScanResults(unsigned int peripheral_count, PERIPHERAL_INFO* scan_results);

  /*!
   * @brief Get all events that have occurred since the last call to GetEvents()
   * @return PERIPHERAL_NO_ERROR if successful; events must be freed using
   * FreeEvents() in this case
   */
  PERIPHERAL_ERROR GetEvents(unsigned int* event_count, PERIPHERAL_EVENT** events);

  /*!
   * @brief Free the memory allocated in GetEvents()
   *
   * Must be called if GetEvents() returns PERIPHERAL_NO_ERROR.
   *
   * @param event_count  The number of events allocated for the events array
   * @param events       The array of allocated events
   */
  void FreeEvents(unsigned int event_count, PERIPHERAL_EVENT* events);

  /*!
   * @brief Send an input event to the specified peripheral
   * @param peripheralIndex The index of the device receiving the input event
   * @param event The input event
   * @return true if the event was handled, false otherwise
   */
  bool SendEvent(const PERIPHERAL_EVENT* event);
  ///}

  /// @name Joystick operations
  /*!
   * @note #define PERIPHERAL_ADDON_JOYSTICKS before including kodi_peripheral_dll.h
   * in the add-on if the add-on provides joysticks and add provides_joysticks="true"
   * to the kodi.peripheral extension point node in addon.xml.
   */
  ///{
#ifdef PERIPHERAL_ADDON_JOYSTICKS
  /*!
   * @brief Get extended info about an attached joystick
   * @param index  The joystick's driver index
   * @param info   The container for the allocated joystick info
   * @return PERIPHERAL_NO_ERROR if successful; array must be freed using
   *         FreeJoystickInfo() in this case
   */
  PERIPHERAL_ERROR GetJoystickInfo(unsigned int index, JOYSTICK_INFO* info);

  /*!
   * @brief Free the memory allocated in GetJoystickInfo()
   */
  void FreeJoystickInfo(JOYSTICK_INFO* info);

  /*!
   * @brief Get the features that allow translating the joystick into the controller profile
   * @param joystick      The device's joystick properties; unknown values may be left at their default
   * @param controller_id The controller profile being requested, e.g. game.controller.default
   * @param feature_count The number of features allocated for the features array
   * @param features      The array of allocated features
   * @return PERIPHERAL_NO_ERROR if successful; array must be freed using
   *         FreeButtonMap() in this case
   */
  PERIPHERAL_ERROR GetFeatures(const JOYSTICK_INFO* joystick, const char* controller_id,
                               unsigned int* feature_count, JOYSTICK_FEATURE** features);

  /*!
   * @brief Free the memory allocated in GetFeatures()
   *
   * Must be called if GetFeatures() returns PERIPHERAL_NO_ERROR.
   *
   * @param feature_count  The number of features allocated for the features array
   * @param features       The array of allocated features
   */
  void FreeFeatures(unsigned int feature_count, JOYSTICK_FEATURE* features);

  /*!
   * @brief Add or update joystick features
   * @param joystick      The device's joystick properties; unknown values may be left at their default
   * @param controller_id The game controller profile being updated
   * @param feature_count The number of features int the features array
   * @param features      The array of features
   * @return PERIPHERAL_NO_ERROR if successful
   */
  PERIPHERAL_ERROR MapFeatures(const JOYSTICK_INFO* joystick, const char* controller_id,
                               unsigned int feature_count, JOYSTICK_FEATURE* features);

  /*!
   * @brief Reset the button map for the given joystick and controller profile ID
   * @param joystick      The device's joystick properties
   * @param controller_id The game controller profile being reset
   */
  void ResetButtonMap(const JOYSTICK_INFO* joystick, const char* controller_id);
  
  /*!
   * @brief Powers off the given joystick if supported
   * @param index  The joystick's driver index
   */
  void PowerOffJoystick(unsigned int index);
#endif
  ///}

  /*!
   * Called by the frontend to assign the function pointers of this add-on to
   * pClient. Note that get_addon() is defined here, so it will be available in
   * all compiled peripheral add-ons.
   */
  void __declspec(dllexport) get_addon(struct PeripheralAddon* pClient)
  {
    pClient->GetPeripheralAPIVersion        = GetPeripheralAPIVersion;
    pClient->GetMinimumPeripheralAPIVersion = GetMinimumPeripheralAPIVersion;
    pClient->GetAddonCapabilities           = GetAddonCapabilities;
    pClient->PerformDeviceScan              = PerformDeviceScan;
    pClient->FreeScanResults                = FreeScanResults;
    pClient->GetEvents                      = GetEvents;
    pClient->FreeEvents                     = FreeEvents;
    pClient->SendEvent                      = SendEvent;

#ifdef PERIPHERAL_ADDON_JOYSTICKS
    pClient->GetJoystickInfo                = GetJoystickInfo;
    pClient->FreeJoystickInfo               = FreeJoystickInfo;
    pClient->GetFeatures                    = GetFeatures;
    pClient->FreeFeatures                   = FreeFeatures;
    pClient->MapFeatures                    = MapFeatures;
    pClient->ResetButtonMap                 = ResetButtonMap;
    pClient->PowerOffJoystick               = PowerOffJoystick;
#endif
  }

#ifdef __cplusplus
}
#endif

#endif // __PERIPHERAL_DLL_H__
