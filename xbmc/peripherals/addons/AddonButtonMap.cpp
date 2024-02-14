/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonButtonMap.h"

#include "PeripheralAddonTranslator.h"
#include "input/joysticks/JoystickUtils.h"
#include "peripherals/devices/Peripheral.h"
#include "utils/log.h"

#include <algorithm>
#include <assert.h>
#include <mutex>
#include <vector>

using namespace KODI;
using namespace JOYSTICK;
using namespace PERIPHERALS;

CAddonButtonMap::CAddonButtonMap(CPeripheral* device,
                                 const std::weak_ptr<CPeripheralAddon>& addon,
                                 const std::string& strControllerId)
  : m_device(device), m_addon(addon), m_strControllerId(strControllerId)
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

std::string CAddonButtonMap::Location(void) const
{
  return m_device->Location();
}

bool CAddonButtonMap::Load(void)
{
  std::string controllerAppearance;
  FeatureMap features;
  DriverMap driverMap;
  PrimitiveVector ignoredPrimitives;

  bool bSuccess = false;
  if (auto addon = m_addon.lock())
  {
    bSuccess |= addon->GetAppearance(m_device, controllerAppearance);
    bSuccess |= addon->GetFeatures(m_device, m_strControllerId, features);
    bSuccess |= addon->GetIgnoredPrimitives(m_device, ignoredPrimitives);
  }

  // GetFeatures() was changed to always return false if no features were
  // retrieved. Check here, just in case its contract is changed or violated in
  // the future.
  if (bSuccess && features.empty())
    bSuccess = false;

  if (bSuccess)
    driverMap = CreateLookupTable(features);
  else
    CLog::Log(LOGDEBUG, "Failed to load button map for \"{}\"", m_device->Location());

  {
    std::unique_lock<CCriticalSection> lock(m_mutex);
    m_controllerAppearance = std::move(controllerAppearance);
    m_features = std::move(features);
    m_driverMap = std::move(driverMap);
    m_ignoredPrimitives = CPeripheralAddonTranslator::TranslatePrimitives(ignoredPrimitives);
  }

  return true;
}

void CAddonButtonMap::Reset(void)
{
  if (auto addon = m_addon.lock())
    addon->ResetButtonMap(m_device, m_strControllerId);
}

bool CAddonButtonMap::IsEmpty(void) const
{
  std::unique_lock<CCriticalSection> lock(m_mutex);

  return m_driverMap.empty();
}

std::string CAddonButtonMap::GetAppearance() const
{
  std::unique_lock<CCriticalSection> lock(m_mutex);

  return m_controllerAppearance;
}

bool CAddonButtonMap::SetAppearance(const std::string& controllerId) const
{
  if (auto addon = m_addon.lock())
    return addon->SetAppearance(m_device, controllerId);

  return false;
}

bool CAddonButtonMap::GetFeature(const CDriverPrimitive& primitive, FeatureName& feature)
{
  std::unique_lock<CCriticalSection> lock(m_mutex);

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

  std::unique_lock<CCriticalSection> lock(m_mutex);

  FeatureMap::const_iterator it = m_features.find(feature);
  if (it != m_features.end())
    type = CPeripheralAddonTranslator::TranslateFeatureType(it->second.Type());

  return type;
}

bool CAddonButtonMap::GetScalar(const FeatureName& feature, CDriverPrimitive& primitive)
{
  bool retVal(false);

  std::unique_lock<CCriticalSection> lock(m_mutex);

  FeatureMap::const_iterator it = m_features.find(feature);
  if (it != m_features.end())
  {
    const kodi::addon::JoystickFeature& addonFeature = it->second;

    if (addonFeature.Type() == JOYSTICK_FEATURE_TYPE_SCALAR ||
        addonFeature.Type() == JOYSTICK_FEATURE_TYPE_MOTOR)
    {
      primitive = CPeripheralAddonTranslator::TranslatePrimitive(
          addonFeature.Primitive(JOYSTICK_SCALAR_PRIMITIVE));
      retVal = true;
    }
  }

  return retVal;
}

void CAddonButtonMap::AddScalar(const FeatureName& feature, const CDriverPrimitive& primitive)
{
  const bool bMotor = (primitive.Type() == PRIMITIVE_TYPE::MOTOR);

  kodi::addon::JoystickFeature scalar(feature, bMotor ? JOYSTICK_FEATURE_TYPE_MOTOR
                                                      : JOYSTICK_FEATURE_TYPE_SCALAR);
  scalar.SetPrimitive(JOYSTICK_SCALAR_PRIMITIVE,
                      CPeripheralAddonTranslator::TranslatePrimitive(primitive));

  if (auto addon = m_addon.lock())
    addon->MapFeature(m_device, m_strControllerId, scalar);
}

bool CAddonButtonMap::GetAnalogStick(const FeatureName& feature,
                                     JOYSTICK::ANALOG_STICK_DIRECTION direction,
                                     JOYSTICK::CDriverPrimitive& primitive)
{
  bool retVal(false);

  std::unique_lock<CCriticalSection> lock(m_mutex);

  FeatureMap::const_iterator it = m_features.find(feature);
  if (it != m_features.end())
  {
    const kodi::addon::JoystickFeature& addonFeature = it->second;

    if (addonFeature.Type() == JOYSTICK_FEATURE_TYPE_ANALOG_STICK)
    {
      primitive = CPeripheralAddonTranslator::TranslatePrimitive(
          addonFeature.Primitive(GetAnalogStickIndex(direction)));
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

  JOYSTICK_FEATURE_PRIMITIVE primitiveIndex = GetAnalogStickIndex(direction);
  kodi::addon::DriverPrimitive addonPrimitive =
      CPeripheralAddonTranslator::TranslatePrimitive(primitive);

  kodi::addon::JoystickFeature analogStick(feature, JOYSTICK_FEATURE_TYPE_ANALOG_STICK);

  {
    std::unique_lock<CCriticalSection> lock(m_mutex);
    auto it = m_features.find(feature);
    if (it != m_features.end())
      analogStick = it->second;
  }

  const bool bModified = (primitive != CPeripheralAddonTranslator::TranslatePrimitive(
                                           analogStick.Primitive(primitiveIndex)));
  if (bModified)
    analogStick.SetPrimitive(primitiveIndex, addonPrimitive);

  if (auto addon = m_addon.lock())
    addon->MapFeature(m_device, m_strControllerId, analogStick);

  // Because each direction is mapped individually, we need to refresh the
  // feature each time a new direction is mapped.
  if (bModified)
    Load();
}

bool CAddonButtonMap::GetRelativePointer(const FeatureName& feature,
                                         JOYSTICK::RELATIVE_POINTER_DIRECTION direction,
                                         JOYSTICK::CDriverPrimitive& primitive)
{
  bool retVal(false);

  std::unique_lock<CCriticalSection> lock(m_mutex);

  FeatureMap::const_iterator it = m_features.find(feature);
  if (it != m_features.end())
  {
    const kodi::addon::JoystickFeature& addonFeature = it->second;

    if (addonFeature.Type() == JOYSTICK_FEATURE_TYPE_RELPOINTER)
    {
      primitive = CPeripheralAddonTranslator::TranslatePrimitive(
          addonFeature.Primitive(GetRelativePointerIndex(direction)));
      retVal = primitive.IsValid();
    }
  }

  return retVal;
}

void CAddonButtonMap::AddRelativePointer(const FeatureName& feature,
                                         JOYSTICK::RELATIVE_POINTER_DIRECTION direction,
                                         const JOYSTICK::CDriverPrimitive& primitive)
{
  using namespace JOYSTICK;

  JOYSTICK_FEATURE_PRIMITIVE primitiveIndex = GetRelativePointerIndex(direction);
  kodi::addon::DriverPrimitive addonPrimitive =
      CPeripheralAddonTranslator::TranslatePrimitive(primitive);

  kodi::addon::JoystickFeature relPointer(feature, JOYSTICK_FEATURE_TYPE_RELPOINTER);

  {
    std::unique_lock<CCriticalSection> lock(m_mutex);
    auto it = m_features.find(feature);
    if (it != m_features.end())
      relPointer = it->second;
  }

  const bool bModified = (primitive != CPeripheralAddonTranslator::TranslatePrimitive(
                                           relPointer.Primitive(primitiveIndex)));
  if (bModified)
    relPointer.SetPrimitive(primitiveIndex, addonPrimitive);

  if (auto addon = m_addon.lock())
    addon->MapFeature(m_device, m_strControllerId, relPointer);

  // Because each direction is mapped individually, we need to refresh the
  // feature each time a new direction is mapped.
  if (bModified)
    Load();
}

bool CAddonButtonMap::GetAccelerometer(const FeatureName& feature,
                                       CDriverPrimitive& positiveX,
                                       CDriverPrimitive& positiveY,
                                       CDriverPrimitive& positiveZ)
{
  bool retVal(false);

  std::unique_lock<CCriticalSection> lock(m_mutex);

  FeatureMap::const_iterator it = m_features.find(feature);
  if (it != m_features.end())
  {
    const kodi::addon::JoystickFeature& addonFeature = it->second;

    if (addonFeature.Type() == JOYSTICK_FEATURE_TYPE_ACCELEROMETER)
    {
      positiveX = CPeripheralAddonTranslator::TranslatePrimitive(
          addonFeature.Primitive(JOYSTICK_ACCELEROMETER_POSITIVE_X));
      positiveY = CPeripheralAddonTranslator::TranslatePrimitive(
          addonFeature.Primitive(JOYSTICK_ACCELEROMETER_POSITIVE_Y));
      positiveZ = CPeripheralAddonTranslator::TranslatePrimitive(
          addonFeature.Primitive(JOYSTICK_ACCELEROMETER_POSITIVE_Z));
      retVal = true;
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

  kodi::addon::JoystickFeature accelerometer(feature, JOYSTICK_FEATURE_TYPE_ACCELEROMETER);

  accelerometer.SetPrimitive(JOYSTICK_ACCELEROMETER_POSITIVE_X,
                             CPeripheralAddonTranslator::TranslatePrimitive(positiveX));
  accelerometer.SetPrimitive(JOYSTICK_ACCELEROMETER_POSITIVE_Y,
                             CPeripheralAddonTranslator::TranslatePrimitive(positiveY));
  accelerometer.SetPrimitive(JOYSTICK_ACCELEROMETER_POSITIVE_Z,
                             CPeripheralAddonTranslator::TranslatePrimitive(positiveZ));

  if (auto addon = m_addon.lock())
    addon->MapFeature(m_device, m_strControllerId, accelerometer);
}

bool CAddonButtonMap::GetWheel(const KODI::JOYSTICK::FeatureName& feature,
                               KODI::JOYSTICK::WHEEL_DIRECTION direction,
                               KODI::JOYSTICK::CDriverPrimitive& primitive)
{
  bool retVal(false);

  std::unique_lock<CCriticalSection> lock(m_mutex);

  FeatureMap::const_iterator it = m_features.find(feature);
  if (it != m_features.end())
  {
    const kodi::addon::JoystickFeature& addonFeature = it->second;

    if (addonFeature.Type() == JOYSTICK_FEATURE_TYPE_WHEEL)
    {
      primitive = CPeripheralAddonTranslator::TranslatePrimitive(
          addonFeature.Primitive(GetPrimitiveIndex(direction)));
      retVal = primitive.IsValid();
    }
  }

  return retVal;
}

void CAddonButtonMap::AddWheel(const KODI::JOYSTICK::FeatureName& feature,
                               KODI::JOYSTICK::WHEEL_DIRECTION direction,
                               const KODI::JOYSTICK::CDriverPrimitive& primitive)
{
  using namespace JOYSTICK;

  JOYSTICK_FEATURE_PRIMITIVE primitiveIndex = GetPrimitiveIndex(direction);
  kodi::addon::DriverPrimitive addonPrimitive =
      CPeripheralAddonTranslator::TranslatePrimitive(primitive);

  kodi::addon::JoystickFeature joystickFeature(feature, JOYSTICK_FEATURE_TYPE_WHEEL);

  {
    std::unique_lock<CCriticalSection> lock(m_mutex);
    auto it = m_features.find(feature);
    if (it != m_features.end())
      joystickFeature = it->second;
  }

  const bool bModified = (primitive != CPeripheralAddonTranslator::TranslatePrimitive(
                                           joystickFeature.Primitive(primitiveIndex)));
  if (bModified)
    joystickFeature.SetPrimitive(primitiveIndex, addonPrimitive);

  if (auto addon = m_addon.lock())
    addon->MapFeature(m_device, m_strControllerId, joystickFeature);

  // Because each direction is mapped individually, we need to refresh the
  // feature each time a new direction is mapped.
  if (bModified)
    Load();
}

bool CAddonButtonMap::GetThrottle(const KODI::JOYSTICK::FeatureName& feature,
                                  KODI::JOYSTICK::THROTTLE_DIRECTION direction,
                                  KODI::JOYSTICK::CDriverPrimitive& primitive)
{
  bool retVal(false);

  std::unique_lock<CCriticalSection> lock(m_mutex);

  FeatureMap::const_iterator it = m_features.find(feature);
  if (it != m_features.end())
  {
    const kodi::addon::JoystickFeature& addonFeature = it->second;

    if (addonFeature.Type() == JOYSTICK_FEATURE_TYPE_THROTTLE)
    {
      primitive = CPeripheralAddonTranslator::TranslatePrimitive(
          addonFeature.Primitive(GetPrimitiveIndex(direction)));
      retVal = primitive.IsValid();
    }
  }

  return retVal;
}

void CAddonButtonMap::AddThrottle(const KODI::JOYSTICK::FeatureName& feature,
                                  KODI::JOYSTICK::THROTTLE_DIRECTION direction,
                                  const KODI::JOYSTICK::CDriverPrimitive& primitive)
{
  using namespace JOYSTICK;

  JOYSTICK_FEATURE_PRIMITIVE primitiveIndex = GetPrimitiveIndex(direction);
  kodi::addon::DriverPrimitive addonPrimitive =
      CPeripheralAddonTranslator::TranslatePrimitive(primitive);

  kodi::addon::JoystickFeature joystickFeature(feature, JOYSTICK_FEATURE_TYPE_THROTTLE);

  {
    std::unique_lock<CCriticalSection> lock(m_mutex);
    auto it = m_features.find(feature);
    if (it != m_features.end())
      joystickFeature = it->second;
  }

  const bool bModified = (primitive != CPeripheralAddonTranslator::TranslatePrimitive(
                                           joystickFeature.Primitive(primitiveIndex)));
  if (bModified)
    joystickFeature.SetPrimitive(primitiveIndex, addonPrimitive);

  if (auto addon = m_addon.lock())
    addon->MapFeature(m_device, m_strControllerId, joystickFeature);

  // Because each direction is mapped individually, we need to refresh the
  // feature each time a new direction is mapped.
  if (bModified)
    Load();
}

bool CAddonButtonMap::GetKey(const FeatureName& feature, CDriverPrimitive& primitive)
{
  bool retVal(false);

  std::unique_lock<CCriticalSection> lock(m_mutex);

  FeatureMap::const_iterator it = m_features.find(feature);
  if (it != m_features.end())
  {
    const kodi::addon::JoystickFeature& addonFeature = it->second;

    if (addonFeature.Type() == JOYSTICK_FEATURE_TYPE_KEY)
    {
      primitive = CPeripheralAddonTranslator::TranslatePrimitive(
          addonFeature.Primitive(JOYSTICK_SCALAR_PRIMITIVE));
      retVal = true;
    }
  }

  return retVal;
}

void CAddonButtonMap::AddKey(const FeatureName& feature, const CDriverPrimitive& primitive)
{
  kodi::addon::JoystickFeature scalar(feature, JOYSTICK_FEATURE_TYPE_KEY);
  scalar.SetPrimitive(JOYSTICK_SCALAR_PRIMITIVE,
                      CPeripheralAddonTranslator::TranslatePrimitive(primitive));

  if (auto addon = m_addon.lock())
    addon->MapFeature(m_device, m_strControllerId, scalar);
}

void CAddonButtonMap::SetIgnoredPrimitives(
    const std::vector<JOYSTICK::CDriverPrimitive>& primitives)
{
  if (auto addon = m_addon.lock())
    addon->SetIgnoredPrimitives(m_device,
                                CPeripheralAddonTranslator::TranslatePrimitives(primitives));
}

bool CAddonButtonMap::IsIgnored(const JOYSTICK::CDriverPrimitive& primitive)
{
  return std::find(m_ignoredPrimitives.begin(), m_ignoredPrimitives.end(), primitive) !=
         m_ignoredPrimitives.end();
}

bool CAddonButtonMap::GetAxisProperties(unsigned int axisIndex, int& center, unsigned int& range)
{
  std::unique_lock<CCriticalSection> lock(m_mutex);

  for (const auto& it : m_driverMap)
  {
    const CDriverPrimitive& primitive = it.first;

    if (primitive.Type() != PRIMITIVE_TYPE::SEMIAXIS)
      continue;

    if (primitive.Index() != axisIndex)
      continue;

    center = primitive.Center();
    range = primitive.Range();
    return true;
  }

  return false;
}

void CAddonButtonMap::SaveButtonMap()
{
  if (auto addon = m_addon.lock())
    addon->SaveButtonMap(m_device);
}

void CAddonButtonMap::RevertButtonMap()
{
  if (auto addon = m_addon.lock())
    addon->RevertButtonMap(m_device);
}

CAddonButtonMap::DriverMap CAddonButtonMap::CreateLookupTable(const FeatureMap& features)
{
  using namespace JOYSTICK;

  DriverMap driverMap;

  for (const auto& it : features)
  {
    const kodi::addon::JoystickFeature& feature = it.second;

    switch (feature.Type())
    {
      case JOYSTICK_FEATURE_TYPE_SCALAR:
      case JOYSTICK_FEATURE_TYPE_KEY:
      {
        driverMap[CPeripheralAddonTranslator::TranslatePrimitive(
            feature.Primitive(JOYSTICK_SCALAR_PRIMITIVE))] = it.first;
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
          driverMap[CPeripheralAddonTranslator::TranslatePrimitive(feature.Primitive(primitive))] =
              it.first;
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
          CDriverPrimitive translatedPrimitive =
              CPeripheralAddonTranslator::TranslatePrimitive(feature.Primitive(primitive));
          driverMap[translatedPrimitive] = it.first;

          // Map opposite semiaxis
          CDriverPrimitive oppositePrimitive = CDriverPrimitive(
              translatedPrimitive.Index(), 0, translatedPrimitive.SemiAxisDirection() * -1, 1);
          driverMap[oppositePrimitive] = it.first;
        }
        break;
      }

      case JOYSTICK_FEATURE_TYPE_WHEEL:
      {
        std::vector<JOYSTICK_FEATURE_PRIMITIVE> primitives = {
            JOYSTICK_WHEEL_LEFT,
            JOYSTICK_WHEEL_RIGHT,
        };

        for (auto primitive : primitives)
          driverMap[CPeripheralAddonTranslator::TranslatePrimitive(feature.Primitive(primitive))] =
              it.first;
        break;
      }

      case JOYSTICK_FEATURE_TYPE_THROTTLE:
      {
        std::vector<JOYSTICK_FEATURE_PRIMITIVE> primitives = {
            JOYSTICK_THROTTLE_UP,
            JOYSTICK_THROTTLE_DOWN,
        };

        for (auto primitive : primitives)
          driverMap[CPeripheralAddonTranslator::TranslatePrimitive(feature.Primitive(primitive))] =
              it.first;
        break;
      }

      case JOYSTICK_FEATURE_TYPE_RELPOINTER:
      {
        std::vector<JOYSTICK_FEATURE_PRIMITIVE> primitives = {
            JOYSTICK_RELPOINTER_UP,
            JOYSTICK_RELPOINTER_DOWN,
            JOYSTICK_RELPOINTER_RIGHT,
            JOYSTICK_RELPOINTER_LEFT,
        };

        for (auto primitive : primitives)
          driverMap[CPeripheralAddonTranslator::TranslatePrimitive(feature.Primitive(primitive))] =
              it.first;
        break;
      }

      default:
        break;
    }
  }

  return driverMap;
}

JOYSTICK_FEATURE_PRIMITIVE CAddonButtonMap::GetAnalogStickIndex(
    JOYSTICK::ANALOG_STICK_DIRECTION dir)
{
  using namespace JOYSTICK;

  switch (dir)
  {
    case ANALOG_STICK_DIRECTION::UP:
      return JOYSTICK_ANALOG_STICK_UP;
    case ANALOG_STICK_DIRECTION::DOWN:
      return JOYSTICK_ANALOG_STICK_DOWN;
    case ANALOG_STICK_DIRECTION::RIGHT:
      return JOYSTICK_ANALOG_STICK_RIGHT;
    case ANALOG_STICK_DIRECTION::LEFT:
      return JOYSTICK_ANALOG_STICK_LEFT;
    default:
      break;
  }

  return static_cast<JOYSTICK_FEATURE_PRIMITIVE>(0);
}

JOYSTICK_FEATURE_PRIMITIVE CAddonButtonMap::GetRelativePointerIndex(
    JOYSTICK::RELATIVE_POINTER_DIRECTION dir)
{
  using namespace JOYSTICK;

  switch (dir)
  {
    case RELATIVE_POINTER_DIRECTION::UP:
      return JOYSTICK_RELPOINTER_UP;
    case RELATIVE_POINTER_DIRECTION::DOWN:
      return JOYSTICK_RELPOINTER_DOWN;
    case RELATIVE_POINTER_DIRECTION::RIGHT:
      return JOYSTICK_RELPOINTER_RIGHT;
    case RELATIVE_POINTER_DIRECTION::LEFT:
      return JOYSTICK_RELPOINTER_LEFT;
    default:
      break;
  }

  return static_cast<JOYSTICK_FEATURE_PRIMITIVE>(0);
}

JOYSTICK_FEATURE_PRIMITIVE CAddonButtonMap::GetPrimitiveIndex(JOYSTICK::WHEEL_DIRECTION dir)
{
  using namespace JOYSTICK;

  switch (dir)
  {
    case WHEEL_DIRECTION::RIGHT:
      return JOYSTICK_WHEEL_RIGHT;
    case WHEEL_DIRECTION::LEFT:
      return JOYSTICK_WHEEL_LEFT;
    default:
      break;
  }

  return static_cast<JOYSTICK_FEATURE_PRIMITIVE>(0);
}

JOYSTICK_FEATURE_PRIMITIVE CAddonButtonMap::GetPrimitiveIndex(JOYSTICK::THROTTLE_DIRECTION dir)
{
  using namespace JOYSTICK;

  switch (dir)
  {
    case THROTTLE_DIRECTION::UP:
      return JOYSTICK_THROTTLE_UP;
    case THROTTLE_DIRECTION::DOWN:
      return JOYSTICK_THROTTLE_DOWN;
    default:
      break;
  }

  return static_cast<JOYSTICK_FEATURE_PRIMITIVE>(0);
}
