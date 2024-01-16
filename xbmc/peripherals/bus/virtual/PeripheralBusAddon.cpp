/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PeripheralBusAddon.h"

#include "ServiceBroker.h"
#include "addons/AddonEvents.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "messaging/helpers/DialogHelper.h"
#include "peripherals/Peripherals.h"
#include "peripherals/addons/PeripheralAddon.h"
#include "peripherals/devices/PeripheralJoystick.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include <algorithm>
#include <memory>
#include <mutex>

using namespace KODI;
using namespace PERIPHERALS;

CPeripheralBusAddon::CPeripheralBusAddon(CPeripherals& manager)
  : CPeripheralBus("PeripBusAddon", manager, PERIPHERAL_BUS_ADDON)
{
  using namespace ADDON;

  CServiceBroker::GetAddonMgr().Events().Subscribe(this, &CPeripheralBusAddon::OnEvent);

  UpdateAddons();
}

CPeripheralBusAddon::~CPeripheralBusAddon()
{
  using namespace ADDON;

  CServiceBroker::GetAddonMgr().Events().Unsubscribe(this);

  // stop everything before destroying any (loaded) addons
  Clear();

  // destroy any (loaded) addons
  for (const auto& addon : m_addons)
    addon->DestroyAddon();

  m_failedAddons.clear();
  m_addons.clear();
}

bool CPeripheralBusAddon::GetAddonWithButtonMap(PeripheralAddonPtr& addon) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  auto it = std::find_if(m_addons.begin(), m_addons.end(),
                         [](const PeripheralAddonPtr& addon) { return addon->HasButtonMaps(); });

  if (it != m_addons.end())
  {
    addon = *it;
    return true;
  }

  return false;
}

bool CPeripheralBusAddon::GetAddonWithButtonMap(const CPeripheral* device,
                                                PeripheralAddonPtr& addon) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  // If device is from an add-on, try to use that add-on
  if (device && device->GetBusType() == PERIPHERAL_BUS_ADDON)
  {
    PeripheralAddonPtr addonWithButtonMap;
    unsigned int index;
    if (SplitLocation(device->Location(), addonWithButtonMap, index))
    {
      if (addonWithButtonMap->HasButtonMaps())
        addon = std::move(addonWithButtonMap);
      else
        CLog::Log(LOGDEBUG, "Add-on {} doesn't provide button maps for its controllers",
                  addonWithButtonMap->ID());
    }
  }

  if (!addon)
  {
    auto it = std::find_if(m_addons.begin(), m_addons.end(),
                           [](const PeripheralAddonPtr& addon) { return addon->HasButtonMaps(); });

    if (it != m_addons.end())
      addon = *it;
  }

  return addon.get() != nullptr;
}

bool CPeripheralBusAddon::PerformDeviceScan(PeripheralScanResults& results)
{
  PeripheralAddonVector addons;
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    addons = m_addons;
  }

  for (const auto& addon : addons)
    addon->PerformDeviceScan(results);

  // Scan during bus initialization must return true or bus gets deleted
  return true;
}

bool CPeripheralBusAddon::InitializeProperties(CPeripheral& peripheral)
{
  if (!CPeripheralBus::InitializeProperties(peripheral))
    return false;

  bool bSuccess = false;

  PeripheralAddonPtr addon;
  unsigned int index;

  if (SplitLocation(peripheral.Location(), addon, index))
  {
    switch (peripheral.Type())
    {
      case PERIPHERAL_JOYSTICK:
        bSuccess =
            addon->GetJoystickProperties(index, static_cast<CPeripheralJoystick&>(peripheral));
        break;

      default:
        break;
    }
  }

  return bSuccess;
}

bool CPeripheralBusAddon::SendRumbleEvent(const std::string& strLocation,
                                          unsigned int motorIndex,
                                          float magnitude)
{
  bool bHandled = false;

  PeripheralAddonPtr addon;
  unsigned int peripheralIndex;
  if (SplitLocation(strLocation, addon, peripheralIndex))
    bHandled = addon->SendRumbleEvent(peripheralIndex, motorIndex, magnitude);

  return bHandled;
}

void CPeripheralBusAddon::ProcessEvents(void)
{
  PeripheralAddonVector addons;

  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    addons = m_addons;
  }

  for (const auto& addon : addons)
    addon->ProcessEvents();
}

void CPeripheralBusAddon::EnableButtonMapping()
{
  using namespace ADDON;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  PeripheralAddonPtr dummy;

  if (!GetAddonWithButtonMap(dummy))
  {
    std::vector<AddonInfoPtr> disabledAddons;
    CServiceBroker::GetAddonMgr().GetDisabledAddonInfos(disabledAddons, AddonType::PERIPHERALDLL);
    if (!disabledAddons.empty())
      PromptEnableAddons(disabledAddons);
  }
}

void CPeripheralBusAddon::PowerOff(const std::string& strLocation)
{
  PeripheralAddonPtr addon;
  unsigned int peripheralIndex;
  if (SplitLocation(strLocation, addon, peripheralIndex))
    addon->PowerOffJoystick(peripheralIndex);
}

void CPeripheralBusAddon::UnregisterRemovedDevices(const PeripheralScanResults& results)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  PeripheralVector removedPeripherals;

  for (const auto& addon : m_addons)
    addon->UnregisterRemovedDevices(results, removedPeripherals);

  for (const auto& peripheral : removedPeripherals)
    m_manager.OnDeviceDeleted(*this, *peripheral);
}

void CPeripheralBusAddon::Register(const PeripheralPtr& peripheral)
{
  if (!peripheral)
    return;

  PeripheralAddonPtr addon;
  unsigned int peripheralIndex;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (SplitLocation(peripheral->Location(), addon, peripheralIndex))
  {
    if (addon->Register(peripheralIndex, peripheral))
      m_manager.OnDeviceAdded(*this, *peripheral);
  }
}

void CPeripheralBusAddon::GetFeatures(std::vector<PeripheralFeature>& features) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& addon : m_addons)
    addon->GetFeatures(features);
}

bool CPeripheralBusAddon::HasFeature(const PeripheralFeature feature) const
{
  bool bReturn(false);
  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& addon : m_addons)
    bReturn = bReturn || addon->HasFeature(feature);
  return bReturn;
}

PeripheralPtr CPeripheralBusAddon::GetPeripheral(const std::string& strLocation) const
{
  PeripheralPtr peripheral;
  PeripheralAddonPtr addon;
  unsigned int peripheralIndex;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (SplitLocation(strLocation, addon, peripheralIndex))
    peripheral = addon->GetPeripheral(peripheralIndex);

  return peripheral;
}

PeripheralPtr CPeripheralBusAddon::GetByPath(const std::string& strPath) const
{
  PeripheralPtr result;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  for (const auto& addon : m_addons)
  {
    PeripheralPtr peripheral = addon->GetByPath(strPath);
    if (peripheral)
    {
      result = peripheral;
      break;
    }
  }

  return result;
}

bool CPeripheralBusAddon::SupportsFeature(PeripheralFeature feature) const
{
  bool bSupportsFeature = false;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& addon : m_addons)
    bSupportsFeature |= addon->SupportsFeature(feature);

  return bSupportsFeature;
}

unsigned int CPeripheralBusAddon::GetPeripheralsWithFeature(PeripheralVector& results,
                                                            const PeripheralFeature feature) const
{
  unsigned int iReturn = 0;
  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& addon : m_addons)
    iReturn += addon->GetPeripheralsWithFeature(results, feature);
  return iReturn;
}

unsigned int CPeripheralBusAddon::GetNumberOfPeripherals(void) const
{
  unsigned int iReturn = 0;
  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& addon : m_addons)
    iReturn += addon->GetNumberOfPeripherals();
  return iReturn;
}

unsigned int CPeripheralBusAddon::GetNumberOfPeripheralsWithId(const int iVendorId,
                                                               const int iProductId) const
{
  unsigned int iReturn = 0;
  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& addon : m_addons)
    iReturn += addon->GetNumberOfPeripheralsWithId(iVendorId, iProductId);
  return iReturn;
}

void CPeripheralBusAddon::GetDirectory(const std::string& strPath, CFileItemList& items) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& addon : m_addons)
    addon->GetDirectory(strPath, items);
}

void CPeripheralBusAddon::OnEvent(const ADDON::AddonEvent& event)
{
  if (typeid(event) == typeid(ADDON::AddonEvents::Enabled) ||
      typeid(event) == typeid(ADDON::AddonEvents::ReInstalled))
  {
    if (CServiceBroker::GetAddonMgr().HasType(event.addonId, ADDON::AddonType::PERIPHERALDLL))
      UpdateAddons();
  }
  else if (typeid(event) == typeid(ADDON::AddonEvents::Disabled))
  {
    if (CServiceBroker::GetAddonMgr().HasType(event.addonId, ADDON::AddonType::PERIPHERALDLL))
      UnRegisterAddon(event.addonId);
  }
  else if (typeid(event) == typeid(ADDON::AddonEvents::UnInstalled))
  {
    UnRegisterAddon(event.addonId);
  }
}

bool CPeripheralBusAddon::SplitLocation(const std::string& strLocation,
                                        PeripheralAddonPtr& addon,
                                        unsigned int& peripheralIndex) const
{
  std::vector<std::string> parts = StringUtils::Split(strLocation, "/");
  if (parts.size() == 2)
  {
    addon.reset();

    std::unique_lock<CCriticalSection> lock(m_critSection);

    const std::string& strAddonId = parts[0];
    for (const auto& addonIt : m_addons)
    {
      if (addonIt->ID() == strAddonId)
      {
        addon = addonIt;
        break;
      }
    }

    if (addon)
    {
      const char* strJoystickIndex = parts[1].c_str();
      char* p = NULL;
      peripheralIndex = strtol(strJoystickIndex, &p, 10);
      if (strJoystickIndex != p)
        return true;
    }
  }
  return false;
}

void CPeripheralBusAddon::UpdateAddons(void)
{
  using namespace ADDON;

  auto GetPeripheralAddonID = [](const PeripheralAddonPtr& addon) { return addon->ID(); };
  auto GetAddonID = [](const AddonInfoPtr& addon) { return addon->ID(); };

  std::set<std::string> currentIds;
  std::set<std::string> newIds;

  std::set<std::string> added;
  std::set<std::string> removed;

  // Get new add-ons
  std::vector<AddonInfoPtr> newAddons;
  CServiceBroker::GetAddonMgr().GetAddonInfos(newAddons, true, AddonType::PERIPHERALDLL);
  std::transform(newAddons.begin(), newAddons.end(), std::inserter(newIds, newIds.end()),
                 GetAddonID);

  std::unique_lock<CCriticalSection> lock(m_critSection);

  // Get current add-ons
  std::transform(m_addons.begin(), m_addons.end(), std::inserter(currentIds, currentIds.end()),
                 GetPeripheralAddonID);
  std::transform(m_failedAddons.begin(), m_failedAddons.end(),
                 std::inserter(currentIds, currentIds.end()), GetPeripheralAddonID);

  // Differences
  std::set_difference(newIds.begin(), newIds.end(), currentIds.begin(), currentIds.end(),
                      std::inserter(added, added.end()));
  std::set_difference(currentIds.begin(), currentIds.end(), newIds.begin(), newIds.end(),
                      std::inserter(removed, removed.end()));

  // Register new add-ons
  for (const std::string& addonId : added)
  {
    CLog::Log(LOGDEBUG, "Add-on bus: Registering add-on {}", addonId);

    auto GetAddon = [&addonId](const AddonInfoPtr& addon) { return addon->ID() == addonId; };

    auto it = std::find_if(newAddons.begin(), newAddons.end(), GetAddon);
    if (it != newAddons.end())
    {
      PeripheralAddonPtr newAddon = std::make_shared<CPeripheralAddon>(*it, m_manager);
      if (newAddon)
      {
        bool bCreated;

        {
          CSingleExit exit(m_critSection);
          bCreated = newAddon->CreateAddon();
        }

        if (bCreated)
          m_addons.emplace_back(std::move(newAddon));
        else
          m_failedAddons.emplace_back(std::move(newAddon));
      }
    }
  }

  // Destroy removed add-ons
  for (const std::string& addonId : removed)
  {
    UnRegisterAddon(addonId);
  }
}

void CPeripheralBusAddon::UnRegisterAddon(const std::string& addonId)
{
  PeripheralAddonPtr erased;
  auto ErasePeripheralAddon = [&addonId, &erased](const PeripheralAddonPtr& addon)
  {
    if (addon->ID() == addonId)
    {
      erased = addon;
      return true;
    }
    return false;
  };

  m_addons.erase(std::remove_if(m_addons.begin(), m_addons.end(), ErasePeripheralAddon),
                 m_addons.end());
  if (!erased)
    m_failedAddons.erase(
        std::remove_if(m_failedAddons.begin(), m_failedAddons.end(), ErasePeripheralAddon),
        m_failedAddons.end());

  if (erased)
  {
    CLog::Log(LOGDEBUG, "Add-on bus: Unregistered add-on {}", addonId);
    CSingleExit exit(m_critSection);
    erased->DestroyAddon();
  }
}

void CPeripheralBusAddon::PromptEnableAddons(
    const std::vector<std::shared_ptr<ADDON::CAddonInfo>>& disabledAddons)
{
  using namespace ADDON;
  using namespace MESSAGING::HELPERS;

  // True if the user confirms enabling the disabled peripheral add-on
  bool bAccepted = false;

  auto itAddon = std::find_if(disabledAddons.begin(), disabledAddons.end(),
                              [](const AddonInfoPtr& addonInfo)
                              { return CPeripheralAddon::ProvidesJoysticks(addonInfo); });

  if (itAddon != disabledAddons.end())
  {
    // "Unable to configure controllers"
    // "Controller configuration depends on a disabled add-on. Would you like to enable it?"
    bAccepted =
        (ShowYesNoDialogLines(CVariant{35017}, CVariant{35018}) == DialogResponse::CHOICE_YES);
  }

  if (bAccepted)
  {
    for (const auto& addonInfo : disabledAddons)
    {
      if (CPeripheralAddon::ProvidesJoysticks(addonInfo))
        CServiceBroker::GetAddonMgr().EnableAddon(addonInfo->ID());
    }
  }
}
