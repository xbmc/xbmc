/*
 *      Copyright (C) 2014-2017 Team Kodi
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
#pragma once

#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "addons/binary-addons/BinaryAddonBase.h"
#include "guilib/IWindowManagerCallback.h"
#include "peripherals/PeripheralTypes.h"
#include "peripherals/bus/PeripheralBus.h"
#include "threads/CriticalSection.h"
#include "utils/Observer.h"

#include <memory>
#include <string>
#include <vector>

namespace PERIPHERALS
{
  class CPeripheralBusAddon : public CPeripheralBus
  {
  public:
    CPeripheralBusAddon(CPeripherals& manager);
    virtual ~CPeripheralBusAddon(void);

    void UpdateAddons(void);

    /*!
    * \brief Get peripheral add-on that can provide button maps
    */
    bool GetAddonWithButtonMap(PeripheralAddonPtr &addon) const;

    /*!
     * \brief Get peripheral add-on that can provide button maps for the given device
     */
    bool GetAddonWithButtonMap(const CPeripheral* device, PeripheralAddonPtr &addon) const;

    /*!
     * \brief Set the rumble state of a rumble motor
     *
     * \param strLocation The location of the peripheral with the motor
     * \param motorIndex  The index of the motor being rumbled
     * \param magnitude   The amount of vibration in the closed interval [0.0, 1.0]
     *
     * \return true if the rumble motor's state is set, false otherwise
     *
     * TODO: Move declaration to parent class
     */
    bool SendRumbleEvent(const std::string& strLocation, unsigned int motorIndex, float magnitude);

    // Inherited from CPeripheralBus
    virtual bool         InitializeProperties(CPeripheral& peripheral) override;
    virtual void         Register(const PeripheralPtr& peripheral) override;
    virtual void         GetFeatures(std::vector<PeripheralFeature> &features) const override;
    virtual bool         HasFeature(const PeripheralFeature feature) const override;
    virtual PeripheralPtr GetPeripheral(const std::string &strLocation) const override;
    virtual PeripheralPtr GetByPath(const std::string &strPath) const override;
    virtual bool         SupportsFeature(PeripheralFeature feature) const override;
    virtual int          GetPeripheralsWithFeature(PeripheralVector &results, const PeripheralFeature feature) const override;
    virtual size_t       GetNumberOfPeripherals(void) const override;
    virtual size_t       GetNumberOfPeripheralsWithId(const int iVendorId, const int iProductId) const override;
    virtual void         GetDirectory(const std::string &strPath, CFileItemList &items) const override;
    virtual void         ProcessEvents(void) override;
    virtual void         EnableButtonMapping() override;
    virtual void         PowerOff(const std::string& strLocation) override;

    bool SplitLocation(const std::string& strLocation, PeripheralAddonPtr& addon, unsigned int& peripheralIndex) const;

  protected:
    // Inherited from CPeripheralBus
    virtual bool PerformDeviceScan(PeripheralScanResults &results) override;
    virtual void UnregisterRemovedDevices(const PeripheralScanResults &results) override;

  private:
    void OnEvent(const ADDON::AddonEvent& event);

    void PromptEnableAddons(const ADDON::BinaryAddonBaseList& disabledAddons);

    PeripheralAddonVector m_addons;
    PeripheralAddonVector m_failedAddons;
    CCriticalSection      m_critSection;
  };
  using PeripheralBusAddonPtr = std::shared_ptr<CPeripheralBusAddon>;
}
