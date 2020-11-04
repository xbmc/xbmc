/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/addon-instance/peripheral/PeripheralUtils.h"
#include "peripherals/PeripheralTypes.h"
#include "peripherals/bus/PeripheralBus.h"
#include "threads/CriticalSection.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

struct PeripheralBusGCControllerWrapper;

namespace PERIPHERALS
{
class CPeripheralBusGCController : public CPeripheralBus
{
public:
  explicit CPeripheralBusGCController(CPeripherals& manager);
  ~CPeripheralBusGCController() override;

  // specialisation of CPeripheralBus
  bool InitializeProperties(CPeripheral& peripheral) override;
  void Initialise(void) override;
  void ProcessEvents() override;

  bool PerformDeviceScan(PeripheralScanResults& results) override;
  PeripheralScanResults GetInputDevices();

  void callOnDeviceAdded(const std::string& strLocation);
  void callOnDeviceRemoved(const std::string& strLocation);

  void SetScanResults(const PeripheralScanResults& resScanResults);

  const std::string& getDeviceLocationPrefix()
  {
    // Initialize the static variable
    static std::string DeviceLocationPrefix("darwin/inputdevice/");
    return DeviceLocationPrefix;
  }


private:
  void GetEvents(std::vector<kodi::addon::PeripheralEvent>& events);
  std::unique_ptr<PeripheralBusGCControllerWrapper> m_peripheralGCController;
  std::string GetDeviceLocation(int deviceId);
  bool GetDeviceId(const std::string& deviceLocation, int& deviceId);

  PeripheralScanResults m_scanResults;
  CCriticalSection m_critSectionStates;
  CCriticalSection m_critSectionResults;
};
} // namespace PERIPHERALS
