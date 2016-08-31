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
#include "peripherals/Peripherals.h"
#include "peripherals/addons/PeripheralAddon.h"
#include "peripherals/devices/PeripheralJoystick.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include <algorithm>
#include <memory>

using namespace ADDON;
using namespace PERIPHERALS;

CPeripheralBusAddon::CPeripheralBusAddon(CPeripherals *manager) :
    CPeripheralBus("PeripBusAddon", manager, PERIPHERAL_BUS_ADDON)
{
  CAddonMgr::GetInstance().RegisterAddonMgrCallback(ADDON_PERIPHERALDLL, this);
  CAddonMgr::GetInstance().Events().Subscribe(this, &CPeripheralBusAddon::OnEvent);

  UpdateAddons();
}

CPeripheralBusAddon::~CPeripheralBusAddon()
{
  CAddonMgr::GetInstance().Events().Unsubscribe(this);
  CAddonMgr::GetInstance().UnregisterAddonMgrCallback(ADDON_PERIPHERALDLL);

  // stop everything before destroying any (loaded) addons
  Clear();

  // destroy any (loaded) addons
  m_failedAddons.clear();
  m_addons.clear();
}

bool CPeripheralBusAddon::GetAddon(const std::string &strId, AddonPtr &addon) const
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

unsigned int CPeripheralBusAddon::GetAddonCount(void) const
{
  CSingleLock lock(m_critSection);
  return m_addons.size();
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

bool CPeripheralBusAddon::InitializeProperties(CPeripheral* peripheral)
{
  bool bSuccess = false;

  PeripheralAddonPtr addon;
  unsigned int index;

  if (SplitLocation(peripheral->Location(), addon, index))
  {
    switch (peripheral->Type())
    {
      case PERIPHERAL_JOYSTICK:
        bSuccess = addon->GetJoystickProperties(index, *static_cast<CPeripheralJoystick*>(peripheral));
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

void CPeripheralBusAddon::UnregisterRemovedDevices(const PeripheralScanResults &results)
{
  CSingleLock lock(m_critSection);

  std::vector<CPeripheral*> removedPeripherals;

  for (const auto& addon : m_addons)
    addon->UnregisterRemovedDevices(results, removedPeripherals);

  for (const auto& peripheral : removedPeripherals)
  {
    m_manager->OnDeviceDeleted(*this, *peripheral);
    delete peripheral;
  }
}

void CPeripheralBusAddon::Register(CPeripheral* peripheral)
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

CPeripheral *CPeripheralBusAddon::GetPeripheral(const std::string &strLocation) const
{
  CPeripheral*       peripheral(NULL);
  PeripheralAddonPtr addon;
  unsigned int       peripheralIndex;

  CSingleLock lock(m_critSection);

  if (SplitLocation(strLocation, addon, peripheralIndex))
    peripheral = addon->GetPeripheral(peripheralIndex);

  return peripheral;
}

CPeripheral *CPeripheralBusAddon::GetByPath(const std::string &strPath) const
{
  CSingleLock lock(m_critSection);

  for (const auto& addon : m_addons)
  {
    CPeripheral* peripheral = addon->GetByPath(strPath);
    if (peripheral)
      return peripheral;
  }

  return NULL;
}

int CPeripheralBusAddon::GetPeripheralsWithFeature(std::vector<CPeripheral *> &results, const PeripheralFeature feature) const
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
  if (typeid(event) == typeid(AddonEvents::InstalledChanged))
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
  auto GetPeripheralAddonID = [](const PeripheralAddonPtr& addon) { return addon->ID(); };
  auto GetAddonID = [](const AddonPtr& addon) { return addon->ID(); };

  std::set<std::string> currentIds;
  std::set<std::string> newIds;

  std::set<std::string> added;
  std::set<std::string> removed;

  // Get current add-ons
  PeripheralAddonVector currentAddons;
  PeripheralAddonVector failedAddons;
  {
    CSingleLock lock(m_critSection);
    currentAddons = m_addons;
    failedAddons = m_failedAddons;
  }
  std::transform(currentAddons.begin(), currentAddons.end(), std::inserter(currentIds, currentIds.end()), GetPeripheralAddonID);
  std::transform(failedAddons.begin(), failedAddons.end(), std::inserter(currentIds, currentIds.end()), GetPeripheralAddonID);

  // Get new add-ons
  VECADDONS newAddons;
  CAddonMgr::GetInstance().GetAddons(newAddons, ADDON_PERIPHERALDLL);
  std::transform(newAddons.begin(), newAddons.end(), std::inserter(newIds, newIds.end()), GetAddonID);

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
        if (newAddon->CreateAddon() == ADDON_STATUS_OK)
          currentAddons.push_back(newAddon);
        else
          failedAddons.push_back(newAddon);
      }
    }
  }

  // Destroy removed add-ons
  for (const std::string& addonId : removed)
  {
    CLog::Log(LOGDEBUG, "Add-on bus: Unregistering add-on %s", addonId.c_str());

    auto ErasePeripheralAddon = [addonId](const PeripheralAddonPtr& addon)
      {
        if (addon->ID() == addonId)
        {
          addon->Destroy();
          return true;
        }
        return false;
      };

    currentAddons.erase(std::remove_if(currentAddons.begin(), currentAddons.end(), ErasePeripheralAddon), currentAddons.end());
    failedAddons.erase(std::remove_if(failedAddons.begin(), failedAddons.end(), ErasePeripheralAddon), failedAddons.end());
  }

  // Record results
  {
    CSingleLock lock(m_critSection);
    m_addons = std::move(currentAddons);
    m_failedAddons = std::move(failedAddons);
  }
}
