/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/joysticks/interfaces/IButtonMap.h"

#include <memory>
#include <string>

namespace PERIPHERALS
{
class CPeripheral;
}

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 *
 * \brief A fallback buttonmap to be used with the default keyboard profile
 */
class CDefaultButtonMap : public JOYSTICK::IButtonMap
{
public:
  CDefaultButtonMap(PERIPHERALS::CPeripheral* device, std::string strControllerId);

  ~CDefaultButtonMap() override;

  // Implementation of IButtonMap
  std::string ControllerID() const override { return m_strControllerId; }
  std::string Location() const override;
  bool Load() override;
  void Reset() override {}
  bool IsEmpty() const override { return false; }
  std::string GetAppearance() const override;
  bool SetAppearance(const std::string& controllerId) const override { return false; }
  bool GetFeature(const JOYSTICK::CDriverPrimitive& primitive,
                  JOYSTICK::FeatureName& feature) override;
  JOYSTICK::FEATURE_TYPE GetFeatureType(const JOYSTICK::FeatureName& feature) override
  {
    return JOYSTICK::FEATURE_TYPE::UNKNOWN;
  }
  bool GetScalar(const JOYSTICK::FeatureName& feature,
                 JOYSTICK::CDriverPrimitive& primitive) override
  {
    return false;
  }
  void AddScalar(const JOYSTICK::FeatureName& feature,
                 const JOYSTICK::CDriverPrimitive& primitive) override
  {
  }
  bool GetAnalogStick(const JOYSTICK::FeatureName& feature,
                      JOYSTICK::ANALOG_STICK_DIRECTION direction,
                      JOYSTICK::CDriverPrimitive& primitive) override
  {
    return false;
  }
  void AddAnalogStick(const JOYSTICK::FeatureName& feature,
                      JOYSTICK::ANALOG_STICK_DIRECTION direction,
                      const JOYSTICK::CDriverPrimitive& primitive) override
  {
  }
  bool GetRelativePointer(const JOYSTICK::FeatureName& feature,
                          JOYSTICK::RELATIVE_POINTER_DIRECTION direction,
                          JOYSTICK::CDriverPrimitive& primitive) override
  {
    return false;
  }
  void AddRelativePointer(const JOYSTICK::FeatureName& feature,
                          JOYSTICK::RELATIVE_POINTER_DIRECTION direction,
                          const JOYSTICK::CDriverPrimitive& primitive) override
  {
  }
  bool GetAccelerometer(const JOYSTICK::FeatureName& feature,
                        JOYSTICK::CDriverPrimitive& positiveX,
                        JOYSTICK::CDriverPrimitive& positiveY,
                        JOYSTICK::CDriverPrimitive& positiveZ) override
  {
    return false;
  }
  void AddAccelerometer(const JOYSTICK::FeatureName& feature,
                        const JOYSTICK::CDriverPrimitive& positiveX,
                        const JOYSTICK::CDriverPrimitive& positiveY,
                        const JOYSTICK::CDriverPrimitive& positiveZ) override
  {
  }
  bool GetWheel(const JOYSTICK::FeatureName& feature,
                JOYSTICK::WHEEL_DIRECTION direction,
                JOYSTICK::CDriverPrimitive& primitive) override
  {
    return false;
  }
  void AddWheel(const JOYSTICK::FeatureName& feature,
                JOYSTICK::WHEEL_DIRECTION direction,
                const JOYSTICK::CDriverPrimitive& primitive) override
  {
  }
  bool GetThrottle(const JOYSTICK::FeatureName& feature,
                   JOYSTICK::THROTTLE_DIRECTION direction,
                   JOYSTICK::CDriverPrimitive& primitive) override
  {
    return false;
  }
  void AddThrottle(const JOYSTICK::FeatureName& feature,
                   JOYSTICK::THROTTLE_DIRECTION direction,
                   const JOYSTICK::CDriverPrimitive& primitive) override
  {
  }
  bool GetKey(const JOYSTICK::FeatureName& feature, JOYSTICK::CDriverPrimitive& primitive) override
  {
    return false;
  }
  void AddKey(const JOYSTICK::FeatureName& feature,
              const JOYSTICK::CDriverPrimitive& primitive) override
  {
  }
  void SetIgnoredPrimitives(const std::vector<JOYSTICK::CDriverPrimitive>& primitives) override {}
  bool IsIgnored(const JOYSTICK::CDriverPrimitive& primitive) override { return false; }
  bool GetAxisProperties(unsigned int axisIndex, int& center, unsigned int& range) override
  {
    return false;
  }
  void SaveButtonMap() override {}
  void RevertButtonMap() override {}

private:
  // Construction parameters
  PERIPHERALS::CPeripheral* const m_device;
  const std::string m_strControllerId;
};
} // namespace GAME
} // namespace KODI
