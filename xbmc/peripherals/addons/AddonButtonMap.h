/*
 *      Copyright (C) 2014-2017 Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "PeripheralAddon.h" // for FeatureMap
#include "addons/kodi-addon-dev-kit/include/kodi/addon-instance/PeripheralUtils.h"
#include "input/joysticks/interfaces/IButtonMap.h"
#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/JoystickTypes.h"
#include "peripherals/PeripheralTypes.h"
#include "threads/CriticalSection.h"

namespace PERIPHERALS
{
  class CPeripheral;

  class CAddonButtonMap : public KODI::JOYSTICK::IButtonMap
  {
  public:
    CAddonButtonMap(CPeripheral* device, const std::weak_ptr<CPeripheralAddon>& addon, const std::string& strControllerId);

    ~CAddonButtonMap(void) override;

    // Implementation of IButtonMap
    std::string ControllerID(void) const override { return m_strControllerId; }

    std::string DeviceName(void) const override;

    bool Load(void) override;

    void Reset(void) override;

    bool IsEmpty(void) const override;

    bool GetFeature(
      const KODI::JOYSTICK::CDriverPrimitive& primitive,
      KODI::JOYSTICK::FeatureName& feature
    ) override;

    KODI::JOYSTICK::FEATURE_TYPE GetFeatureType(
      const KODI::JOYSTICK::FeatureName& feature
    ) override;

    bool GetScalar(
      const KODI::JOYSTICK::FeatureName& feature,
      KODI::JOYSTICK::CDriverPrimitive& primitive
    ) override;

    void AddScalar(
      const KODI::JOYSTICK::FeatureName& feature,
      const KODI::JOYSTICK::CDriverPrimitive& primitive
    ) override;

    bool GetAnalogStick(
      const KODI::JOYSTICK::FeatureName& feature,
      KODI::JOYSTICK::ANALOG_STICK_DIRECTION direction,
      KODI::JOYSTICK::CDriverPrimitive& primitive
    ) override;

    void AddAnalogStick(
      const KODI::JOYSTICK::FeatureName& feature,
      KODI::JOYSTICK::ANALOG_STICK_DIRECTION direction,
      const KODI::JOYSTICK::CDriverPrimitive& primitive
    ) override;

    bool GetRelativePointer(
      const KODI::JOYSTICK::FeatureName& feature,
      KODI::JOYSTICK::RELATIVE_POINTER_DIRECTION direction,
      KODI::JOYSTICK::CDriverPrimitive& primitive
    ) override;

    void AddRelativePointer(
      const KODI::JOYSTICK::FeatureName& feature,
      KODI::JOYSTICK::RELATIVE_POINTER_DIRECTION direction,
      const KODI::JOYSTICK::CDriverPrimitive& primitive
    ) override;

    bool GetAccelerometer(
      const KODI::JOYSTICK::FeatureName& feature,
      KODI::JOYSTICK::CDriverPrimitive& positiveX,
      KODI::JOYSTICK::CDriverPrimitive& positiveY,
      KODI::JOYSTICK::CDriverPrimitive& positiveZ
    ) override;

    void AddAccelerometer(
      const KODI::JOYSTICK::FeatureName& feature,
      const KODI::JOYSTICK::CDriverPrimitive& positiveX,
      const KODI::JOYSTICK::CDriverPrimitive& positiveY,
      const KODI::JOYSTICK::CDriverPrimitive& positiveZ
    ) override;

    bool GetWheel(
      const KODI::JOYSTICK::FeatureName& feature,
      KODI::JOYSTICK::WHEEL_DIRECTION direction,
      KODI::JOYSTICK::CDriverPrimitive& primitive
    ) override;

    void AddWheel(
      const KODI::JOYSTICK::FeatureName& feature,
      KODI::JOYSTICK::WHEEL_DIRECTION direction,
      const KODI::JOYSTICK::CDriverPrimitive& primitive
    ) override;

    bool GetThrottle(
      const KODI::JOYSTICK::FeatureName& feature,
      KODI::JOYSTICK::THROTTLE_DIRECTION direction,
      KODI::JOYSTICK::CDriverPrimitive& primitive
    ) override;

    void AddThrottle(
      const KODI::JOYSTICK::FeatureName& feature,
      KODI::JOYSTICK::THROTTLE_DIRECTION direction,
      const KODI::JOYSTICK::CDriverPrimitive& primitive
    ) override;

    bool GetKey(
      const KODI::JOYSTICK::FeatureName& feature,
      KODI::JOYSTICK::CDriverPrimitive& primitive
    ) override;

    void AddKey(
      const KODI::JOYSTICK::FeatureName& feature,
      const KODI::JOYSTICK::CDriverPrimitive& primitive
    ) override;

    void SetIgnoredPrimitives(const std::vector<KODI::JOYSTICK::CDriverPrimitive>& primitives) override;

    bool IsIgnored(const KODI::JOYSTICK::CDriverPrimitive& primitive) override;

    bool GetAxisProperties(unsigned int axisIndex, int& center, unsigned int& range) override;

    void SaveButtonMap() override;

    void RevertButtonMap() override;

  private:
    typedef std::map<KODI::JOYSTICK::CDriverPrimitive, KODI::JOYSTICK::FeatureName> DriverMap;
    typedef std::vector<KODI::JOYSTICK::CDriverPrimitive> JoystickPrimitiveVector;

    // Utility functions
    static DriverMap CreateLookupTable(const FeatureMap& features);

    static JOYSTICK_FEATURE_PRIMITIVE GetAnalogStickIndex(KODI::JOYSTICK::ANALOG_STICK_DIRECTION dir);
    static JOYSTICK_FEATURE_PRIMITIVE GetRelativePointerIndex(KODI::JOYSTICK::RELATIVE_POINTER_DIRECTION dir);
    static JOYSTICK_FEATURE_PRIMITIVE GetPrimitiveIndex(KODI::JOYSTICK::WHEEL_DIRECTION dir);
    static JOYSTICK_FEATURE_PRIMITIVE GetPrimitiveIndex(KODI::JOYSTICK::THROTTLE_DIRECTION dir);

    CPeripheral* const  m_device;
    std::weak_ptr<CPeripheralAddon>  m_addon;
    const std::string   m_strControllerId;
    FeatureMap          m_features;
    DriverMap           m_driverMap;
    JoystickPrimitiveVector m_ignoredPrimitives;
    CCriticalSection    m_mutex;
  };
}
