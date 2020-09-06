/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"
#include "peripheral/PeripheralUtils.h"

#ifdef __cplusplus
namespace kodi
{
namespace addon
{

class ATTRIBUTE_HIDDEN CInstancePeripheral : public IAddonInstance
{
public:
  CInstancePeripheral()
    : IAddonInstance(ADDON_INSTANCE_PERIPHERAL, GetKodiTypeVersion(ADDON_INSTANCE_PERIPHERAL))
  {
    if (CAddonBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstancePeripheral: Creation of more as one in single "
                             "instance way is not allowed!");

    SetAddonStruct(CAddonBase::m_interface->firstKodiInstance);
    CAddonBase::m_interface->globalSingleInstance = this;
  }

  explicit CInstancePeripheral(KODI_HANDLE instance, const std::string& kodiVersion = "")
    : IAddonInstance(ADDON_INSTANCE_PERIPHERAL,
                     !kodiVersion.empty() ? kodiVersion
                                          : GetKodiTypeVersion(ADDON_INSTANCE_PERIPHERAL))
  {
    if (CAddonBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstancePeripheral: Creation of multiple together with "
                             "single instance way is not allowed!");

    SetAddonStruct(instance);
  }

  ~CInstancePeripheral() override = default;

  /// @name Peripheral operations
  ///{
  /*!
   * @brief Get the list of features that this add-on provides
   * @param capabilities The add-on's capabilities.
   * @remarks Valid implementation required.
   *
   * Called by the frontend to query the add-on's capabilities and supported
   * peripherals. All capabilities that the add-on supports should be set to true.
   *
   */
  virtual void GetCapabilities(PERIPHERAL_CAPABILITIES& capabilities) {}

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
  virtual PERIPHERAL_ERROR PerformDeviceScan(unsigned int* peripheral_count,
                                             PERIPHERAL_INFO** scan_results)
  {
    return PERIPHERAL_ERROR_NOT_IMPLEMENTED;
  }

  /*!
   * @brief Free the memory allocated in PerformDeviceScan()
   *
   * Must be called if PerformDeviceScan() returns PERIPHERAL_NO_ERROR.
   *
   * @param peripheral_count  The number of events allocated for the events array
   * @param scan_results      The array of allocated peripherals
   */
  virtual void FreeScanResults(unsigned int peripheral_count, PERIPHERAL_INFO* scan_results) {}

  /*!
   * @brief Get all events that have occurred since the last call to GetEvents()
   * @return PERIPHERAL_NO_ERROR if successful; events must be freed using
   * FreeEvents() in this case
   */
  virtual PERIPHERAL_ERROR GetEvents(unsigned int* event_count, PERIPHERAL_EVENT** events)
  {
    return PERIPHERAL_ERROR_NOT_IMPLEMENTED;
  }

  /*!
   * @brief Free the memory allocated in GetEvents()
   *
   * Must be called if GetEvents() returns PERIPHERAL_NO_ERROR.
   *
   * @param event_count  The number of events allocated for the events array
   * @param events       The array of allocated events
   */
  virtual void FreeEvents(unsigned int event_count, PERIPHERAL_EVENT* events) {}

  /*!
   * @brief Send an input event to the peripheral
   * @param event The input event
   * @return true if the event was handled, false otherwise
   */
  virtual bool SendEvent(const PERIPHERAL_EVENT* event) { return false; }
  ///}

  /// @name Joystick operations
  /*!
   * @note #define PERIPHERAL_ADDON_JOYSTICKS before including kodi_peripheral_dll.h
   * in the add-on if the add-on provides joysticks and add provides_joysticks="true"
   * to the kodi.peripheral extension point node in addon.xml.
   */
  ///{
  /*!
   * @brief Get extended info about an attached joystick
   * @param index  The joystick's driver index
   * @param info   The container for the allocated joystick info
   * @return PERIPHERAL_NO_ERROR if successful; array must be freed using
   *         FreeJoystickInfo() in this case
   */
  virtual PERIPHERAL_ERROR GetJoystickInfo(unsigned int index, JOYSTICK_INFO* info)
  {
    return PERIPHERAL_ERROR_NOT_IMPLEMENTED;
  }

  /*!
   * @brief Free the memory allocated in GetJoystickInfo()
   */
  virtual void FreeJoystickInfo(JOYSTICK_INFO* info) {}

  /*!
   * @brief Get the features that allow translating the joystick into the controller profile
   * @param joystick      The device's joystick properties; unknown values may be left at their default
   * @param controller_id The controller profile being requested, e.g. game.controller.default
   * @param feature_count The number of features allocated for the features array
   * @param features      The array of allocated features
   * @return PERIPHERAL_NO_ERROR if successful; array must be freed using
   *         FreeButtonMap() in this case
   */
  virtual PERIPHERAL_ERROR GetFeatures(const JOYSTICK_INFO* joystick,
                                       const char* controller_id,
                                       unsigned int* feature_count,
                                       JOYSTICK_FEATURE** features)
  {
    return PERIPHERAL_ERROR_NOT_IMPLEMENTED;
  }

  /*!
   * @brief Free the memory allocated in GetFeatures()
   *
   * Must be called if GetFeatures() returns PERIPHERAL_NO_ERROR.
   *
   * @param feature_count  The number of features allocated for the features array
   * @param features       The array of allocated features
   */
  virtual void FreeFeatures(unsigned int feature_count, JOYSTICK_FEATURE* features) {}

  /*!
   * @brief Add or update joystick features
   * @param joystick      The device's joystick properties; unknown values may be left at their default
   * @param controller_id The game controller profile being updated
   * @param feature_count The number of features in the features array
   * @param features      The array of features
   * @return PERIPHERAL_NO_ERROR if successful
   */
  virtual PERIPHERAL_ERROR MapFeatures(const JOYSTICK_INFO* joystick,
                                       const char* controller_id,
                                       unsigned int feature_count,
                                       const JOYSTICK_FEATURE* features)
  {
    return PERIPHERAL_ERROR_NOT_IMPLEMENTED;
  }

  /*!
   * @brief Get the driver primitives that should be ignored while mapping the device
   * @param joystick        The device's joystick properties; unknown values may be left at their default
   * @param primitive_count The number of features allocated for the primitives array
   * @param primitives      The array of allocated driver primitives to be ignored
   * @return PERIPHERAL_NO_ERROR if successful; array must be freed using
   *         FreePrimitives() in this case
   */
  virtual PERIPHERAL_ERROR GetIgnoredPrimitives(const JOYSTICK_INFO* joystick,
                                                unsigned int* primitive_count,
                                                JOYSTICK_DRIVER_PRIMITIVE** primitives)
  {
    return PERIPHERAL_ERROR_NOT_IMPLEMENTED;
  }

  /*!
   * @brief Free the memory allocated in GetIgnoredPrimitives()
   *
   * Must be called if GetIgnoredPrimitives() returns PERIPHERAL_NO_ERROR.
   *
   * @param primitive_count  The number of driver primitives allocated for the primitives array
   * @param primitives       The array of allocated driver primitives
   */
  virtual void FreePrimitives(unsigned int primitive_count, JOYSTICK_DRIVER_PRIMITIVE* primitives)
  {
  }

  /*!
   * @brief Set the list of driver primitives that are ignored for the device
   * @param joystick         The device's joystick properties; unknown values may be left at their default
   * @param primitive_count  The number of driver features in the primitives array
   * @param primitives       The array of driver primitives to ignore
   * @return PERIPHERAL_NO_ERROR if successful
   */
  virtual PERIPHERAL_ERROR SetIgnoredPrimitives(const JOYSTICK_INFO* joystick,
                                                unsigned int primitive_count,
                                                const JOYSTICK_DRIVER_PRIMITIVE* primitives)
  {
    return PERIPHERAL_ERROR_NOT_IMPLEMENTED;
  }

  /*!
   * @brief Save the button map for the given joystick
   * @param joystick      The device's joystick properties
   */
  virtual void SaveButtonMap(const JOYSTICK_INFO* joystick) {}

  /*!
   * @brief Revert the button map to the last time it was loaded or committed to disk
   * @param joystick      The device's joystick properties
   */
  virtual void RevertButtonMap(const JOYSTICK_INFO* joystick) {}

  /*!
   * @brief Reset the button map for the given joystick and controller profile ID
   * @param joystick      The device's joystick properties
   * @param controller_id The game controller profile being reset
   */
  virtual void ResetButtonMap(const JOYSTICK_INFO* joystick, const char* controller_id) {}

  /*!
   * @brief Powers off the given joystick if supported
   * @param index  The joystick's driver index
   */
  virtual void PowerOffJoystick(unsigned int index) {}

  const std::string AddonPath() const { return m_instanceData->props->addon_path; }

  const std::string UserPath() const { return m_instanceData->props->user_path; }

  /*!
   * @brief Trigger a scan for peripherals
   *
   * The add-on calls this if a change in hardware is detected.
   */
  void TriggerScan(void)
  {
    return m_instanceData->toKodi->trigger_scan(m_instanceData->toKodi->kodiInstance);
  }

  /*!
   * @brief Notify the frontend that button maps have changed
   *
   * @param[optional] deviceName The name of the device to refresh, or empty/null for all devices
   * @param[optional] controllerId The controller ID to refresh, or empty/null for all controllers
   */
  void RefreshButtonMaps(const std::string& deviceName = "", const std::string& controllerId = "")
  {
    return m_instanceData->toKodi->refresh_button_maps(m_instanceData->toKodi->kodiInstance,
                                                       deviceName.c_str(), controllerId.c_str());
  }

  /*!
   * @brief Return the number of features belonging to the specified controller
   *
   * @param controllerId    The controller ID to enumerate
   * @param type[optional]  Type to filter by, or JOYSTICK_FEATURE_TYPE_UNKNOWN for all features
   *
   * @return The number of features matching the request parameters
   */
  unsigned int FeatureCount(const std::string& controllerId,
                            JOYSTICK_FEATURE_TYPE type = JOYSTICK_FEATURE_TYPE_UNKNOWN)
  {
    return m_instanceData->toKodi->feature_count(m_instanceData->toKodi->kodiInstance,
                                                 controllerId.c_str(), type);
  }

  /*!
   * @brief Return the type of the feature
   *
   * @param controllerId    The controller ID to check
   * @param featureName     The feature to check
   *
   * @return The type of the specified feature, or JOYSTICK_FEATURE_TYPE_UNKNOWN
   * if unknown
   */
  JOYSTICK_FEATURE_TYPE FeatureType(const std::string& controllerId, const std::string& featureName)
  {
    return m_instanceData->toKodi->feature_type(m_instanceData->toKodi->kodiInstance,
                                                controllerId.c_str(), featureName.c_str());
  }

private:
  void SetAddonStruct(KODI_HANDLE instance)
  {
    if (instance == nullptr)
      throw std::logic_error("kodi::addon::CInstancePeripheral: Creation with empty addon "
                             "structure not allowed, table must be given from Kodi!");

    m_instanceData = static_cast<AddonInstance_Peripheral*>(instance);
    m_instanceData->toAddon->addonInstance = this;

    m_instanceData->toAddon->get_capabilities = ADDON_GetCapabilities;
    m_instanceData->toAddon->perform_device_scan = ADDON_PerformDeviceScan;
    m_instanceData->toAddon->free_scan_results = ADDON_FreeScanResults;
    m_instanceData->toAddon->get_events = ADDON_GetEvents;
    m_instanceData->toAddon->free_events = ADDON_FreeEvents;
    m_instanceData->toAddon->send_event = ADDON_SendEvent;

    m_instanceData->toAddon->get_joystick_info = ADDON_GetJoystickInfo;
    m_instanceData->toAddon->free_joystick_info = ADDON_FreeJoystickInfo;
    m_instanceData->toAddon->get_features = ADDON_GetFeatures;
    m_instanceData->toAddon->free_features = ADDON_FreeFeatures;
    m_instanceData->toAddon->map_features = ADDON_MapFeatures;
    m_instanceData->toAddon->get_ignored_primitives = ADDON_GetIgnoredPrimitives;
    m_instanceData->toAddon->free_primitives = ADDON_FreePrimitives;
    m_instanceData->toAddon->set_ignored_primitives = ADDON_SetIgnoredPrimitives;
    m_instanceData->toAddon->save_button_map = ADDON_SaveButtonMap;
    m_instanceData->toAddon->revert_button_map = ADDON_RevertButtonMap;
    m_instanceData->toAddon->reset_button_map = ADDON_ResetButtonMap;
    m_instanceData->toAddon->power_off_joystick = ADDON_PowerOffJoystick;
  }

  inline static void ADDON_GetCapabilities(const AddonInstance_Peripheral* addonInstance,
                                           PERIPHERAL_CAPABILITIES* capabilities)
  {
    static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->GetCapabilities(*capabilities);
  }

  inline static PERIPHERAL_ERROR ADDON_PerformDeviceScan(
      const AddonInstance_Peripheral* addonInstance,
      unsigned int* peripheral_count,
      PERIPHERAL_INFO** scan_results)
  {
    return static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->PerformDeviceScan(peripheral_count, scan_results);
  }

  inline static void ADDON_FreeScanResults(const AddonInstance_Peripheral* addonInstance,
                                           unsigned int peripheral_count,
                                           PERIPHERAL_INFO* scan_results)
  {
    static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->FreeScanResults(peripheral_count, scan_results);
  }

  inline static PERIPHERAL_ERROR ADDON_GetEvents(const AddonInstance_Peripheral* addonInstance,
                                                 unsigned int* event_count,
                                                 PERIPHERAL_EVENT** events)
  {
    return static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->GetEvents(event_count, events);
  }

  inline static void ADDON_FreeEvents(const AddonInstance_Peripheral* addonInstance,
                                      unsigned int event_count,
                                      PERIPHERAL_EVENT* events)
  {
    static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->FreeEvents(event_count, events);
  }

  inline static bool ADDON_SendEvent(const AddonInstance_Peripheral* addonInstance,
                                     const PERIPHERAL_EVENT* event)
  {
    return static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->SendEvent(event);
  }


  inline static PERIPHERAL_ERROR ADDON_GetJoystickInfo(
      const AddonInstance_Peripheral* addonInstance, unsigned int index, JOYSTICK_INFO* info)
  {
    return static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->GetJoystickInfo(index, info);
  }

  inline static void ADDON_FreeJoystickInfo(const AddonInstance_Peripheral* addonInstance,
                                            JOYSTICK_INFO* info)
  {
    static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->FreeJoystickInfo(info);
  }

  inline static PERIPHERAL_ERROR ADDON_GetFeatures(const AddonInstance_Peripheral* addonInstance,
                                                   const JOYSTICK_INFO* joystick,
                                                   const char* controller_id,
                                                   unsigned int* feature_count,
                                                   JOYSTICK_FEATURE** features)
  {
    return static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->GetFeatures(joystick, controller_id, feature_count, features);
  }

  inline static void ADDON_FreeFeatures(const AddonInstance_Peripheral* addonInstance,
                                        unsigned int feature_count,
                                        JOYSTICK_FEATURE* features)
  {
    static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->FreeFeatures(feature_count, features);
  }

  inline static PERIPHERAL_ERROR ADDON_MapFeatures(const AddonInstance_Peripheral* addonInstance,
                                                   const JOYSTICK_INFO* joystick,
                                                   const char* controller_id,
                                                   unsigned int feature_count,
                                                   const JOYSTICK_FEATURE* features)
  {
    return static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->MapFeatures(joystick, controller_id, feature_count, features);
  }

  inline static PERIPHERAL_ERROR ADDON_GetIgnoredPrimitives(
      const AddonInstance_Peripheral* addonInstance,
      const JOYSTICK_INFO* joystick,
      unsigned int* primitive_count,
      JOYSTICK_DRIVER_PRIMITIVE** primitives)
  {
    return static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->GetIgnoredPrimitives(joystick, primitive_count, primitives);
  }

  inline static void ADDON_FreePrimitives(const AddonInstance_Peripheral* addonInstance,
                                          unsigned int primitive_count,
                                          JOYSTICK_DRIVER_PRIMITIVE* primitives)
  {
    static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->FreePrimitives(primitive_count, primitives);
  }

  inline static PERIPHERAL_ERROR ADDON_SetIgnoredPrimitives(
      const AddonInstance_Peripheral* addonInstance,
      const JOYSTICK_INFO* joystick,
      unsigned int primitive_count,
      const JOYSTICK_DRIVER_PRIMITIVE* primitives)
  {
    return static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->SetIgnoredPrimitives(joystick, primitive_count, primitives);
  }

  inline static void ADDON_SaveButtonMap(const AddonInstance_Peripheral* addonInstance,
                                         const JOYSTICK_INFO* joystick)
  {
    static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->SaveButtonMap(joystick);
  }

  inline static void ADDON_RevertButtonMap(const AddonInstance_Peripheral* addonInstance,
                                           const JOYSTICK_INFO* joystick)
  {
    static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->RevertButtonMap(joystick);
  }

  inline static void ADDON_ResetButtonMap(const AddonInstance_Peripheral* addonInstance,
                                          const JOYSTICK_INFO* joystick,
                                          const char* controller_id)
  {
    static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->ResetButtonMap(joystick, controller_id);
  }

  inline static void ADDON_PowerOffJoystick(const AddonInstance_Peripheral* addonInstance,
                                            unsigned int index)
  {
    static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->PowerOffJoystick(index);
  }

  AddonInstance_Peripheral* m_instanceData;
};

} /* namespace addon */
} /* namespace kodi */
#endif /* __cplusplus */
