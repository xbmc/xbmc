#pragma once
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

#include <memory>
#include <vector>

#include "peripherals/PeripheralTypes.h"
#include "threads/Thread.h"

class CFileItemList;

namespace PERIPHERALS
{
  class CPeripheral;
  class CPeripherals;

  /*!
   * @class CPeripheralBus
   * This represents a bus on the system. By default, this bus instance will scan for changes every 5 seconds.
   * If this bus only has to be updated after a notification sent by the system, set m_bNeedsPolling to false
   * in the constructor, and implement the OnDeviceAdded(), OnDeviceChanged() and OnDeviceRemoved() methods.
   *
   * The PerformDeviceScan() method has to be implemented by each specific bus implementation.
   */
  class CPeripheralBus : protected CThread
  {
  public:
    CPeripheralBus(const std::string &threadname, CPeripherals *manager, PeripheralBusType type);
    virtual ~CPeripheralBus(void) { Clear(); }

    /*!
     * @return The bus type
     */
    const PeripheralBusType Type(void) const { return m_type; }

    /*!
     * @return True if this bus needs to be polled for changes, false if this bus performs updates via callbacks
     */
    bool NeedsPolling(void) const { CSingleLock lock(m_critSection); return m_bNeedsPolling; }

    /*!
     * @brief Get the instance of the peripheral at the given location.
     * @param strLocation The location.
     * @return The peripheral or NULL if it wasn't found.
     */
    virtual CPeripheral *GetPeripheral(const std::string &strLocation) const;

    /*!
     * @brief Check whether a peripheral is present at the given location.
     * @param strLocation The location.
     * @return True when a peripheral was found, false otherwise.
     */
    virtual bool HasPeripheral(const std::string &strLocation) const;

    /*!
     * @brief Get all peripheral instances that have the given feature.
     * @param results The list of results.
     * @param feature The feature to search for.
     * @return The number of devices that have been found.
     */
    virtual int GetPeripheralsWithFeature(std::vector<CPeripheral *> &results, const PeripheralFeature feature) const;

    virtual size_t GetNumberOfPeripherals() const;
    virtual size_t GetNumberOfPeripheralsWithId(const int iVendorId, const int iProductId) const;

    /*!
     * @brief Get all features that are supported by devices on this bus.
     * @param features All features.
     */
    virtual void GetFeatures(std::vector<PeripheralFeature> &features) const;

    /*!
     * @brief Check whether there is at least one device present with the given feature.
     * @param feature The feature to check for.
     * @return True when at least one device was found with this feature, false otherwise.
     */
    virtual bool HasFeature(const PeripheralFeature feature) const;

    /*!
     * @brief Callback method for when a device has been added. Will perform a device scan.
     * @param strLocation The location of the device that has been added.
     */
    virtual void OnDeviceAdded(const std::string &strLocation);

    /*!
     * @brief Callback method for when a device has been changed. Will perform a device scan.
     * @param strLocation The location of the device that has been changed.
     */
    virtual void OnDeviceChanged(const std::string &strLocation);

    /*!
     * @brief Callback method for when a device has been removed. Will perform a device scan.
     * @param strLocation The location of the device that has been removed.
     */
    virtual void OnDeviceRemoved(const std::string &strLocation);

    /*!
     * @brief Initialise this bus and start a polling thread if this bus needs polling.
     */
    virtual void Initialise(void);

    /*!
     * @brief Stop the polling thread and clear all known devices on this bus.
     */
    virtual void Clear(void);

    /*!
     * @brief Scan for devices.
     */
    virtual void TriggerDeviceScan(void);

    /*!
     * @brief Get all fileitems for a path.
     * @param strPath The path to the directory to get the items from.
     * @param items The item list.
     */
    virtual void GetDirectory(const std::string &strPath, CFileItemList &items) const;

    /*!
     * @brief Get the instance of a peripheral given it's path.
     * @param strPath The path to the peripheral.
     * @return The peripheral or NULL if it wasn't found.
     */
    virtual CPeripheral *GetByPath(const std::string &strPath) const;

    /*!
     * @brief Register a new peripheral on this bus.
     * @param peripheral The peripheral to register.
     */
    virtual void Register(CPeripheral *peripheral);

    virtual bool FindComPort(std::string &strLocation) { return false; }

    virtual bool IsInitialised(void) const { CSingleLock lock(m_critSection); return m_bInitialised; }

    /*!
     * \brief Poll for events
     */
    virtual void ProcessEvents(void) { }

  protected:
    virtual void Process(void);
    virtual bool ScanForDevices(void);
    virtual void UnregisterRemovedDevices(const PeripheralScanResults &results);
    virtual void RegisterNewDevices(const PeripheralScanResults &results);

    /*!
     * @brief Scan for devices on this bus and add them to the results list. This will have to be implemented for each bus.
     * @param results The result list.
     * @return True when the scan was successful, false otherwise.
     */
    virtual bool PerformDeviceScan(PeripheralScanResults &results) = 0;

    std::vector<CPeripheral *> m_peripherals;
    int                        m_iRescanTime;
    bool                       m_bInitialised;
    bool                       m_bIsStarted;
    bool                       m_bNeedsPolling; /*!< true when this bus needs to be polled for new devices, false when it uses callbacks to notify this bus of changed */
    CPeripherals *const        m_manager;
    const PeripheralBusType    m_type;
    CCriticalSection           m_critSection;
    CEvent                     m_triggerEvent;
  };
  using PeripheralBusPtr = std::shared_ptr<CPeripheralBus>;
}
