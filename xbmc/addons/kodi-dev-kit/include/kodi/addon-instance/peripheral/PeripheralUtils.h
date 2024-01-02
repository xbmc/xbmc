/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../AddonBase.h"
#include "../../c-api/addon-instance/peripheral.h"

#ifdef __cplusplus

#include <array> // Requires c++11
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#define PERIPHERAL_SAFE_DELETE(x) \
  do \
  { \
    delete (x); \
    (x) = NULL; \
  } while (0)
#define PERIPHERAL_SAFE_DELETE_ARRAY(x) \
  do \
  { \
    delete[](x); \
    (x) = NULL; \
  } while (0)

namespace kodi
{
namespace addon
{

class CInstancePeripheral;

/*!
 * Utility class to manipulate arrays of peripheral types.
 */
template<class THE_CLASS, typename THE_STRUCT>
class PeripheralVector
{
public:
  static void ToStructs(const std::vector<THE_CLASS>& vecObjects, THE_STRUCT** pStructs)
  {
    if (!pStructs)
      return;

    if (vecObjects.empty())
    {
      *pStructs = NULL;
    }
    else
    {
      (*pStructs) = new THE_STRUCT[vecObjects.size()];
      for (unsigned int i = 0; i < vecObjects.size(); i++)
        vecObjects.at(i).ToStruct((*pStructs)[i]);
    }
  }

  static void ToStructs(const std::vector<THE_CLASS*>& vecObjects, THE_STRUCT** pStructs)
  {
    if (!pStructs)
      return;

    if (vecObjects.empty())
    {
      *pStructs = NULL;
    }
    else
    {
      *pStructs = new THE_STRUCT[vecObjects.size()];
      for (unsigned int i = 0; i < vecObjects.size(); i++)
        vecObjects.at(i)->ToStruct((*pStructs)[i]);
    }
  }

  static void ToStructs(const std::vector<std::shared_ptr<THE_CLASS>>& vecObjects,
                        THE_STRUCT** pStructs)
  {
    if (!pStructs)
      return;

    if (vecObjects.empty())
    {
      *pStructs = NULL;
    }
    else
    {
      *pStructs = new THE_STRUCT[vecObjects.size()];
      for (unsigned int i = 0; i < vecObjects.size(); i++)
        vecObjects.at(i)->ToStruct((*pStructs)[i]);
    }
  }

  static void FreeStructs(unsigned int structCount, THE_STRUCT* structs)
  {
    if (structs)
    {
      for (unsigned int i = 0; i < structCount; i++)
        THE_CLASS::FreeStruct(structs[i]);
    }
    PERIPHERAL_SAFE_DELETE_ARRAY(structs);
  }
};

//==============================================================================
/// @defgroup cpp_kodi_addon_peripheral_Defs_PeripheralCapabilities class PeripheralCapabilities
/// @ingroup cpp_kodi_addon_peripheral_Defs_General
/// @brief **%Peripheral add-on capabilities**\n
/// This class is needed to tell Kodi which options are supported on the addon.
///
/// If a capability is set to **true**, then the corresponding methods from
/// @ref cpp_kodi_addon_peripheral "kodi::addon::CInstancePeripheral" need to be
/// implemented.
///
/// As default them all set to **false**.
///
/// Used on @ref kodi::addon::CInstancePeripheral::GetCapabilities().
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_peripheral_Defs_PeripheralCapabilities_Help
///
///@{
class PeripheralCapabilities : public CStructHdl<PeripheralCapabilities, PERIPHERAL_CAPABILITIES>
{
  /*! \cond PRIVATE */
  friend class CInstancePeripheral;
  /*! \endcond */

public:
  /*! \cond PRIVATE */
  PeripheralCapabilities()
  {
    m_cStructure->provides_joysticks = false;
    m_cStructure->provides_joystick_rumble = false;
    m_cStructure->provides_joystick_power_off = false;
    m_cStructure->provides_buttonmaps = false;
  }

  PeripheralCapabilities(const PeripheralCapabilities& data) : CStructHdl(data) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_peripheral_Defs_PeripheralCapabilities_Help Value Help
  /// @ingroup cpp_kodi_addon_peripheral_Defs_PeripheralCapabilities
  /// ----------------------------------------------------------------------------
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_peripheral_Defs_PeripheralCapabilities :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|----------
  /// | **Provides joysticks** | `boolean` | @ref PeripheralCapabilities::SetProvidesJoysticks "SetProvidesJoysticks" | @ref PeripheralCapabilities::GetProvidesJoysticks "GetProvidesJoysticks"
  /// | **Provides joystick rumble** | `boolean` | @ref PeripheralCapabilities::SetProvidesJoystickRumble "SetProvidesJoystickRumble" | @ref PeripheralCapabilities::GetProvidesJoystickRumble "GetProvidesJoystickRumble"
  /// | **Provides joystick power off** | `boolean` | @ref PeripheralCapabilities::SetProvidesJoystickPowerOff "SetProvidesJoystickPowerOff" | @ref PeripheralCapabilities::GetProvidesJoystickPowerOff "GetProvidesJoystickPowerOff"
  /// | **Provides button maps** | `boolean` | @ref PeripheralCapabilities::SetProvidesButtonmaps "SetProvidesButtonmaps" | @ref PeripheralCapabilities::GetProvidesButtonmaps "GetProvidesButtonmaps"

  /// @addtogroup cpp_kodi_addon_peripheral_Defs_PeripheralCapabilities
  ///@{

  /// @brief Set true if the add-on provides joysticks.
  void SetProvidesJoysticks(bool providesJoysticks)
  {
    m_cStructure->provides_joysticks = providesJoysticks;
  }

  /// @brief To get with @ref SetProvidesJoysticks changed values.
  bool GetProvidesJoysticks() const { return m_cStructure->provides_joysticks; }

  /// @brief Set true if the add-on provides joystick rumble.
  void SetProvidesJoystickRumble(bool providesJoystickRumble)
  {
    m_cStructure->provides_joystick_rumble = providesJoystickRumble;
  }

  /// @brief To get with @ref SetProvidesJoystickRumble changed values.
  bool GetProvidesJoystickRumble() const { return m_cStructure->provides_joystick_rumble; }

  /// @brief Set true if the add-on provides power off about joystick.
  void SetProvidesJoystickPowerOff(bool providesJoystickPowerOff)
  {
    m_cStructure->provides_joystick_power_off = providesJoystickPowerOff;
  }

  /// @brief To get with @ref SetProvidesJoystickPowerOff changed values.
  bool GetProvidesJoystickPowerOff() const { return m_cStructure->provides_joystick_power_off; }

  /// @brief Set true if the add-on provides button maps.
  void SetProvidesButtonmaps(bool providesButtonmaps)
  {
    m_cStructure->provides_buttonmaps = providesButtonmaps;
  }

  /// @brief To get with @ref SetProvidesButtonmaps changed values.
  bool GetProvidesButtonmaps() const { return m_cStructure->provides_buttonmaps; }

  ///@}

private:
  PeripheralCapabilities(const PERIPHERAL_CAPABILITIES* data) : CStructHdl(data) {}
  PeripheralCapabilities(PERIPHERAL_CAPABILITIES* data) : CStructHdl(data) {}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_peripheral_Defs_Peripheral_Peripheral class Peripheral
/// @ingroup cpp_kodi_addon_peripheral_Defs_Peripheral
/// @brief **Wrapper class providing peripheral information**\n
/// Classes can extend %Peripheral to inherit peripheral properties.
///
/// Used on @ref kodi::addon::CInstancePeripheral::PerformDeviceScan().
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_peripheral_Defs_Peripheral_Peripheral_Help
///
///@{
class Peripheral
{
public:
  /// @defgroup cpp_kodi_addon_peripheral_Defs_Peripheral_Peripheral_Help Value Help
  /// @ingroup cpp_kodi_addon_peripheral_Defs_Peripheral_Peripheral
  /// ----------------------------------------------------------------------------
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_peripheral_Defs_Peripheral_Peripheral :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|----------
  /// | **%Peripheral type** | @ref PERIPHERAL_TYPE | @ref Peripheral::SetType "SetType" | @ref Peripheral::Type "Type"
  /// | **%Peripheral name** | `const std::string&` | @ref Peripheral::SetName "SetName" | @ref Peripheral::Name "Name"
  /// | **%Peripheral vendor id** | `uint16_t` | @ref Peripheral::SetVendorID "SetVendorID" | @ref Peripheral::VendorID "VendorID"
  /// | **%Peripheral product id** | `uint16_t` | @ref Peripheral::SetProductID "SetProductID" | @ref Peripheral::ProductID "ProductID"
  /// | **%Peripheral index** | `unsigned int` | @ref Peripheral::SetIndex "SetIndex" | @ref Peripheral::Index "Index"
  ///
  /// Further are following included:
  /// - @ref Peripheral::Peripheral "Peripheral(PERIPHERAL_TYPE type = PERIPHERAL_TYPE_UNKNOWN, const std::string& strName = \"\")": Class constructor.
  /// - @ref Peripheral::IsVidPidKnown "IsVidPidKnown()": To check VID and PID are known.
  ///

  /// @addtogroup cpp_kodi_addon_peripheral_Defs_Peripheral_Peripheral
  ///@{

  /// @brief Constructor.
  ///
  /// @param[in] type [optional] Peripheral type, or @ref PERIPHERAL_TYPE_UNKNOWN
  ///                 as default
  /// @param[in] strName [optional] Name of related peripheral
  Peripheral(PERIPHERAL_TYPE type = PERIPHERAL_TYPE_UNKNOWN, const std::string& strName = "")
    : m_type(type), m_strName(strName)
  {
  }

  /// @brief Destructor.
  virtual ~Peripheral(void) = default;

  /// @brief Comparison operator
  ///
  /// @note The index is not included in the comparison because it is affected
  ///       by the presence of other peripherals
  bool operator==(const Peripheral& rhs) const
  {
    // clang-format off
    return m_type == rhs.m_type &&
           m_strName == rhs.m_strName &&
           m_vendorId == rhs.m_vendorId &&
           m_productId == rhs.m_productId;
    // clang-format on
  }

  /// @brief Get peripheral type.
  ///
  /// @return Type defined with @ref PERIPHERAL_TYPE
  PERIPHERAL_TYPE Type(void) const { return m_type; }

  /// @brief Get peripheral name.
  ///
  /// @return Name string of peripheral
  const std::string& Name(void) const { return m_strName; }

  /// @brief Get peripheral vendor id.
  ///
  /// @return Vendor id
  uint16_t VendorID(void) const { return m_vendorId; }

  /// @brief Get peripheral product id.
  ///
  /// @return Product id
  uint16_t ProductID(void) const { return m_productId; }

  /// @brief Get peripheral index identifier.
  ///
  /// @return Index number
  unsigned int Index(void) const { return m_index; }

  /// @brief Check VID and PID are known.
  ///
  /// @return true if VID and PID are not 0
  ///
  /// @note Derived property: VID and PID are `0x0000` if unknown
  bool IsVidPidKnown(void) const { return m_vendorId != 0 || m_productId != 0; }

  /// @brief Set peripheral type.
  ///
  /// @param[in] type Type to set
  void SetType(PERIPHERAL_TYPE type) { m_type = type; }

  /// @brief Set peripheral name.
  ///
  /// @param[in] strName Name to set
  void SetName(const std::string& strName) { m_strName = strName; }

  /// @brief Set peripheral vendor id.
  ///
  /// @param[in] vendorId Type to set
  void SetVendorID(uint16_t vendorId) { m_vendorId = vendorId; }

  /// @brief Set peripheral product identifier.
  ///
  /// @param[in] productId Type to set
  void SetProductID(uint16_t productId) { m_productId = productId; }

  /// @brief Set peripheral index.
  ///
  /// @param[in] index Type to set
  void SetIndex(unsigned int index) { m_index = index; }

  ///@}

  explicit Peripheral(const PERIPHERAL_INFO& info)
    : m_type(info.type),
      m_strName(info.name ? info.name : ""),
      m_vendorId(info.vendor_id),
      m_productId(info.product_id),
      m_index(info.index)
  {
  }

  void ToStruct(PERIPHERAL_INFO& info) const
  {
    info.type = m_type;
    info.name = new char[m_strName.size() + 1];
    info.vendor_id = m_vendorId;
    info.product_id = m_productId;
    info.index = m_index;

    std::strcpy(info.name, m_strName.c_str());
  }

  static void FreeStruct(PERIPHERAL_INFO& info) { PERIPHERAL_SAFE_DELETE_ARRAY(info.name); }

private:
  PERIPHERAL_TYPE m_type;
  std::string m_strName;
  uint16_t m_vendorId = 0;
  uint16_t m_productId = 0;
  unsigned int m_index = 0;
};
///@}
//------------------------------------------------------------------------------

typedef PeripheralVector<Peripheral, PERIPHERAL_INFO> Peripherals;

//==============================================================================
/// @defgroup cpp_kodi_addon_peripheral_Defs_Peripheral_PeripheralEvent class PeripheralEvent
/// @ingroup cpp_kodi_addon_peripheral_Defs_Peripheral
/// @brief **Wrapper class for %peripheral events**\n
/// To handle data of change events between add-on and Kodi.
///
/// Used on @ref kodi::addon::CInstancePeripheral::GetEvents() and
/// @ref kodi::addon::CInstancePeripheral::SendEvent().
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_peripheral_Defs_Peripheral_PeripheralEvent_Help
///
///@{
class PeripheralEvent
{
public:
  /// @defgroup cpp_kodi_addon_peripheral_Defs_Peripheral_PeripheralEvent_Help Value Help
  /// @ingroup cpp_kodi_addon_peripheral_Defs_Peripheral_PeripheralEvent
  /// ----------------------------------------------------------------------------
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_peripheral_Defs_Peripheral_PeripheralEvent :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|----------
  /// | **%Peripheral event type** | @ref PERIPHERAL_EVENT_TYPE | @ref PeripheralEvent::SetType "SetType" | @ref PeripheralEvent::Type "Type"
  /// | **%Peripheral index** | `unsigned int` | @ref PeripheralEvent::SetPeripheralIndex "SetPeripheralIndex" | @ref PeripheralEvent::PeripheralIndex "PeripheralIndex"
  /// | **%Peripheral event driver index** | `unsigned int` | @ref PeripheralEvent::SetDriverIndex "SetDriverIndex" | @ref PeripheralEvent::DriverIndex "DriverIndex"
  /// | **%Peripheral event button state** | @ref JOYSTICK_STATE_BUTTON | @ref PeripheralEvent::SetButtonState "SetButtonState" | @ref PeripheralEvent::ButtonState "ButtonState"
  /// | **%Peripheral event hat state** | @ref JOYSTICK_STATE_HAT | @ref PeripheralEvent::SetHatState "SetHatState" | @ref PeripheralEvent::HatState "HatState"
  /// | **%Peripheral event axis state** | @ref JOYSTICK_STATE_AXIS (`float`) | @ref PeripheralEvent::SetAxisState "SetAxisState" | @ref PeripheralEvent::AxisState "AxisState"
  /// | **%Peripheral event motor state** | @ref JOYSTICK_STATE_MOTOR (`float`) | @ref PeripheralEvent::SetMotorState "SetMotorState" | @ref PeripheralEvent::MotorState "MotorState"
  ///
  /// Further are several class constructors with values included.

  /// @addtogroup cpp_kodi_addon_peripheral_Defs_Peripheral_PeripheralEvent
  ///@{

  /// @brief Constructor.
  PeripheralEvent() = default;

  /// @brief Constructor.
  ///
  /// @param[in] peripheralIndex %Peripheral index
  /// @param[in] buttonIndex Button index
  /// @param[in] state Joystick state button
  PeripheralEvent(unsigned int peripheralIndex,
                  unsigned int buttonIndex,
                  JOYSTICK_STATE_BUTTON state)
    : m_type(PERIPHERAL_EVENT_TYPE_DRIVER_BUTTON),
      m_peripheralIndex(peripheralIndex),
      m_driverIndex(buttonIndex),
      m_buttonState(state)
  {
  }

  /// @brief Constructor.
  ///
  /// @param[in] peripheralIndex %Peripheral index
  /// @param[in] hatIndex Hat index
  /// @param[in] state Joystick state hat
  PeripheralEvent(unsigned int peripheralIndex, unsigned int hatIndex, JOYSTICK_STATE_HAT state)
    : m_type(PERIPHERAL_EVENT_TYPE_DRIVER_HAT),
      m_peripheralIndex(peripheralIndex),
      m_driverIndex(hatIndex),
      m_hatState(state)
  {
  }

  /// @brief Constructor.
  ///
  /// @param[in] peripheralIndex %Peripheral index
  /// @param[in] axisIndex Axis index
  /// @param[in] state Joystick state axis
  PeripheralEvent(unsigned int peripheralIndex, unsigned int axisIndex, JOYSTICK_STATE_AXIS state)
    : m_type(PERIPHERAL_EVENT_TYPE_DRIVER_AXIS),
      m_peripheralIndex(peripheralIndex),
      m_driverIndex(axisIndex),
      m_axisState(state)
  {
  }

  /// @brief Get type of event.
  ///
  /// @return Type defined with @ref PERIPHERAL_EVENT_TYPE
  PERIPHERAL_EVENT_TYPE Type(void) const { return m_type; }

  /// @brief Get peripheral index.
  ///
  /// @return %Peripheral index number
  unsigned int PeripheralIndex(void) const { return m_peripheralIndex; }

  /// @brief Get driver index.
  ///
  /// @return Driver index number
  unsigned int DriverIndex(void) const { return m_driverIndex; }

  /// @brief Get button state.
  ///
  /// @return Button state as @ref JOYSTICK_STATE_BUTTON
  JOYSTICK_STATE_BUTTON ButtonState(void) const { return m_buttonState; }

  /// @brief Get hat state.
  ///
  /// @return Hat state
  JOYSTICK_STATE_HAT HatState(void) const { return m_hatState; }

  /// @brief Get axis state.
  ///
  /// @return Axis state
  JOYSTICK_STATE_AXIS AxisState(void) const { return m_axisState; }

  /// @brief Get motor state.
  ///
  /// @return Motor state
  JOYSTICK_STATE_MOTOR MotorState(void) const { return m_motorState; }

  /// @brief Set type of event.
  ///
  /// @param[in] type Type defined with @ref PERIPHERAL_EVENT_TYPE
  void SetType(PERIPHERAL_EVENT_TYPE type) { m_type = type; }

  /// @brief Set peripheral index.
  ///
  /// @param[in] index %Peripheral index number
  void SetPeripheralIndex(unsigned int index) { m_peripheralIndex = index; }

  /// @brief Set driver index.
  ///
  /// @param[in] index Driver index number
  void SetDriverIndex(unsigned int index) { m_driverIndex = index; }

  /// @brief Set button state.
  ///
  /// @param[in] state Button state as @ref JOYSTICK_STATE_BUTTON
  void SetButtonState(JOYSTICK_STATE_BUTTON state) { m_buttonState = state; }

  /// @brief Set hat state.
  ///
  /// @param[in] state Hat state as @ref JOYSTICK_STATE_HAT (float)
  void SetHatState(JOYSTICK_STATE_HAT state) { m_hatState = state; }

  /// @brief Set axis state.
  ///
  /// @param[in] state Axis state as @ref JOYSTICK_STATE_AXIS (float)
  void SetAxisState(JOYSTICK_STATE_AXIS state) { m_axisState = state; }

  /// @brief Set motor state.
  ///
  /// @param[in] state Motor state as @ref JOYSTICK_STATE_MOTOR (float)
  void SetMotorState(JOYSTICK_STATE_MOTOR state) { m_motorState = state; }

  ///@}

  explicit PeripheralEvent(const PERIPHERAL_EVENT& event)
    : m_type(event.type),
      m_peripheralIndex(event.peripheral_index),
      m_driverIndex(event.driver_index),
      m_buttonState(event.driver_button_state),
      m_hatState(event.driver_hat_state),
      m_axisState(event.driver_axis_state),
      m_motorState(event.motor_state)
  {
  }

  void ToStruct(PERIPHERAL_EVENT& event) const
  {
    event.type = m_type;
    event.peripheral_index = m_peripheralIndex;
    event.driver_index = m_driverIndex;
    event.driver_button_state = m_buttonState;
    event.driver_hat_state = m_hatState;
    event.driver_axis_state = m_axisState;
    event.motor_state = m_motorState;
  }

  static void FreeStruct(PERIPHERAL_EVENT& event) { (void)event; }

private:
  PERIPHERAL_EVENT_TYPE m_type = PERIPHERAL_EVENT_TYPE_NONE;
  unsigned int m_peripheralIndex = 0;
  unsigned int m_driverIndex = 0;
  JOYSTICK_STATE_BUTTON m_buttonState = JOYSTICK_STATE_BUTTON_UNPRESSED;
  JOYSTICK_STATE_HAT m_hatState = JOYSTICK_STATE_HAT_UNPRESSED;
  JOYSTICK_STATE_AXIS m_axisState = 0.0f;
  JOYSTICK_STATE_MOTOR m_motorState = 0.0f;
};
///@}
//------------------------------------------------------------------------------

typedef PeripheralVector<PeripheralEvent, PERIPHERAL_EVENT> PeripheralEvents;

//==============================================================================
/// @defgroup cpp_kodi_addon_peripheral_Defs_Joystick_Joystick class Joystick
/// @ingroup cpp_kodi_addon_peripheral_Defs_Joystick
/// @brief **Wrapper class providing additional joystick information**\n
/// This is a child class to expand another class with necessary joystick data.
///
/// For data not provided by @ref cpp_kodi_addon_peripheral_Defs_Peripheral_Peripheral.
///
/// Used on:
/// - @ref kodi::addon::CInstancePeripheral::GetJoystickInfo()
/// - @ref kodi::addon::CInstancePeripheral::GetFeatures().
/// - @ref kodi::addon::CInstancePeripheral::MapFeatures().
/// - @ref kodi::addon::CInstancePeripheral::GetIgnoredPrimitives().
/// - @ref kodi::addon::CInstancePeripheral::SetIgnoredPrimitives().
/// - @ref kodi::addon::CInstancePeripheral::SaveButtonMap().
/// - @ref kodi::addon::CInstancePeripheral::RevertButtonMap().
/// - @ref kodi::addon::CInstancePeripheral::ResetButtonMap().
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_peripheral_Defs_Joystick_Joystick_Help
///
///@{
class Joystick : public Peripheral
{
public:
  /// @defgroup cpp_kodi_addon_peripheral_Defs_Joystick_Joystick_Help Value Help
  /// @ingroup cpp_kodi_addon_peripheral_Defs_Joystick_Joystick
  /// ----------------------------------------------------------------------------
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_peripheral_Defs_Joystick_Joystick :</b>
  /// | Name | Type | Class | Set call | Get call
  /// |------|------|-------|----------|----------
  /// | **%Joystick provider** | `const std::string&` | @ref Joystick | @ref Joystick::SetProvider "SetProvider" | @ref Joystick::Provider "Provider"
  /// | **%Joystick requested port** | `int` | @ref Joystick | @ref Joystick::SetRequestedPort "SetRequestedPort" | @ref Joystick::RequestedPort "RequestedPort"
  /// | **%Joystick button count** | `unsigned int` | @ref Joystick | @ref Joystick::SetButtonCount "SetButtonCount" | @ref Joystick::ButtonCount "ButtonCount"
  /// | **%Joystick hat count** | `unsigned int` | @ref Joystick | @ref Joystick::SetHatCount "SetHatCount" | @ref Joystick::HatCount "HatCount"
  /// | **%Joystick axis count** | `unsigned int` | @ref Joystick | @ref Joystick::SetAxisCount "SetAxisCount" | @ref Joystick::AxisCount "AxisCount"
  /// | **%Joystick motor count** | `unsigned int` | @ref Joystick | @ref Joystick::SetMotorCount "SetMotorCount" | @ref Joystick::MotorCount "MotorCount"
  /// | **%Joystick support power off** | `bool` | @ref Joystick | @ref Joystick::SetSupportsPowerOff "SetSupportsPowerOff" | @ref Joystick::SupportsPowerOff "SupportsPowerOff"
  /// | **%Peripheral type** | @ref PERIPHERAL_TYPE | @ref Peripheral | @ref Peripheral::SetType "SetType" | @ref Peripheral::Type "Type"
  /// | **%Peripheral name** | `const std::string&` | @ref Peripheral | @ref Peripheral::SetName "SetName" | @ref Peripheral::Name "Name"
  /// | **%Peripheral vendor id** | `uint16_t` | @ref Peripheral | @ref Peripheral::SetVendorID "SetVendorID" | @ref Peripheral::VendorID "VendorID"
  /// | **%Peripheral product id** | `uint16_t` | @ref Peripheral | @ref Peripheral::SetProductID "SetProductID" | @ref Peripheral::ProductID "ProductID"
  /// | **%Peripheral index** | `unsigned int` | @ref Peripheral | @ref Peripheral::SetIndex "SetIndex" | @ref Peripheral::Index "Index"
  ///
  /// Further are following included:
  /// - @ref Joystick::Joystick "Joystick(const std::string& provider = \"\", const std::string& strName = \"\")"
  /// - @ref Joystick::operator= "Joystick& operator=(const Joystick& rhs)"
  /// - @ref Peripheral::IsVidPidKnown "IsVidPidKnown()": To check VID and PID are known.
  ///

  /// @addtogroup cpp_kodi_addon_peripheral_Defs_Joystick_Joystick
  ///@{

  /// @brief Constructor.
  ///
  /// @param[in] provider [optional] Provide name
  /// @param[in] strName [optional] Name of related joystick
  Joystick(const std::string& provider = "", const std::string& strName = "")
    : Peripheral(PERIPHERAL_TYPE_JOYSTICK, strName),
      m_provider(provider),
      m_requestedPort(NO_PORT_REQUESTED)
  {
  }

  /// @brief Class copy constructor.
  ///
  /// @param[in] other Other class to copy on construct here
  Joystick(const Joystick& other) : Peripheral(other) { *this = other; }

  /// @brief Destructor.
  ///
  ~Joystick(void) override = default;

  /// @brief Copy data from another @ref Joystick class to here.
  ///
  /// @param[in] other Other class to copy here
  Joystick& operator=(const Joystick& rhs)
  {
    if (this != &rhs)
    {
      Peripheral::operator=(rhs);

      m_provider = rhs.m_provider;
      m_requestedPort = rhs.m_requestedPort;
      m_buttonCount = rhs.m_buttonCount;
      m_hatCount = rhs.m_hatCount;
      m_axisCount = rhs.m_axisCount;
      m_motorCount = rhs.m_motorCount;
      m_supportsPowerOff = rhs.m_supportsPowerOff;
    }
    return *this;
  }

  /// @brief Comparison operator
  bool operator==(const Joystick& rhs) const
  {
    // clang-format off
    return Peripheral::operator==(rhs) &&
           m_provider == rhs.m_provider &&
           m_requestedPort == rhs.m_requestedPort &&
           m_buttonCount == rhs.m_buttonCount &&
           m_hatCount == rhs.m_hatCount &&
           m_axisCount == rhs.m_axisCount &&
           m_motorCount == rhs.m_motorCount &&
           m_supportsPowerOff == rhs.m_supportsPowerOff;
    // clang-format on
  }

  /// @brief Get provider name.
  ///
  /// @return Name of provider
  const std::string& Provider(void) const { return m_provider; }

  /// @brief Get requested port number.
  ///
  /// @return Port
  int RequestedPort(void) const { return m_requestedPort; }

  /// @brief Get button count.
  ///
  /// @return Button count
  unsigned int ButtonCount(void) const { return m_buttonCount; }

  /// @brief Get hat count.
  ///
  /// @return Hat count
  unsigned int HatCount(void) const { return m_hatCount; }

  /// @brief Get axis count.
  ///
  /// @return Axis count
  unsigned int AxisCount(void) const { return m_axisCount; }

  /// @brief Get motor count.
  ///
  /// @return Motor count
  unsigned int MotorCount(void) const { return m_motorCount; }

  /// @brief Get supports power off.
  ///
  /// @return True if power off is supported, false otherwise
  bool SupportsPowerOff(void) const { return m_supportsPowerOff; }

  /// @brief Set provider name.
  ///
  /// @param[in] provider Name of provider
  void SetProvider(const std::string& provider) { m_provider = provider; }

  /// @brief Get requested port number.
  ///
  /// @param[in] requestedPort Port
  void SetRequestedPort(int requestedPort) { m_requestedPort = requestedPort; }

  /// @brief Get button count.
  ///
  /// @param[in] buttonCount Button count
  void SetButtonCount(unsigned int buttonCount) { m_buttonCount = buttonCount; }

  /// @brief Get hat count.
  ///
  /// @param[in] hatCount Hat count
  void SetHatCount(unsigned int hatCount) { m_hatCount = hatCount; }

  /// @brief Get axis count.
  ///
  /// @param[in] axisCount Axis count
  void SetAxisCount(unsigned int axisCount) { m_axisCount = axisCount; }

  /// @brief Get motor count.
  ///
  /// @param[in] motorCount Motor count
  void SetMotorCount(unsigned int motorCount) { m_motorCount = motorCount; }

  /// @brief Get supports power off.
  ///
  /// @param[in] supportsPowerOff True if power off is supported, false otherwise
  void SetSupportsPowerOff(bool supportsPowerOff) { m_supportsPowerOff = supportsPowerOff; }

  ///@}

  explicit Joystick(const JOYSTICK_INFO& info)
    : Peripheral(info.peripheral),
      m_provider(info.provider ? info.provider : ""),
      m_requestedPort(info.requested_port),
      m_buttonCount(info.button_count),
      m_hatCount(info.hat_count),
      m_axisCount(info.axis_count),
      m_motorCount(info.motor_count),
      m_supportsPowerOff(info.supports_poweroff)
  {
  }

  void ToStruct(JOYSTICK_INFO& info) const
  {
    Peripheral::ToStruct(info.peripheral);

    info.provider = new char[m_provider.size() + 1];
    info.requested_port = m_requestedPort;
    info.button_count = m_buttonCount;
    info.hat_count = m_hatCount;
    info.axis_count = m_axisCount;
    info.motor_count = m_motorCount;
    info.supports_poweroff = m_supportsPowerOff;

    std::strcpy(info.provider, m_provider.c_str());
  }

  static void FreeStruct(JOYSTICK_INFO& info)
  {
    Peripheral::FreeStruct(info.peripheral);

    PERIPHERAL_SAFE_DELETE_ARRAY(info.provider);
  }

private:
  std::string m_provider;
  int m_requestedPort;
  unsigned int m_buttonCount = 0;
  unsigned int m_hatCount = 0;
  unsigned int m_axisCount = 0;
  unsigned int m_motorCount = 0;
  bool m_supportsPowerOff = false;
};
///@}
//------------------------------------------------------------------------------

typedef PeripheralVector<Joystick, JOYSTICK_INFO> Joysticks;

class JoystickFeature;

//==============================================================================
/// @defgroup cpp_kodi_addon_peripheral_Defs_Joystick_DriverPrimitive class DriverPrimitive
/// @ingroup cpp_kodi_addon_peripheral_Defs_Joystick
/// @brief **Base class for joystick driver primitives**
///
/// A driver primitive can be:
///
///   1. a button
///   2. a hat direction
///   3. a semiaxis (either the positive or negative half of an axis)
///   4. a motor
///   5. a keyboard key
///   6. a mouse button
///   7. a relative pointer direction
///
/// The type determines the fields in use:
///
///    Button:
///       - driver index
///
///    Hat direction:
///       - driver index
///       - hat direction
///
///    Semiaxis:
///       - driver index
///       - center
///       - semiaxis direction
///       - range
///
///    Motor:
///       - driver index
///
///    Key:
///       - key code
///
///    Mouse button:
///       - driver index
///
///    Relative pointer direction:
///       - relative pointer direction
///
///@{
struct DriverPrimitive
{
protected:
  /*!
   * @brief Construct a driver primitive of the specified type
   */
  DriverPrimitive(JOYSTICK_DRIVER_PRIMITIVE_TYPE type, unsigned int driverIndex)
    : m_type(type), m_driverIndex(driverIndex)
  {
  }

public:
  /// @addtogroup cpp_kodi_addon_peripheral_Defs_Joystick_DriverPrimitive
  ///@{

  /// @brief Construct an invalid driver primitive.
  DriverPrimitive(void) = default;

  /// @brief Construct a driver primitive representing a joystick button.
  ///
  /// @param[in] buttonIndex Index
  /// @return Created class
  static DriverPrimitive CreateButton(unsigned int buttonIndex)
  {
    return DriverPrimitive(JOYSTICK_DRIVER_PRIMITIVE_TYPE_BUTTON, buttonIndex);
  }

  /// @brief Construct a driver primitive representing one of the four direction
  /// arrows on a dpad.
  ///
  /// @param[in] hatIndex Hat index
  /// @param[in] direction With @ref JOYSTICK_DRIVER_HAT_DIRECTION defined direction
  DriverPrimitive(unsigned int hatIndex, JOYSTICK_DRIVER_HAT_DIRECTION direction)
    : m_type(JOYSTICK_DRIVER_PRIMITIVE_TYPE_HAT_DIRECTION),
      m_driverIndex(hatIndex),
      m_hatDirection(direction)
  {
  }

  /// @brief Construct a driver primitive representing the positive or negative
  /// half of an axis.
  ///
  /// @param[in] axisIndex Axis index
  /// @param[in] center Center
  /// @param[in] direction With @ref JOYSTICK_DRIVER_HAT_DIRECTION defined direction
  /// @param[in] range Range
  DriverPrimitive(unsigned int axisIndex,
                  int center,
                  JOYSTICK_DRIVER_SEMIAXIS_DIRECTION direction,
                  unsigned int range)
    : m_type(JOYSTICK_DRIVER_PRIMITIVE_TYPE_SEMIAXIS),
      m_driverIndex(axisIndex),
      m_center(center),
      m_semiAxisDirection(direction),
      m_range(range)
  {
  }

  /// @brief Construct a driver primitive representing a motor.
  ///
  /// @param[in] motorIndex Motor index number
  /// @return Constructed driver primitive representing a motor
  static DriverPrimitive CreateMotor(unsigned int motorIndex)
  {
    return DriverPrimitive(JOYSTICK_DRIVER_PRIMITIVE_TYPE_MOTOR, motorIndex);
  }

  /// @brief Construct a driver primitive representing a key on a keyboard.
  ///
  /// @param[in] keycode Keycode to use
  DriverPrimitive(std::string keycode)
    : m_type(JOYSTICK_DRIVER_PRIMITIVE_TYPE_KEY), m_keycode(std::move(keycode))
  {
  }

  /// @brief Construct a driver primitive representing a mouse button.
  ///
  /// @param[in] buttonIndex Index
  /// @return Constructed driver primitive representing a mouse button
  static DriverPrimitive CreateMouseButton(JOYSTICK_DRIVER_MOUSE_INDEX buttonIndex)
  {
    return DriverPrimitive(JOYSTICK_DRIVER_PRIMITIVE_TYPE_MOUSE_BUTTON,
                           static_cast<unsigned int>(buttonIndex));
  }

  /// @brief Construct a driver primitive representing one of the four
  /// direction in which a relative pointer can move
  ///
  /// @param[in] direction With @ref JOYSTICK_DRIVER_RELPOINTER_DIRECTION defined direction
  DriverPrimitive(JOYSTICK_DRIVER_RELPOINTER_DIRECTION direction)
    : m_type(JOYSTICK_DRIVER_PRIMITIVE_TYPE_RELPOINTER_DIRECTION), m_relPointerDirection(direction)
  {
  }

  /// @brief Get type of primitive.
  ///
  /// @return The with @ref JOYSTICK_DRIVER_PRIMITIVE_TYPE defined type
  JOYSTICK_DRIVER_PRIMITIVE_TYPE Type(void) const { return m_type; }

  /// @brief Get driver index.
  ///
  /// @return Index number
  unsigned int DriverIndex(void) const { return m_driverIndex; }

  /// @brief Get hat direction
  ///
  /// @return The with @ref JOYSTICK_DRIVER_HAT_DIRECTION defined direction
  JOYSTICK_DRIVER_HAT_DIRECTION HatDirection(void) const { return m_hatDirection; }

  /// @brief Get center
  ///
  /// @return Center
  int Center(void) const { return m_center; }

  /// @brief Get semi axis direction
  ///
  /// @return With @ref JOYSTICK_DRIVER_SEMIAXIS_DIRECTION defined direction
  JOYSTICK_DRIVER_SEMIAXIS_DIRECTION SemiAxisDirection(void) const { return m_semiAxisDirection; }

  /// @brief Get range.
  ///
  /// @return Range
  unsigned int Range(void) const { return m_range; }

  /// @brief Get key code as string.
  ///
  /// @return Key code
  const std::string& Keycode(void) const { return m_keycode; }

  /// @brief Get mouse index
  ///
  /// @return With @ref JOYSTICK_DRIVER_MOUSE_INDEX defined mouse index
  JOYSTICK_DRIVER_MOUSE_INDEX MouseIndex(void) const
  {
    return static_cast<JOYSTICK_DRIVER_MOUSE_INDEX>(m_driverIndex);
  }

  /// @brief Get relative pointer direction.
  ///
  /// @return With @ref JOYSTICK_DRIVER_RELPOINTER_DIRECTION defined direction
  JOYSTICK_DRIVER_RELPOINTER_DIRECTION RelPointerDirection(void) const
  {
    return m_relPointerDirection;
  }

  /// @brief Compare this with another class of this type.
  ///
  /// @param[in] other Other class to compare
  /// @return True if they are equal, false otherwise
  bool operator==(const DriverPrimitive& other) const
  {
    if (m_type == other.m_type)
    {
      switch (m_type)
      {
        case JOYSTICK_DRIVER_PRIMITIVE_TYPE_BUTTON:
        {
          return m_driverIndex == other.m_driverIndex;
        }
        case JOYSTICK_DRIVER_PRIMITIVE_TYPE_HAT_DIRECTION:
        {
          return m_driverIndex == other.m_driverIndex && m_hatDirection == other.m_hatDirection;
        }
        case JOYSTICK_DRIVER_PRIMITIVE_TYPE_SEMIAXIS:
        {
          return m_driverIndex == other.m_driverIndex && m_center == other.m_center &&
                 m_semiAxisDirection == other.m_semiAxisDirection && m_range == other.m_range;
        }
        case JOYSTICK_DRIVER_PRIMITIVE_TYPE_KEY:
        {
          return m_keycode == other.m_keycode;
        }
        case JOYSTICK_DRIVER_PRIMITIVE_TYPE_MOTOR:
        {
          return m_driverIndex == other.m_driverIndex;
        }
        case JOYSTICK_DRIVER_PRIMITIVE_TYPE_MOUSE_BUTTON:
        {
          return m_driverIndex == other.m_driverIndex;
        }
        case JOYSTICK_DRIVER_PRIMITIVE_TYPE_RELPOINTER_DIRECTION:
        {
          return m_relPointerDirection == other.m_relPointerDirection;
        }
        default:
          break;
      }
    }
    return false;
  }

  ///@}

  explicit DriverPrimitive(const JOYSTICK_DRIVER_PRIMITIVE& primitive) : m_type(primitive.type)
  {
    switch (m_type)
    {
      case JOYSTICK_DRIVER_PRIMITIVE_TYPE_BUTTON:
      {
        m_driverIndex = primitive.button.index;
        break;
      }
      case JOYSTICK_DRIVER_PRIMITIVE_TYPE_HAT_DIRECTION:
      {
        m_driverIndex = primitive.hat.index;
        m_hatDirection = primitive.hat.direction;
        break;
      }
      case JOYSTICK_DRIVER_PRIMITIVE_TYPE_SEMIAXIS:
      {
        m_driverIndex = primitive.semiaxis.index;
        m_center = primitive.semiaxis.center;
        m_semiAxisDirection = primitive.semiaxis.direction;
        m_range = primitive.semiaxis.range;
        break;
      }
      case JOYSTICK_DRIVER_PRIMITIVE_TYPE_MOTOR:
      {
        m_driverIndex = primitive.motor.index;
        break;
      }
      case JOYSTICK_DRIVER_PRIMITIVE_TYPE_KEY:
      {
        m_keycode = primitive.key.keycode;
        break;
      }
      case JOYSTICK_DRIVER_PRIMITIVE_TYPE_MOUSE_BUTTON:
      {
        m_driverIndex = primitive.mouse.button;
        break;
      }
      case JOYSTICK_DRIVER_PRIMITIVE_TYPE_RELPOINTER_DIRECTION:
      {
        m_relPointerDirection = primitive.relpointer.direction;
        break;
      }
      default:
        break;
    }
  }

  void ToStruct(JOYSTICK_DRIVER_PRIMITIVE& driver_primitive) const
  {
    driver_primitive.type = m_type;
    switch (m_type)
    {
      case JOYSTICK_DRIVER_PRIMITIVE_TYPE_BUTTON:
      {
        driver_primitive.button.index = m_driverIndex;
        break;
      }
      case JOYSTICK_DRIVER_PRIMITIVE_TYPE_HAT_DIRECTION:
      {
        driver_primitive.hat.index = m_driverIndex;
        driver_primitive.hat.direction = m_hatDirection;
        break;
      }
      case JOYSTICK_DRIVER_PRIMITIVE_TYPE_SEMIAXIS:
      {
        driver_primitive.semiaxis.index = m_driverIndex;
        driver_primitive.semiaxis.center = m_center;
        driver_primitive.semiaxis.direction = m_semiAxisDirection;
        driver_primitive.semiaxis.range = m_range;
        break;
      }
      case JOYSTICK_DRIVER_PRIMITIVE_TYPE_MOTOR:
      {
        driver_primitive.motor.index = m_driverIndex;
        break;
      }
      case JOYSTICK_DRIVER_PRIMITIVE_TYPE_KEY:
      {
        const size_t size = sizeof(driver_primitive.key.keycode);
        std::strncpy(driver_primitive.key.keycode, m_keycode.c_str(), size - 1);
        driver_primitive.key.keycode[size - 1] = '\0';
        break;
      }
      case JOYSTICK_DRIVER_PRIMITIVE_TYPE_MOUSE_BUTTON:
      {
        driver_primitive.mouse.button = static_cast<JOYSTICK_DRIVER_MOUSE_INDEX>(m_driverIndex);
        break;
      }
      case JOYSTICK_DRIVER_PRIMITIVE_TYPE_RELPOINTER_DIRECTION:
      {
        driver_primitive.relpointer.direction = m_relPointerDirection;
        break;
      }
      default:
        break;
    }
  }

  static void FreeStruct(JOYSTICK_DRIVER_PRIMITIVE& primitive) { (void)primitive; }

private:
  JOYSTICK_DRIVER_PRIMITIVE_TYPE m_type = JOYSTICK_DRIVER_PRIMITIVE_TYPE_UNKNOWN;
  unsigned int m_driverIndex = 0;
  JOYSTICK_DRIVER_HAT_DIRECTION m_hatDirection = JOYSTICK_DRIVER_HAT_UNKNOWN;
  int m_center = 0;
  JOYSTICK_DRIVER_SEMIAXIS_DIRECTION m_semiAxisDirection = JOYSTICK_DRIVER_SEMIAXIS_UNKNOWN;
  unsigned int m_range = 1;
  std::string m_keycode;
  JOYSTICK_DRIVER_RELPOINTER_DIRECTION m_relPointerDirection = JOYSTICK_DRIVER_RELPOINTER_UNKNOWN;
};
///@}
//------------------------------------------------------------------------------

typedef PeripheralVector<DriverPrimitive, JOYSTICK_DRIVER_PRIMITIVE> DriverPrimitives;

//==============================================================================
/// @defgroup cpp_kodi_addon_peripheral_Defs_Joystick_JoystickFeature class JoystickFeature
/// @ingroup cpp_kodi_addon_peripheral_Defs_Joystick
/// @brief **Base class for joystick feature primitives**
///
/// Class for joystick features. A feature can be:
///
///   1. scalar *[1]*
///   2. analog stick
///   3. accelerometer
///   4. motor
///   5. relative pointer *[2]*
///   6. absolute pointer
///   7. wheel
///   8. throttle
///   9. keyboard key
///
/// *[1]* All three driver primitives (buttons, hats and axes) have a state that
///       can be represented using a single scalar value. For this reason,
///       features that map to a single primitive are called "scalar features".
///
/// *[2]* Relative pointers are similar to analog sticks, but they use
///       relative distances instead of positions.
///
///@{
class JoystickFeature
{
public:
  /// @addtogroup cpp_kodi_addon_peripheral_Defs_Joystick_JoystickFeature
  ///@{

  /// @brief Class constructor.
  ///
  /// @param[in] name [optional] Name of the feature
  /// @param[in] type [optional] Type of the feature, @ref JOYSTICK_FEATURE_TYPE_UNKNOWN
  ///                 as default
  JoystickFeature(const std::string& name = "",
                  JOYSTICK_FEATURE_TYPE type = JOYSTICK_FEATURE_TYPE_UNKNOWN)
    : m_name(name), m_type(type), m_primitives{}
  {
  }

  /// @brief Class copy constructor.
  ///
  /// @param[in] other Other class to copy on construct here
  JoystickFeature(const JoystickFeature& other) { *this = other; }

  /// @brief Copy data from another @ref JoystickFeature class to here.
  ///
  /// @param[in] other Other class to copy here
  JoystickFeature& operator=(const JoystickFeature& rhs)
  {
    if (this != &rhs)
    {
      m_name = rhs.m_name;
      m_type = rhs.m_type;
      m_primitives = rhs.m_primitives;
    }
    return *this;
  }

  /// @brief Compare this with another class of this type.
  ///
  /// @param[in] other Other class to compare
  /// @return True if they are equal, false otherwise
  bool operator==(const JoystickFeature& other) const
  {
    return m_name == other.m_name && m_type == other.m_type && m_primitives == other.m_primitives;
  }

  /// @brief Get name of feature.
  ///
  /// @return Name of feature
  const std::string& Name(void) const { return m_name; }

  /// @brief Get name of feature.
  ///
  /// @return Type of feature defined with @ref JOYSTICK_FEATURE_TYPE
  JOYSTICK_FEATURE_TYPE Type(void) const { return m_type; }

  /// @brief Check this feature is valid.
  ///
  /// @return True if valid (type != JOYSTICK_FEATURE_TYPE_UNKNOWN), false otherwise
  bool IsValid() const { return m_type != JOYSTICK_FEATURE_TYPE_UNKNOWN; }

  /// @brief Set name of feature.
  ///
  /// @param[in] name Name of feature
  void SetName(const std::string& name) { m_name = name; }

  /// @brief Set type of feature.
  ///
  /// @param[in] type Type of feature
  void SetType(JOYSTICK_FEATURE_TYPE type) { m_type = type; }

  /// @brief Set type as invalid.
  void SetInvalid(void) { m_type = JOYSTICK_FEATURE_TYPE_UNKNOWN; }

  /// @brief Get primitive of feature by wanted type.
  ///
  /// @param[in] which Type of feature, defined with @ref JOYSTICK_FEATURE_PRIMITIVE
  /// @return Primitive of asked type
  const DriverPrimitive& Primitive(JOYSTICK_FEATURE_PRIMITIVE which) const
  {
    return m_primitives[which];
  }

  /// @brief Set primitive for feature by wanted type.
  ///
  /// @param[in] which Type of feature, defined with @ref JOYSTICK_FEATURE_PRIMITIVE
  /// @param[in] primitive The with @ref DriverPrimitive defined primitive to set
  void SetPrimitive(JOYSTICK_FEATURE_PRIMITIVE which, const DriverPrimitive& primitive)
  {
    m_primitives[which] = primitive;
  }

  /// @brief Get all primitives on this class.
  ///
  /// @return Array list of primitives
  std::array<DriverPrimitive, JOYSTICK_PRIMITIVE_MAX>& Primitives() { return m_primitives; }

  /// @brief Get all primitives on this class (as constant).
  ///
  /// @return Constant array list of primitives
  const std::array<DriverPrimitive, JOYSTICK_PRIMITIVE_MAX>& Primitives() const
  {
    return m_primitives;
  }

  ///@}

  explicit JoystickFeature(const JOYSTICK_FEATURE& feature)
    : m_name(feature.name ? feature.name : ""), m_type(feature.type)
  {
    for (unsigned int i = 0; i < JOYSTICK_PRIMITIVE_MAX; i++)
      m_primitives[i] = DriverPrimitive(feature.primitives[i]);
  }

  void ToStruct(JOYSTICK_FEATURE& feature) const
  {
    feature.name = new char[m_name.length() + 1];
    feature.type = m_type;
    for (unsigned int i = 0; i < JOYSTICK_PRIMITIVE_MAX; i++)
      m_primitives[i].ToStruct(feature.primitives[i]);

    std::strcpy(feature.name, m_name.c_str());
  }

  static void FreeStruct(JOYSTICK_FEATURE& feature) { PERIPHERAL_SAFE_DELETE_ARRAY(feature.name); }

private:
  std::string m_name;
  JOYSTICK_FEATURE_TYPE m_type;
  std::array<DriverPrimitive, JOYSTICK_PRIMITIVE_MAX> m_primitives;
};
///@}
//------------------------------------------------------------------------------

typedef PeripheralVector<JoystickFeature, JOYSTICK_FEATURE> JoystickFeatures;

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
