/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PeripheralAddon.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "PeripheralAddonTranslator.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerManager.h"
#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/interfaces/IButtonMap.h"
#include "peripherals/Peripherals.h"
#include "peripherals/bus/virtual/PeripheralBusAddon.h"
#include "peripherals/devices/PeripheralJoystick.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <mutex>
#include <shared_mutex>
#include <string.h>
#include <utility>

using namespace KODI;
using namespace JOYSTICK;
using namespace PERIPHERALS;
using namespace XFILE;

#define KEYBOARD_BUTTON_MAP_NAME "Keyboard"
#define KEYBOARD_PROVIDER "application"

#define MOUSE_BUTTON_MAP_NAME "Mouse"
#define MOUSE_PROVIDER "application"

#ifndef SAFE_DELETE
#define SAFE_DELETE(p) \
  do \
  { \
    delete (p); \
    (p) = NULL; \
  } while (0)
#endif

CPeripheralAddon::CPeripheralAddon(const ADDON::AddonInfoPtr& addonInfo, CPeripherals& manager)
  : IAddonInstanceHandler(ADDON_INSTANCE_PERIPHERAL, addonInfo), m_manager(manager)
{
  m_bProvidesJoysticks =
      addonInfo->Type(ADDON::AddonType::PERIPHERALDLL)->GetValue("@provides_joysticks").asBoolean();
  m_bProvidesButtonMaps = addonInfo->Type(ADDON::AddonType::PERIPHERALDLL)
                              ->GetValue("@provides_buttonmaps")
                              .asBoolean();

  // Create "C" interface structures, used as own parts to prevent API problems on update
  m_ifc.peripheral = new AddonInstance_Peripheral;
  m_ifc.peripheral->props = new AddonProps_Peripheral();
  m_ifc.peripheral->toAddon = new KodiToAddonFuncTable_Peripheral();
  m_ifc.peripheral->toKodi = new AddonToKodiFuncTable_Peripheral();

  ResetProperties();
}

CPeripheralAddon::~CPeripheralAddon(void)
{
  DestroyAddon();

  if (m_ifc.peripheral)
  {
    delete m_ifc.peripheral->toAddon;
    delete m_ifc.peripheral->toKodi;
    delete m_ifc.peripheral->props;
  }
  delete m_ifc.peripheral;
}

void CPeripheralAddon::ResetProperties(void)
{
  // Initialise members
  m_strUserPath = CSpecialProtocol::TranslatePath(Profile());
  m_strClientPath = CSpecialProtocol::TranslatePath(Path());

  m_ifc.peripheral->props->user_path = m_strUserPath.c_str();
  m_ifc.peripheral->props->addon_path = m_strClientPath.c_str();

  m_ifc.peripheral->toKodi->kodiInstance = this;
  m_ifc.peripheral->toKodi->feature_count = cb_feature_count;
  m_ifc.peripheral->toKodi->feature_type = cb_feature_type;
  m_ifc.peripheral->toKodi->refresh_button_maps = cb_refresh_button_maps;
  m_ifc.peripheral->toKodi->trigger_scan = cb_trigger_scan;

  memset(m_ifc.peripheral->toAddon, 0, sizeof(KodiToAddonFuncTable_Peripheral));
}

bool CPeripheralAddon::CreateAddon(void)
{
  std::unique_lock<CSharedSection> lock(m_dllSection);

  // Reset all properties to defaults
  ResetProperties();

  // Create directory for user data
  if (!CDirectory::Exists(m_strUserPath))
    CDirectory::Create(m_strUserPath);

  // Initialise the add-on
  CLog::Log(LOGDEBUG, "PERIPHERAL - {} - creating peripheral add-on instance '{}'", __FUNCTION__,
            Name());

  if (CreateInstance() != ADDON_STATUS_OK)
    return false;

  if (!GetAddonProperties())
  {
    DestroyInstance();
    return false;
  }

  return true;
}

void CPeripheralAddon::DestroyAddon()
{
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    m_peripherals.clear();
  }

  {
    std::unique_lock<CCriticalSection> lock(m_buttonMapMutex);
    // only clear buttonMaps but don't delete them as they are owned by a
    // CAddonJoystickInputHandling instance
    m_buttonMaps.clear();
  }

  {
    std::unique_lock<CSharedSection> lock(m_dllSection);
    DestroyInstance();
  }
}

bool CPeripheralAddon::GetAddonProperties(void)
{
  PERIPHERAL_CAPABILITIES addonCapabilities = {};

  // Get the capabilities
  m_ifc.peripheral->toAddon->get_capabilities(m_ifc.peripheral, &addonCapabilities);

  // Verify capabilities against addon.xml
  if (m_bProvidesJoysticks != addonCapabilities.provides_joysticks)
  {
    CLog::Log(
        LOGERROR,
        "PERIPHERAL - Add-on '{}': provides_joysticks'({}) in add-on DLL  doesn't match "
        "'provides_joysticks'({}) in addon.xml. Please contact the developer of this add-on: {}",
        Name(), addonCapabilities.provides_joysticks ? "true" : "false",
        m_bProvidesJoysticks ? "true" : "false", Author());
    return false;
  }
  if (m_bProvidesButtonMaps != addonCapabilities.provides_buttonmaps)
  {
    CLog::Log(
        LOGERROR,
        "PERIPHERAL - Add-on '{}': provides_buttonmaps' ({}) in add-on DLL  doesn't match "
        "'provides_buttonmaps' ({}) in addon.xml. Please contact the developer of this add-on: {}",
        Name(), addonCapabilities.provides_buttonmaps ? "true" : "false",
        m_bProvidesButtonMaps ? "true" : "false", Author());
    return false;
  }

  // Read properties that depend on underlying driver
  m_bSupportsJoystickRumble = addonCapabilities.provides_joystick_rumble;
  m_bSupportsJoystickPowerOff = addonCapabilities.provides_joystick_power_off;

  return true;
}

bool CPeripheralAddon::Register(unsigned int peripheralIndex, const PeripheralPtr& peripheral)
{
  if (!peripheral)
    return false;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_peripherals.find(peripheralIndex) == m_peripherals.end())
  {
    if (peripheral->Type() == PERIPHERAL_JOYSTICK)
    {
      m_peripherals[peripheralIndex] = std::static_pointer_cast<CPeripheralJoystick>(peripheral);

      CLog::Log(LOGINFO, "{} - new {} device registered on {}->{}: {}", __FUNCTION__,
                PeripheralTypeTranslator::TypeToString(peripheral->Type()),
                PeripheralTypeTranslator::BusTypeToString(PERIPHERAL_BUS_ADDON),
                peripheral->Location(), peripheral->DeviceName());

      return true;
    }
  }
  return false;
}

void CPeripheralAddon::UnregisterRemovedDevices(const PeripheralScanResults& results,
                                                PeripheralVector& removedPeripherals)
{
  std::vector<unsigned int> removedIndexes;

  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    for (auto& it : m_peripherals)
    {
      const PeripheralPtr& peripheral = it.second;
      PeripheralScanResult updatedDevice(PERIPHERAL_BUS_ADDON);
      if (!results.GetDeviceOnLocation(peripheral->Location(), &updatedDevice) ||
          *peripheral != updatedDevice)
      {
        // Device removed
        removedIndexes.push_back(it.first);
      }
    }
  }

  for (auto index : removedIndexes)
  {
    auto it = m_peripherals.find(index);
    const PeripheralPtr& peripheral = it->second;
    CLog::Log(LOGINFO, "{} - device removed from {}/{}: {} ({}:{})", __FUNCTION__,
              PeripheralTypeTranslator::TypeToString(peripheral->Type()), peripheral->Location(),
              peripheral->DeviceName(), peripheral->VendorIdAsString(),
              peripheral->ProductIdAsString());
    UnregisterButtonMap(peripheral.get());
    peripheral->OnDeviceRemoved();
    removedPeripherals.push_back(peripheral);
    m_peripherals.erase(it);
  }
}

bool CPeripheralAddon::HasFeature(const PeripheralFeature feature) const
{
  if (feature == FEATURE_JOYSTICK)
    return m_bProvidesJoysticks;

  return false;
}

void CPeripheralAddon::GetFeatures(std::vector<PeripheralFeature>& features) const
{
  if (m_bProvidesJoysticks &&
      std::find(features.begin(), features.end(), FEATURE_JOYSTICK) == features.end())
    features.push_back(FEATURE_JOYSTICK);
}

PeripheralPtr CPeripheralAddon::GetPeripheral(unsigned int index) const
{
  PeripheralPtr peripheral;
  std::unique_lock<CCriticalSection> lock(m_critSection);
  auto it = m_peripherals.find(index);
  if (it != m_peripherals.end())
    peripheral = it->second;
  return peripheral;
}

PeripheralPtr CPeripheralAddon::GetByPath(const std::string& strPath) const
{
  PeripheralPtr result;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& it : m_peripherals)
  {
    if (StringUtils::EqualsNoCase(strPath, it.second->FileLocation()))
    {
      result = it.second;
      break;
    }
  }

  return result;
}

bool CPeripheralAddon::SupportsFeature(PeripheralFeature feature) const
{
  switch (feature)
  {
    case FEATURE_RUMBLE:
      return m_bSupportsJoystickRumble;
    case FEATURE_POWER_OFF:
      return m_bSupportsJoystickPowerOff;
    default:
      break;
  }

  return false;
}

unsigned int CPeripheralAddon::GetPeripheralsWithFeature(PeripheralVector& results,
                                                         const PeripheralFeature feature) const
{
  unsigned int iReturn = 0;
  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& it : m_peripherals)
  {
    if (it.second->HasFeature(feature))
    {
      results.push_back(it.second);
      ++iReturn;
    }
  }
  return iReturn;
}

unsigned int CPeripheralAddon::GetNumberOfPeripherals(void) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return static_cast<unsigned int>(m_peripherals.size());
}

unsigned int CPeripheralAddon::GetNumberOfPeripheralsWithId(const int iVendorId,
                                                            const int iProductId) const
{
  unsigned int iReturn = 0;
  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& it : m_peripherals)
  {
    if (it.second->VendorId() == iVendorId && it.second->ProductId() == iProductId)
      iReturn++;
  }

  return iReturn;
}

void CPeripheralAddon::GetDirectory(const std::string& strPath, CFileItemList& items) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& it : m_peripherals)
  {
    const PeripheralPtr& peripheral = it.second;
    if (peripheral->IsHidden())
      continue;

    CFileItemPtr peripheralFile(new CFileItem(peripheral->DeviceName()));
    peripheralFile->SetPath(peripheral->FileLocation());
    peripheralFile->SetProperty("vendor", peripheral->VendorIdAsString());
    peripheralFile->SetProperty("product", peripheral->ProductIdAsString());
    peripheralFile->SetProperty(
        "bus", PeripheralTypeTranslator::BusTypeToString(peripheral->GetBusType()));
    peripheralFile->SetProperty("location", peripheral->Location());
    peripheralFile->SetProperty("class",
                                PeripheralTypeTranslator::TypeToString(peripheral->Type()));
    peripheralFile->SetProperty("version", peripheral->GetVersionInfo());
    peripheralFile->SetArt("icon", peripheral->GetIcon());
    items.Add(peripheralFile);
  }
}

bool CPeripheralAddon::PerformDeviceScan(PeripheralScanResults& results)
{
  unsigned int peripheralCount;
  PERIPHERAL_INFO* pScanResults;
  PERIPHERAL_ERROR retVal;

  std::shared_lock<CSharedSection> lock(m_dllSection);

  if (!m_ifc.peripheral->toAddon->perform_device_scan)
    return false;

  LogError(retVal = m_ifc.peripheral->toAddon->perform_device_scan(m_ifc.peripheral,
                                                                   &peripheralCount, &pScanResults),
           "PerformDeviceScan()");

  if (retVal == PERIPHERAL_NO_ERROR)
  {
    for (unsigned int i = 0; i < peripheralCount; i++)
    {
      kodi::addon::Peripheral peripheral(pScanResults[i]);
      PeripheralScanResult result(PERIPHERAL_BUS_ADDON);
      switch (peripheral.Type())
      {
        case PERIPHERAL_TYPE_JOYSTICK:
          result.m_type = PERIPHERAL_JOYSTICK;
          break;
        default:
          continue;
      }

      result.m_strDeviceName = peripheral.Name();
      result.m_strLocation = StringUtils::Format("{}/{}", ID(), peripheral.Index());
      result.m_iVendorId = peripheral.VendorID();
      result.m_iProductId = peripheral.ProductID();
      result.m_mappedType = PERIPHERAL_JOYSTICK;
      result.m_mappedBusType = PERIPHERAL_BUS_ADDON;
      result.m_iSequence = 0;

      if (!results.ContainsResult(result))
        results.m_results.push_back(result);
    }

    m_ifc.peripheral->toAddon->free_scan_results(m_ifc.peripheral, peripheralCount, pScanResults);

    return true;
  }

  return false;
}

bool CPeripheralAddon::ProcessEvents(void)
{
  if (!m_bProvidesJoysticks)
    return false;

  std::shared_lock<CSharedSection> lock(m_dllSection);

  if (!m_ifc.peripheral->toAddon->get_events)
    return false;

  PERIPHERAL_ERROR retVal;

  unsigned int eventCount = 0;
  PERIPHERAL_EVENT* pEvents = nullptr;

  LogError(retVal = m_ifc.peripheral->toAddon->get_events(m_ifc.peripheral, &eventCount, &pEvents),
           "GetEvents()");
  if (retVal == PERIPHERAL_NO_ERROR)
  {
    for (unsigned int i = 0; i < eventCount; i++)
    {
      kodi::addon::PeripheralEvent event(pEvents[i]);
      PeripheralPtr device = GetPeripheral(event.PeripheralIndex());
      if (!device)
        continue;

      switch (device->Type())
      {
        case PERIPHERAL_JOYSTICK:
        {
          std::shared_ptr<CPeripheralJoystick> joystickDevice =
              std::static_pointer_cast<CPeripheralJoystick>(device);

          switch (event.Type())
          {
            case PERIPHERAL_EVENT_TYPE_DRIVER_BUTTON:
            {
              const bool bPressed = (event.ButtonState() == JOYSTICK_STATE_BUTTON_PRESSED);
              joystickDevice->OnButtonMotion(event.DriverIndex(), bPressed);
              break;
            }
            case PERIPHERAL_EVENT_TYPE_DRIVER_HAT:
            {
              const HAT_STATE state =
                  CPeripheralAddonTranslator::TranslateHatState(event.HatState());
              joystickDevice->OnHatMotion(event.DriverIndex(), state);
              break;
            }
            case PERIPHERAL_EVENT_TYPE_DRIVER_AXIS:
            {
              joystickDevice->OnAxisMotion(event.DriverIndex(), event.AxisState());
              break;
            }
            default:
              break;
          }
          break;
        }
        default:
          break;
      }
    }

    for (const auto& it : m_peripherals)
    {
      if (it.second->Type() == PERIPHERAL_JOYSTICK)
        std::static_pointer_cast<CPeripheralJoystick>(it.second)->OnInputFrame();
    }

    m_ifc.peripheral->toAddon->free_events(m_ifc.peripheral, eventCount, pEvents);

    return true;
  }

  return false;
}

bool CPeripheralAddon::SendRumbleEvent(unsigned int peripheralIndex,
                                       unsigned int driverIndex,
                                       float magnitude)
{
  if (!m_bProvidesJoysticks)
    return false;

  std::shared_lock<CSharedSection> lock(m_dllSection);

  if (!m_ifc.peripheral->toAddon->send_event)
    return false;

  PERIPHERAL_EVENT eventStruct = {};

  eventStruct.peripheral_index = peripheralIndex;
  eventStruct.type = PERIPHERAL_EVENT_TYPE_SET_MOTOR;
  eventStruct.driver_index = driverIndex;
  eventStruct.motor_state = magnitude;

  return m_ifc.peripheral->toAddon->send_event(m_ifc.peripheral, &eventStruct);
}

bool CPeripheralAddon::GetJoystickProperties(unsigned int index, CPeripheralJoystick& joystick)
{
  if (!m_bProvidesJoysticks)
    return false;

  std::shared_lock<CSharedSection> lock(m_dllSection);

  if (!m_ifc.peripheral->toAddon->get_joystick_info)
    return false;

  PERIPHERAL_ERROR retVal;

  JOYSTICK_INFO joystickStruct;

  LogError(retVal = m_ifc.peripheral->toAddon->get_joystick_info(m_ifc.peripheral, index,
                                                                 &joystickStruct),
           "GetJoystickInfo()");
  if (retVal == PERIPHERAL_NO_ERROR)
  {
    kodi::addon::Joystick addonJoystick(joystickStruct);
    SetJoystickInfo(joystick, addonJoystick);

    m_ifc.peripheral->toAddon->free_joystick_info(m_ifc.peripheral, &joystickStruct);

    return true;
  }

  return false;
}

bool CPeripheralAddon::GetAppearance(const CPeripheral* device, std::string& controllerId)
{
  if (!m_bProvidesButtonMaps)
    return false;

  std::shared_lock<CSharedSection> lock(m_dllSection);

  if (!m_ifc.peripheral->toAddon->get_appearance)
    return false;

  PERIPHERAL_ERROR retVal;

  kodi::addon::Joystick joystickInfo;
  GetJoystickInfo(device, joystickInfo);

  JOYSTICK_INFO joystickStruct;
  joystickInfo.ToStruct(joystickStruct);

  char strControllerId[1024]{};

  LogError(retVal = m_ifc.peripheral->toAddon->get_appearance(
               m_ifc.peripheral, &joystickStruct, strControllerId, sizeof(strControllerId)),
           "GetAppearance()");

  kodi::addon::Joystick::FreeStruct(joystickStruct);

  if (retVal == PERIPHERAL_NO_ERROR)
  {
    controllerId = strControllerId;
    return true;
  }

  return false;
}

bool CPeripheralAddon::SetAppearance(const CPeripheral* device, const std::string& controllerId)
{
  if (!m_bProvidesButtonMaps)
    return false;

  std::shared_lock<CSharedSection> lock(m_dllSection);

  if (!m_ifc.peripheral->toAddon->set_appearance)
    return false;

  PERIPHERAL_ERROR retVal;

  kodi::addon::Joystick joystickInfo;
  GetJoystickInfo(device, joystickInfo);

  JOYSTICK_INFO joystickStruct;
  joystickInfo.ToStruct(joystickStruct);

  LogError(retVal = m_ifc.peripheral->toAddon->set_appearance(m_ifc.peripheral, &joystickStruct,
                                                              controllerId.c_str()),
           "SetAppearance()");

  kodi::addon::Joystick::FreeStruct(joystickStruct);

  return retVal == PERIPHERAL_NO_ERROR;
}

bool CPeripheralAddon::GetFeatures(const CPeripheral* device,
                                   const std::string& strControllerId,
                                   FeatureMap& features)
{
  if (!m_bProvidesButtonMaps)
    return false;

  std::shared_lock<CSharedSection> lock(m_dllSection);

  if (!m_ifc.peripheral->toAddon->get_features)
    return false;

  PERIPHERAL_ERROR retVal;

  kodi::addon::Joystick joystickInfo;
  GetJoystickInfo(device, joystickInfo);

  JOYSTICK_INFO joystickStruct;
  joystickInfo.ToStruct(joystickStruct);

  unsigned int featureCount = 0;
  JOYSTICK_FEATURE* pFeatures = nullptr;

  LogError(retVal = m_ifc.peripheral->toAddon->get_features(m_ifc.peripheral, &joystickStruct,
                                                            strControllerId.c_str(), &featureCount,
                                                            &pFeatures),
           "GetFeatures()");

  kodi::addon::Joystick::FreeStruct(joystickStruct);

  if (retVal == PERIPHERAL_NO_ERROR)
  {
    for (unsigned int i = 0; i < featureCount; i++)
    {
      kodi::addon::JoystickFeature feature(pFeatures[i]);
      if (feature.Type() != JOYSTICK_FEATURE_TYPE_UNKNOWN)
        features[feature.Name()] = feature;
    }

    m_ifc.peripheral->toAddon->free_features(m_ifc.peripheral, featureCount, pFeatures);

    return true;
  }

  return false;
}

bool CPeripheralAddon::MapFeature(const CPeripheral* device,
                                  const std::string& strControllerId,
                                  const kodi::addon::JoystickFeature& feature)
{
  if (!m_bProvidesButtonMaps)
    return false;

  std::shared_lock<CSharedSection> lock(m_dllSection);

  if (!m_ifc.peripheral->toAddon->map_features)
    return false;

  PERIPHERAL_ERROR retVal;

  kodi::addon::Joystick joystickInfo;
  GetJoystickInfo(device, joystickInfo);

  JOYSTICK_INFO joystickStruct;
  joystickInfo.ToStruct(joystickStruct);

  JOYSTICK_FEATURE addonFeature;
  feature.ToStruct(addonFeature);

  LogError(retVal = m_ifc.peripheral->toAddon->map_features(
               m_ifc.peripheral, &joystickStruct, strControllerId.c_str(), 1, &addonFeature),
           "MapFeatures()");

  kodi::addon::Joystick::FreeStruct(joystickStruct);
  kodi::addon::JoystickFeature::FreeStruct(addonFeature);

  return retVal == PERIPHERAL_NO_ERROR;
}

bool CPeripheralAddon::GetIgnoredPrimitives(const CPeripheral* device, PrimitiveVector& primitives)
{
  if (!m_bProvidesButtonMaps)
    return false;

  std::shared_lock<CSharedSection> lock(m_dllSection);

  if (!m_ifc.peripheral->toAddon->get_ignored_primitives)
    return false;

  PERIPHERAL_ERROR retVal;

  kodi::addon::Joystick joystickInfo;
  GetJoystickInfo(device, joystickInfo);

  JOYSTICK_INFO joystickStruct;
  joystickInfo.ToStruct(joystickStruct);

  unsigned int primitiveCount = 0;
  JOYSTICK_DRIVER_PRIMITIVE* pPrimitives = nullptr;

  LogError(retVal = m_ifc.peripheral->toAddon->get_ignored_primitives(
               m_ifc.peripheral, &joystickStruct, &primitiveCount, &pPrimitives),
           "GetIgnoredPrimitives()");

  kodi::addon::Joystick::FreeStruct(joystickStruct);

  if (retVal == PERIPHERAL_NO_ERROR)
  {
    for (unsigned int i = 0; i < primitiveCount; i++)
      primitives.emplace_back(pPrimitives[i]);

    m_ifc.peripheral->toAddon->free_primitives(m_ifc.peripheral, primitiveCount, pPrimitives);

    return true;
  }

  return false;
}

bool CPeripheralAddon::SetIgnoredPrimitives(const CPeripheral* device,
                                            const PrimitiveVector& primitives)
{
  if (!m_bProvidesButtonMaps)
    return false;

  std::shared_lock<CSharedSection> lock(m_dllSection);

  if (!m_ifc.peripheral->toAddon->set_ignored_primitives)
    return false;

  PERIPHERAL_ERROR retVal;

  kodi::addon::Joystick joystickInfo;
  GetJoystickInfo(device, joystickInfo);

  JOYSTICK_INFO joystickStruct;
  joystickInfo.ToStruct(joystickStruct);

  JOYSTICK_DRIVER_PRIMITIVE* addonPrimitives = nullptr;
  kodi::addon::DriverPrimitives::ToStructs(primitives, &addonPrimitives);
  const unsigned int primitiveCount = static_cast<unsigned int>(primitives.size());

  LogError(retVal = m_ifc.peripheral->toAddon->set_ignored_primitives(
               m_ifc.peripheral, &joystickStruct, primitiveCount, addonPrimitives),
           "SetIgnoredPrimitives()");

  kodi::addon::Joystick::FreeStruct(joystickStruct);
  kodi::addon::DriverPrimitives::FreeStructs(primitiveCount, addonPrimitives);

  return retVal == PERIPHERAL_NO_ERROR;
}

void CPeripheralAddon::SaveButtonMap(const CPeripheral* device)
{
  if (!m_bProvidesButtonMaps)
    return;

  std::shared_lock<CSharedSection> lock(m_dllSection);

  if (!m_ifc.peripheral->toAddon->save_button_map)
    return;

  kodi::addon::Joystick joystickInfo;
  GetJoystickInfo(device, joystickInfo);

  JOYSTICK_INFO joystickStruct;
  joystickInfo.ToStruct(joystickStruct);

  m_ifc.peripheral->toAddon->save_button_map(m_ifc.peripheral, &joystickStruct);

  kodi::addon::Joystick::FreeStruct(joystickStruct);

  // Notify observing button maps
  RefreshButtonMaps(device->DeviceName());
}

void CPeripheralAddon::RevertButtonMap(const CPeripheral* device)
{
  if (!m_bProvidesButtonMaps)
    return;

  std::shared_lock<CSharedSection> lock(m_dllSection);

  if (!m_ifc.peripheral->toAddon->revert_button_map)
    return;

  kodi::addon::Joystick joystickInfo;
  GetJoystickInfo(device, joystickInfo);

  JOYSTICK_INFO joystickStruct;
  joystickInfo.ToStruct(joystickStruct);

  m_ifc.peripheral->toAddon->revert_button_map(m_ifc.peripheral, &joystickStruct);

  kodi::addon::Joystick::FreeStruct(joystickStruct);
}

void CPeripheralAddon::ResetButtonMap(const CPeripheral* device, const std::string& strControllerId)
{
  if (!m_bProvidesButtonMaps)
    return;

  kodi::addon::Joystick joystickInfo;
  GetJoystickInfo(device, joystickInfo);

  JOYSTICK_INFO joystickStruct;
  joystickInfo.ToStruct(joystickStruct);

  m_ifc.peripheral->toAddon->reset_button_map(m_ifc.peripheral, &joystickStruct,
                                              strControllerId.c_str());

  kodi::addon::Joystick::FreeStruct(joystickStruct);

  // Notify observing button maps
  RefreshButtonMaps(device->DeviceName());
}

void CPeripheralAddon::PowerOffJoystick(unsigned int index)
{
  if (!HasFeature(FEATURE_JOYSTICK))
    return;

  if (!SupportsFeature(FEATURE_POWER_OFF))
    return;

  std::shared_lock<CSharedSection> lock(m_dllSection);

  if (!m_ifc.peripheral->toAddon->power_off_joystick)
    return;

  m_ifc.peripheral->toAddon->power_off_joystick(m_ifc.peripheral, index);
}

void CPeripheralAddon::RegisterButtonMap(CPeripheral* device, IButtonMap* buttonMap)
{
  std::unique_lock<CCriticalSection> lock(m_buttonMapMutex);

  UnregisterButtonMap(buttonMap);
  m_buttonMaps.emplace_back(device, buttonMap);
}

void CPeripheralAddon::UnregisterButtonMap(IButtonMap* buttonMap)
{
  std::unique_lock<CCriticalSection> lock(m_buttonMapMutex);

  for (auto it = m_buttonMaps.begin(); it != m_buttonMaps.end(); ++it)
  {
    if (it->second == buttonMap)
    {
      m_buttonMaps.erase(it);
      break;
    }
  }
}

void CPeripheralAddon::UnregisterButtonMap(CPeripheral* device)
{
  std::unique_lock<CCriticalSection> lock(m_buttonMapMutex);

  m_buttonMaps.erase(
      std::remove_if(m_buttonMaps.begin(), m_buttonMaps.end(),
                     [device](const std::pair<CPeripheral*, JOYSTICK::IButtonMap*>& buttonMap)
                     { return buttonMap.first == device; }),
      m_buttonMaps.end());
}

void CPeripheralAddon::RefreshButtonMaps(const std::string& strDeviceName /* = "" */)
{
  std::unique_lock<CCriticalSection> lock(m_buttonMapMutex);

  for (auto it = m_buttonMaps.begin(); it != m_buttonMaps.end(); ++it)
  {
    if (strDeviceName.empty() || strDeviceName == it->first->DeviceName())
      it->second->Load();
  }
}

bool CPeripheralAddon::ProvidesJoysticks(const ADDON::AddonInfoPtr& addonInfo)
{
  return addonInfo->Type(ADDON::AddonType::PERIPHERALDLL)
      ->GetValue("@provides_joysticks")
      .asBoolean();
}

bool CPeripheralAddon::ProvidesButtonMaps(const ADDON::AddonInfoPtr& addonInfo)
{
  return addonInfo->Type(ADDON::AddonType::PERIPHERALDLL)
      ->GetValue("@provides_buttonmaps")
      .asBoolean();
}

void CPeripheralAddon::TriggerDeviceScan()
{
  m_manager.TriggerDeviceScan(PERIPHERAL_BUS_ADDON);
}

unsigned int CPeripheralAddon::FeatureCount(const std::string& controllerId,
                                            JOYSTICK_FEATURE_TYPE type) const
{
  using namespace GAME;

  unsigned int count = 0;

  CControllerManager& controllerProfiles = m_manager.GetControllerProfiles();
  ControllerPtr controller = controllerProfiles.GetController(controllerId);
  if (controller)
    count = controller->FeatureCount(CPeripheralAddonTranslator::TranslateFeatureType(type));

  return count;
}

JOYSTICK_FEATURE_TYPE CPeripheralAddon::FeatureType(const std::string& controllerId,
                                                    const std::string& featureName) const
{
  using namespace GAME;

  JOYSTICK_FEATURE_TYPE type = JOYSTICK_FEATURE_TYPE_UNKNOWN;

  CControllerManager& controllerProfiles = m_manager.GetControllerProfiles();
  ControllerPtr controller = controllerProfiles.GetController(controllerId);
  if (controller)
    type = CPeripheralAddonTranslator::TranslateFeatureType(controller->FeatureType(featureName));

  return type;
}

void CPeripheralAddon::GetPeripheralInfo(const CPeripheral* device,
                                         kodi::addon::Peripheral& peripheralInfo)
{
  peripheralInfo.SetType(CPeripheralAddonTranslator::TranslateType(device->Type()));
  peripheralInfo.SetName(device->DeviceName());
  peripheralInfo.SetVendorID(device->VendorId());
  peripheralInfo.SetProductID(device->ProductId());
}

void CPeripheralAddon::GetJoystickInfo(const CPeripheral* device,
                                       kodi::addon::Joystick& joystickInfo)
{
  GetPeripheralInfo(device, joystickInfo);

  if (device->Type() == PERIPHERAL_JOYSTICK)
  {
    const CPeripheralJoystick* joystick = static_cast<const CPeripheralJoystick*>(device);
    joystickInfo.SetProvider(joystick->Provider());
    joystickInfo.SetButtonCount(joystick->ButtonCount());
    joystickInfo.SetHatCount(joystick->HatCount());
    joystickInfo.SetAxisCount(joystick->AxisCount());
    joystickInfo.SetMotorCount(joystick->MotorCount());
    joystickInfo.SetSupportsPowerOff(joystick->SupportsPowerOff());
  }
  else if (device->Type() == PERIPHERAL_KEYBOARD || device->Type() == PERIPHERAL_MOUSE)
  {
    joystickInfo.SetName(GetDeviceName(device->Type())); // Override name with non-localized version
    joystickInfo.SetProvider(GetProvider(device->Type()));
  }
}

void CPeripheralAddon::SetJoystickInfo(CPeripheralJoystick& joystick,
                                       const kodi::addon::Joystick& joystickInfo)
{
  joystick.SetProvider(joystickInfo.Provider());
  joystick.SetRequestedPort(joystickInfo.RequestedPort());
  joystick.SetButtonCount(joystickInfo.ButtonCount());
  joystick.SetHatCount(joystickInfo.HatCount());
  joystick.SetAxisCount(joystickInfo.AxisCount());
  joystick.SetMotorCount(joystickInfo.MotorCount());
  joystick.SetSupportsPowerOff(joystickInfo.SupportsPowerOff());
}

bool CPeripheralAddon::LogError(const PERIPHERAL_ERROR error, const char* strMethod) const
{
  if (error != PERIPHERAL_NO_ERROR)
  {
    CLog::Log(LOGERROR, "PERIPHERAL - {} - addon '{}' returned an error: {}", strMethod, Name(),
              CPeripheralAddonTranslator::TranslateError(error));
    return false;
  }
  return true;
}

std::string CPeripheralAddon::GetDeviceName(PeripheralType type)
{
  switch (type)
  {
    case PERIPHERAL_KEYBOARD:
      return KEYBOARD_BUTTON_MAP_NAME;
    case PERIPHERAL_MOUSE:
      return MOUSE_BUTTON_MAP_NAME;
    default:
      break;
  }

  return "";
}

std::string CPeripheralAddon::GetProvider(PeripheralType type)
{
  switch (type)
  {
    case PERIPHERAL_KEYBOARD:
      return KEYBOARD_PROVIDER;
    case PERIPHERAL_MOUSE:
      return MOUSE_PROVIDER;
    default:
      break;
  }

  return "";
}

void CPeripheralAddon::cb_trigger_scan(void* kodiInstance)
{
  if (kodiInstance == nullptr)
    return;

  static_cast<CPeripheralAddon*>(kodiInstance)->TriggerDeviceScan();
}

void CPeripheralAddon::cb_refresh_button_maps(void* kodiInstance,
                                              const char* deviceName,
                                              const char* controllerId)
{
  if (!kodiInstance)
    return;

  static_cast<CPeripheralAddon*>(kodiInstance)->RefreshButtonMaps(deviceName ? deviceName : "");
}

unsigned int CPeripheralAddon::cb_feature_count(void* kodiInstance,
                                                const char* controllerId,
                                                JOYSTICK_FEATURE_TYPE type)
{
  if (kodiInstance == nullptr || controllerId == nullptr)
    return 0;

  return static_cast<CPeripheralAddon*>(kodiInstance)->FeatureCount(controllerId, type);
}

JOYSTICK_FEATURE_TYPE CPeripheralAddon::cb_feature_type(void* kodiInstance,
                                                        const char* controllerId,
                                                        const char* featureName)
{
  if (kodiInstance == nullptr || controllerId == nullptr || featureName == nullptr)
    return JOYSTICK_FEATURE_TYPE_UNKNOWN;

  return static_cast<CPeripheralAddon*>(kodiInstance)->FeatureType(controllerId, featureName);
}
