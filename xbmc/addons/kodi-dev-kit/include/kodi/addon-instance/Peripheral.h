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

//##############################################################################
/// @defgroup cpp_kodi_addon_peripheral_Defs Definitions, structures and enumerators
/// @ingroup cpp_kodi_addon_peripheral
/// @brief %Peripheral add-on general variables
///
/// Used to exchange the available options between Kodi and addon.
///
///

//##############################################################################
/// @defgroup cpp_kodi_addon_peripheral_Defs_General 1. General
/// @ingroup cpp_kodi_addon_peripheral_Defs
/// @brief **%Peripheral add-on general variables**\n
/// Used to exchange the available options between Kodi and addon.
///
/// This group also includes @ref cpp_kodi_addon_peripheral_Defs_PeripheralCapabilities
/// with which Kodi an @ref kodi::addon::CInstancePeripheral::GetCapabilities()
/// queries the supported **modules** of the addon.
///

//##############################################################################
/// @defgroup cpp_kodi_addon_peripheral_Defs_Peripheral 2. Peripheral
/// @ingroup cpp_kodi_addon_peripheral_Defs
/// @brief **%Peripheral add-on operation variables**\n
/// Used to exchange the available options between Kodi and addon.
///

//##############################################################################
/// @defgroup cpp_kodi_addon_peripheral_Defs_Event 3. Event
/// @ingroup cpp_kodi_addon_peripheral_Defs
/// @brief **%Event add-on operation variables**\n
/// Used to exchange the available options between Kodi and addon.
///

//##############################################################################
/// @defgroup cpp_kodi_addon_peripheral_Defs_Joystick 4. Joystick
/// @ingroup cpp_kodi_addon_peripheral_Defs
/// @brief **%Joystick add-on operation variables**\n
/// Used to exchange the available options between Kodi and addon.
///

//==============================================================================
/// @addtogroup cpp_kodi_addon_peripheral
/// @brief \cpp_class{ kodi::addon::CInstancePeripheral }
/// **%Peripheral add-on instance**
///
/// The peripheral add-ons provides access to many joystick and gamepad
/// interfaces across various platforms. An input addon is used to map the
/// buttons/axis on your physical input device, to the buttons/axis of your
/// virtual system. This is necessary because different retro systems usually
/// have different button layouts. A controller configuration utility is also
/// in the works.
///
/// ----------------------------------------------------------------------------
///
/// Here is an example of what the <b>`addon.xml.in`</b> would look like for an
/// peripheral addon:
///
/// ~~~~~~~~~~~~~{.xml}
/// <?xml version="1.0" encoding="UTF-8"?>
/// <addon
///   id="peripheral.myspecialnamefor"
///   version="1.0.0"
///   name="My special peripheral addon"
///   provider-name="Your Name">
///   <requires>@ADDON_DEPENDS@</requires>
///   <extension
///     point="kodi.peripheral"
///     provides_joysticks="true"
///     provides_buttonmaps="true"
///     library_@PLATFORM@="@LIBRARY_FILENAME@"/>
///   <extension point="xbmc.addon.metadata">
///     <summary lang="en_GB">My peripheral addon</summary>
///     <description lang="en_GB">My peripheral addon description</description>
///     <platform>@PLATFORM@</platform>
///   </extension>
/// </addon>
/// ~~~~~~~~~~~~~
///
/// Description to peripheral related addon.xml values:
/// | Name                          | Description
/// |:------------------------------|----------------------------------------
/// | <b>`provides_joysticks`</b>   | Set to "true" if addon provides joystick support.
/// | <b>`provides_buttonmaps`</b>  | Set to "true" if button map is used and supported by addon.
/// | <b>`point`</b>                | Addon type specification<br>At all addon types and for this kind always <b>"kodi.peripheral"</b>.
/// | <b>`library_@PLATFORM@`</b>   | Sets the used library name, which is automatically set by cmake at addon build.
///
/// @remark For more detailed description of the <b>`addon.xml`</b>, see also https://kodi.wiki/view/Addon.xml.
///
///
/// --------------------------------------------------------------------------
///
/// **Here is an example of how addon can be used as a single:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/addon-instance/Peripheral.h>
///
/// class CMyPeripheralAddon : public kodi::addon::CAddonBase,
///                            public kodi::addon::CInstancePeripheral
/// {
/// public:
///   CMyPeripheralAddon();
///
///   void GetCapabilities(kodi::addon::PeripheralCapabilities& capabilities) override;
///   ...
/// };
///
/// CMyPeripheralAddon::CMyPeripheralAddon()
/// {
///   ...
/// }
///
/// void CMyPeripheralAddon::GetCapabilities(kodi::addon::PeripheralCapabilities& capabilities)
/// {
///   capabilities.SetProvidesJoysticks(true);
///   capabilities.SetProvidesButtonmaps(true);
///   ...
/// }
///
/// ADDONCREATOR(CMyPeripheralAddon)
/// ~~~~~~~~~~~~~
///
/// @note It is imperative to use the necessary functions of this class in the
/// addon.
///
/// --------------------------------------------------------------------------
///
///
/// **Here is another example where the peripheral is used together with
/// other instance types:**
///
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/addon-instance/Peripheral.h>
///
/// class CMyPeripheralAddon : public kodi::addon::CInstancePeripheral
/// {
/// public:
///   CMyPeripheralAddon(const kodi::addon::IInstanceInfo& instance);
///
///   void GetCapabilities(kodi::addon::PeripheralCapabilities& capabilities) override;
///   ...
/// };
///
/// CMyPeripheralAddon::CMyPeripheralAddon(const kodi::addon::IInstanceInfo& instance)
///   : CInstancePeripheral(instance)
/// {
///   ...
/// }
///
/// void CMyPeripheralAddon::GetCapabilities(kodi::addon::PeripheralCapabilities& capabilities)
/// {
///   capabilities.SetProvidesJoysticks(true);
///   capabilities.SetProvidesButtonmaps(true);
///   ...
/// }
///
/// //----------------------------------------------------------------------
///
/// class CMyAddon : public kodi::addon::CAddonBase
/// {
/// public:
///   CMyAddon() = default;
///   ADDON_STATUS CreateInstance(const kodi::addon::IInstanceInfo& instance,
///                               KODI_ADDON_INSTANCE_HDL& hdl) override;
/// };
///
/// // If you use only one instance in your add-on, can be instanceType and
/// // instanceID ignored
/// ADDON_STATUS CMyAddon::CreateInstance(const kodi::addon::IInstanceInfo& instance,
///                                       KODI_ADDON_INSTANCE_HDL& hdl)
/// {
///   if (instance.IsType(ADDON_INSTANCE_PERIPHERAL))
///   {
///     kodi::Log(ADDON_LOG_INFO, "Creating my peripheral addon");
///     addonInstance = new CMyPeripheralAddon(instance);
///     return ADDON_STATUS_OK;
///   }
///   else if (...)
///   {
///     ...
///   }
///   return ADDON_STATUS_UNKNOWN;
/// }
///
/// ADDONCREATOR(CMyAddon)
/// ~~~~~~~~~~~~~
///
/// The destruction of the example class `CMyPeripheralAddon` is called from
/// Kodi's header. Manually deleting the add-on instance is not required.
///
class ATTR_DLL_LOCAL CInstancePeripheral : public IAddonInstance
{
public:
  //============================================================================
  /// @ingroup cpp_kodi_addon_peripheral
  /// @brief Peripheral class constructor.
  ///
  /// Used by an add-on that only supports peripheral.
  ///
  CInstancePeripheral()
    : IAddonInstance(IInstanceInfo(CPrivateBase::m_interface->firstKodiInstance))
  {
    if (CPrivateBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstancePeripheral: Creation of more as one in single "
                             "instance way is not allowed!");

    SetAddonStruct(CPrivateBase::m_interface->firstKodiInstance);
    CPrivateBase::m_interface->globalSingleInstance = this;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_peripheral
  /// @brief Peripheral addon class constructor used to support multiple
  /// instance types.
  ///
  /// @param[in] instance The instance value given to
  ///                     <b>`kodi::addon::CAddonBase::CreateInstance(...)`</b>.
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  //////*Here's example about the use of this:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// class CMyPeripheralAddon : public kodi::addon::CInstancePeripheral
  /// {
  /// public:
  ///   CMyPeripheralAddon(const kodi::addon::IInstanceInfo& instance)
  ///     : kodi::addon::CInstancePeripheral(instance)
  ///   {
  ///      ...
  ///   }
  ///
  ///   ...
  /// };
  ///
  /// ADDON_STATUS CMyAddon::CreateInstance(const kodi::addon::IInstanceInfo& instance,
  ///                                       KODI_ADDON_INSTANCE_HDL& hdl)
  /// {
  ///   kodi::Log(ADDON_LOG_INFO, "Creating my peripheral");
  ///   hdl = new CMyPeripheralAddon(instance,);
  ///   return ADDON_STATUS_OK;
  /// }
  /// ~~~~~~~~~~~~~
  ///
  explicit CInstancePeripheral(const IInstanceInfo& instance) : IAddonInstance(instance)
  {
    if (CPrivateBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstancePeripheral: Creation of multiple together with "
                             "single instance way is not allowed!");

    SetAddonStruct(instance);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_peripheral
  /// @brief Destructor.
  ///
  ~CInstancePeripheral() override = default;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_peripheral_peripheralOp 1. Peripheral operations
  /// @ingroup cpp_kodi_addon_peripheral
  /// @brief %Peripheral operations to handle control about.
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **%Peripheral parts in interface:**\n
  /// Copy this to your project and extend with your parts or leave functions
  /// complete away where not used or supported.
  ///
  /// @copydetails cpp_kodi_addon_peripheral_peripheralOp_header_addon_auto_check
  /// @copydetails cpp_kodi_addon_peripheral_peripheralOp_source_addon_auto_check
  ///
  ///@{

  //============================================================================
  /// @brief Get the list of features that this add-on provides.
  ///
  /// Called by the frontend to query the add-on's capabilities and supported
  /// peripherals. All capabilities that the add-on supports should be set to true.
  ///
  /// @param[out] capabilities The add-on's capabilities
  ///
  /// @remarks Valid implementation required.
  ///
  ///
  /// ----------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_peripheral_Defs_PeripheralCapabilities_Help
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// void CMyPeripheralAddon::GetCapabilities(kodi::addon::PeripheralCapabilities& capabilities)
  /// {
  ///   capabilities.SetProvidesJoysticks(true);
  ///   capabilities.SetProvidesButtonmaps(true);
  /// }
  /// ~~~~~~~~~~~~~
  ///
  virtual void GetCapabilities(kodi::addon::PeripheralCapabilities& capabilities) {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Perform a scan for joysticks
  ///
  /// The frontend calls this when a hardware change is detected. If an add-on
  /// detects a hardware change, it can trigger this function using the
  /// @ref TriggerScan() callback.
  ///
  /// @param[in] scan_results Assigned to allocated memory
  /// @return @ref PERIPHERAL_NO_ERROR if successful
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_peripheral_Defs_Peripheral_Peripheral_Help
  ///
  virtual PERIPHERAL_ERROR PerformDeviceScan(
      std::vector<std::shared_ptr<kodi::addon::Peripheral>>& scan_results)
  {
    return PERIPHERAL_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Get all events that have occurred since the last call to
  /// @ref GetEvents().
  ///
  /// @param[out] events List of available events within addon
  /// @return @ref PERIPHERAL_NO_ERROR if successful
  ///
  /// ----------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_peripheral_Defs_Peripheral_PeripheralEvent_Help
  ///
  virtual PERIPHERAL_ERROR GetEvents(std::vector<kodi::addon::PeripheralEvent>& events)
  {
    return PERIPHERAL_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Send an input event to the peripheral.
  ///
  /// @param[in] event The input event
  /// @return true if the event was handled, false otherwise
  ///
  virtual bool SendEvent(const kodi::addon::PeripheralEvent& event) { return false; }
  //----------------------------------------------------------------------------

  ///@}

  //============================================================================
  /// @defgroup cpp_kodi_addon_peripheral_joystickOp 2. Joystick operations
  /// @ingroup cpp_kodi_addon_peripheral
  /// @brief %Joystick operations to handle control about.
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **%Joystick parts in interface:**\n
  /// Copy this to your project and extend with your parts or leave functions
  /// complete away where not used or supported.
  ///
  /// @copydetails cpp_kodi_addon_peripheral_joystickOp_header_addon_auto_check
  /// @copydetails cpp_kodi_addon_peripheral_joystickOp_source_addon_auto_check
  ///
  ///@{

  //============================================================================
  /// @brief Get extended info about an attached joystick.
  ///
  /// @param[in] index The joystick's driver index
  /// @param[out] info The container for the allocated joystick info
  /// @return @ref PERIPHERAL_NO_ERROR if successful
  ///
  ///
  /// ----------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_peripheral_Defs_Joystick_Joystick_Help
  ///
  virtual PERIPHERAL_ERROR GetJoystickInfo(unsigned int index, kodi::addon::Joystick& info)
  {
    return PERIPHERAL_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Get the ID of the controller that best represents the peripheral's
  /// appearance.
  ///
  /// @param[in] joystick The device's joystick properties; unknown values may
  ///                     be left at their default
  /// @param[out] controllerId The controller ID of the appearance, or empty
  ///                          if the appearance is unknown
  ///
  /// @return @ref PERIPHERAL_NO_ERROR if successful
  ///
  virtual PERIPHERAL_ERROR GetAppearance(const kodi::addon::Joystick& joystick,
                                         std::string& controllerId)
  {
    return PERIPHERAL_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Set the ID of the controller that best represents the peripheral's
  /// appearance.
  ///
  /// @param[in] joystick The device's joystick properties; unknown values may
  ///                     be left at their default
  /// @param[in] controllerId The controller ID of the appearance
  ///
  /// @return @ref PERIPHERAL_NO_ERROR if successful
  ///
  virtual PERIPHERAL_ERROR SetAppearance(const kodi::addon::Joystick& joystick,
                                         const std::string& controllerId)
  {
    return PERIPHERAL_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Get the features that allow translating the joystick into the
  /// controller profile.
  ///
  /// @param[in] joystick The device's joystick properties; unknown values may
  ///                     be left at their default
  /// @param[in] controller_id The controller profile being requested, e.g.
  ///                          `game.controller.default`
  /// @param[out] features The array of allocated features
  /// @return @ref PERIPHERAL_NO_ERROR if successful
  ///
  virtual PERIPHERAL_ERROR GetFeatures(const kodi::addon::Joystick& joystick,
                                       const std::string& controller_id,
                                       std::vector<kodi::addon::JoystickFeature>& features)
  {
    return PERIPHERAL_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Add or update joystick features.
  ///
  /// @param[in] joystick The device's joystick properties; unknown values may be
  ///                 left at their default
  /// @param[in] controller_id The game controller profile being updated
  /// @param[in] features The array of features
  /// @return @ref PERIPHERAL_NO_ERROR if successful
  ///
  virtual PERIPHERAL_ERROR MapFeatures(const kodi::addon::Joystick& joystick,
                                       const std::string& controller_id,
                                       const std::vector<kodi::addon::JoystickFeature>& features)
  {
    return PERIPHERAL_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Get the driver primitives that should be ignored while mapping the
  /// device.
  ///
  /// @param[in] joystick The device's joystick properties; unknown values may
  ///                     be left at their default
  /// @param[out] primitives The array of allocated driver primitives to be
  ///                        ignored
  /// @return @ref PERIPHERAL_NO_ERROR if successful
  ///
  virtual PERIPHERAL_ERROR GetIgnoredPrimitives(
      const kodi::addon::Joystick& joystick, std::vector<kodi::addon::DriverPrimitive>& primitives)
  {
    return PERIPHERAL_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Set the list of driver primitives that are ignored for the device.
  ///
  /// @param[in] joystick The device's joystick properties; unknown values may be left at their default
  /// @param[in] primitives The array of driver primitives to ignore
  /// @return @ref PERIPHERAL_NO_ERROR if successful
  ///
  virtual PERIPHERAL_ERROR SetIgnoredPrimitives(
      const kodi::addon::Joystick& joystick,
      const std::vector<kodi::addon::DriverPrimitive>& primitives)
  {
    return PERIPHERAL_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Save the button map for the given joystick.
  ///
  /// @param[in] joystick The device's joystick properties
  ///
  virtual void SaveButtonMap(const kodi::addon::Joystick& joystick) {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Revert the button map to the last time it was loaded or committed to disk
  /// @param[in] joystick The device's joystick properties
  ///
  virtual void RevertButtonMap(const kodi::addon::Joystick& joystick) {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Reset the button map for the given joystick and controller profile ID
  /// @param[in] joystick      The device's joystick properties
  /// @param[in] controller_id The game controller profile being reset
  ///
  virtual void ResetButtonMap(const kodi::addon::Joystick& joystick,
                              const std::string& controller_id)
  {
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Powers off the given joystick if supported
  /// @param[in] index The joystick's driver index
  ///
  virtual void PowerOffJoystick(unsigned int index) {}
  //----------------------------------------------------------------------------

  ///@}

  //============================================================================
  /// @defgroup cpp_kodi_addon_peripheral_callbacks 3. Callback functions
  /// @ingroup cpp_kodi_addon_peripheral
  /// @brief Callback to Kodi functions.
  ///
  ///@{

  //============================================================================
  /// @brief Used to get the full path where the add-on is installed.
  ///
  /// @return The add-on installation path
  ///
  const std::string AddonPath() const { return m_instanceData->props->addon_path; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Used to get the full path to the add-on's user profile.
  ///
  /// @note The trailing folder (consisting of the add-on's ID) is not created
  /// by default. If it is needed, you must call kodi::vfs::CreateDirectory()
  /// to create the folder.
  ///
  /// @return Path to the user profile
  ///
  const std::string UserPath() const { return m_instanceData->props->user_path; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Trigger a scan for peripherals
  ///
  /// The add-on calls this if a change in hardware is detected.
  ///
  void TriggerScan(void)
  {
    return m_instanceData->toKodi->trigger_scan(m_instanceData->toKodi->kodiInstance);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Notify the frontend that button maps have changed.
  ///
  /// @param[in] deviceName [optional] The name of the device to refresh, or
  ///                        empty/null for all devices
  /// @param[in] controllerId [optional] The controller ID to refresh, or
  ///                         empty/null for all controllers
  ///
  void RefreshButtonMaps(const std::string& deviceName = "", const std::string& controllerId = "")
  {
    return m_instanceData->toKodi->refresh_button_maps(m_instanceData->toKodi->kodiInstance,
                                                       deviceName.c_str(), controllerId.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Return the number of features belonging to the specified
  /// controller.
  ///
  /// @param[in] controllerId The controller ID to enumerate
  /// @param[in] type [optional] Type to filter by, or @ref JOYSTICK_FEATURE_TYPE_UNKNOWN
  ///                 for all features
  /// @return The number of features matching the request parameters
  ///
  unsigned int FeatureCount(const std::string& controllerId,
                            JOYSTICK_FEATURE_TYPE type = JOYSTICK_FEATURE_TYPE_UNKNOWN)
  {
    return m_instanceData->toKodi->feature_count(m_instanceData->toKodi->kodiInstance,
                                                 controllerId.c_str(), type);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Return the type of the feature.
  ///
  /// @param[in] controllerId The controller ID to check
  /// @param[in] featureName The feature to check
  /// @return The type of the specified feature, or @ref JOYSTICK_FEATURE_TYPE_UNKNOWN
  /// if unknown
  ///
  JOYSTICK_FEATURE_TYPE FeatureType(const std::string& controllerId, const std::string& featureName)
  {
    return m_instanceData->toKodi->feature_type(m_instanceData->toKodi->kodiInstance,
                                                controllerId.c_str(), featureName.c_str());
  }
  //----------------------------------------------------------------------------

  ///@}

private:
  void SetAddonStruct(KODI_ADDON_INSTANCE_STRUCT* instance)
  {
    instance->hdl = this;

    instance->peripheral->toAddon->get_capabilities = ADDON_GetCapabilities;
    instance->peripheral->toAddon->perform_device_scan = ADDON_PerformDeviceScan;
    instance->peripheral->toAddon->free_scan_results = ADDON_FreeScanResults;
    instance->peripheral->toAddon->get_events = ADDON_GetEvents;
    instance->peripheral->toAddon->free_events = ADDON_FreeEvents;
    instance->peripheral->toAddon->send_event = ADDON_SendEvent;

    instance->peripheral->toAddon->get_joystick_info = ADDON_GetJoystickInfo;
    instance->peripheral->toAddon->free_joystick_info = ADDON_FreeJoystickInfo;
    instance->peripheral->toAddon->get_appearance = ADDON_GetAppearance;
    instance->peripheral->toAddon->set_appearance = ADDON_SetAppearance;
    instance->peripheral->toAddon->get_features = ADDON_GetFeatures;
    instance->peripheral->toAddon->free_features = ADDON_FreeFeatures;
    instance->peripheral->toAddon->map_features = ADDON_MapFeatures;
    instance->peripheral->toAddon->get_ignored_primitives = ADDON_GetIgnoredPrimitives;
    instance->peripheral->toAddon->free_primitives = ADDON_FreePrimitives;
    instance->peripheral->toAddon->set_ignored_primitives = ADDON_SetIgnoredPrimitives;
    instance->peripheral->toAddon->save_button_map = ADDON_SaveButtonMap;
    instance->peripheral->toAddon->revert_button_map = ADDON_RevertButtonMap;
    instance->peripheral->toAddon->reset_button_map = ADDON_ResetButtonMap;
    instance->peripheral->toAddon->power_off_joystick = ADDON_PowerOffJoystick;

    m_instanceData = instance->peripheral;
    m_instanceData->toAddon->addonInstance = this;
  }

  inline static void ADDON_GetCapabilities(const AddonInstance_Peripheral* addonInstance,
                                           PERIPHERAL_CAPABILITIES* capabilities)
  {
    if (!addonInstance || !capabilities)
      return;

    kodi::addon::PeripheralCapabilities peripheralCapabilities(capabilities);
    static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->GetCapabilities(peripheralCapabilities);
  }

  inline static PERIPHERAL_ERROR ADDON_PerformDeviceScan(
      const AddonInstance_Peripheral* addonInstance,
      unsigned int* peripheral_count,
      PERIPHERAL_INFO** scan_results)
  {
    if (!addonInstance || !peripheral_count || !scan_results)
      return PERIPHERAL_ERROR_INVALID_PARAMETERS;

    std::vector<std::shared_ptr<kodi::addon::Peripheral>> peripherals;
    PERIPHERAL_ERROR err = static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
                               ->PerformDeviceScan(peripherals);
    if (err == PERIPHERAL_NO_ERROR)
    {
      *peripheral_count = static_cast<unsigned int>(peripherals.size());
      kodi::addon::Peripherals::ToStructs(peripherals, scan_results);
    }

    return err;
  }

  inline static void ADDON_FreeScanResults(const AddonInstance_Peripheral* addonInstance,
                                           unsigned int peripheral_count,
                                           PERIPHERAL_INFO* scan_results)
  {
    if (!addonInstance)
      return;

    kodi::addon::Peripherals::FreeStructs(peripheral_count, scan_results);
  }

  inline static PERIPHERAL_ERROR ADDON_GetEvents(const AddonInstance_Peripheral* addonInstance,
                                                 unsigned int* event_count,
                                                 PERIPHERAL_EVENT** events)
  {
    if (!addonInstance || !event_count || !events)
      return PERIPHERAL_ERROR_INVALID_PARAMETERS;

    std::vector<kodi::addon::PeripheralEvent> peripheralEvents;
    PERIPHERAL_ERROR err = static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
                               ->GetEvents(peripheralEvents);
    if (err == PERIPHERAL_NO_ERROR)
    {
      *event_count = static_cast<unsigned int>(peripheralEvents.size());
      kodi::addon::PeripheralEvents::ToStructs(peripheralEvents, events);
    }

    return err;
  }

  inline static void ADDON_FreeEvents(const AddonInstance_Peripheral* addonInstance,
                                      unsigned int event_count,
                                      PERIPHERAL_EVENT* events)
  {
    if (!addonInstance)
      return;

    kodi::addon::PeripheralEvents::FreeStructs(event_count, events);
  }

  inline static bool ADDON_SendEvent(const AddonInstance_Peripheral* addonInstance,
                                     const PERIPHERAL_EVENT* event)
  {
    if (!addonInstance || !event)
      return false;
    return static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->SendEvent(kodi::addon::PeripheralEvent(*event));
  }

  inline static PERIPHERAL_ERROR ADDON_GetJoystickInfo(
      const AddonInstance_Peripheral* addonInstance, unsigned int index, JOYSTICK_INFO* info)
  {
    if (!addonInstance || !info)
      return PERIPHERAL_ERROR_INVALID_PARAMETERS;

    kodi::addon::Joystick addonInfo;
    PERIPHERAL_ERROR err = static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
                               ->GetJoystickInfo(index, addonInfo);
    if (err == PERIPHERAL_NO_ERROR)
    {
      addonInfo.ToStruct(*info);
    }

    return err;
  }

  inline static void ADDON_FreeJoystickInfo(const AddonInstance_Peripheral* addonInstance,
                                            JOYSTICK_INFO* info)
  {
    if (!addonInstance)
      return;

    kodi::addon::Joystick::FreeStruct(*info);
  }

  inline static PERIPHERAL_ERROR ADDON_GetAppearance(const AddonInstance_Peripheral* addonInstance,
                                                     const JOYSTICK_INFO* joystick,
                                                     char* buffer,
                                                     unsigned int bufferSize)
  {
    if (addonInstance == nullptr || joystick == nullptr || buffer == nullptr || bufferSize == 0)
      return PERIPHERAL_ERROR_INVALID_PARAMETERS;

    kodi::addon::Joystick addonJoystick(*joystick);
    std::string controllerId;

    PERIPHERAL_ERROR err = static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
                               ->GetAppearance(addonJoystick, controllerId);
    if (err == PERIPHERAL_NO_ERROR)
    {
      std::strncpy(buffer, controllerId.c_str(), bufferSize - 1);
      buffer[bufferSize - 1] = '\0';
    }

    return err;
  }

  inline static PERIPHERAL_ERROR ADDON_SetAppearance(const AddonInstance_Peripheral* addonInstance,
                                                     const JOYSTICK_INFO* joystick,
                                                     const char* controllerId)
  {
    if (addonInstance == nullptr || joystick == nullptr || controllerId == nullptr)
      return PERIPHERAL_ERROR_INVALID_PARAMETERS;

    kodi::addon::Joystick addonJoystick(*joystick);
    std::string strControllerId(controllerId);

    return static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->SetAppearance(addonJoystick, strControllerId);
  }

  inline static PERIPHERAL_ERROR ADDON_GetFeatures(const AddonInstance_Peripheral* addonInstance,
                                                   const JOYSTICK_INFO* joystick,
                                                   const char* controller_id,
                                                   unsigned int* feature_count,
                                                   JOYSTICK_FEATURE** features)
  {
    if (!addonInstance || !joystick || !controller_id || !feature_count || !features)
      return PERIPHERAL_ERROR_INVALID_PARAMETERS;

    kodi::addon::Joystick addonJoystick(*joystick);
    std::vector<kodi::addon::JoystickFeature> featuresVector;

    PERIPHERAL_ERROR err = static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
                               ->GetFeatures(addonJoystick, controller_id, featuresVector);
    if (err == PERIPHERAL_NO_ERROR)
    {
      *feature_count = static_cast<unsigned int>(featuresVector.size());
      kodi::addon::JoystickFeatures::ToStructs(featuresVector, features);
    }

    return err;
  }

  inline static void ADDON_FreeFeatures(const AddonInstance_Peripheral* addonInstance,
                                        unsigned int feature_count,
                                        JOYSTICK_FEATURE* features)
  {
    if (!addonInstance)
      return;

    kodi::addon::JoystickFeatures::FreeStructs(feature_count, features);
  }

  inline static PERIPHERAL_ERROR ADDON_MapFeatures(const AddonInstance_Peripheral* addonInstance,
                                                   const JOYSTICK_INFO* joystick,
                                                   const char* controller_id,
                                                   unsigned int feature_count,
                                                   const JOYSTICK_FEATURE* features)
  {
    if (!addonInstance || !joystick || !controller_id || (feature_count > 0 && !features))
      return PERIPHERAL_ERROR_INVALID_PARAMETERS;

    kodi::addon::Joystick addonJoystick(*joystick);
    std::vector<kodi::addon::JoystickFeature> primitiveVector;

    for (unsigned int i = 0; i < feature_count; i++)
      primitiveVector.emplace_back(*(features + i));

    return static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->MapFeatures(addonJoystick, controller_id, primitiveVector);
  }

  inline static PERIPHERAL_ERROR ADDON_GetIgnoredPrimitives(
      const AddonInstance_Peripheral* addonInstance,
      const JOYSTICK_INFO* joystick,
      unsigned int* primitive_count,
      JOYSTICK_DRIVER_PRIMITIVE** primitives)
  {
    if (!addonInstance || !joystick || !primitive_count || !primitives)
      return PERIPHERAL_ERROR_INVALID_PARAMETERS;

    kodi::addon::Joystick addonJoystick(*joystick);
    std::vector<kodi::addon::DriverPrimitive> primitiveVector;

    PERIPHERAL_ERROR err = static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
                               ->GetIgnoredPrimitives(addonJoystick, primitiveVector);
    if (err == PERIPHERAL_NO_ERROR)
    {
      *primitive_count = static_cast<unsigned int>(primitiveVector.size());
      kodi::addon::DriverPrimitives::ToStructs(primitiveVector, primitives);
    }

    return err;
  }

  inline static void ADDON_FreePrimitives(const AddonInstance_Peripheral* addonInstance,
                                          unsigned int primitive_count,
                                          JOYSTICK_DRIVER_PRIMITIVE* primitives)
  {
    if (!addonInstance)
      return;

    kodi::addon::DriverPrimitives::FreeStructs(primitive_count, primitives);
  }

  inline static PERIPHERAL_ERROR ADDON_SetIgnoredPrimitives(
      const AddonInstance_Peripheral* addonInstance,
      const JOYSTICK_INFO* joystick,
      unsigned int primitive_count,
      const JOYSTICK_DRIVER_PRIMITIVE* primitives)
  {
    if (!addonInstance || !joystick || (primitive_count > 0 && !primitives))
      return PERIPHERAL_ERROR_INVALID_PARAMETERS;

    kodi::addon::Joystick addonJoystick(*joystick);
    std::vector<kodi::addon::DriverPrimitive> primitiveVector;

    for (unsigned int i = 0; i < primitive_count; i++)
      primitiveVector.emplace_back(*(primitives + i));

    return static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->SetIgnoredPrimitives(addonJoystick, primitiveVector);
  }

  inline static void ADDON_SaveButtonMap(const AddonInstance_Peripheral* addonInstance,
                                         const JOYSTICK_INFO* joystick)
  {
    if (!addonInstance || !joystick)
      return;

    kodi::addon::Joystick addonJoystick(*joystick);
    static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->SaveButtonMap(addonJoystick);
  }

  inline static void ADDON_RevertButtonMap(const AddonInstance_Peripheral* addonInstance,
                                           const JOYSTICK_INFO* joystick)
  {
    if (!addonInstance || !joystick)
      return;

    kodi::addon::Joystick addonJoystick(*joystick);
    static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->RevertButtonMap(addonJoystick);
  }

  inline static void ADDON_ResetButtonMap(const AddonInstance_Peripheral* addonInstance,
                                          const JOYSTICK_INFO* joystick,
                                          const char* controller_id)
  {
    if (!addonInstance || !joystick || !controller_id)
      return;

    kodi::addon::Joystick addonJoystick(*joystick);
    static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->ResetButtonMap(addonJoystick, controller_id);
  }

  inline static void ADDON_PowerOffJoystick(const AddonInstance_Peripheral* addonInstance,
                                            unsigned int index)
  {
    if (!addonInstance)
      return;

    static_cast<CInstancePeripheral*>(addonInstance->toAddon->addonInstance)
        ->PowerOffJoystick(index);
  }

  AddonInstance_Peripheral* m_instanceData;
};

} /* namespace addon */
} /* namespace kodi */
#endif /* __cplusplus */
