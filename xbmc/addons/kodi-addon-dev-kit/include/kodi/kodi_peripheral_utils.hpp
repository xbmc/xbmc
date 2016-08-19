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

#include "kodi_peripheral_types.h"

#include <array> // Requires c++11
#include <cstring>
#include <map>
#include <string>
#include <vector>

#define PERIPHERAL_SAFE_DELETE(x)        do { delete   (x); (x) = NULL; } while (0)
#define PERIPHERAL_SAFE_DELETE_ARRAY(x)  do { delete[] (x); (x) = NULL; } while (0)

namespace ADDON
{
  /*!
   * Utility class to manipulate arrays of peripheral types.
   */
  template <class THE_CLASS, typename THE_STRUCT>
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

  /*!
   * ADDON::Peripheral
   *
   * Wrapper class providing peripheral information. Classes can extend
   * Peripheral to inherit peripheral properties.
   */
  class Peripheral
  {
  public:
    Peripheral(PERIPHERAL_TYPE type = PERIPHERAL_TYPE_UNKNOWN, const std::string& strName = "") :
      m_type(type),
      m_strName(strName),
      m_vendorId(0),
      m_productId(0),
      m_index(0)
    {
    }

    Peripheral(const PERIPHERAL_INFO& info) :
      m_type(info.type),
      m_strName(info.name ? info.name : ""),
      m_vendorId(info.vendor_id),
      m_productId(info.product_id),
      m_index(info.index)
    {
    }

    virtual ~Peripheral(void) { }

    PERIPHERAL_TYPE    Type(void) const      { return m_type; }
    const std::string& Name(void) const      { return m_strName; }
    uint16_t           VendorID(void) const  { return m_vendorId; }
    uint16_t           ProductID(void) const { return m_productId; }
    unsigned int       Index(void) const     { return m_index; }

    // Derived property: VID and PID are 0x0000 if unknown
    bool IsVidPidKnown(void) const { return m_vendorId != 0 || m_productId != 0; }

    void SetType(PERIPHERAL_TYPE type)       { m_type      = type; }
    void SetName(const std::string& strName) { m_strName   = strName; }
    void SetVendorID(uint16_t vendorId)      { m_vendorId  = vendorId; }
    void SetProductID(uint16_t productId)    { m_productId = productId; }
    void SetIndex(unsigned int index)        { m_index     = index; }

    void ToStruct(PERIPHERAL_INFO& info) const
    {
      info.type       = m_type;
      info.name       = new char[m_strName.size() + 1];
      info.vendor_id  = m_vendorId;
      info.product_id = m_productId;
      info.index      = m_index;

      std::strcpy(info.name, m_strName.c_str());
    }

    static void FreeStruct(PERIPHERAL_INFO& info)
    {
      PERIPHERAL_SAFE_DELETE_ARRAY(info.name);
    }

  private:
    PERIPHERAL_TYPE  m_type;
    std::string      m_strName;
    uint16_t         m_vendorId;
    uint16_t         m_productId;
    unsigned int     m_index;
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
    PeripheralEvent(void) :
      m_event()
    {
    }

    PeripheralEvent(unsigned int peripheralIndex, unsigned int buttonIndex, JOYSTICK_STATE_BUTTON state) :
      m_event()
    {
      SetType(PERIPHERAL_EVENT_TYPE_DRIVER_BUTTON);
      SetPeripheralIndex(peripheralIndex);
      SetDriverIndex(buttonIndex);
      SetButtonState(state);
    }

    PeripheralEvent(unsigned int peripheralIndex, unsigned int hatIndex, JOYSTICK_STATE_HAT state) :
      m_event()
    {
      SetType(PERIPHERAL_EVENT_TYPE_DRIVER_HAT);
      SetPeripheralIndex(peripheralIndex);
      SetDriverIndex(hatIndex);
      SetHatState(state);
    }

    PeripheralEvent(unsigned int peripheralIndex, unsigned int axisIndex, JOYSTICK_STATE_AXIS state) :
      m_event()
    {
      SetType(PERIPHERAL_EVENT_TYPE_DRIVER_AXIS);
      SetPeripheralIndex(peripheralIndex);
      SetDriverIndex(axisIndex);
      SetAxisState(state);
    }

    PeripheralEvent(const PERIPHERAL_EVENT& event) :
      m_event(event)
    {
    }

    PERIPHERAL_EVENT_TYPE Type(void) const            { return m_event.type; }
    unsigned int          PeripheralIndex(void) const { return m_event.peripheral_index; }
    unsigned int          DriverIndex(void) const     { return m_event.driver_index; }
    JOYSTICK_STATE_BUTTON ButtonState(void) const     { return m_event.driver_button_state; }
    JOYSTICK_STATE_HAT    HatState(void) const        { return m_event.driver_hat_state; }
    JOYSTICK_STATE_AXIS   AxisState(void) const       { return m_event.driver_axis_state; }
    JOYSTICK_STATE_MOTOR  MotorState(void) const      { return m_event.motor_state; }

    void SetType(PERIPHERAL_EVENT_TYPE type)         { m_event.type                = type; }
    void SetPeripheralIndex(unsigned int index)      { m_event.peripheral_index    = index; }
    void SetDriverIndex(unsigned int index)          { m_event.driver_index        = index; }
    void SetButtonState(JOYSTICK_STATE_BUTTON state) { m_event.driver_button_state = state; }
    void SetHatState(JOYSTICK_STATE_HAT state)       { m_event.driver_hat_state    = state; }
    void SetAxisState(JOYSTICK_STATE_AXIS state)     { m_event.driver_axis_state   = state; }
    void SetMotorState(JOYSTICK_STATE_MOTOR state)   { m_event.motor_state         = state; }

    void ToStruct(PERIPHERAL_EVENT& event) const
    {
      event = m_event;
    }

    static void FreeStruct(PERIPHERAL_EVENT& event)
    {
      (void)event;
    }

  private:
    PERIPHERAL_EVENT m_event;
  };

  typedef PeripheralVector<PeripheralEvent, PERIPHERAL_EVENT> PeripheralEvents;

  /*!
   * ADDON::Joystick
   *
   * Wrapper class providing additional joystick information not provided by
   * ADDON::Peripheral.
   */
  class Joystick : public Peripheral
  {
  public:
    Joystick(const std::string& provider = "", const std::string& strName = "") :
      Peripheral(PERIPHERAL_TYPE_JOYSTICK, strName),
      m_provider(provider),
      m_requestedPort(NO_PORT_REQUESTED),
      m_buttonCount(0),
      m_hatCount(0),
      m_axisCount(0),
      m_motorCount(0),
      m_supportsPowerOff(false)
    {
    }

    Joystick(const Joystick& other)
    {
      *this = other;
    }

    Joystick(const JOYSTICK_INFO& info) :
      Peripheral(info.peripheral),
      m_provider(info.provider ? info.provider : ""),
      m_requestedPort(info.requested_port),
      m_buttonCount(info.button_count),
      m_hatCount(info.hat_count),
      m_axisCount(info.axis_count),
      m_motorCount(info.motor_count),
      m_supportsPowerOff(info.supports_poweroff)
    {
    }

    virtual ~Joystick(void) { }

    Joystick& operator=(const Joystick& rhs)
    {
      if (this != &rhs)
      {
        Peripheral::operator=(rhs);

        m_provider      = rhs.m_provider;
        m_requestedPort = rhs.m_requestedPort;
        m_buttonCount   = rhs.m_buttonCount;
        m_hatCount      = rhs.m_hatCount;
        m_axisCount     = rhs.m_axisCount;
        m_motorCount    = rhs.m_motorCount;
        m_supportsPowerOff = rhs.m_supportsPowerOff;
      }
      return *this;
    }

    const std::string& Provider(void) const      { return m_provider; }
    int                RequestedPort(void) const { return m_requestedPort; }
    unsigned int       ButtonCount(void) const   { return m_buttonCount; }
    unsigned int       HatCount(void) const      { return m_hatCount; }
    unsigned int       AxisCount(void) const     { return m_axisCount; }
    unsigned int       MotorCount(void) const    { return m_motorCount; }
    bool               SupportsPowerOff(void) const { return m_supportsPowerOff; }

    // Derived property: Counts are unknown if all are zero
    bool AreElementCountsKnown(void) const { return m_buttonCount != 0 || m_hatCount != 0 || m_axisCount != 0; }

    void SetProvider(const std::string& provider)     { m_provider      = provider; }
    void SetRequestedPort(int requestedPort)          { m_requestedPort = requestedPort; }
    void SetButtonCount(unsigned int buttonCount)     { m_buttonCount   = buttonCount; }
    void SetHatCount(unsigned int hatCount)           { m_hatCount      = hatCount; }
    void SetAxisCount(unsigned int axisCount)         { m_axisCount     = axisCount; }
    void SetMotorCount(unsigned int motorCount)       { m_motorCount    = motorCount; }
    void SetSupportsPowerOff(bool supportsPowerOff)   { m_supportsPowerOff = supportsPowerOff; }

    void ToStruct(JOYSTICK_INFO& info) const
    {
      Peripheral::ToStruct(info.peripheral);

      info.provider       = new char[m_provider.size() + 1];
      info.requested_port = m_requestedPort;
      info.button_count   = m_buttonCount;
      info.hat_count      = m_hatCount;
      info.axis_count     = m_axisCount;
      info.motor_count    = m_motorCount;
      info.supports_poweroff = m_supportsPowerOff;

      std::strcpy(info.provider, m_provider.c_str());
    }

    static void FreeStruct(JOYSTICK_INFO& info)
    {
      Peripheral::FreeStruct(info.peripheral);

      PERIPHERAL_SAFE_DELETE_ARRAY(info.provider);
    }

  private:
    std::string                   m_provider;
    int                           m_requestedPort;
    unsigned int                  m_buttonCount;
    unsigned int                  m_hatCount;
    unsigned int                  m_axisCount;
    unsigned int                  m_motorCount;
    bool                          m_supportsPowerOff;
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
   *       - semiaxis direction
   *
   *    Motor:
   *       - driver index
   */
  struct DriverPrimitive
  {
  protected:
    /*!
     * \brief Construct a driver primitive of the specified type
     */
    DriverPrimitive(JOYSTICK_DRIVER_PRIMITIVE_TYPE type, unsigned int driverIndex) :
      m_type(type),
      m_driverIndex(driverIndex),
      m_hatDirection(JOYSTICK_DRIVER_HAT_UNKNOWN),
      m_semiAxisDirection(JOYSTICK_DRIVER_SEMIAXIS_UNKNOWN)
    {
    }

  public:
    /*!
     * \brief Construct an invalid driver primitive
     */
    DriverPrimitive(void) :
      m_type(JOYSTICK_DRIVER_PRIMITIVE_TYPE_UNKNOWN),
      m_driverIndex(0),
      m_hatDirection(JOYSTICK_DRIVER_HAT_UNKNOWN),
      m_semiAxisDirection(JOYSTICK_DRIVER_SEMIAXIS_UNKNOWN)
    {
    }

    /*!
     * \brief Construct a driver primitive representing a button
     */
    static DriverPrimitive CreateButton(unsigned int buttonIndex)
    {
      return DriverPrimitive(JOYSTICK_DRIVER_PRIMITIVE_TYPE_BUTTON, buttonIndex);
    }

    /*!
     * \brief Construct a driver primitive representing one of the four direction
     *        arrows on a dpad
     */
    DriverPrimitive(unsigned int hatIndex, JOYSTICK_DRIVER_HAT_DIRECTION direction) :
      m_type(JOYSTICK_DRIVER_PRIMITIVE_TYPE_HAT_DIRECTION),
      m_driverIndex(hatIndex),
      m_hatDirection(direction),
      m_semiAxisDirection(JOYSTICK_DRIVER_SEMIAXIS_UNKNOWN)
    {
    }

    /*!
     * \brief Construct a driver primitive representing the positive or negative
     *        half of an axis
     */
    DriverPrimitive(unsigned int axisIndex, JOYSTICK_DRIVER_SEMIAXIS_DIRECTION direction) :
      m_type(JOYSTICK_DRIVER_PRIMITIVE_TYPE_SEMIAXIS),
      m_driverIndex(axisIndex),
      m_hatDirection(JOYSTICK_DRIVER_HAT_UNKNOWN),
      m_semiAxisDirection(direction)
    {
    }

    /*!
     * \brief Construct a driver primitive representing a motor
     */
    static DriverPrimitive CreateMotor(unsigned int motorIndex)
    {
      return DriverPrimitive(JOYSTICK_DRIVER_PRIMITIVE_TYPE_MOTOR, motorIndex);
    }

    DriverPrimitive(const JOYSTICK_DRIVER_PRIMITIVE& primitive) :
      m_type(primitive.type),
      m_driverIndex(0),
      m_hatDirection(JOYSTICK_DRIVER_HAT_UNKNOWN),
      m_semiAxisDirection(JOYSTICK_DRIVER_SEMIAXIS_UNKNOWN)
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
          m_driverIndex  = primitive.hat.index;
          m_hatDirection = primitive.hat.direction;
          break;
        }
        case JOYSTICK_DRIVER_PRIMITIVE_TYPE_SEMIAXIS:
        {
          m_driverIndex       = primitive.semiaxis.index;
          m_semiAxisDirection = primitive.semiaxis.direction;
          break;
        }
        case JOYSTICK_DRIVER_PRIMITIVE_TYPE_MOTOR:
        {
          m_driverIndex = primitive.motor.index;
          break;
        }
        default:
          break;
      }
    }

    JOYSTICK_DRIVER_PRIMITIVE_TYPE     Type(void) const { return m_type; }
    unsigned int                       DriverIndex(void) const { return m_driverIndex; }
    JOYSTICK_DRIVER_HAT_DIRECTION      HatDirection(void) const { return m_hatDirection; }
    JOYSTICK_DRIVER_SEMIAXIS_DIRECTION SemiAxisDirection(void) const { return m_semiAxisDirection; }

    bool operator==(const DriverPrimitive& other) const
    {
      if (m_type == other.m_type)
      {
        switch (m_type)
        {
          case JOYSTICK_DRIVER_PRIMITIVE_TYPE_BUTTON:
          case JOYSTICK_DRIVER_PRIMITIVE_TYPE_MOTOR:
          {
            return m_driverIndex == other.m_driverIndex;
          }
          case JOYSTICK_DRIVER_PRIMITIVE_TYPE_HAT_DIRECTION:
          {
            return m_driverIndex  == other.m_driverIndex &&
                   m_hatDirection == other.m_hatDirection;
          }
          case JOYSTICK_DRIVER_PRIMITIVE_TYPE_SEMIAXIS:
          {
            return m_driverIndex       == other.m_driverIndex &&
                   m_semiAxisDirection == other.m_semiAxisDirection;
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
          driver_primitive.hat.index     = m_driverIndex;
          driver_primitive.hat.direction = m_hatDirection;
          break;
        }
        case JOYSTICK_DRIVER_PRIMITIVE_TYPE_SEMIAXIS:
        {
          driver_primitive.semiaxis.index     = m_driverIndex;
          driver_primitive.semiaxis.direction = m_semiAxisDirection;
          break;
        }
        case JOYSTICK_DRIVER_PRIMITIVE_TYPE_MOTOR:
        {
          driver_primitive.motor.index = m_driverIndex;
          break;
        }
        default:
          break;
      }
    }

  private:
    JOYSTICK_DRIVER_PRIMITIVE_TYPE     m_type;
    unsigned int                       m_driverIndex;
    JOYSTICK_DRIVER_HAT_DIRECTION      m_hatDirection;
    JOYSTICK_DRIVER_SEMIAXIS_DIRECTION m_semiAxisDirection;
  };

  /*!
   * ADDON::JoystickFeature
   *
   * Class for joystick features. A feature can be:
   *
   *   1) scalar[1]
   *   2) analog stick
   *   3) accelerometer
   *   4) motor
   *
   * [1] All three driver primitives (buttons, hats and axes) have a state that
   *     can be represented using a single scalar value. For this reason,
   *     features that map to a single primitive are called "scalar features".
   *
   * Features can be mapped to a variable number of driver primitives. The names
   * of the primitives for each feature are:
   *
   *    Scalar feature:
   *       - primitive
   *
   *    Analog stick:
   *       - up
   *       - down
   *       - right
   *       - left
   *
   *    Accelerometer:
   *       - positive X
   *       - positive Y
   *       - positive Z
   *
   *    Motor:
   *       - primitive
   */
  class JoystickFeature
  {
  public:
    enum { MAX_PRIMITIVES = 4 };

    JoystickFeature(const std::string& name = "", JOYSTICK_FEATURE_TYPE type = JOYSTICK_FEATURE_TYPE_UNKNOWN) :
      m_name(name),
      m_type(type)
    {
    }

    JoystickFeature(const JoystickFeature& other)
    {
      *this = other;
    }

    JoystickFeature(const JOYSTICK_FEATURE& feature) :
      m_name(feature.name ? feature.name : ""),
      m_type(feature.type)
    {
      switch (m_type)
      {
        case JOYSTICK_FEATURE_TYPE_SCALAR:
          SetPrimitive(feature.scalar.primitive);
          break;
        case JOYSTICK_FEATURE_TYPE_ANALOG_STICK:
          SetUp(feature.analog_stick.up);
          SetDown(feature.analog_stick.down);
          SetRight(feature.analog_stick.right);
          SetLeft(feature.analog_stick.left);
          break;
        case JOYSTICK_FEATURE_TYPE_ACCELEROMETER:
          SetPositiveX(feature.accelerometer.positive_x);
          SetPositiveY(feature.accelerometer.positive_y);
          SetPositiveZ(feature.accelerometer.positive_z);
          break;
        case JOYSTICK_FEATURE_TYPE_MOTOR:
          SetPrimitive(feature.motor.primitive);
          break;
        default:
          break;
      }
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
      return m_name == other.m_name &&
             m_type == other.m_type &&
             m_primitives == other.m_primitives;
    }

    const std::string& Name(void) const { return m_name; }
    JOYSTICK_FEATURE_TYPE Type(void) const { return m_type; }

    void SetName(const std::string& name) { m_name = name; }
    void SetType(JOYSTICK_FEATURE_TYPE type) { m_type = type; }

    // Scalar methods
    const DriverPrimitive& Primitive(void) const { return m_primitives[0]; }
    void SetPrimitive(const DriverPrimitive& primitive) { m_primitives[0] = primitive; }

    // Analog stick methods
    const DriverPrimitive& Up(void) const { return m_primitives[0]; }
    const DriverPrimitive& Down(void) const { return m_primitives[1]; }
    const DriverPrimitive& Right(void) const { return m_primitives[2]; }
    const DriverPrimitive& Left(void) const { return m_primitives[3]; }
    void SetUp(const DriverPrimitive& primitive) { m_primitives[0] = primitive; }
    void SetDown(const DriverPrimitive& primitive) { m_primitives[1] = primitive; }
    void SetRight(const DriverPrimitive& primitive) { m_primitives[2] = primitive; }
    void SetLeft(const DriverPrimitive& primitive) { m_primitives[3] = primitive; }

    // Accelerometer methods
    const DriverPrimitive& PositiveX(void) const { return m_primitives[0]; }
    const DriverPrimitive& PositiveY(void) const { return m_primitives[1]; }
    const DriverPrimitive& PositiveZ(void) const { return m_primitives[2]; }
    void SetPositiveX(const DriverPrimitive& primitive) { m_primitives[0] = primitive; }
    void SetPositiveY(const DriverPrimitive& primitive) { m_primitives[1] = primitive; }
    void SetPositiveZ(const DriverPrimitive& primitive) { m_primitives[2] = primitive; }

    // Access primitives
    std::array<DriverPrimitive, MAX_PRIMITIVES>& Primitives() { return m_primitives; }
    const std::array<DriverPrimitive, MAX_PRIMITIVES>& Primitives() const { return m_primitives; }

    void ToStruct(JOYSTICK_FEATURE& feature) const
    {
      feature.name = new char[m_name.length() + 1];
      feature.type = m_type;
      switch (m_type)
      {
        case JOYSTICK_FEATURE_TYPE_SCALAR:
          Primitive().ToStruct(feature.scalar.primitive);
          break;
        case JOYSTICK_FEATURE_TYPE_ANALOG_STICK:
          Up().ToStruct(feature.analog_stick.up);
          Down().ToStruct(feature.analog_stick.down);
          Right().ToStruct(feature.analog_stick.right);
          Left().ToStruct(feature.analog_stick.left);
          break;
        case JOYSTICK_FEATURE_TYPE_ACCELEROMETER:
          PositiveX().ToStruct(feature.accelerometer.positive_x);
          PositiveY().ToStruct(feature.accelerometer.positive_y);
          PositiveZ().ToStruct(feature.accelerometer.positive_z);
          break;
        case JOYSTICK_FEATURE_TYPE_MOTOR:
          Primitive().ToStruct(feature.motor.primitive);
          break;
        default:
          break;
      }
      std::strcpy(feature.name, m_name.c_str());
    }

    static void FreeStruct(JOYSTICK_FEATURE& feature)
    {
      PERIPHERAL_SAFE_DELETE_ARRAY(feature.name);
    }

  private:
    std::string                  m_name;
    JOYSTICK_FEATURE_TYPE        m_type;
    std::array<DriverPrimitive, MAX_PRIMITIVES> m_primitives;
  };

  typedef PeripheralVector<JoystickFeature, JOYSTICK_FEATURE> JoystickFeatures;
}
