/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PeripheralBusDarwinEmbedded.h"

#include "ServiceBroker.h"
#include "addons/kodi-dev-kit/include/kodi/addon-instance/peripheral/PeripheralUtils.h"
#include "peripherals/bus/PeripheralBus.h"
#include "peripherals/devices/PeripheralJoystick.h"
#include "threads/CriticalSection.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include "platform/darwin/ios-common/peripherals/InputKey.h"
#import "platform/darwin/ios-common/peripherals/PeripheralBusDarwinEmbeddedManager.h"

#define JOYSTICK_PROVIDER_DARWINEMBEDDED "darwinembedded"

struct PeripheralBusDarwinEmbeddedWrapper
{
  CBPeripheralBusDarwinEmbeddedManager* callbackClass;
};

PERIPHERALS::CPeripheralBusDarwinEmbedded::CPeripheralBusDarwinEmbedded(CPeripherals& manager)
  : CPeripheralBus("PeripBusDarwinEmbedded", manager, PERIPHERAL_BUS_DARWINEMBEDDED)
{
  m_peripheralDarwinEmbedded = std::make_unique<PeripheralBusDarwinEmbeddedWrapper>();
  m_peripheralDarwinEmbedded->callbackClass =
      [[CBPeripheralBusDarwinEmbeddedManager alloc] initWithName:this];
  m_bNeedsPolling = false;

  // get all currently connected input devices
  m_scanResults = GetInputDevices();
}

PERIPHERALS::CPeripheralBusDarwinEmbedded::~CPeripheralBusDarwinEmbedded()
{
}

bool PERIPHERALS::CPeripheralBusDarwinEmbedded::InitializeProperties(CPeripheral& peripheral)
{
  // Returns true regardless, why is it necessary?
  if (!CPeripheralBus::InitializeProperties(peripheral))
    return false;

  if (peripheral.Type() != PERIPHERALS::PERIPHERAL_JOYSTICK)
  {
    CLog::Log(LOGWARNING, "CPeripheralBusDarwinEmbedded: invalid peripheral type: %s",
              PERIPHERALS::PeripheralTypeTranslator::TypeToString(peripheral.Type()));
    return false;
  }

  // deviceId will be our playerIndex
  int deviceId;
  if (!GetDeviceId(peripheral.Location(), deviceId))
  {
    CLog::Log(LOGWARNING,
              "CPeripheralBusDarwinEmbedded: failed to initialize properties for peripheral \"%s\"",
              peripheral.Location().c_str());
    return false;
  }

  CLog::Log(LOGDEBUG, "CPeripheralBusDarwinEmbedded: Initializing device \"{}\"",
            peripheral.DeviceName());

  auto& joystick = static_cast<CPeripheralJoystick&>(peripheral);

  joystick.SetRequestedPort(deviceId);
  joystick.SetProvider(JOYSTICK_PROVIDER_DARWINEMBEDDED);

  auto controllerType = [m_peripheralDarwinEmbedded->callbackClass GetControllerType:deviceId];

  switch (controllerType)
  {
    case GCCONTROLLER_TYPE::EXTENDED:
      // Extended Gamepad
      joystick.SetButtonCount([m_peripheralDarwinEmbedded->callbackClass
          GetControllerButtonCount:deviceId
                withControllerType:controllerType]);
      joystick.SetAxisCount([m_peripheralDarwinEmbedded->callbackClass
          GetControllerAxisCount:deviceId
              withControllerType:controllerType]);
      break;
    case GCCONTROLLER_TYPE::MICRO:
      // Micro Gamepad
      joystick.SetButtonCount([m_peripheralDarwinEmbedded->callbackClass
          GetControllerButtonCount:deviceId
                withControllerType:controllerType]);
      joystick.SetAxisCount([m_peripheralDarwinEmbedded->callbackClass
          GetControllerAxisCount:deviceId
              withControllerType:controllerType]);
      break;
    default:
      CLog::Log(LOGDEBUG, "CPeripheralBusDarwinEmbedded: Unknown Controller Type");
      return false;
  }

  CLog::Log(LOGDEBUG, "CPeripheralBusDarwinEmbedded: Device has %u buttons and %u axes",
            joystick.ButtonCount(), joystick.AxisCount());

  return true;
}

void PERIPHERALS::CPeripheralBusDarwinEmbedded::Initialise(void)
{
  CPeripheralBus::Initialise();
  TriggerDeviceScan();
}

bool PERIPHERALS::CPeripheralBusDarwinEmbedded::PerformDeviceScan(PeripheralScanResults& results)
{
  CSingleLock lock(m_critSectionResults);
  results = m_scanResults;

  return true;
}

void PERIPHERALS::CPeripheralBusDarwinEmbedded::SetScanResults(
    const PERIPHERALS::PeripheralScanResults resScanResults)
{
  CSingleLock lock(m_critSectionResults);
  m_scanResults = resScanResults;
}

void PERIPHERALS::CPeripheralBusDarwinEmbedded::GetEvents(
    std::vector<kodi::addon::PeripheralEvent>& events)
{
  CSingleLock lock(m_critSectionStates);
  std::vector<kodi::addon::PeripheralEvent> digitalEvents;
  digitalEvents = [m_peripheralDarwinEmbedded->callbackClass GetButtonEvents];

  std::vector<kodi::addon::PeripheralEvent> axisEvents;
  axisEvents = [m_peripheralDarwinEmbedded->callbackClass GetAxisEvents];

  events.reserve(digitalEvents.size() + axisEvents.size()); // preallocate memory
  events.insert(events.end(), digitalEvents.begin(), digitalEvents.end());
  events.insert(events.end(), axisEvents.begin(), axisEvents.end());
}

bool PERIPHERALS::CPeripheralBusDarwinEmbedded::GetDeviceId(const std::string& deviceLocation,
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

void PERIPHERALS::CPeripheralBusDarwinEmbedded::ProcessEvents()
{
  std::vector<kodi::addon::PeripheralEvent> events;
  {
    CSingleLock lock(m_critSectionStates);

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
    CSingleLock lock(m_critSectionStates);
    //! @todo Multiple controller handling
    PeripheralPtr device = GetPeripheral(GetDeviceLocation(0));

    if (device && device->Type() == PERIPHERAL_JOYSTICK)
      static_cast<CPeripheralJoystick*>(device.get())->ProcessAxisMotions();
  }
}

std::string PERIPHERALS::CPeripheralBusDarwinEmbedded::GetDeviceLocation(int deviceId)
{
  return [m_peripheralDarwinEmbedded->callbackClass GetDeviceLocation:deviceId];
}

PERIPHERALS::PeripheralScanResults PERIPHERALS::CPeripheralBusDarwinEmbedded::GetInputDevices()
{
  CLog::Log(LOGINFO, "CPeripheralBusDarwinEmbedded: scanning for input devices...");

  return [m_peripheralDarwinEmbedded->callbackClass GetInputDevices];
}

void PERIPHERALS::CPeripheralBusDarwinEmbedded::callOnDeviceAdded(const std::string strLocation)
{
  OnDeviceAdded(strLocation);
}

void PERIPHERALS::CPeripheralBusDarwinEmbedded::callOnDeviceRemoved(const std::string strLocation)
{
  OnDeviceRemoved(strLocation);
}
