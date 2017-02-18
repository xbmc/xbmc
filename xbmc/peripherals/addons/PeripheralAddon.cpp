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

#include "PeripheralAddon.h"
#include "ServiceBroker.h"
#include "AddonButtonMap.h"
#include "PeripheralAddonTranslator.h"
#include "addons/AddonManager.h"
#include "addons/kodi-addon-dev-kit/include/kodi/addon-instance/Peripheral.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "games/GameServices.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/IButtonMap.h"
#include "input/joysticks/IDriverHandler.h"
#include "input/joysticks/JoystickTranslator.h"
#include "input/joysticks/JoystickUtils.h"
#include "peripherals/Peripherals.h"
#include "peripherals/bus/virtual/PeripheralBusAddon.h"
#include "peripherals/devices/PeripheralJoystick.h"
#include "peripherals/devices/PeripheralJoystickEmulation.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <string.h>
#include <utility>

using namespace KODI;
using namespace JOYSTICK;
using namespace PERIPHERALS;
using namespace XFILE;

#define JOYSTICK_EMULATION_BUTTON_MAP_NAME  "Keyboard"
#define JOYSTICK_EMULATION_PROVIDER         "application"

#ifndef SAFE_DELETE
  #define SAFE_DELETE(p)  do { delete (p); (p) = NULL; } while (0)
#endif

CPeripheralAddon::CPeripheralAddon(ADDON::AddonInfoPtr addonInfo)
  : IAddonInstanceHandler(ADDON::ADDON_PERIPHERALDLL, addonInfo)
{
  m_bProvidesJoysticks = addonInfo->Type(ADDON::ADDON_PERIPHERALDLL)->GetValue("@provides_joysticks").asBoolean();
  m_bProvidesButtonMaps = addonInfo->Type(ADDON::ADDON_PERIPHERALDLL)->GetValue("@provides_buttonmaps").asBoolean();

  ResetProperties();
}

CPeripheralAddon::~CPeripheralAddon(void)
{
  // delete all peripherals provided by this addon
  for (const auto& peripheral : m_peripherals)
  {
    if (CServiceBroker::GetSettings().GetBool(CSettings::SETTING_INPUT_CONTROLLERPOWEROFF))
    {
      // shutdown the joystick if it is supported
      if (peripheral.second->Type() == PERIPHERAL_JOYSTICK)
      {
        std::shared_ptr<CPeripheralJoystick> joystick = std::static_pointer_cast<CPeripheralJoystick>(peripheral.second);
        if (joystick->SupportsPowerOff())
          PowerOffJoystick(peripheral.first);
      }
    }
  }
  m_peripherals.clear();

  // only clear buttonMaps but don't delete them as they are owned by a CAddonJoystickInputHandling instance
  m_buttonMaps.clear();

  DestroyInstance();
}

void CPeripheralAddon::ResetProperties(void)
{
  // Initialise members
  m_strUserPath        = CSpecialProtocol::TranslatePath(Profile());
  m_strClientPath      = CSpecialProtocol::TranslatePath(Path());

  memset(&m_struct, 0, sizeof(m_struct));

  m_struct.props.user_path = m_strUserPath.c_str();
  m_struct.props.addon_path = m_strClientPath.c_str();
  m_struct.toKodi.kodiInstance = this;
  m_struct.toKodi.TriggerScan = trigger_scan;
  m_struct.toKodi.RefreshButtonMaps = refresh_button_maps;
  m_struct.toKodi.FeatureCount = feature_count;
}

bool CPeripheralAddon::CreateAddon(void)
{
  // Reset all properties to defaults
  ResetProperties();

  // Create directory for user data
  if (!CDirectory::Exists(m_strUserPath))
    CDirectory::Create(m_strUserPath);

  // Initialise the add-on
  CLog::Log(LOGDEBUG, "PERIPHERAL - %s - creating peripheral add-on instance '%s'", __FUNCTION__, Name().c_str());
  if (CreateInstance(ADDON_INSTANCE_PERIPHERAL, &m_struct, reinterpret_cast<KODI_HANDLE*>(&m_addonInstance)))
  {
    if (!GetAddonProperties())
    {
      DestroyInstance();
      return false;
    }
  }

  return true;
}

bool CPeripheralAddon::GetAddonProperties(void)
{
  PERIPHERAL_CAPABILITIES addonCapabilities = { };

  // Get the capabilities
  m_struct.toAddon.GetCapabilities(m_addonInstance, &addonCapabilities);

  // Verify capabilities against addon.xml
  if (m_bProvidesJoysticks != addonCapabilities.provides_joysticks)
  {
    CLog::Log(LOGERROR, "PERIPHERAL - Add-on '%s': provides_joysticks'(%s) in add-on DLL  doesn't match 'provides_joysticks'(%s) in addon.xml. Please contact the developer of this add-on: %s",
        Name().c_str(), addonCapabilities.provides_joysticks ? "true" : "false",
        m_bProvidesJoysticks ? "true" : "false", Author().c_str());
    return false;
  }
  if (m_bProvidesButtonMaps != addonCapabilities.provides_buttonmaps)
  {
    CLog::Log(LOGERROR, "PERIPHERAL - Add-on '%s': provides_buttonmaps' (%s) in add-on DLL  doesn't match 'provides_buttonmaps' (%s) in addon.xml. Please contact the developer of this add-on: %s",
        Name().c_str(), addonCapabilities.provides_buttonmaps ? "true" : "false",
        m_bProvidesButtonMaps ? "true" : "false", Author().c_str());
    return false;
  }

  return true;
}

bool CPeripheralAddon::Register(unsigned int peripheralIndex, const PeripheralPtr& peripheral)
{
  if (!peripheral)
    return false;

  CSingleLock lock(m_critSection);

  if (m_peripherals.find(peripheralIndex) == m_peripherals.end())
  {
    if (peripheral->Type() == PERIPHERAL_JOYSTICK)
    {
      m_peripherals[peripheralIndex] = std::static_pointer_cast<CPeripheralJoystick>(peripheral);

      CLog::Log(LOGNOTICE, "%s - new %s device registered on %s->%s: %s",
          __FUNCTION__, PeripheralTypeTranslator::TypeToString(peripheral->Type()),
          PeripheralTypeTranslator::BusTypeToString(PERIPHERAL_BUS_ADDON),
          peripheral->Location().c_str(), peripheral->DeviceName().c_str());

      return true;
    }
  }
  return false;
}

void CPeripheralAddon::UnregisterRemovedDevices(const PeripheralScanResults &results, PeripheralVector& removedPeripherals)
{
  CSingleLock lock(m_critSection);
  std::vector<unsigned int> removedIndexes;
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
  lock.Leave();

  for (auto index : removedIndexes)
  {
    auto it = m_peripherals.find(index);
    const PeripheralPtr& peripheral = it->second;
    CLog::Log(LOGNOTICE, "%s - device removed from %s/%s: %s (%s:%s)", __FUNCTION__, PeripheralTypeTranslator::TypeToString(peripheral->Type()), peripheral->Location().c_str(), peripheral->DeviceName().c_str(), peripheral->VendorIdAsString(), peripheral->ProductIdAsString());
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

void CPeripheralAddon::GetFeatures(std::vector<PeripheralFeature> &features) const
{
  if (m_bProvidesJoysticks && std::find(features.begin(), features.end(), FEATURE_JOYSTICK) == features.end())
    features.push_back(FEATURE_JOYSTICK);
}

PeripheralPtr CPeripheralAddon::GetPeripheral(unsigned int index) const
{
  PeripheralPtr peripheral;
  CSingleLock lock(m_critSection);
  auto it = m_peripherals.find(index);
  if (it != m_peripherals.end())
    peripheral = it->second;
  return peripheral;
}

PeripheralPtr CPeripheralAddon::GetByPath(const std::string &strPath) const
{
  PeripheralPtr result;

  CSingleLock lock(m_critSection);
  for (auto it : m_peripherals)
  {
    if (StringUtils::EqualsNoCase(strPath, it.second->FileLocation()))
    {
      result = it.second;
      break;
    }
  }

  return result;
}

int CPeripheralAddon::GetPeripheralsWithFeature(PeripheralVector &results, const PeripheralFeature feature) const
{
  int iReturn(0);
  CSingleLock lock(m_critSection);
  for (auto it : m_peripherals)
  {
    if (it.second->HasFeature(feature))
    {
      results.push_back(it.second);
      ++iReturn;
    }
  }
  return iReturn;
}

size_t CPeripheralAddon::GetNumberOfPeripherals(void) const
{
  CSingleLock lock(m_critSection);
  return m_peripherals.size();
}

size_t CPeripheralAddon::GetNumberOfPeripheralsWithId(const int iVendorId, const int iProductId) const
{
  int iReturn(0);
  CSingleLock lock(m_critSection);
  for (auto it : m_peripherals)
  {
    if (it.second->VendorId() == iVendorId &&
        it.second->ProductId() == iProductId)
      iReturn++;
  }

  return iReturn;
}

void CPeripheralAddon::GetDirectory(const std::string &strPath, CFileItemList &items) const
{
  CSingleLock lock(m_critSection);
  for (auto it : m_peripherals)
  {
    const PeripheralPtr& peripheral = it.second;
    if (peripheral->IsHidden())
      continue;

    CFileItemPtr peripheralFile(new CFileItem(peripheral->DeviceName()));
    peripheralFile->SetPath(peripheral->FileLocation());
    peripheralFile->SetProperty("vendor", peripheral->VendorIdAsString());
    peripheralFile->SetProperty("product", peripheral->ProductIdAsString());
    peripheralFile->SetProperty("bus", PeripheralTypeTranslator::BusTypeToString(peripheral->GetBusType()));
    peripheralFile->SetProperty("location", peripheral->Location());
    peripheralFile->SetProperty("class", PeripheralTypeTranslator::TypeToString(peripheral->Type()));
    peripheralFile->SetProperty("version", peripheral->GetVersionInfo());
    peripheralFile->SetIconImage(peripheral->GetIcon());
    items.Add(peripheralFile);
  }
}

bool CPeripheralAddon::PerformDeviceScan(PeripheralScanResults &results)
{
  unsigned int      peripheralCount;
  PERIPHERAL_INFO*  pScanResults;
  PERIPHERAL_ERROR  retVal;

  LogError(retVal = m_struct.toAddon.PerformDeviceScan(m_addonInstance, &peripheralCount, &pScanResults), "PerformDeviceScan()");
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
      result.m_strLocation   = StringUtils::Format("%s/%d", ID().c_str(), peripheral.Index());
      result.m_iVendorId     = peripheral.VendorID();
      result.m_iProductId    = peripheral.ProductID();
      result.m_mappedType    = PERIPHERAL_JOYSTICK;
      result.m_mappedBusType = PERIPHERAL_BUS_ADDON;
      result.m_iSequence     = 0;

      if (!results.ContainsResult(result))
        results.m_results.push_back(result);
    }

    m_struct.toAddon.FreeScanResults(m_addonInstance, peripheralCount, pScanResults);

    return true;
  }

  return false;
}

bool CPeripheralAddon::ProcessEvents(void)
{
  if (!m_bProvidesJoysticks)
    return false;

  PERIPHERAL_ERROR retVal;

  unsigned int      eventCount = 0;
  PERIPHERAL_EVENT* pEvents = NULL;

  LogError(retVal = m_struct.toAddon.GetEvents(m_addonInstance, &eventCount, &pEvents), "GetEvents()");
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
        std::shared_ptr<CPeripheralJoystick> joystickDevice = std::static_pointer_cast<CPeripheralJoystick>(device);

        switch (event.Type())
        {
          case PERIPHERAL_EVENT_TYPE_DRIVER_BUTTON:
          {
            const bool bPressed = (event.ButtonState() == JOYSTICK_STATE_BUTTON_PRESSED);
            CLog::Log(LOGDEBUG, "Button [ %u ] on %s %s", event.DriverIndex(),
                      joystickDevice->DeviceName().c_str(), bPressed ? "pressed" : "released");
            if (joystickDevice->OnButtonMotion(event.DriverIndex(), bPressed))
              CLog::Log(LOGDEBUG, "Joystick button event handled");
            break;
          }
          case PERIPHERAL_EVENT_TYPE_DRIVER_HAT:
          {
            const HAT_STATE state = CPeripheralAddonTranslator::TranslateHatState(event.HatState());
            CLog::Log(LOGDEBUG, "Hat [ %u ] on %s %s", event.DriverIndex(),
                      joystickDevice->DeviceName().c_str(), CJoystickTranslator::HatStateToString(state));
            if (joystickDevice->OnHatMotion(event.DriverIndex(), state))
              CLog::Log(LOGDEBUG, "Joystick hat event handled");
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

    for (auto it : m_peripherals)
    {
      if (it.second->Type() == PERIPHERAL_JOYSTICK)
        std::static_pointer_cast<CPeripheralJoystick>(it.second)->ProcessAxisMotions();
    }

    m_struct.toAddon.FreeEvents(m_addonInstance, eventCount, pEvents);

    return true;
  }

  return false;
}

bool CPeripheralAddon::SendRumbleEvent(unsigned int peripheralIndex, unsigned int driverIndex, float magnitude)
{
  PERIPHERAL_EVENT eventStruct = { };

  eventStruct.peripheral_index = peripheralIndex;
  eventStruct.type             = PERIPHERAL_EVENT_TYPE_SET_MOTOR;
  eventStruct.driver_index     = driverIndex;
  eventStruct.motor_state      = magnitude;

  return m_struct.toAddon.SendEvent(m_addonInstance, &eventStruct);
}

bool CPeripheralAddon::GetJoystickProperties(unsigned int index, CPeripheralJoystick& joystick)
{
  if (!m_bProvidesJoysticks)
    return false;

  PERIPHERAL_ERROR retVal;

  JOYSTICK_INFO joystickStruct;

  LogError(retVal = m_struct.toAddon.GetJoystickInfo(m_addonInstance, index, &joystickStruct), "GetJoystickInfo()");
  if (retVal == PERIPHERAL_NO_ERROR)
  {
    kodi::addon::Joystick addonJoystick(joystickStruct);
    SetJoystickInfo(joystick, addonJoystick);

    m_struct.toAddon.FreeJoystickInfo(m_addonInstance, &joystickStruct);

    return true;
  }

  return false;
}

bool CPeripheralAddon::GetFeatures(const CPeripheral* device,
                                   const std::string& strControllerId,
                                   FeatureMap& features)
{
  if (!m_bProvidesButtonMaps)
    return false;

  PERIPHERAL_ERROR retVal;

  kodi::addon::Joystick joystickInfo;
  GetJoystickInfo(device, joystickInfo);

  JOYSTICK_INFO joystickStruct;
  joystickInfo.ToStruct(joystickStruct);

  unsigned int      featureCount = 0;
  JOYSTICK_FEATURE* pFeatures = NULL;

  LogError(retVal = m_struct.toAddon.GetFeatures(m_addonInstance, &joystickStruct, strControllerId.c_str(),
                                           &featureCount, &pFeatures), "GetFeatures()");
  if (retVal == PERIPHERAL_NO_ERROR)
  {
    for (unsigned int i = 0; i < featureCount; i++)
    {
      kodi::addon::JoystickFeature feature(pFeatures[i]);
      if (feature.Type() != JOYSTICK_FEATURE_TYPE_UNKNOWN)
        features[feature.Name()] = std::move(feature);
    }

    m_struct.toAddon.FreeFeatures(m_addonInstance, featureCount, pFeatures);

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

  PERIPHERAL_ERROR retVal;

  kodi::addon::Joystick joystickInfo;
  GetJoystickInfo(device, joystickInfo);

  JOYSTICK_INFO joystickStruct;
  joystickInfo.ToStruct(joystickStruct);

  JOYSTICK_FEATURE addonFeature;
  feature.ToStruct(addonFeature);

  LogError(retVal = m_struct.toAddon.MapFeatures(m_addonInstance, &joystickStruct, strControllerId.c_str(),
                                                 1, &addonFeature), "MapFeatures()");
  return retVal == PERIPHERAL_NO_ERROR;
}

bool CPeripheralAddon::GetIgnoredPrimitives(const CPeripheral* device, PrimitiveVector& primitives)
{
  if (!m_bProvidesButtonMaps)
    return false;

  PERIPHERAL_ERROR retVal;

  kodi::addon::Joystick joystickInfo;
  GetJoystickInfo(device, joystickInfo);

  JOYSTICK_INFO joystickStruct;
  joystickInfo.ToStruct(joystickStruct);

  unsigned int primitiveCount = 0;
  JOYSTICK_DRIVER_PRIMITIVE* pPrimitives = nullptr;

  LogError(retVal = m_struct.toAddon.GetIgnoredPrimitives(m_addonInstance, &joystickStruct, &primitiveCount,
                                                      &pPrimitives), "GetIgnoredPrimitives()");
  if (retVal == PERIPHERAL_NO_ERROR)
  {
    for (unsigned int i = 0; i < primitiveCount; i++)
      primitives.emplace_back(pPrimitives[i]);

    m_struct.toAddon.FreePrimitives(m_addonInstance, primitiveCount, pPrimitives);

    return true;
  }

  return false;

}

bool CPeripheralAddon::SetIgnoredPrimitives(const CPeripheral* device, const PrimitiveVector& primitives)
{
  if (!m_bProvidesButtonMaps)
    return false;

  PERIPHERAL_ERROR retVal;

  kodi::addon::Joystick joystickInfo;
  GetJoystickInfo(device, joystickInfo);

  JOYSTICK_INFO joystickStruct;
  joystickInfo.ToStruct(joystickStruct);

  JOYSTICK_DRIVER_PRIMITIVE* addonPrimitives = nullptr;
  kodi::addon::DriverPrimitives::ToStructs(primitives, &addonPrimitives);

  LogError(retVal = m_struct.toAddon.SetIgnoredPrimitives(m_addonInstance, &joystickStruct,
        primitives.size(), addonPrimitives), "SetIgnoredPrimitives()");

  kodi::addon::DriverPrimitives::FreeStructs(primitives.size(), addonPrimitives);

  return retVal == PERIPHERAL_NO_ERROR;
}

void CPeripheralAddon::SaveButtonMap(const CPeripheral* device)
{
  if (!m_bProvidesButtonMaps)
    return;

  kodi::addon::Joystick joystickInfo;
  GetJoystickInfo(device, joystickInfo);

  JOYSTICK_INFO joystickStruct;
  joystickInfo.ToStruct(joystickStruct);

  m_struct.toAddon.SaveButtonMap(m_addonInstance, &joystickStruct);

  // Notify observing button maps
  RefreshButtonMaps(device->DeviceName());
}

void CPeripheralAddon::RevertButtonMap(const CPeripheral* device)
{
  if (!m_bProvidesButtonMaps)
    return;

  kodi::addon::Joystick joystickInfo;
  GetJoystickInfo(device, joystickInfo);

  JOYSTICK_INFO joystickStruct;
  joystickInfo.ToStruct(joystickStruct);

  m_struct.toAddon.RevertButtonMap(m_addonInstance, &joystickStruct);
}

void CPeripheralAddon::ResetButtonMap(const CPeripheral* device, const std::string& strControllerId)
{
  if (!m_bProvidesButtonMaps)
    return;

  kodi::addon::Joystick joystickInfo;
  GetJoystickInfo(device, joystickInfo);

  JOYSTICK_INFO joystickStruct;
  joystickInfo.ToStruct(joystickStruct);

  m_struct.toAddon.ResetButtonMap(m_addonInstance, &joystickStruct, strControllerId.c_str());

  // Notify observing button maps
  RefreshButtonMaps(device->DeviceName());
}

void CPeripheralAddon::PowerOffJoystick(unsigned int index)
{
  if (!HasFeature(FEATURE_JOYSTICK))
    return;

  m_struct.toAddon.PowerOffJoystick(m_addonInstance, index);
}

void CPeripheralAddon::RegisterButtonMap(CPeripheral* device, IButtonMap* buttonMap)
{
  CSingleLock lock(m_buttonMapMutex);

  UnregisterButtonMap(buttonMap);
  m_buttonMaps.push_back(std::make_pair(device, buttonMap));
}

void CPeripheralAddon::UnregisterButtonMap(IButtonMap* buttonMap)
{
  CSingleLock lock(m_buttonMapMutex);

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
  CSingleLock lock(m_buttonMapMutex);

  m_buttonMaps.erase(std::remove_if(m_buttonMaps.begin(), m_buttonMaps.end(),
    [device](const std::pair<CPeripheral*, JOYSTICK::IButtonMap*>& buttonMap)
    {
      return buttonMap.first == device;
    }), m_buttonMaps.end());
}

void CPeripheralAddon::RefreshButtonMaps(const std::string& strDeviceName /* = "" */)
{
  CSingleLock lock(m_buttonMapMutex);

  for (auto it = m_buttonMaps.begin(); it != m_buttonMaps.end(); ++it)
  {
    if (strDeviceName.empty() || strDeviceName == it->first->DeviceName())
      it->second->Load();
  }
}

void CPeripheralAddon::GetPeripheralInfo(const CPeripheral* device, kodi::addon::Peripheral& peripheralInfo)
{
  peripheralInfo.SetType(CPeripheralAddonTranslator::TranslateType(device->Type()));
  peripheralInfo.SetName(device->DeviceName());
  peripheralInfo.SetVendorID(device->VendorId());
  peripheralInfo.SetProductID(device->ProductId());
}

void CPeripheralAddon::GetJoystickInfo(const CPeripheral* device, kodi::addon::Joystick& joystickInfo)
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
  else if (device->Type() == PERIPHERAL_JOYSTICK_EMULATION)
  {
    const CPeripheralJoystickEmulation* joystick = static_cast<const CPeripheralJoystickEmulation*>(device);
    joystickInfo.SetName(JOYSTICK_EMULATION_BUTTON_MAP_NAME); // Override name with non-localized version
    joystickInfo.SetProvider(JOYSTICK_EMULATION_PROVIDER);
    joystickInfo.SetIndex(joystick->ControllerNumber());
  }
}

void CPeripheralAddon::SetJoystickInfo(CPeripheralJoystick& joystick, const kodi::addon::Joystick& joystickInfo)
{
  joystick.SetProvider(joystickInfo.Provider());
  joystick.SetRequestedPort(joystickInfo.RequestedPort());
  joystick.SetButtonCount(joystickInfo.ButtonCount());
  joystick.SetHatCount(joystickInfo.HatCount());
  joystick.SetAxisCount(joystickInfo.AxisCount());
  joystick.SetMotorCount(joystickInfo.MotorCount());
  joystick.SetSupportsPowerOff(joystickInfo.SupportsPowerOff());
}

bool CPeripheralAddon::LogError(const PERIPHERAL_ERROR error, const char *strMethod) const
{
  if (error != PERIPHERAL_NO_ERROR)
  {
    CLog::Log(LOGERROR, "PERIPHERAL - %s - addon '%s' returned an error: %s",
        strMethod, Name().c_str(), CPeripheralAddonTranslator::TranslateError(error));
    return false;
  }
  return true;
}

void CPeripheralAddon::trigger_scan(void* kodiInstance)
{
  g_peripherals.TriggerDeviceScan(PERIPHERAL_BUS_ADDON);
}

void CPeripheralAddon::refresh_button_maps(void* kodiInstance, const char* deviceName, const char* controllerId)
{
  CPeripheralAddon* instance = static_cast<CPeripheralAddon*>(kodiInstance);
  if (!instance || !deviceName || !controllerId)
  {
    CLog::Log(LOGERROR, "kodi::gui::DialogOK:%s - invalid data (instance='%p', deviceName='%p', controllerId='%p')",
                                        __FUNCTION__, instance, deviceName, controllerId);
    return;
  }

  instance->RefreshButtonMaps(deviceName);
}

unsigned int CPeripheralAddon::feature_count(void* kodiInstance, const char* controllerId, JOYSTICK_FEATURE_TYPE type)
{
  using namespace GAME;

  unsigned int count = 0;

  CGameServices& gameServices = CServiceBroker::GetGameServices();
  ControllerPtr controller = gameServices.GetController(controllerId);
  if (controller)
    count = controller->Layout().FeatureCount(CPeripheralAddonTranslator::TranslateFeatureType(type));

  return count;
}