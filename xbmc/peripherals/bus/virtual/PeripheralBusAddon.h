/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "peripherals/PeripheralTypes.h"
#include "peripherals/bus/PeripheralBus.h"

#include <memory>
#include <string>
#include <vector>

namespace ADDON
{
struct AddonEvent;
class CAddonInfo;
} // namespace ADDON

namespace PERIPHERALS
{
/*!
 * \ingroup peripherals
 */
class CPeripheralBusAddon : public CPeripheralBus
{
public:
  explicit CPeripheralBusAddon(CPeripherals& manager);
  ~CPeripheralBusAddon(void) override;

  void UpdateAddons(void);

  /*!
   * \brief Get peripheral add-on that can provide button maps
   */
  bool GetAddonWithButtonMap(PeripheralAddonPtr& addon) const;

  /*!
   * \brief Get peripheral add-on that can provide button maps for the given device
   */
  bool GetAddonWithButtonMap(const CPeripheral* device, PeripheralAddonPtr& addon) const;

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
  bool InitializeProperties(CPeripheral& peripheral) override;
  void Register(const PeripheralPtr& peripheral) override;
  void GetFeatures(std::vector<PeripheralFeature>& features) const override;
  bool HasFeature(const PeripheralFeature feature) const override;
  PeripheralPtr GetPeripheral(const std::string& strLocation) const override;
  PeripheralPtr GetByPath(const std::string& strPath) const override;
  bool SupportsFeature(PeripheralFeature feature) const override;
  unsigned int GetPeripheralsWithFeature(PeripheralVector& results,
                                         const PeripheralFeature feature) const override;
  unsigned int GetNumberOfPeripherals(void) const override;
  unsigned int GetNumberOfPeripheralsWithId(const int iVendorId,
                                            const int iProductId) const override;
  void GetDirectory(const std::string& strPath, CFileItemList& items) const override;
  void ProcessEvents(void) override;
  void EnableButtonMapping() override;
  void PowerOff(const std::string& strLocation) override;

  bool SplitLocation(const std::string& strLocation,
                     PeripheralAddonPtr& addon,
                     unsigned int& peripheralIndex) const;

protected:
  // Inherited from CPeripheralBus
  bool PerformDeviceScan(PeripheralScanResults& results) override;
  void UnregisterRemovedDevices(const PeripheralScanResults& results) override;

private:
  void OnEvent(const ADDON::AddonEvent& event);
  void UnRegisterAddon(const std::string& addonId);

  void PromptEnableAddons(const std::vector<std::shared_ptr<ADDON::CAddonInfo>>& disabledAddons);

  PeripheralAddonVector m_addons;
  PeripheralAddonVector m_failedAddons;
};
using PeripheralBusAddonPtr = std::shared_ptr<CPeripheralBusAddon>;
} // namespace PERIPHERALS
