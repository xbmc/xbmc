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

#include "AddonButtonMap.h"
#include "PeripheralAddonTranslator.h"
#include "input/joysticks/JoystickUtils.h"
#include "peripherals/devices/Peripheral.h"
#include "utils/log.h"

#include <assert.h>
#include <vector>

using namespace JOYSTICK;
using namespace PERIPHERALS;

CAddonButtonMap::CAddonButtonMap(CPeripheral* device, const std::weak_ptr<CPeripheralAddon>& addon, const std::string& strControllerId)
  : m_device(device),
    m_addon(addon),
    m_strControllerId(strControllerId)
{
  auto peripheralAddon = m_addon.lock();
  assert(peripheralAddon != nullptr);

  peripheralAddon->RegisterButtonMap(device, this);
}

CAddonButtonMap::~CAddonButtonMap(void)
{
  if (auto addon = m_addon.lock())
    addon->UnregisterButtonMap(this);
}

bool CAddonButtonMap::Load(void)
{
  m_features.clear();
  m_driverMap.clear();

  bool bSuccess = false;
  if (auto addon = m_addon.lock())
    bSuccess = addon->GetFeatures(m_device, m_strControllerId, m_features);

  // GetFeatures() was changed to always return false if no features were
  // retrieved. Check here, just in case its contract is changed or violated in
  // the future.
  if (bSuccess && m_features.empty())
    bSuccess = false;

  if (bSuccess)
    m_driverMap = CreateLookupTable(m_features);
  else
    CLog::Log(LOGDEBUG, "Failed to load button map for \"%s\"", m_device->DeviceName().c_str());

  return true;
}

void CAddonButtonMap::Reset(void)
{
  if (auto addon = m_addon.lock())
    addon->ResetButtonMap(m_device, m_strControllerId);
}

bool CAddonButtonMap::GetFeature(const CDriverPrimitive& primitive, FeatureName& feature)
{
  DriverMap::const_iterator it = m_driverMap.find(primitive);
  if (it != m_driverMap.end())
  {
    feature = it->second;
    return true;
  }

  return false;
}

FEATURE_TYPE CAddonButtonMap::GetFeatureType(const FeatureName& feature)
{
  FEATURE_TYPE type = FEATURE_TYPE::UNKNOWN;

  FeatureMap::const_iterator it = m_features.find(feature);
  if (it != m_features.end())
    type = CPeripheralAddonTranslator::TranslateFeatureType(it->second.Type());

  return type;
}

bool CAddonButtonMap::GetScalar(const FeatureName& feature, CDriverPrimitive& primitive)
{
  bool retVal(false);

  FeatureMap::const_iterator it = m_features.find(feature);
  if (it != m_features.end())
  {
    const ADDON::JoystickFeature& addonFeature = it->second;

    if (addonFeature.Type() == JOYSTICK_FEATURE_TYPE_SCALAR ||
        addonFeature.Type() == JOYSTICK_FEATURE_TYPE_MOTOR)
    {
      primitive = CPeripheralAddonTranslator::TranslatePrimitive(addonFeature.Primitive(JOYSTICK_SCALAR_PRIMITIVE));
      retVal = true;
    }
  }

  return retVal;
}

void CAddonButtonMap::AddScalar(const FeatureName& feature, const CDriverPrimitive& primitive)
{
  const bool bMotor = (primitive.Type() == PRIMITIVE_TYPE::MOTOR);

  ADDON::JoystickFeature scalar(feature, bMotor ? JOYSTICK_FEATURE_TYPE_MOTOR : JOYSTICK_FEATURE_TYPE_SCALAR);
  scalar.SetPrimitive(JOYSTICK_SCALAR_PRIMITIVE, CPeripheralAddonTranslator::TranslatePrimitive(primitive));

  if (auto addon = m_addon.lock())
    addon->MapFeature(m_device, m_strControllerId, scalar);
}

bool CAddonButtonMap::GetAnalogStick(const FeatureName& feature,
                                     JOYSTICK::ANALOG_STICK_DIRECTION direction,
                                     JOYSTICK::CDriverPrimitive& primitive)
{
  bool retVal(false);

  FeatureMap::const_iterator it = m_features.find(feature);
  if (it != m_features.end())
  {
    const ADDON::JoystickFeature& addonFeature = it->second;

    if (addonFeature.Type() == JOYSTICK_FEATURE_TYPE_ANALOG_STICK)
    {
      primitive = CPeripheralAddonTranslator::TranslatePrimitive(addonFeature.Primitive(GetPrimitiveIndex(direction)));
      retVal = primitive.IsValid();
    }
  }

  return retVal;
}

void CAddonButtonMap::AddAnalogStick(const FeatureName& feature,
                                     JOYSTICK::ANALOG_STICK_DIRECTION direction,
                                     const JOYSTICK::CDriverPrimitive& primitive)
{
  using namespace JOYSTICK;

  JOYSTICK_FEATURE_PRIMITIVE primitiveIndex = GetPrimitiveIndex(direction);
  ADDON::DriverPrimitive addonPrimitive = CPeripheralAddonTranslator::TranslatePrimitive(primitive);

  ADDON::JoystickFeature analogStick(feature, JOYSTICK_FEATURE_TYPE_ANALOG_STICK);

  auto it = m_features.find(feature);
  if (it != m_features.end())
    analogStick = it->second;

  const bool bModified = (primitive != CPeripheralAddonTranslator::TranslatePrimitive(analogStick.Primitive(primitiveIndex)));
  if (bModified)
    analogStick.SetPrimitive(primitiveIndex, addonPrimitive);

  if (auto addon = m_addon.lock())
    addon->MapFeature(m_device, m_strControllerId, analogStick);

  if (bModified)
    Load();
}

bool CAddonButtonMap::GetAccelerometer(const FeatureName& feature,
                                               CDriverPrimitive& positiveX,
                                               CDriverPrimitive& positiveY,
                                               CDriverPrimitive& positiveZ)
{
  bool retVal(false);

  FeatureMap::const_iterator it = m_features.find(feature);
  if (it != m_features.end())
  {
    const ADDON::JoystickFeature& addonFeature = it->second;

    if (addonFeature.Type() == JOYSTICK_FEATURE_TYPE_ACCELEROMETER)
    {
      positiveX = CPeripheralAddonTranslator::TranslatePrimitive(addonFeature.Primitive(JOYSTICK_ACCELEROMETER_POSITIVE_X));
      positiveY = CPeripheralAddonTranslator::TranslatePrimitive(addonFeature.Primitive(JOYSTICK_ACCELEROMETER_POSITIVE_Y));
      positiveZ = CPeripheralAddonTranslator::TranslatePrimitive(addonFeature.Primitive(JOYSTICK_ACCELEROMETER_POSITIVE_Z));
      retVal    = true;
    }
  }

  return retVal;
}

void CAddonButtonMap::AddAccelerometer(const FeatureName& feature,
                                               const CDriverPrimitive& positiveX,
                                               const CDriverPrimitive& positiveY,
                                               const CDriverPrimitive& positiveZ)
{
  using namespace JOYSTICK;

  ADDON::JoystickFeature accelerometer(feature, JOYSTICK_FEATURE_TYPE_ACCELEROMETER);

  accelerometer.SetPrimitive(JOYSTICK_ACCELEROMETER_POSITIVE_X, CPeripheralAddonTranslator::TranslatePrimitive(positiveX));
  accelerometer.SetPrimitive(JOYSTICK_ACCELEROMETER_POSITIVE_Y, CPeripheralAddonTranslator::TranslatePrimitive(positiveY));
  accelerometer.SetPrimitive(JOYSTICK_ACCELEROMETER_POSITIVE_Z, CPeripheralAddonTranslator::TranslatePrimitive(positiveZ));

  m_features[feature] = accelerometer;

  if (auto addon = m_addon.lock())
    addon->MapFeature(m_device, m_strControllerId, accelerometer);

  Load();
}

void CAddonButtonMap::SaveButtonMap()
{
  if (auto addon = m_addon.lock())
    addon->SaveButtonMap(m_device);
}

CAddonButtonMap::DriverMap CAddonButtonMap::CreateLookupTable(const FeatureMap& features)
{
  using namespace JOYSTICK;

  DriverMap driverMap;

  for (FeatureMap::const_iterator it = features.begin(); it != features.end(); ++it)
  {
    const ADDON::JoystickFeature& feature = it->second;

    switch (feature.Type())
    {
      case JOYSTICK_FEATURE_TYPE_SCALAR:
      {
        driverMap[CPeripheralAddonTranslator::TranslatePrimitive(feature.Primitive(JOYSTICK_SCALAR_PRIMITIVE))] = it->first;
        break;
      }

      case JOYSTICK_FEATURE_TYPE_ANALOG_STICK:
      {
        std::vector<JOYSTICK_FEATURE_PRIMITIVE> primitives = {
          JOYSTICK_ANALOG_STICK_UP,
          JOYSTICK_ANALOG_STICK_DOWN,
          JOYSTICK_ANALOG_STICK_RIGHT,
          JOYSTICK_ANALOG_STICK_LEFT,
        };

        for (auto primitive : primitives)
          driverMap[CPeripheralAddonTranslator::TranslatePrimitive(feature.Primitive(primitive))] = it->first;
        break;
      }

      case JOYSTICK_FEATURE_TYPE_ACCELEROMETER:
      {
        std::vector<JOYSTICK_FEATURE_PRIMITIVE> primitives = {
          JOYSTICK_ACCELEROMETER_POSITIVE_X,
          JOYSTICK_ACCELEROMETER_POSITIVE_Y,
          JOYSTICK_ACCELEROMETER_POSITIVE_Z,
        };

        for (auto primitive : primitives)
        {
          CDriverPrimitive translatedPrimitive = CPeripheralAddonTranslator::TranslatePrimitive(feature.Primitive(primitive));
          driverMap[translatedPrimitive] = it->first;

          // Map opposite semiaxis
          CDriverPrimitive oppositePrimitive = CDriverPrimitive(translatedPrimitive.Index(), translatedPrimitive.SemiAxisDirection() * -1);
          driverMap[oppositePrimitive] = it->first;
        }
        break;
      }
        
      default:
        break;
    }
  }
  
  return driverMap;
}

JOYSTICK_FEATURE_PRIMITIVE CAddonButtonMap::GetPrimitiveIndex(JOYSTICK::ANALOG_STICK_DIRECTION dir)
{
  using namespace JOYSTICK;

  switch (dir)
  {
  case ANALOG_STICK_DIRECTION::UP:    return JOYSTICK_ANALOG_STICK_UP;
  case ANALOG_STICK_DIRECTION::DOWN:  return JOYSTICK_ANALOG_STICK_DOWN;
  case ANALOG_STICK_DIRECTION::RIGHT: return JOYSTICK_ANALOG_STICK_RIGHT;
  case ANALOG_STICK_DIRECTION::LEFT:  return JOYSTICK_ANALOG_STICK_LEFT;
  default: break;
  }

  return static_cast<JOYSTICK_FEATURE_PRIMITIVE>(0);
}
