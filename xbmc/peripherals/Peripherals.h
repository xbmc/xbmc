#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "bus/PeripheralBus.h"
#include "devices/Peripheral.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"

class CFileItemList;
class CSetting;
class CSettingsCategory;
class TiXmlElement;

namespace PERIPHERALS
{
  #define g_peripherals CPeripherals::Get()

  class CPeripherals
  {
  public:
    static CPeripherals &Get(void);
    virtual ~CPeripherals(void);

    /*!
     * @brief Initialise the peripherals manager.
     */
    virtual void Initialise(void);

    /*!
     * @brief Clear all data known by the peripherals manager.
     */
    virtual void Clear(void);

    /*!
     * @brief Get the instance of the peripheral at the given location.
     * @param strLocation The location.
     * @param busType The bus to query. Default (PERIPHERAL_BUS_UNKNOWN) searches all busses.
     * @return The peripheral or NULL if it wasn't found.
     */
    virtual CPeripheral *GetPeripheralAtLocation(const CStdString &strLocation, PeripheralBusType busType = PERIPHERAL_BUS_UNKNOWN) const;

    /*!
     * @brief Check whether a peripheral is present at the given location.
     * @param strLocation The location.
     * @param busType The bus to query. Default (PERIPHERAL_BUS_UNKNOWN) searches all busses.
     * @return True when a peripheral was found, false otherwise.
     */
    virtual bool HasPeripheralAtLocation(const CStdString &strLocation, PeripheralBusType busType = PERIPHERAL_BUS_UNKNOWN) const;

    /*!
     * @brief Get the bus that holds the device with the given location.
     * @param strLocation The location.
     * @return The bus or NULL if no device was found.
     */
    virtual CPeripheralBus *GetBusWithDevice(const CStdString &strLocation) const;

    /*!
     * @brief Get all peripheral instances that have the given feature.
     * @param results The list of results.
     * @param feature The feature to search for.
     * @param busType The bus to query. Default (PERIPHERAL_BUS_UNKNOWN) searches all busses.
     * @return The number of devices that have been found.
     */
    virtual int GetPeripheralsWithFeature(std::vector<CPeripheral *> &results, const PeripheralFeature feature, PeripheralBusType busType = PERIPHERAL_BUS_UNKNOWN) const;

    size_t GetNumberOfPeripherals() const;

    /*!
     * @brief Check whether there is at least one device present with the given feature.
     * @param feature The feature to check for.
     * @param busType The bus to query. Default (PERIPHERAL_BUS_UNKNOWN) searches all busses.
     * @return True when at least one device was found with this feature, false otherwise.
     */
    virtual bool HasPeripheralWithFeature(const PeripheralFeature feature, PeripheralBusType busType = PERIPHERAL_BUS_UNKNOWN) const;

    /*!
     * @brief Called when a device has been added to a bus.
     * @param bus The bus the device was added to.
     * @param peripheral The peripheral that has been added.
     */
    virtual void OnDeviceAdded(const CPeripheralBus &bus, const CPeripheral &peripheral);

    /*!
     * @brief Called when a device has been deleted from a bus.
     * @param bus The bus from which the device removed.
     * @param peripheral The peripheral that has been removed.
     */
    virtual void OnDeviceDeleted(const CPeripheralBus &bus, const CPeripheral &peripheral);

    /*!
     * @brief Creates a new instance of a peripheral.
     * @param bus The bus on which this peripheral is present.
     * @param type The type of the new peripheral.
     * @param strLocation The location on the bus.
     * @return The new peripheral or NULL if it could not be created.
     */
    CPeripheral *CreatePeripheral(CPeripheralBus &bus, const PeripheralType type, const CStdString &strLocation, int iVendorId = 0, int iProductId = 0);

    /*!
     * @brief Add the settings that are defined in the mappings file to the peripheral (if there is anything defined).
     * @param peripheral The peripheral to get the settings for.
     */
    void GetSettingsFromMapping(CPeripheral &peripheral) const;

    /*!
     * @brief Trigger a device scan on all known busses
     */
    virtual void TriggerDeviceScan(const PeripheralBusType type = PERIPHERAL_BUS_UNKNOWN);

    /*!
     * @brief Get the instance of a bus given it's type.
     * @param type The bus type.
     * @return The bus or NULL if it wasn't found.
     */
    virtual CPeripheralBus *GetBusByType(const PeripheralBusType type) const;

    /*!
     * @brief Get all fileitems for a path.
     * @param strPath The path to the directory to get the items from.
     * @param items The item list.
     */
    virtual void GetDirectory(const CStdString &strPath, CFileItemList &items) const;

    /*!
     * @brief Get the instance of a peripheral given it's path.
     * @param strPath The path to the peripheral.
     * @return The peripheral or NULL if it wasn't found.
     */
    virtual CPeripheral *GetByPath(const CStdString &strPath) const;

  private:
    CPeripherals(void);
    bool LoadMappings(void);
    int GetMappingForDevice(const CPeripheralBus &bus, const PeripheralType classType, int iVendorId, int iProductId) const;
    static void GetSettingsFromMappingsFile(TiXmlElement *xmlNode, std::map<CStdString, CSetting *> &m_settings);

    bool                                 m_bInitialised;
    bool                                 m_bIsStarted;
#if !defined(HAVE_LIBCEC)
    bool                                 m_bMissingLibCecWarningDisplayed;
#endif
    std::vector<CPeripheralBus *>        m_busses;
    std::vector<PeripheralDeviceMapping> m_mappings;
    CSettingsCategory *                  m_settings;
    CCriticalSection                     m_critSection;
  };
}
