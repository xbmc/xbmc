/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "peripherals/PeripheralTypes.h"
#include "peripherals/bus/PeripheralBus.h"
#include "threads/CriticalSection.h"

#include "platform/android/activity/IInputDeviceCallbacks.h"
#include "platform/android/activity/IInputDeviceEventHandler.h"
#include "platform/android/peripherals/AndroidJoystickState.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

class CJNIViewInputDevice;

namespace PERIPHERALS
{
class CPeripheralBusAndroid : public CPeripheralBus,
                              public IInputDeviceCallbacks,
                              public IInputDeviceEventHandler
{
public:
  explicit CPeripheralBusAndroid(CPeripherals& manager);
  ~CPeripheralBusAndroid() override;

  // specialisation of CPeripheralBus
  bool InitializeProperties(CPeripheral& peripheral) override;
  void Initialise(void) override;
  void ProcessEvents() override;

  // implementations of IInputDeviceCallbacks
  void OnInputDeviceAdded(int deviceId) override;
  void OnInputDeviceChanged(int deviceId) override;
  void OnInputDeviceRemoved(int deviceId) override;

  // implementation of IInputDeviceEventHandler
  bool OnInputDeviceEvent(const AInputEvent* event) override;

protected:
  // implementation of CPeripheralBus
  bool PerformDeviceScan(PeripheralScanResults& results) override;

private:
  static PeripheralScanResults GetInputDevices();

  static std::string GetDeviceLocation(int deviceId);
  static bool GetDeviceId(const std::string& deviceLocation, int& deviceId);

  static bool ConvertToPeripheralScanResult(const CJNIViewInputDevice& inputDevice,
                                            PeripheralScanResult& peripheralScanResult);

  static void LogInputDevice(const CJNIViewInputDevice& device);
  static std::vector<std::pair<int, const char*>> GetInputSources();

  mutable std::map<int, CAndroidJoystickState> m_joystickStates;
  PeripheralScanResults m_scanResults;
  CCriticalSection m_critSectionStates;
  CCriticalSection m_critSectionResults;
};
using PeripheralBusAndroidPtr = std::shared_ptr<CPeripheralBusAndroid>;
} // namespace PERIPHERALS
