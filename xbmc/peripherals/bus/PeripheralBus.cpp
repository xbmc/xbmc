/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PeripheralBus.h"

#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "peripherals/Peripherals.h"
#include "peripherals/devices/Peripheral.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

using namespace PERIPHERALS;

#define PERIPHERAL_DEFAULT_RESCAN_INTERVAL 5000

CPeripheralBus::CPeripheralBus(const std::string& threadname,
                               CPeripherals& manager,
                               PeripheralBusType type)
  : CThread(threadname.c_str()),
    m_iRescanTime(PERIPHERAL_DEFAULT_RESCAN_INTERVAL),
    m_bNeedsPolling(true),
    m_manager(manager),
    m_type(type),
    m_triggerEvent(true)
{
}

bool CPeripheralBus::InitializeProperties(CPeripheral& peripheral)
{
  return true;
}

void CPeripheralBus::OnDeviceAdded(const std::string& strLocation)
{
  ScanForDevices();
}

void CPeripheralBus::OnDeviceChanged(const std::string& strLocation)
{
  ScanForDevices();
}

void CPeripheralBus::OnDeviceRemoved(const std::string& strLocation)
{
  ScanForDevices();
}

void CPeripheralBus::Clear(void)
{
  if (m_bNeedsPolling)
  {
    StopThread(false);
    m_triggerEvent.Set();
    StopThread(true);
  }

  CSingleLock lock(m_critSection);

  m_peripherals.clear();
}

void CPeripheralBus::UnregisterRemovedDevices(const PeripheralScanResults& results)
{
  PeripheralVector removedPeripherals;

  {
    CSingleLock lock(m_critSection);
    for (int iDevicePtr = (int)m_peripherals.size() - 1; iDevicePtr >= 0; iDevicePtr--)
    {
      const PeripheralPtr& peripheral = m_peripherals.at(iDevicePtr);
      PeripheralScanResult updatedDevice(m_type);
      if (!results.GetDeviceOnLocation(peripheral->Location(), &updatedDevice) ||
          *peripheral != updatedDevice)
      {
        /* device removed */
        removedPeripherals.push_back(peripheral);
        m_peripherals.erase(m_peripherals.begin() + iDevicePtr);
      }
    }
  }

  for (auto& peripheral : removedPeripherals)
  {
    std::vector<PeripheralFeature> features;
    peripheral->GetFeatures(features);
    bool peripheralHasFeatures =
        features.size() > 1 || (features.size() == 1 && features.at(0) != FEATURE_UNKNOWN);
    if (peripheral->Type() != PERIPHERAL_UNKNOWN || peripheralHasFeatures)
    {
      CLog::Log(LOGINFO, "%s - device removed from %s/%s: %s (%s:%s)", __FUNCTION__,
                PeripheralTypeTranslator::TypeToString(peripheral->Type()),
                peripheral->Location().c_str(), peripheral->DeviceName().c_str(),
                peripheral->VendorIdAsString(), peripheral->ProductIdAsString());
      peripheral->OnDeviceRemoved();
    }

    m_manager.OnDeviceDeleted(*this, *peripheral);
  }
}

void CPeripheralBus::RegisterNewDevices(const PeripheralScanResults& results)
{
  for (unsigned int iResultPtr = 0; iResultPtr < results.m_results.size(); iResultPtr++)
  {
    const PeripheralScanResult& result = results.m_results.at(iResultPtr);
    if (!HasPeripheral(result.m_strLocation))
      m_manager.CreatePeripheral(*this, result);
  }
}

bool CPeripheralBus::ScanForDevices(void)
{
  bool bReturn(false);

  PeripheralScanResults results;
  if (PerformDeviceScan(results))
  {
    UnregisterRemovedDevices(results);
    RegisterNewDevices(results);

    m_manager.NotifyObservers(ObservableMessagePeripheralsChanged);

    bReturn = true;
  }

  return bReturn;
}

bool CPeripheralBus::HasFeature(const PeripheralFeature feature) const
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);
  for (unsigned int iPeripheralPtr = 0; iPeripheralPtr < m_peripherals.size(); iPeripheralPtr++)
  {
    if (m_peripherals.at(iPeripheralPtr)->HasFeature(feature))
    {
      bReturn = true;
      break;
    }
  }
  return bReturn;
}

void CPeripheralBus::GetFeatures(std::vector<PeripheralFeature>& features) const
{
  CSingleLock lock(m_critSection);
  for (unsigned int iPeripheralPtr = 0; iPeripheralPtr < m_peripherals.size(); iPeripheralPtr++)
    m_peripherals.at(iPeripheralPtr)->GetFeatures(features);
}

PeripheralPtr CPeripheralBus::GetPeripheral(const std::string& strLocation) const
{
  PeripheralPtr result;
  CSingleLock lock(m_critSection);
  for (auto& peripheral : m_peripherals)
  {
    if (peripheral->Location() == strLocation)
    {
      result = peripheral;
      break;
    }
  }
  return result;
}

unsigned int CPeripheralBus::GetPeripheralsWithFeature(PeripheralVector& results,
                                                       const PeripheralFeature feature) const
{
  unsigned int iReturn = 0;
  CSingleLock lock(m_critSection);
  for (auto& peripheral : m_peripherals)
  {
    if (peripheral->HasFeature(feature))
    {
      results.push_back(peripheral);
      ++iReturn;
    }
  }

  return iReturn;
}

unsigned int CPeripheralBus::GetNumberOfPeripheralsWithId(const int iVendorId,
                                                          const int iProductId) const
{
  unsigned int iReturn = 0;
  CSingleLock lock(m_critSection);
  for (const auto& peripheral : m_peripherals)
  {
    if (peripheral->VendorId() == iVendorId && peripheral->ProductId() == iProductId)
      iReturn++;
  }

  return iReturn;
}

void CPeripheralBus::Process(void)
{
  while (!m_bStop)
  {
    m_triggerEvent.Reset();

    if (!ScanForDevices())
      break;

    // depending on bus implementation
    // needsPolling can be set properly
    // only after initial scan.
    // if this is the case, bail out.
    if (!m_bNeedsPolling)
      break;

    if (!m_bStop)
      m_triggerEvent.WaitMSec(m_iRescanTime);
  }
}

void CPeripheralBus::Initialise(void)
{
  bool bNeedsPolling = false;

  if (!IsRunning())
  {
    CSingleLock lock(m_critSection);
    bNeedsPolling = m_bNeedsPolling;
  }

  if (bNeedsPolling)
  {
    m_triggerEvent.Reset();
    Create();
    SetPriority(-1);
  }
}

void CPeripheralBus::Register(const PeripheralPtr& peripheral)
{
  if (!peripheral)
    return;

  bool bPeripheralAdded = false;

  {
    CSingleLock lock(m_critSection);
    if (!HasPeripheral(peripheral->Location()))
    {
      m_peripherals.push_back(peripheral);
      bPeripheralAdded = true;
    }
  }

  if (bPeripheralAdded)
  {
    CLog::Log(LOGINFO, "%s - new %s device registered on %s->%s: %s (%s:%s)", __FUNCTION__,
              PeripheralTypeTranslator::TypeToString(peripheral->Type()),
              PeripheralTypeTranslator::BusTypeToString(m_type), peripheral->Location().c_str(),
              peripheral->DeviceName().c_str(), peripheral->VendorIdAsString(),
              peripheral->ProductIdAsString());
    m_manager.OnDeviceAdded(*this, *peripheral);
  }
}

void CPeripheralBus::TriggerDeviceScan(void)
{
  bool bNeedsPolling;

  {
    CSingleLock lock(m_critSection);
    bNeedsPolling = m_bNeedsPolling;
  }

  if (bNeedsPolling)
    m_triggerEvent.Set();
  else
    ScanForDevices();
}

bool CPeripheralBus::HasPeripheral(const std::string& strLocation) const
{
  return (GetPeripheral(strLocation) != NULL);
}

void CPeripheralBus::GetDirectory(const std::string& strPath, CFileItemList& items) const
{
  std::string strDevPath;
  CSingleLock lock(m_critSection);
  for (const auto& peripheral : m_peripherals)
  {
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

    std::string strVersion(peripheral->GetVersionInfo());
    if (strVersion.empty())
      strVersion = g_localizeStrings.Get(13205);

    std::string strDetails =
        StringUtils::Format("%s %s", g_localizeStrings.Get(24051).c_str(), strVersion.c_str());
    if (peripheral->GetBusType() == PERIPHERAL_BUS_CEC && !peripheral->GetSettingBool("enabled"))
      strDetails = StringUtils::Format("%s: %s", g_localizeStrings.Get(126).c_str(),
                                       g_localizeStrings.Get(13106).c_str());

    peripheralFile->SetProperty("version", strVersion);
    peripheralFile->SetLabel2(strDetails);
    peripheralFile->SetArt("icon", "DefaultAddon.png");
    items.Add(peripheralFile);
  }
}

PeripheralPtr CPeripheralBus::GetByPath(const std::string& strPath) const
{
  PeripheralPtr result;

  std::string strDevPath;
  CSingleLock lock(m_critSection);
  for (auto& peripheral : m_peripherals)
  {
    if (StringUtils::EqualsNoCase(strPath, peripheral->FileLocation()))
    {
      result = peripheral;
      break;
    }
  }

  return result;
}

unsigned int CPeripheralBus::GetNumberOfPeripherals() const
{
  CSingleLock lock(m_critSection);
  return static_cast<unsigned int>(m_peripherals.size());
}
