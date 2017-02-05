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

#include "PeripheralAddon.h" // for FeatureMap
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_peripheral_utils.hpp"
#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/IButtonMap.h"
#include "input/joysticks/JoystickTypes.h"
#include "peripherals/PeripheralTypes.h"
#include "threads/CriticalSection.h"

namespace PERIPHERALS
{
  class CPeripheral;

  class CAddonButtonMap : public JOYSTICK::IButtonMap
  {
  public:
    CAddonButtonMap(CPeripheral* device, const std::weak_ptr<CPeripheralAddon>& addon, const std::string& strControllerId);

    virtual ~CAddonButtonMap(void);

    // Implementation of IButtonMap
    virtual std::string ControllerID(void) const override { return m_strControllerId; }

    virtual std::string DeviceName(void) const override;

    virtual bool Load(void) override;

    virtual void Reset(void) override;

    virtual bool IsEmpty(void) const override;

    virtual bool GetFeature(
      const JOYSTICK::CDriverPrimitive& primitive,
      JOYSTICK::FeatureName& feature
    ) override;

    virtual JOYSTICK::FEATURE_TYPE GetFeatureType(
      const JOYSTICK::FeatureName& feature
    ) override;

    virtual bool GetScalar(
      const JOYSTICK::FeatureName& feature,
      JOYSTICK::CDriverPrimitive& primitive
    ) override;

    virtual void AddScalar(
      const JOYSTICK::FeatureName& feature,
      const JOYSTICK::CDriverPrimitive& primitive
    ) override;

    virtual bool GetAnalogStick(
      const JOYSTICK::FeatureName& feature,
      JOYSTICK::ANALOG_STICK_DIRECTION direction,
      JOYSTICK::CDriverPrimitive& primitive
    ) override;

    virtual void AddAnalogStick(
        const JOYSTICK::FeatureName& feature,
        JOYSTICK::ANALOG_STICK_DIRECTION direction,
        const JOYSTICK::CDriverPrimitive& primitive
    ) override;

    virtual bool GetAccelerometer(
      const JOYSTICK::FeatureName& feature,
      JOYSTICK::CDriverPrimitive& positiveX,
      JOYSTICK::CDriverPrimitive& positiveY,
      JOYSTICK::CDriverPrimitive& positiveZ
    ) override;

    virtual void AddAccelerometer(
      const JOYSTICK::FeatureName& feature,
      const JOYSTICK::CDriverPrimitive& positiveX,
      const JOYSTICK::CDriverPrimitive& positiveY,
      const JOYSTICK::CDriverPrimitive& positiveZ
    ) override;

    virtual void SetIgnoredPrimitives(const std::vector<JOYSTICK::CDriverPrimitive>& primitives) override;

    virtual bool IsIgnored(const JOYSTICK::CDriverPrimitive& primitive) override;

    virtual bool GetAxisProperties(unsigned int axisIndex, int& center, unsigned int& range) override;

    virtual void SaveButtonMap() override;

    virtual void RevertButtonMap() override;

  private:
    typedef std::map<JOYSTICK::CDriverPrimitive, JOYSTICK::FeatureName> DriverMap;
    typedef std::vector<JOYSTICK::CDriverPrimitive> JoystickPrimitiveVector;

    // Utility functions
    static DriverMap CreateLookupTable(const FeatureMap& features);

    static JOYSTICK_FEATURE_PRIMITIVE GetPrimitiveIndex(JOYSTICK::ANALOG_STICK_DIRECTION dir);

    CPeripheral* const  m_device;
    std::weak_ptr<CPeripheralAddon>  m_addon;
    const std::string   m_strControllerId;
    FeatureMap          m_features;
    DriverMap           m_driverMap;
    JoystickPrimitiveVector m_ignoredPrimitives;
    CCriticalSection    m_mutex;
  };
}
