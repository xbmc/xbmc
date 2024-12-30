/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "PeripheralBusGCControllerManager.h"

#include "peripherals/PeripheralTypes.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include "platform/darwin/peripherals/InputKey.h"
#import "platform/darwin/peripherals/Input_Gamecontroller.h"
#include "platform/darwin/peripherals/PeripheralBusGCController.h"

#include <mutex>

#pragma mark - objc implementation

@implementation CBPeripheralBusGCControllerManager
{
  PERIPHERALS::CPeripheralBusGCController* parentClass;
  std::vector<kodi::addon::PeripheralEvent> m_digitalEvents;
  std::vector<kodi::addon::PeripheralEvent> m_axisEvents;
  CCriticalSection m_eventMutex;
}

#pragma mark - callbackClass inputdevices

- (PERIPHERALS::PeripheralScanResults)GetInputDevices
{
  PERIPHERALS::PeripheralScanResults scanresults = {};

  scanresults = [self.input_GC GetGCDevices];

  return scanresults;
}

- (void)DeviceAdded:(int)deviceID
{
  parentClass->SetScanResults([self GetInputDevices]);
  parentClass->callOnDeviceAdded([self GetDeviceLocation:deviceID]);
}

- (void)DeviceRemoved:(int)deviceID
{
  parentClass->SetScanResults([self GetInputDevices]);
  parentClass->callOnDeviceRemoved([self GetDeviceLocation:deviceID]);
}

#pragma mark - init

- (instancetype)initWithName:(PERIPHERALS::CPeripheralBusGCController*)initClass
{
  self = [super init];

  parentClass = initClass;

  _input_GC = [[Input_GCController alloc] initWithName:self];

  return self;
}

- (void)SetDigitalEvent:(kodi::addon::PeripheralEvent)event
{
  std::unique_lock<CCriticalSection> lock(m_eventMutex);

  m_digitalEvents.emplace_back(event);
}

- (void)SetAxisEvent:(kodi::addon::PeripheralEvent)event
{
  std::unique_lock<CCriticalSection> lock(m_eventMutex);

  m_axisEvents.emplace_back(event);
}

#pragma mark - GetEvents

- (std::vector<kodi::addon::PeripheralEvent>)GetAxisEvents
{
  std::vector<kodi::addon::PeripheralEvent> events;
  std::unique_lock<CCriticalSection> lock(m_eventMutex);

  for (unsigned int i = 0; i < m_axisEvents.size(); i++)
    events.emplace_back(m_axisEvents[i]);

  m_axisEvents.clear();

  return events;
}

- (std::vector<kodi::addon::PeripheralEvent>)GetButtonEvents
{
  std::vector<kodi::addon::PeripheralEvent> events;

  std::unique_lock<CCriticalSection> lock(m_eventMutex);
  // Only report a single event per button (avoids dropping rapid presses)
  std::vector<kodi::addon::PeripheralEvent> repeatButtons;

  for (const auto& digitalEvent : m_digitalEvents)
  {
    auto HasButton = [&digitalEvent](const auto& event) {
      if (event.Type() == PERIPHERAL_EVENT_TYPE_DRIVER_BUTTON)
        return event.DriverIndex() == digitalEvent.DriverIndex();
      return false;
    };

    if (std::find_if(events.begin(), events.end(), HasButton) == events.end())
      events.emplace_back(digitalEvent);
    else
      repeatButtons.emplace_back(digitalEvent);
  }

  m_digitalEvents.swap(repeatButtons);

  return events;
}

- (int)GetControllerAxisCount:(int)deviceId withControllerType:(GCCONTROLLER_TYPE)controllerType
{
  int axisCount = 0;
  if (controllerType == GCCONTROLLER_TYPE::EXTENDED)
  {
    // Base GCController axis = X/Y Left, X/Y Right thumb
    // Potentially L/R trigger are axis buttons - not implemented
    axisCount = 4;
  }
  else if (controllerType == GCCONTROLLER_TYPE::MICRO)
    axisCount = 0;

  return axisCount;
}

- (int)GetControllerButtonCount:(int)deviceId withControllerType:(GCCONTROLLER_TYPE)controllerType
{
  int buttonCount = 0;
  if (controllerType == GCCONTROLLER_TYPE::EXTENDED)
  {
    // Base GCController buttons = Menu, 4 dpad, 2 trigger, 2 shoulder, 4 face
    // As of *OS 13, there are possibly 3 optional buttons - Options, Left/Right Thumbstick button
    buttonCount = 13;
    buttonCount += [self.input_GC checkOptionalButtons:deviceId];
  }
  else if (controllerType == GCCONTROLLER_TYPE::MICRO)
    buttonCount = 6;

  return buttonCount;
}

#pragma mark - callbackClass Controller ID matching

- (GCCONTROLLER_TYPE)GetControllerType:(int)deviceID
{

  auto gcinputtype = [self.input_GC GetGCControllerType:deviceID];

  if (gcinputtype != GCCONTROLLER_TYPE::NOTFOUND)
    return gcinputtype;

  return GCCONTROLLER_TYPE::UNKNOWN;
}

- (std::string)GetDeviceLocation:(int)deviceId
{
  return StringUtils::Format("{}{}", parentClass->getDeviceLocationPrefix(), deviceId);
}

#pragma mark - Logging Utils

- (void)displayMessage:(NSString*)message controllerID:(int)controllerID
{
  // Only log if message has any data
  if (message)
  {
    CLog::Log(LOGDEBUG, "CBPeripheralBusGCControllerManager: inputhandler - ID {} - Action {}",
              controllerID, message.UTF8String);
  }
}

@end
