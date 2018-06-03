#pragma once
/*
 *      Copyright (C) 2016-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */
 
#include <memory>
#include <string>

#include "peripherals/PeripheralTypes.h"
#include "peripherals/bus/PeripheralBus.h"
#include "platform/android/peripherals/AndroidJoystickState.h"
#include "platform/android/activity/IInputDeviceCallbacks.h"
#include "platform/android/activity/IInputDeviceEventHandler.h"
#include "threads/CriticalSection.h"

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
    bool PerformDeviceScan(PeripheralScanResults &results) override;

  private:
    static PeripheralScanResults GetInputDevices();

    static std::string GetDeviceLocation(int deviceId);
    static bool GetDeviceId(const std::string& deviceLocation, int& deviceId);

    static bool ConvertToPeripheralScanResult(const CJNIViewInputDevice& inputDevice, PeripheralScanResult& peripheralScanResult);

    mutable std::map<int, CAndroidJoystickState> m_joystickStates;
    PeripheralScanResults m_scanResults;
    CCriticalSection m_critSectionStates;
    CCriticalSection m_critSectionResults;
  };
  using PeripheralBusAndroidPtr = std::shared_ptr<CPeripheralBusAndroid>;
}
