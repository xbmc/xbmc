/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PeripheralBus.h"
#include "peripherals/Peripherals.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "FileItem.h"

using namespace std;
using namespace PERIPHERALS;

#define PERIPHERAL_DEFAULT_RESCAN_INTERVAL 5000

CPeripheralBus::CPeripheralBus(const std::string &threadname, CPeripherals *manager, PeripheralBusType type) :
    CThread(threadname.c_str()),
    m_iRescanTime(PERIPHERAL_DEFAULT_RESCAN_INTERVAL),
    m_bInitialised(false),
    m_bIsStarted(false),
    m_bNeedsPolling(true),
    m_manager(manager),
    m_type(type),
    m_triggerEvent(true)
{
}

void CPeripheralBus::OnDeviceAdded(const std::string &strLocation)
{
  ScanForDevices();
}

void CPeripheralBus::OnDeviceChanged(const std::string &strLocation)
{
  ScanForDevices();
}

void CPeripheralBus::OnDeviceRemoved(const std::string &strLocation)
{
  ScanForDevices();
}

void CPeripheralBus::Clear(void)
{
  if (m_bNeedsPolling)
  {
    m_bStop = true;
    m_triggerEvent.Set();
    StopThread(true);
  }

  CSingleLock lock(m_critSection);
  for (unsigned int iPeripheralPtr = 0; iPeripheralPtr < m_peripherals.size(); iPeripheralPtr++)
    delete m_peripherals.at(iPeripheralPtr);
  m_peripherals.clear();
}

void CPeripheralBus::UnregisterRemovedDevices(const PeripheralScanResults &results)
{
  CSingleLock lock(m_critSection);
  vector<CPeripheral *> removedPeripherals;
  for (int iDevicePtr = (int) m_peripherals.size() - 1; iDevicePtr >= 0; iDevicePtr--)
  {
    CPeripheral *peripheral = m_peripherals.at(iDevicePtr);
    PeripheralScanResult updatedDevice(m_type);
    if (!results.GetDeviceOnLocation(peripheral->Location(), &updatedDevice) ||
        *peripheral != updatedDevice)
    {
      /* device removed */
      removedPeripherals.push_back(peripheral);
      m_peripherals.erase(m_peripherals.begin() + iDevicePtr);
    }
  }
  lock.Leave();

  for (unsigned int iDevicePtr = 0; iDevicePtr < removedPeripherals.size(); iDevicePtr++)
  {
    CPeripheral *peripheral = removedPeripherals.at(iDevicePtr);
    vector<PeripheralFeature> features;
    peripheral->GetFeatures(features);
    bool peripheralHasFeatures = features.size() > 1 || (features.size() == 1 && features.at(0) != FEATURE_UNKNOWN);
    if (peripheral->Type() != PERIPHERAL_UNKNOWN || peripheralHasFeatures)
    {
      CLog::Log(LOGNOTICE, "%s - device removed from %s/%s: %s (%s:%s)", __FUNCTION__, PeripheralTypeTranslator::TypeToString(peripheral->Type()), peripheral->Location().c_str(), peripheral->DeviceName().c_str(), peripheral->VendorIdAsString(), peripheral->ProductIdAsString());
      peripheral->OnDeviceRemoved();
    }

    m_manager->OnDeviceDeleted(*this, *peripheral);
    delete peripheral;
  }
}

void CPeripheralBus::RegisterNewDevices(const PeripheralScanResults &results)
{
  CSingleLock lock(m_critSection);
  for (unsigned int iResultPtr = 0; iResultPtr < results.m_results.size(); iResultPtr++)
  {
    const PeripheralScanResult& result = results.m_results.at(iResultPtr);
    if (!HasPeripheral(result.m_strLocation))
      g_peripherals.CreatePeripheral(*this, result);
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

    CPeripherals::Get().NotifyObservers(ObservableMessagePeripheralsChanged);

    bReturn = true;
  }

  m_bInitialised = true;
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

void CPeripheralBus::GetFeatures(std::vector<PeripheralFeature> &features) const
{
  CSingleLock lock(m_critSection);
  for (unsigned int iPeripheralPtr = 0; iPeripheralPtr < m_peripherals.size(); iPeripheralPtr++)
    m_peripherals.at(iPeripheralPtr)->GetFeatures(features);
}

CPeripheral *CPeripheralBus::GetPeripheral(const std::string &strLocation) const
{
  CPeripheral *peripheral(NULL);
  CSingleLock lock(m_critSection);
  for (unsigned int iPeripheralPtr = 0; iPeripheralPtr < m_peripherals.size(); iPeripheralPtr++)
  {
    if (m_peripherals.at(iPeripheralPtr)->Location() == strLocation)
    {
      peripheral = m_peripherals.at(iPeripheralPtr);
      break;
    }
  }
  return peripheral;
}

int CPeripheralBus::GetPeripheralsWithFeature(vector<CPeripheral *> &results, const PeripheralFeature feature) const
{
  int iReturn(0);
  CSingleLock lock(m_critSection);
  for (unsigned int iPeripheralPtr = 0; iPeripheralPtr < m_peripherals.size(); iPeripheralPtr++)
  {
    if (m_peripherals.at(iPeripheralPtr)->HasFeature(feature))
    {
      results.push_back(m_peripherals.at(iPeripheralPtr));
      ++iReturn;
    }
  }

  return iReturn;
}

size_t CPeripheralBus::GetNumberOfPeripheralsWithId(const int iVendorId, const int iProductId) const
{
  int iReturn(0);
  CSingleLock lock(m_critSection);
  for (unsigned int iPeripheralPtr = 0; iPeripheralPtr < m_peripherals.size(); iPeripheralPtr++)
  {
    if (m_peripherals.at(iPeripheralPtr)->VendorId() == iVendorId &&
        m_peripherals.at(iPeripheralPtr)->ProductId() == iProductId)
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

    if (!m_bStop)
      m_triggerEvent.WaitMSec(m_iRescanTime);
  }

  m_bIsStarted = false;
}

bool CPeripheralBus::Initialise(void)
{
  CSingleLock lock(m_critSection);
  if (!m_bIsStarted)
  {
    /* do an initial scan of the bus */
    m_bIsStarted = ScanForDevices();

    if (m_bIsStarted && m_bNeedsPolling)
    {
      lock.Leave();
      m_triggerEvent.Reset();
      Create();
      SetPriority(-1);
    }
  }

  return m_bIsStarted;
}

void CPeripheralBus::Register(CPeripheral *peripheral)
{
  if (!peripheral)
    return;

  CSingleLock lock(m_critSection);
  if (!HasPeripheral(peripheral->Location()))
  {
    m_peripherals.push_back(peripheral);
    CLog::Log(LOGNOTICE, "%s - new %s device registered on %s->%s: %s (%s:%s)", __FUNCTION__, PeripheralTypeTranslator::TypeToString(peripheral->Type()), PeripheralTypeTranslator::BusTypeToString(m_type), peripheral->Location().c_str(), peripheral->DeviceName().c_str(), peripheral->VendorIdAsString(), peripheral->ProductIdAsString());
    lock.Leave();

    m_manager->OnDeviceAdded(*this, *peripheral);
  }
}

void CPeripheralBus::TriggerDeviceScan(void)
{
  CSingleLock lock(m_critSection);
  if (m_bNeedsPolling)
  {
    lock.Leave();
    m_triggerEvent.Set();
  }
  else
  {
    lock.Leave();
    ScanForDevices();
  }
}

bool CPeripheralBus::HasPeripheral(const std::string &strLocation) const
{
  return (GetPeripheral(strLocation) != NULL);
}

void CPeripheralBus::GetDirectory(const std::string &strPath, CFileItemList &items) const
{
  std::string strDevPath;
  CSingleLock lock(m_critSection);
  for (unsigned int iDevicePtr = 0; iDevicePtr < m_peripherals.size(); iDevicePtr++)
  {
    const CPeripheral *peripheral = m_peripherals.at(iDevicePtr);
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
    items.Add(peripheralFile);
  }
}

CPeripheral *CPeripheralBus::GetByPath(const std::string &strPath) const
{
  std::string strDevPath;
  CSingleLock lock(m_critSection);
  for (unsigned int iDevicePtr = 0; iDevicePtr < m_peripherals.size(); iDevicePtr++)
  {
    if (StringUtils::EqualsNoCase(strPath, m_peripherals.at(iDevicePtr)->FileLocation()))
      return m_peripherals.at(iDevicePtr);
  }

  return NULL;
}

size_t CPeripheralBus::GetNumberOfPeripherals() const
{
  return m_peripherals.size();
}
