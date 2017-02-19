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

#include "PeripheralBusAddon.h"
#include "addons/Addon.h"
#include "addons/AddonManager.h"
#include "messaging/helpers/DialogHelper.h"
#include "peripherals/Peripherals.h"
#include "peripherals/addons/PeripheralAddon.h"
#include "peripherals/devices/PeripheralJoystick.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include <algorithm>
#include <memory>

using namespace KODI;
using namespace PERIPHERALS;

CPeripheralBusAddon::CPeripheralBusAddon(CPeripherals *manager) :
    CPeripheralBus("PeripBusAddon", manager, PERIPHERAL_BUS_ADDON)
{
  using namespace ADDON;

  CAddonMgr::GetInstance().RegisterAddonMgrCallback(ADDON_PERIPHERALDLL, this);
  CAddonMgr::GetInstance().Events().Subscribe(this, &CPeripheralBusAddon::OnEvent);

  UpdateAddons();
}

CPeripheralBusAddon::~CPeripheralBusAddon()
{
  using namespace ADDON;

  CAddonMgr::GetInstance().Events().Unsubscribe(this);
  CAddonMgr::GetInstance().UnregisterAddonMgrCallback(ADDON_PERIPHERALDLL);

  // stop everything before destroying any (loaded) addons
  Clear();

  // destroy any (loaded) addons
  m_failedAddons.clear();
  m_addons.clear();
}

bool CPeripheralBusAddon::GetAddon(const std::string &strId, ADDON::AddonPtr &addon) const
{
  CSingleLock lock(m_critSection);
  for (const auto& addonIt : m_addons)
  {
    if (addonIt->ID() == strId)
    {
      addon = addonIt;
      return true;
    }
  }
  for (const auto& addonIt : m_failedAddons)
  {
    if (addonIt->ID() == strId)
    {
      addon = addonIt;
      return true;
    }
  }
  return false;
}

bool CPeripheralBusAddon::GetAddonWithButtonMap(PeripheralAddonPtr &addon) const
{
  CSingleLock lock(m_critSection);

  auto it = std::find_if(m_addons.begin(), m_addons.end(),
    [](const PeripheralAddonPtr& addon)
    {
      return addon->HasButtonMaps();
    });

  if (it != m_addons.end())
  {
    addon = *it;
    return  true;
  }

  return false;
}

bool CPeripheralBusAddon::GetAddonWithButtonMap(const CPeripheral* device, PeripheralAddonPtr &addon) const
{
  CSingleLock lock(m_critSection);

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
        CLog::Log(LOGDEBUG, "Add-on %s doesn't provide button maps for its controllers", addonWithButtonMap->ID().c_str());
    }
  }

  if (!addon)
  {
    auto it = std::find_if(m_addons.begin(), m_addons.end(),
      [](const PeripheralAddonPtr& addon)
      {
        return addon->HasButtonMaps();
      });

    if (it != m_addons.end())
      addon = *it;
  }

  return addon.get() != nullptr;
}

bool CPeripheralBusAddon::PerformDeviceScan(PeripheralScanResults &results)
{
  PeripheralAddonVector addons;
  {
    CSingleLock lock(m_critSection);
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
        bSuccess = addon->GetJoystickProperties(index, static_cast<CPeripheralJoystick&>(peripheral));
        break;

      default:
        break;
    }
  }

  return bSuccess;
}

bool CPeripheralBusAddon::SendRumbleEvent(const std::string& strLocation, unsigned int motorIndex, float magnitude)
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
    CSingleLock lock(m_critSection);
    addons = m_addons;
  }

  for (const auto& addon : addons)
    addon->ProcessEvents();
}

bool CPeripheralBusAddon::EnableButtonMapping()
{
  using namespace ADDON;

  bool bEnabled = false;

  CSingleLock lock(m_critSection);

  PeripheralAddonPtr dummy;

  if (GetAddonWithButtonMap(dummy))
    bEnabled = true;
  else
  {
    VECADDONS disabledAddons;
    if (CAddonMgr::GetInstance().GetDisabledAddons(disabledAddons, ADDON_PERIPHERALDLL))
      bEnabled = PromptEnableAddons(disabledAddons);
  }

  return bEnabled;
}

void CPeripheralBusAddon::UnregisterRemovedDevices(const PeripheralScanResults &results)
{
  CSingleLock lock(m_critSection);

  PeripheralVector removedPeripherals;

  for (const auto& addon : m_addons)
    addon->UnregisterRemovedDevices(results, removedPeripherals);

  for (const auto& peripheral : removedPeripherals)
    m_manager->OnDeviceDeleted(*this, *peripheral);
}

void CPeripheralBusAddon::Register(const PeripheralPtr& peripheral)
{
  if (!peripheral)
    return;

  PeripheralAddonPtr addon;
  unsigned int       peripheralIndex;

  CSingleLock lock(m_critSection);

  if (SplitLocation(peripheral->Location(), addon, peripheralIndex))
  {
    if (addon->Register(peripheralIndex, peripheral))
      m_manager->OnDeviceAdded(*this, *peripheral);
  }
}

void CPeripheralBusAddon::GetFeatures(std::vector<PeripheralFeature> &features) const
{
  CSingleLock lock(m_critSection);
  for (const auto& addon : m_addons)
    addon->GetFeatures(features);
}

bool CPeripheralBusAddon::HasFeature(const PeripheralFeature feature) const
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);
  for (const auto& addon : m_addons)
    bReturn = bReturn || addon->HasFeature(feature);
  return bReturn;
}

PeripheralPtr CPeripheralBusAddon::GetPeripheral(const std::string &strLocation) const
{
  PeripheralPtr      peripheral;
  PeripheralAddonPtr addon;
  unsigned int       peripheralIndex;

  CSingleLock lock(m_critSection);

  if (SplitLocation(strLocation, addon, peripheralIndex))
    peripheral = addon->GetPeripheral(peripheralIndex);

  return peripheral;
}

PeripheralPtr CPeripheralBusAddon::GetByPath(const std::string &strPath) const
{
  PeripheralPtr result;

  CSingleLock lock(m_critSection);

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

  CSingleLock lock(m_critSection);
  for (const auto& addon : m_addons)
    bSupportsFeature |= addon->SupportsFeature(feature);

  return bSupportsFeature;
}

int CPeripheralBusAddon::GetPeripheralsWithFeature(PeripheralVector &results, const PeripheralFeature feature) const
{
  int iReturn(0);
  CSingleLock lock(m_critSection);
  for (const auto& addon : m_addons)
    iReturn += addon->GetPeripheralsWithFeature(results, feature);
  return iReturn;
}

size_t CPeripheralBusAddon::GetNumberOfPeripherals(void) const
{
  size_t iReturn(0);
  CSingleLock lock(m_critSection);
  for (const auto& addon : m_addons)
    iReturn += addon->GetNumberOfPeripherals();
  return iReturn;
}

size_t CPeripheralBusAddon::GetNumberOfPeripheralsWithId(const int iVendorId, const int iProductId) const
{
  size_t iReturn(0);
  CSingleLock lock(m_critSection);
  for (const auto& addon : m_addons)
    iReturn += addon->GetNumberOfPeripheralsWithId(iVendorId, iProductId);
  return iReturn;
}

void CPeripheralBusAddon::GetDirectory(const std::string &strPath, CFileItemList &items) const
{
  CSingleLock lock(m_critSection);
  for (const auto& addon : m_addons)
    addon->GetDirectory(strPath, items);
}

bool CPeripheralBusAddon::RequestRestart(ADDON::AddonPtr addon, bool datachanged)
{
  // make sure this is a peripheral addon
  PeripheralAddonPtr peripheralAddon = std::dynamic_pointer_cast<CPeripheralAddon>(addon);
  if (peripheralAddon == nullptr)
    return false;

  if (peripheralAddon->CreateAddon() != ADDON_STATUS_OK)
  {
    CSingleLock lock(m_critSection);
    m_addons.erase(std::remove(m_addons.begin(), m_addons.end(), peripheralAddon), m_addons.end());
    m_failedAddons.push_back(peripheralAddon);
  }

  return true;
}

bool CPeripheralBusAddon::RequestRemoval(ADDON::AddonPtr addon)
{
  // make sure this is a peripheral addon
  PeripheralAddonPtr peripheralAddon = std::dynamic_pointer_cast<CPeripheralAddon>(addon);
  if (peripheralAddon == nullptr)
    return false;

  CSingleLock lock(m_critSection);
  // destroy the peripheral addon
  peripheralAddon->Destroy();

  // remove the peripheral addon from the list of addons
  m_addons.erase(std::remove(m_addons.begin(), m_addons.end(), peripheralAddon), m_addons.end());

  return true;
}

void CPeripheralBusAddon::OnEvent(const ADDON::AddonEvent& event)
{
  if (typeid(event) == typeid(ADDON::AddonEvents::Enabled) ||
      typeid(event) == typeid(ADDON::AddonEvents::Disabled) ||
      typeid(event) == typeid(ADDON::AddonEvents::InstalledChanged))
    UpdateAddons();
}

bool CPeripheralBusAddon::SplitLocation(const std::string& strLocation, PeripheralAddonPtr& addon, unsigned int& peripheralIndex) const
{
  std::vector<std::string> parts = StringUtils::Split(strLocation, "/");
  if (parts.size() == 2)
  {
    addon.reset();

    CSingleLock lock(m_critSection);

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
  auto GetAddonID = [](const AddonPtr& addon) { return addon->ID(); };

  std::set<std::string> currentIds;
  std::set<std::string> newIds;

  std::set<std::string> added;
  std::set<std::string> removed;

  // Get new add-ons
  VECADDONS newAddons;
  CAddonMgr::GetInstance().GetAddons(newAddons, ADDON_PERIPHERALDLL);
  std::transform(newAddons.begin(), newAddons.end(), std::inserter(newIds, newIds.end()), GetAddonID);

  CSingleLock lock(m_critSection);

  // Get current add-ons
  std::transform(m_addons.begin(), m_addons.end(), std::inserter(currentIds, currentIds.end()), GetPeripheralAddonID);
  std::transform(m_failedAddons.begin(), m_failedAddons.end(), std::inserter(currentIds, currentIds.end()), GetPeripheralAddonID);

  // Differences
  std::set_difference(newIds.begin(), newIds.end(), currentIds.begin(), currentIds.end(), std::inserter(added, added.end()));
  std::set_difference(currentIds.begin(), currentIds.end(), newIds.begin(), newIds.end(), std::inserter(removed, removed.end()));

  // Register new add-ons
  for (const std::string& addonId : added)
  {
    CLog::Log(LOGDEBUG, "Add-on bus: Registering add-on %s", addonId.c_str());

    auto GetAddon = [addonId](const AddonPtr& addon) { return addon->ID() == addonId; };

    VECADDONS::iterator it = std::find_if(newAddons.begin(), newAddons.end(), GetAddon);
    if (it != newAddons.end())
    {
      PeripheralAddonPtr newAddon = std::dynamic_pointer_cast<CPeripheralAddon>(*it);
      if (newAddon)
      {
        bool bCreated;

        {
          CSingleExit exit(m_critSection);
          bCreated = (newAddon->CreateAddon() == ADDON_STATUS_OK);
        }

        if (bCreated)
          m_addons.push_back(newAddon);
        else
          m_failedAddons.push_back(newAddon);
      }
    }
  }

  // Destroy removed add-ons
  for (const std::string& addonId : removed)
  {
    CLog::Log(LOGDEBUG, "Add-on bus: Unregistering add-on %s", addonId.c_str());

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

    m_addons.erase(std::remove_if(m_addons.begin(), m_addons.end(), ErasePeripheralAddon), m_addons.end());
    if (!erased)
      m_failedAddons.erase(std::remove_if(m_failedAddons.begin(), m_failedAddons.end(), ErasePeripheralAddon), m_failedAddons.end());

    if (erased)
    {
      CSingleExit exit(m_critSection);
      erased->Destroy();
    }
  }
}

bool CPeripheralBusAddon::PromptEnableAddons(const ADDON::VECADDONS& disabledAddons)
{
  using namespace ADDON;
  using namespace MESSAGING::HELPERS;

  // True if the user confirms enabling the disabled peripheral add-on
  bool bAccepted = false;

  auto itAddon = std::find_if(disabledAddons.begin(), disabledAddons.end(),
    [](const AddonPtr& addon)
  {
    return std::static_pointer_cast<CPeripheralAddon>(addon)->HasButtonMaps();
  });

  if (itAddon != disabledAddons.end())
  {
    // "Unable to configure controllers"
    // "Controller configuration depends on a disabled add-on. Would you like to enable it?"
    bAccepted = (ShowYesNoDialogLines(CVariant{ 35017 }, CVariant{ 35018 }) == DialogResponse::YES);
  }

  if (bAccepted)
  {
    for (const AddonPtr& addon : disabledAddons)
    {
      if (std::static_pointer_cast<CPeripheralAddon>(addon)->HasButtonMaps())
        CAddonMgr::GetInstance().EnableAddon(addon->ID());
    }
  }

  PeripheralAddonPtr dummy;
  return GetAddonWithButtonMap(dummy);
}
