/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PeripheralBusGCController.h"

#include "ServiceBroker.h"
#include "addons/kodi-dev-kit/include/kodi/addon-instance/peripheral/PeripheralUtils.h"
#include "peripherals/bus/PeripheralBus.h"
#include "peripherals/devices/PeripheralJoystick.h"
#include "threads/CriticalSection.h"
#include "utils/log.h"

#include "platform/darwin/peripherals/InputKey.h"
#import "platform/darwin/peripherals/PeripheralBusGCControllerManager.h"

#include <mutex>

#define JOYSTICK_PROVIDER_DARWIN_GCController "GCController"

struct PeripheralBusGCControllerWrapper
{
  CBPeripheralBusGCControllerManager* callbackClass;
};

PERIPHERALS::CPeripheralBusGCController::CPeripheralBusGCController(CPeripherals& manager)
  : CPeripheralBus("PeripBusGCController", manager, PERIPHERAL_BUS_GCCONTROLLER)
{
  m_peripheralGCController = std::make_unique<PeripheralBusGCControllerWrapper>();
  m_peripheralGCController->callbackClass =
      [[CBPeripheralBusGCControllerManager alloc] initWithName:this];
  m_bNeedsPolling = false;

  // get all currently connected input devices
  m_scanResults = GetInputDevices();
}

PERIPHERALS::CPeripheralBusGCController::~CPeripheralBusGCController()
{
}

bool PERIPHERALS::CPeripheralBusGCController::InitializeProperties(CPeripheral& peripheral)
{
  // Returns true regardless, why is it necessary?
  if (!CPeripheralBus::InitializeProperties(peripheral))
    return false;

  if (peripheral.Type() != PERIPHERALS::PERIPHERAL_JOYSTICK)
  {
    CLog::Log(LOGWARNING, "CPeripheralBusGCController: invalid peripheral type: {}",
              PERIPHERALS::PeripheralTypeTranslator::TypeToString(peripheral.Type()));
    return false;
  }

  // deviceId will be our playerIndex
  int deviceId;
  if (!GetDeviceId(peripheral.Location(), deviceId))
  {
    CLog::Log(LOGWARNING,
              "CPeripheralBusGCController: failed to initialize properties for peripheral \"{}\"",
              peripheral.Location());
    return false;
  }

  CLog::Log(LOGDEBUG, "CPeripheralBusGCController: Initializing device \"{}\"",
            peripheral.DeviceName());

  auto& joystick = static_cast<CPeripheralJoystick&>(peripheral);

  joystick.SetRequestedPort(deviceId);
  joystick.SetProvider(JOYSTICK_PROVIDER_DARWIN_GCController);

  auto controllerType = [m_peripheralGCController->callbackClass GetControllerType:deviceId];

  switch (controllerType)
  {
    case GCCONTROLLER_TYPE::EXTENDED:
      // Extended Gamepad
      joystick.SetButtonCount([m_peripheralGCController->callbackClass
          GetControllerButtonCount:deviceId
                withControllerType:controllerType]);
      joystick.SetAxisCount([m_peripheralGCController->callbackClass
          GetControllerAxisCount:deviceId
              withControllerType:controllerType]);
      break;
    case GCCONTROLLER_TYPE::MICRO:
      // Micro Gamepad
      joystick.SetButtonCount([m_peripheralGCController->callbackClass
          GetControllerButtonCount:deviceId
                withControllerType:controllerType]);
      joystick.SetAxisCount([m_peripheralGCController->callbackClass
          GetControllerAxisCount:deviceId
              withControllerType:controllerType]);
      break;
    default:
      CLog::Log(LOGDEBUG, "CPeripheralBusGCController: Unknown Controller Type");
      return false;
  }

  CLog::Log(LOGDEBUG, "CPeripheralBusGCController: Device has {} buttons and {} axes",
            joystick.ButtonCount(), joystick.AxisCount());

  return true;
}

void PERIPHERALS::CPeripheralBusGCController::Initialise(void)
{
  CPeripheralBus::Initialise();
  TriggerDeviceScan();
}

bool PERIPHERALS::CPeripheralBusGCController::PerformDeviceScan(PeripheralScanResults& results)
{
  std::unique_lock<CCriticalSection> lock(m_critSectionResults);
  results = m_scanResults;

  return true;
}

void PERIPHERALS::CPeripheralBusGCController::SetScanResults(
    const PERIPHERALS::PeripheralScanResults& resScanResults)
{
  std::unique_lock<CCriticalSection> lock(m_critSectionResults);
  m_scanResults = resScanResults;
}

void PERIPHERALS::CPeripheralBusGCController::GetEvents(
    std::vector<kodi::addon::PeripheralEvent>& events)
{
  std::unique_lock<CCriticalSection> lock(m_critSectionStates);
  std::vector<kodi::addon::PeripheralEvent> digitalEvents;
  digitalEvents = [m_peripheralGCController->callbackClass GetButtonEvents];

  std::vector<kodi::addon::PeripheralEvent> axisEvents;
  axisEvents = [m_peripheralGCController->callbackClass GetAxisEvents];

  events.reserve(digitalEvents.size() + axisEvents.size()); // preallocate memory
  events.insert(events.end(), digitalEvents.begin(), digitalEvents.end());
  events.insert(events.end(), axisEvents.begin(), axisEvents.end());
}

bool PERIPHERALS::CPeripheralBusGCController::GetDeviceId(const std::string& deviceLocation,
                                                          int& deviceId)
{
  if (deviceLocation.empty() ||
      !StringUtils::StartsWith(deviceLocation, getDeviceLocationPrefix()) ||
      deviceLocation.size() <= getDeviceLocationPrefix().size())
    return false;

  std::string strDeviceId = deviceLocation.substr(getDeviceLocationPrefix().size());
  if (!StringUtils::IsNaturalNumber(strDeviceId))
    return false;

  deviceId = static_cast<int>(strtol(strDeviceId.c_str(), nullptr, 10));
  return true;
}

void PERIPHERALS::CPeripheralBusGCController::ProcessEvents()
{
  std::vector<kodi::addon::PeripheralEvent> events;
  {
    std::unique_lock<CCriticalSection> lock(m_critSectionStates);

    //! @todo Multiple controller event processing
    GetEvents(events);
  }

  for (const auto& event : events)
  {
    PeripheralPtr device = GetPeripheral(GetDeviceLocation(event.PeripheralIndex()));
    if (!device || device->Type() != PERIPHERAL_JOYSTICK)
      continue;

    auto joystick = static_cast<CPeripheralJoystick*>(device.get());
    switch (event.Type())
    {
      case PERIPHERAL_EVENT_TYPE_DRIVER_BUTTON:
      {
        const bool bPressed = (event.ButtonState() == JOYSTICK_STATE_BUTTON_PRESSED);
        joystick->OnButtonMotion(event.DriverIndex(), bPressed);
        break;
      }
      case PERIPHERAL_EVENT_TYPE_DRIVER_AXIS:
      {
        joystick->OnAxisMotion(event.DriverIndex(), event.AxisState());
        break;
      }
      default:
        break;
    }
  }
  {
    std::unique_lock<CCriticalSection> lock(m_critSectionStates);
    //! @todo Multiple controller handling
    PeripheralPtr device = GetPeripheral(GetDeviceLocation(0));

    if (device && device->Type() == PERIPHERAL_JOYSTICK)
      static_cast<CPeripheralJoystick*>(device.get())->OnInputFrame();
  }
}

std::string PERIPHERALS::CPeripheralBusGCController::GetDeviceLocation(int deviceId)
{
  return [m_peripheralGCController->callbackClass GetDeviceLocation:deviceId];
}

PERIPHERALS::PeripheralScanResults PERIPHERALS::CPeripheralBusGCController::GetInputDevices()
{
  CLog::Log(LOGINFO, "CPeripheralBusGCController: scanning for input devices...");

  return [m_peripheralGCController->callbackClass GetInputDevices];
}

void PERIPHERALS::CPeripheralBusGCController::callOnDeviceAdded(const std::string& strLocation)
{
  OnDeviceAdded(strLocation);
}

void PERIPHERALS::CPeripheralBusGCController::callOnDeviceRemoved(const std::string& strLocation)
{
  OnDeviceRemoved(strLocation);
}
