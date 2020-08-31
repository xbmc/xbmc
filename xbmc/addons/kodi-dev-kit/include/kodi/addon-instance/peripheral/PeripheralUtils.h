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

  void SetProvidesJoysticks(bool providesJoysticks)
  {
    m_cStructure->provides_joysticks = providesJoysticks;
  }
  bool GetProvidesJoysticks() const { return m_cStructure->provides_joysticks; }

  void SetProvidesJoystickRumble(bool providesJoystickRumble)
  {
    m_cStructure->provides_joystick_rumble = providesJoystickRumble;
  }
  bool GetProvidesJoystickRumble() const { return m_cStructure->provides_joystick_rumble; }

  void SetProvidesJoystickPowerOff(bool providesJoystickPowerOff)
  {
    m_cStructure->provides_joystick_power_off = providesJoystickPowerOff;
  }
  bool GetProvidesJoystickPowerOff() const { return m_cStructure->provides_joystick_power_off; }

  void SetProvidesButtonmaps(bool providesButtonmaps)
  {
    m_cStructure->provides_buttonmaps = providesButtonmaps;
  }
  bool GetProvidesButtonmaps() const { return m_cStructure->provides_buttonmaps; }

private:
  PeripheralCapabilities(const PERIPHERAL_CAPABILITIES* data) : CStructHdl(data) {}
  PeripheralCapabilities(PERIPHERAL_CAPABILITIES* data) : CStructHdl(data) {}
};

/*!
 * ADDON::Peripheral
 *
 * Wrapper class providing peripheral information. Classes can extend
 * Peripheral to inherit peripheral properties.
 */
class Peripheral
{
public:
  Peripheral(PERIPHERAL_TYPE type = PERIPHERAL_TYPE_UNKNOWN, const std::string& strName = "")
    : m_type(type), m_strName(strName)
  {
  }

  explicit Peripheral(const PERIPHERAL_INFO& info)
    : m_type(info.type),
      m_strName(info.name ? info.name : ""),
      m_vendorId(info.vendor_id),
      m_productId(info.product_id),
      m_index(info.index)
  {
  }

  virtual ~Peripheral(void) = default;

  PERIPHERAL_TYPE Type(void) const { return m_type; }
  const std::string& Name(void) const { return m_strName; }
  uint16_t VendorID(void) const { return m_vendorId; }
  uint16_t ProductID(void) const { return m_productId; }
  unsigned int Index(void) const { return m_index; }

  // Derived property: VID and PID are 0x0000 if unknown
  bool IsVidPidKnown(void) const { return m_vendorId != 0 || m_productId != 0; }

  void SetType(PERIPHERAL_TYPE type) { m_type = type; }
  void SetName(const std::string& strName) { m_strName = strName; }
  void SetVendorID(uint16_t vendorId) { m_vendorId = vendorId; }
  void SetProductID(uint16_t productId) { m_productId = productId; }
  void SetIndex(unsigned int index) { m_index = index; }

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

typedef PeripheralVector<Peripheral, PERIPHERAL_INFO> Peripherals;

/*!
 * ADDON::PeripheralEvent
 *
 * Wrapper class for peripheral events.
 */
class PeripheralEvent
{
public:
  PeripheralEvent() = default;

  PeripheralEvent(unsigned int peripheralIndex,
                  unsigned int buttonIndex,
                  JOYSTICK_STATE_BUTTON state)
    : m_type(PERIPHERAL_EVENT_TYPE_DRIVER_BUTTON),
      m_peripheralIndex(peripheralIndex),
      m_driverIndex(buttonIndex),
      m_buttonState(state)
  {
  }

  PeripheralEvent(unsigned int peripheralIndex, unsigned int hatIndex, JOYSTICK_STATE_HAT state)
    : m_type(PERIPHERAL_EVENT_TYPE_DRIVER_HAT),
      m_peripheralIndex(peripheralIndex),
      m_driverIndex(hatIndex),
      m_hatState(state)
  {
  }

  PeripheralEvent(unsigned int peripheralIndex, unsigned int axisIndex, JOYSTICK_STATE_AXIS state)
    : m_type(PERIPHERAL_EVENT_TYPE_DRIVER_AXIS),
      m_peripheralIndex(peripheralIndex),
      m_driverIndex(axisIndex),
      m_axisState(state)
  {
  }

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

  PERIPHERAL_EVENT_TYPE Type(void) const { return m_type; }
  unsigned int PeripheralIndex(void) const { return m_peripheralIndex; }
  unsigned int DriverIndex(void) const { return m_driverIndex; }
  JOYSTICK_STATE_BUTTON ButtonState(void) const { return m_buttonState; }
  JOYSTICK_STATE_HAT HatState(void) const { return m_hatState; }
  JOYSTICK_STATE_AXIS AxisState(void) const { return m_axisState; }
  JOYSTICK_STATE_MOTOR MotorState(void) const { return m_motorState; }

  void SetType(PERIPHERAL_EVENT_TYPE type) { m_type = type; }
  void SetPeripheralIndex(unsigned int index) { m_peripheralIndex = index; }
  void SetDriverIndex(unsigned int index) { m_driverIndex = index; }
  void SetButtonState(JOYSTICK_STATE_BUTTON state) { m_buttonState = state; }
  void SetHatState(JOYSTICK_STATE_HAT state) { m_hatState = state; }
  void SetAxisState(JOYSTICK_STATE_AXIS state) { m_axisState = state; }
  void SetMotorState(JOYSTICK_STATE_MOTOR state) { m_motorState = state; }

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

typedef PeripheralVector<PeripheralEvent, PERIPHERAL_EVENT> PeripheralEvents;

/*!
 * kodi::addon::Joystick
 *
 * Wrapper class providing additional joystick information not provided by
 * ADDON::Peripheral.
 */
class Joystick : public Peripheral
{
public:
  Joystick(const std::string& provider = "", const std::string& strName = "")
    : Peripheral(PERIPHERAL_TYPE_JOYSTICK, strName),
      m_provider(provider),
      m_requestedPort(NO_PORT_REQUESTED)
  {
  }

  Joystick(const Joystick& other) { *this = other; }

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

  ~Joystick(void) override = default;

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

  const std::string& Provider(void) const { return m_provider; }
  int RequestedPort(void) const { return m_requestedPort; }
  unsigned int ButtonCount(void) const { return m_buttonCount; }
  unsigned int HatCount(void) const { return m_hatCount; }
  unsigned int AxisCount(void) const { return m_axisCount; }
  unsigned int MotorCount(void) const { return m_motorCount; }
  bool SupportsPowerOff(void) const { return m_supportsPowerOff; }

  void SetProvider(const std::string& provider) { m_provider = provider; }
  void SetRequestedPort(int requestedPort) { m_requestedPort = requestedPort; }
  void SetButtonCount(unsigned int buttonCount) { m_buttonCount = buttonCount; }
  void SetHatCount(unsigned int hatCount) { m_hatCount = hatCount; }
  void SetAxisCount(unsigned int axisCount) { m_axisCount = axisCount; }
  void SetMotorCount(unsigned int motorCount) { m_motorCount = motorCount; }
  void SetSupportsPowerOff(bool supportsPowerOff) { m_supportsPowerOff = supportsPowerOff; }

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

typedef PeripheralVector<Joystick, JOYSTICK_INFO> Joysticks;

/*!
 * ADDON::DriverPrimitive
 *
 * Base class for joystick driver primitives. A driver primitive can be:
 *
 *   1) a button
 *   2) a hat direction
 *   3) a semiaxis (either the positive or negative half of an axis)
 *   4) a motor
 *   5) a keyboard key
 *   6) a mouse button
 *   7) a relative pointer direction
 *
 * The type determines the fields in use:
 *
 *    Button:
 *       - driver index
 *
 *    Hat direction:
 *       - driver index
 *       - hat direction
 *
 *    Semiaxis:
 *       - driver index
 *       - center
 *       - semiaxis direction
 *       - range
 *
 *    Motor:
 *       - driver index
 *
 *    Key:
 *       - key code
 *
 *    Mouse button:
 *       - driver index
 *
 *    Relative pointer direction:
 *       - relative pointer direction
 */
struct DriverPrimitive
{
protected:
  /*!
   * \brief Construct a driver primitive of the specified type
   */
  DriverPrimitive(JOYSTICK_DRIVER_PRIMITIVE_TYPE type, unsigned int driverIndex)
    : m_type(type), m_driverIndex(driverIndex)
  {
  }

public:
  /*!
   * \brief Construct an invalid driver primitive
   */
  DriverPrimitive(void) = default;

  /*!
   * \brief Construct a driver primitive representing a joystick button
   */
  static DriverPrimitive CreateButton(unsigned int buttonIndex)
  {
    return DriverPrimitive(JOYSTICK_DRIVER_PRIMITIVE_TYPE_BUTTON, buttonIndex);
  }

  /*!
   * \brief Construct a driver primitive representing one of the four direction
   *        arrows on a dpad
   */
  DriverPrimitive(unsigned int hatIndex, JOYSTICK_DRIVER_HAT_DIRECTION direction)
    : m_type(JOYSTICK_DRIVER_PRIMITIVE_TYPE_HAT_DIRECTION),
      m_driverIndex(hatIndex),
      m_hatDirection(direction)
  {
  }

  /*!
   * \brief Construct a driver primitive representing the positive or negative
   *        half of an axis
   */
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

  /*!
   * \brief Construct a driver primitive representing a motor
   */
  static DriverPrimitive CreateMotor(unsigned int motorIndex)
  {
    return DriverPrimitive(JOYSTICK_DRIVER_PRIMITIVE_TYPE_MOTOR, motorIndex);
  }

  /*!
   * \brief Construct a driver primitive representing a key on a keyboard
   */
  DriverPrimitive(std::string keycode)
    : m_type(JOYSTICK_DRIVER_PRIMITIVE_TYPE_KEY), m_keycode(std::move(keycode))
  {
  }

  /*!
   * \brief Construct a driver primitive representing a mouse button
   */
  static DriverPrimitive CreateMouseButton(JOYSTICK_DRIVER_MOUSE_INDEX buttonIndex)
  {
    return DriverPrimitive(JOYSTICK_DRIVER_PRIMITIVE_TYPE_MOUSE_BUTTON,
                           static_cast<unsigned int>(buttonIndex));
  }

  /*!
   * \brief Construct a driver primitive representing one of the four
   *        direction in which a relative pointer can move
   */
  DriverPrimitive(JOYSTICK_DRIVER_RELPOINTER_DIRECTION direction)
    : m_type(JOYSTICK_DRIVER_PRIMITIVE_TYPE_RELPOINTER_DIRECTION), m_relPointerDirection(direction)
  {
  }

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

  JOYSTICK_DRIVER_PRIMITIVE_TYPE Type(void) const { return m_type; }
  unsigned int DriverIndex(void) const { return m_driverIndex; }
  JOYSTICK_DRIVER_HAT_DIRECTION HatDirection(void) const { return m_hatDirection; }
  int Center(void) const { return m_center; }
  JOYSTICK_DRIVER_SEMIAXIS_DIRECTION SemiAxisDirection(void) const { return m_semiAxisDirection; }
  unsigned int Range(void) const { return m_range; }
  const std::string& Keycode(void) const { return m_keycode; }
  JOYSTICK_DRIVER_MOUSE_INDEX MouseIndex(void) const
  {
    return static_cast<JOYSTICK_DRIVER_MOUSE_INDEX>(m_driverIndex);
  }
  JOYSTICK_DRIVER_RELPOINTER_DIRECTION RelPointerDirection(void) const
  {
    return m_relPointerDirection;
  }

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

typedef PeripheralVector<DriverPrimitive, JOYSTICK_DRIVER_PRIMITIVE> DriverPrimitives;

/*!
 * kodi::addon::JoystickFeature
 *
 * Class for joystick features. A feature can be:
 *
 *   1) scalar[1]
 *   2) analog stick
 *   3) accelerometer
 *   4) motor
 *   5) relative pointer[2]
 *   6) absolute pointer
 *   7) wheel
 *   8) throttle
 *   9) keyboard key
 *
 * [1] All three driver primitives (buttons, hats and axes) have a state that
 *     can be represented using a single scalar value. For this reason,
 *     features that map to a single primitive are called "scalar features".
 *
 * [2] Relative pointers are similar to analog sticks, but they use
 *     relative distances instead of positions.
 */
class JoystickFeature
{
public:
  JoystickFeature(const std::string& name = "",
                  JOYSTICK_FEATURE_TYPE type = JOYSTICK_FEATURE_TYPE_UNKNOWN)
    : m_name(name), m_type(type), m_primitives{}
  {
  }

  JoystickFeature(const JoystickFeature& other) { *this = other; }

  explicit JoystickFeature(const JOYSTICK_FEATURE& feature)
    : m_name(feature.name ? feature.name : ""), m_type(feature.type)
  {
    for (unsigned int i = 0; i < JOYSTICK_PRIMITIVE_MAX; i++)
      m_primitives[i] = DriverPrimitive(feature.primitives[i]);
  }

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

  bool operator==(const JoystickFeature& other) const
  {
    return m_name == other.m_name && m_type == other.m_type && m_primitives == other.m_primitives;
  }

  const std::string& Name(void) const { return m_name; }
  JOYSTICK_FEATURE_TYPE Type(void) const { return m_type; }
  bool IsValid() const { return m_type != JOYSTICK_FEATURE_TYPE_UNKNOWN; }

  void SetName(const std::string& name) { m_name = name; }
  void SetType(JOYSTICK_FEATURE_TYPE type) { m_type = type; }
  void SetInvalid(void) { m_type = JOYSTICK_FEATURE_TYPE_UNKNOWN; }

  const DriverPrimitive& Primitive(JOYSTICK_FEATURE_PRIMITIVE which) const
  {
    return m_primitives[which];
  }
  void SetPrimitive(JOYSTICK_FEATURE_PRIMITIVE which, const DriverPrimitive& primitive)
  {
    m_primitives[which] = primitive;
  }

  std::array<DriverPrimitive, JOYSTICK_PRIMITIVE_MAX>& Primitives() { return m_primitives; }
  const std::array<DriverPrimitive, JOYSTICK_PRIMITIVE_MAX>& Primitives() const
  {
    return m_primitives;
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

typedef PeripheralVector<JoystickFeature, JOYSTICK_FEATURE> JoystickFeatures;

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
