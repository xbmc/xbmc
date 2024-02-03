/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "Input_Gamecontroller.h"

#include "addons/kodi-dev-kit/include/kodi/addon-instance/peripheral/PeripheralUtils.h"
#include "threads/CriticalSection.h"
#include "utils/log.h"

#import "platform/darwin/peripherals/InputKey.h"
#import "platform/darwin/peripherals/PeripheralBusGCControllerManager.h"

#include <mutex>

#import <Foundation/Foundation.h>
#import <GameController/GameController.h>

struct PlayerControllerState
{
  BOOL dpadLeftPressed;
  BOOL dpadRightPressed;
  BOOL dpadUpPressed;
  BOOL dpadDownPressed;
  BOOL LeftThumbLeftPressed;
  BOOL LeftThumbRightPressed;
  BOOL LeftThumbUpPressed;
  BOOL LeftThumbDownPressed;
  BOOL RightThumbLeftPressed;
  BOOL RightThumbRightPressed;
  BOOL RightThumbUpPressed;
  BOOL RightThumbDownPressed;
};

@implementation Input_GCController
{
  NSMutableArray* controllerArray;
  // State for each controller
  struct PlayerControllerState controllerState[4];
  CBPeripheralBusGCControllerManager* cbmanager;
  CCriticalSection m_GCMutex;
  CCriticalSection m_controllerMutex;
}

#pragma mark - Notification Observer

- (void)addModeSwitchObserver
{
  // notifications for controller (dis)connect
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(controllerWasConnected:)
                                               name:GCControllerDidConnectNotification
                                             object:nil];
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(controllerWasDisconnected:)
                                               name:GCControllerDidDisconnectNotification
                                             object:nil];
}

#pragma mark - Controller connection

- (void)controllerWasConnected:(NSNotification*)notification
{
  GCController* controller = (GCController*)notification.object;

  [self controllerConnection:controller];
}

- (GCControllerPlayerIndex)getAvailablePlayerIndex
{
  // No known controllers
  if (!controllerArray)
    return GCControllerPlayerIndex1;

  bool player1 = false;
  bool player2 = false;
  bool player3 = false;
  bool player4 = false;

  for (GCController* controller in controllerArray)
  {
    switch (controller.playerIndex)
    {
      case GCControllerPlayerIndex1:
        player1 = true;
        break;
      case GCControllerPlayerIndex2:
        player2 = true;
        break;
      case GCControllerPlayerIndex3:
        player3 = true;
        break;
      case GCControllerPlayerIndex4:
        player4 = true;
        break;
      default:
        break;
    }
  }

  if (!player1)
    return GCControllerPlayerIndex1;
  else if (!player2)
    return GCControllerPlayerIndex2;
  else if (!player3)
    return GCControllerPlayerIndex3;
  else if (!player4)
    return GCControllerPlayerIndex4;
  else
    return GCControllerPlayerIndexUnset;
}

- (void)controllerConnection:(GCController*)controller
{
  // Lock so add/remove events are serialised
  std::unique_lock<CCriticalSection> lock(m_controllerMutex);

  if ([controllerArray containsObject:controller])
  {
    CLog::Log(LOGINFO, "INPUT - GAMECONTROLLER: ignoring input device with ID {} already known",
              [controller.vendorName UTF8String]);
    return;
  }

  controller.playerIndex = [self getAvailablePlayerIndex];

  // set microgamepad to absolute values for dpad (ie center touchpad is 0,0)
  if (controller.microGamepad)
    controller.microGamepad.reportsAbsoluteDpadValues = YES;

  CLog::Log(LOGDEBUG, "INPUT - GAMECONTROLLER: input device with ID {} playerIndex {} added ",
            [controller.vendorName UTF8String], static_cast<int>(controller.playerIndex));
  [controllerArray addObject:controller];

  [cbmanager DeviceAdded:static_cast<int>(controller.playerIndex)];

  [self registerChangeHandler:controller];
}

- (void)registerChangeHandler:(GCController*)controller
{
  if (controller.extendedGamepad)
  {
    CLog::Log(LOGDEBUG, "INPUT - GAMECONTROLLER: extendedGamepad changehandler added");
    // register block for input change detection
    [self extendedValueChangeHandler:controller];
  }
  else if (controller.microGamepad)
  {
    CLog::Log(LOGDEBUG, "INPUT - GAMECONTROLLER: microGamepad changehandler added");
    [self microValueChangeHandler:controller];
  }
  if (@available(iOS 13.0, tvOS 13.0, macOS 10.15, *))
  {
    // Do Nothing - Cant negate @available
  }
  else
  {
    // pausevaluechangehandler only required for <= *os12/macos10.14
    CLog::Log(LOGDEBUG, "INPUT - GAMECONTROLLER: pauseValueChangeHandler added");
    [self pauseValueChangeHandler:controller];
  }
}

#pragma mark - Controller disconnection

- (void)controllerWasDisconnected:(NSNotification*)notification
{
  // Lock so add/remove events are serialised
  std::unique_lock<CCriticalSection> lock(m_controllerMutex);
  // a controller was disconnected
  GCController* controller = (GCController*)notification.object;
  if (!controllerArray)
    return;

  // Reset controller state to ensure default state for next time playerIndex
  controllerState[static_cast<int>(controller.playerIndex)] = {};

  auto i = [controllerArray indexOfObject:controller];

  if (i == NSNotFound)
  {
    CLog::Log(LOGWARNING, "INPUT - GAMECONTROLLER: failed to remove input device {} Not Found ",
              [controller.vendorName UTF8String]);
    return;
  }

  CLog::Log(LOGINFO, "INPUT - GAMECONTROLLER: input device \"{}\" removed",
            [controller.vendorName UTF8String]);

  [controllerArray removeObjectAtIndex:i];
  [cbmanager DeviceRemoved:static_cast<int>(controller.playerIndex)];
}

#pragma mark - GCMicroGamepad valueChangeHandler

- (void)microValueChangeHandler:(GCController*)controller
{
  auto __unsafe_unretained controllerInBlock = controller;
  controller.microGamepad.valueChangedHandler =
      ^(GCMicroGamepad* gamepad, GCControllerElement* element) {
        kodi::addon::PeripheralEvent newEvent;
        newEvent.SetPeripheralIndex(static_cast<unsigned int>(gamepad.controller.playerIndex));

        std::unique_lock<CCriticalSection> lock(m_GCMutex);

        // A button
        if (gamepad.buttonA == element)
        {
          [self setButtonState:gamepad.buttonA
                     withEvent:&newEvent
                   withMessage:@"A Button"
                 withInputInfo:InputValueInfo{GCCONTROLLER_TYPE::MICRO,
                                              GCCONTROLLER_MICRO_GAMEPAD_BUTTON::A}];
        }
        // X button
        else if (gamepad.buttonX == element)
        {
          [self setButtonState:gamepad.buttonX
                     withEvent:&newEvent
                   withMessage:@"X Button"
                 withInputInfo:InputValueInfo{GCCONTROLLER_TYPE::MICRO,
                                              GCCONTROLLER_MICRO_GAMEPAD_BUTTON::X}];
        }
        // d-pad
        else if (gamepad.dpad == element)
        {
          [self checkdpad:gamepad.dpad
                    withEvent:&newEvent
                withInputInfo:InputValueInfo{GCCONTROLLER_TYPE::MICRO}
              withplayerIndex:static_cast<int>(controllerInBlock.playerIndex)];
        }
        if (@available(iOS 13.0, tvOS 13.0, macOS 10.15, *))
        {
          // buttonMenu
          if (gamepad.buttonMenu == element)
          {
            [self setButtonState:static_cast<GCControllerButtonInput*>(element)
                       withEvent:&newEvent
                     withMessage:@"Menu Button"
                   withInputInfo:InputValueInfo{GCCONTROLLER_TYPE::MICRO,
                                                GCCONTROLLER_MICRO_GAMEPAD_BUTTON::MENU}];
          }
        }

        [cbmanager SetDigitalEvent:newEvent];
      };
}

#pragma mark - GCExtendedGamepad valueChangeHandler

- (void)extendedValueChangeHandler:(GCController*)controller
{
  controller.extendedGamepad.valueChangedHandler = ^(GCExtendedGamepad* gamepad,
                                                     GCControllerElement* element) {
    NSString* message;

    kodi::addon::PeripheralEvent newEvent;
    kodi::addon::PeripheralEvent axisEvent;
    auto playerIndex = static_cast<int>(gamepad.controller.playerIndex);
    newEvent.SetPeripheralIndex(playerIndex);
    axisEvent.SetPeripheralIndex(playerIndex);

    std::unique_lock<CCriticalSection> lock(m_GCMutex);

    // left trigger
    if (gamepad.leftTrigger == element)
    {
      message =
          [self setButtonState:gamepad.leftTrigger
                     withEvent:&newEvent
                   withMessage:@"Left Trigger"
                 withInputInfo:InputValueInfo{GCCONTROLLER_TYPE::EXTENDED,
                                              GCCONTROLLER_EXTENDED_GAMEPAD_BUTTON::LEFTTRIGGER}];
    }
    // right trigger
    else if (gamepad.rightTrigger == element)
    {
      message =
          [self setButtonState:gamepad.rightTrigger
                     withEvent:&newEvent
                   withMessage:@"Right Trigger"
                 withInputInfo:InputValueInfo{GCCONTROLLER_TYPE::EXTENDED,
                                              GCCONTROLLER_EXTENDED_GAMEPAD_BUTTON::RIGHTTRIGGER}];
    }
    // left shoulder button
    else if (gamepad.leftShoulder == element)
    {
      message =
          [self setButtonState:gamepad.leftShoulder
                     withEvent:&newEvent
                   withMessage:@"Left Shoulder Button"
                 withInputInfo:InputValueInfo{GCCONTROLLER_TYPE::EXTENDED,
                                              GCCONTROLLER_EXTENDED_GAMEPAD_BUTTON::LEFTSHOULDER}];
    }
    // right shoulder button
    else if (gamepad.rightShoulder == element)
    {
      message =
          [self setButtonState:gamepad.rightShoulder
                     withEvent:&newEvent
                   withMessage:@"Right Shoulder Button"
                 withInputInfo:InputValueInfo{GCCONTROLLER_TYPE::EXTENDED,
                                              GCCONTROLLER_EXTENDED_GAMEPAD_BUTTON::RIGHTSHOULDER}];
    }
    // A button
    else if (gamepad.buttonA == element)
    {
      message = [self setButtonState:gamepad.buttonA
                           withEvent:&newEvent
                         withMessage:@"A Button"
                       withInputInfo:InputValueInfo{GCCONTROLLER_TYPE::EXTENDED,
                                                    GCCONTROLLER_EXTENDED_GAMEPAD_BUTTON::A}];
    }
    // B button
    else if (gamepad.buttonB == element)
    {
      message = [self setButtonState:gamepad.buttonB
                           withEvent:&newEvent
                         withMessage:@"B Button"
                       withInputInfo:InputValueInfo{GCCONTROLLER_TYPE::EXTENDED,
                                                    GCCONTROLLER_EXTENDED_GAMEPAD_BUTTON::B}];
    }
    // X button
    else if (gamepad.buttonX == element)
    {
      message = [self setButtonState:gamepad.buttonX
                           withEvent:&newEvent
                         withMessage:@"X Button"
                       withInputInfo:InputValueInfo{GCCONTROLLER_TYPE::EXTENDED,
                                                    GCCONTROLLER_EXTENDED_GAMEPAD_BUTTON::X}];
    }
    // Y button
    else if (gamepad.buttonY == element)
    {
      message = [self setButtonState:gamepad.buttonY
                           withEvent:&newEvent
                         withMessage:@"Y Button"
                       withInputInfo:InputValueInfo{GCCONTROLLER_TYPE::EXTENDED,
                                                    GCCONTROLLER_EXTENDED_GAMEPAD_BUTTON::Y}];
    }
    // d-pad
    else if (gamepad.dpad == element)
    {
      message = [self checkdpad:gamepad.dpad
                      withEvent:&newEvent
                  withInputInfo:InputValueInfo{GCCONTROLLER_TYPE::EXTENDED}
                withplayerIndex:playerIndex];
    }
    // left stick
    else if (gamepad.leftThumbstick == element)
    {
      message = @"Left Stick";
      message = [self checkthumbstick:gamepad.leftThumbstick
                            withEvent:&axisEvent
                          withMessage:message
                             withAxis:GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::LEFT
                      withplayerIndex:playerIndex];
    }
    // right stick
    else if (gamepad.rightThumbstick == element)
    {
      message = @"Right Stick";
      message = [self checkthumbstick:gamepad.rightThumbstick
                            withEvent:&axisEvent
                          withMessage:message
                             withAxis:GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::RIGHT
                      withplayerIndex:playerIndex];
    }
    if (@available(iOS 12.1, tvOS 12.1, macOS 10.14.1, *))
    {
      // Left Thumbstick Button
      if (gamepad.leftThumbstickButton == element)
      {
        message =
            [self setButtonState:gamepad.leftThumbstickButton
                       withEvent:&newEvent
                     withMessage:@"Left Thumbstick Button"
                   withInputInfo:InputValueInfo{
                                     GCCONTROLLER_TYPE::EXTENDED,
                                     GCCONTROLLER_EXTENDED_GAMEPAD_BUTTON::LEFTTHUMBSTICKBUTTON}];
      }
      // Right Thumbstick Button
      if (gamepad.rightThumbstickButton == element)
      {
        message =
            [self setButtonState:gamepad.rightThumbstickButton
                       withEvent:&newEvent
                     withMessage:@"Right Thumbstick Button"
                   withInputInfo:InputValueInfo{
                                     GCCONTROLLER_TYPE::EXTENDED,
                                     GCCONTROLLER_EXTENDED_GAMEPAD_BUTTON::RIGHTTHUMBSTICKBUTTON}];
      }
    }
    if (@available(iOS 13.0, tvOS 13.0, macOS 10.15, *))
    {
      // buttonMenu
      if (gamepad.buttonMenu == element)
      {
        message = [self setButtonState:static_cast<GCControllerButtonInput*>(element)
                             withEvent:&newEvent
                           withMessage:@"Menu Button"
                         withInputInfo:InputValueInfo{GCCONTROLLER_TYPE::EXTENDED,
                                                      GCCONTROLLER_EXTENDED_GAMEPAD_BUTTON::MENU}];
      }
      // buttonOptions
      else if (gamepad.buttonOptions == element)
      {
        message =
            [self setButtonState:static_cast<GCControllerButtonInput*>(element)
                       withEvent:&newEvent
                     withMessage:@"Option Button"
                   withInputInfo:InputValueInfo{GCCONTROLLER_TYPE::EXTENDED,
                                                GCCONTROLLER_EXTENDED_GAMEPAD_BUTTON::OPTION}];
      }
    }
    if (@available(iOS 14.0, tvOS 14.0, macOS 11.0, *))
    {
      // buttonHome
      if (gamepad.buttonHome == element)
      {
        message = [self setButtonState:static_cast<GCControllerButtonInput*>(element)
                             withEvent:&newEvent
                           withMessage:@"Home Button"
                         withInputInfo:InputValueInfo{GCCONTROLLER_TYPE::EXTENDED,
                                                      GCCONTROLLER_EXTENDED_GAMEPAD_BUTTON::HOME}];
      }
    }
    [cbmanager SetDigitalEvent:newEvent];

    //! @todo Debug Purposes only - excessive log spam
    // utilise spdlog for input compononent logging
    // [cbmanager displayMessage:message controllerID:static_cast<int>(controller.playerIndex)];
  };
}

- (void)pauseValueChangeHandler:(GCController*)controller
{
  controller.controllerPausedHandler = ^(GCController* controller) {
    // check if we're currently paused or not
    // then bring up or remove the paused view controller

    kodi::addon::PeripheralEvent newEvent;
    newEvent.SetPeripheralIndex(static_cast<unsigned int>(controller.playerIndex));
    newEvent.SetType(PERIPHERAL_EVENT_TYPE_DRIVER_BUTTON);

    if (controller.extendedGamepad)
      newEvent.SetDriverIndex(
          static_cast<unsigned int>(GCCONTROLLER_EXTENDED_GAMEPAD_BUTTON::MENU));
    else if (controller.microGamepad)
      newEvent.SetDriverIndex(static_cast<unsigned int>(GCCONTROLLER_MICRO_GAMEPAD_BUTTON::MENU));

    // Button Down event
    newEvent.SetButtonState(JOYSTICK_STATE_BUTTON_PRESSED);
    [cbmanager SetDigitalEvent:newEvent];

    // Button Up Event
    newEvent.SetButtonState(JOYSTICK_STATE_BUTTON_UNPRESSED);
    [cbmanager SetDigitalEvent:newEvent];
  };
}

#pragma mark - valuechangehandler event state change

- (NSString*)setButtonState:(GCControllerButtonInput*)button
                  withEvent:(kodi::addon::PeripheralEvent*)event
                withMessage:(NSString*)message
              withInputInfo:(InputValueInfo)inputInfo
{
  event->SetType(PERIPHERAL_EVENT_TYPE_DRIVER_BUTTON);

  switch (inputInfo.controllerType)
  {
    case GCCONTROLLER_TYPE::EXTENDED:
      event->SetDriverIndex(static_cast<unsigned int>(inputInfo.extendedButton));
      break;
    case GCCONTROLLER_TYPE::MICRO:
      event->SetDriverIndex(static_cast<unsigned int>(inputInfo.microButton));
      break;
    default:
      return [message
          stringByAppendingFormat:@" ERROR:: CONTROLLER_TYPE %d", inputInfo.controllerType];
  }

  if (button.isPressed)
  {
    event->SetButtonState(JOYSTICK_STATE_BUTTON_PRESSED);
    return [message stringByAppendingString:@" Pressed"];
  }
  else
  {
    event->SetButtonState(JOYSTICK_STATE_BUTTON_UNPRESSED);
    return [message stringByAppendingString:@" Released"];
  }
}

- (void)setAxisValue:(GCControllerAxisInput*)axisValue
           withEvent:(kodi::addon::PeripheralEvent*)event
            withAxis:(GCCONTROLLER_EXTENDED_GAMEPAD_AXIS)axis
{
  event->SetType(PERIPHERAL_EVENT_TYPE_DRIVER_AXIS);
  event->SetDriverIndex(static_cast<unsigned int>(axis));
  event->SetAxisState(axisValue.value);
}

- (PERIPHERALS::PeripheralScanResults)GetGCDevices
{
  PERIPHERALS::PeripheralScanResults scanresults;

  if (controllerArray.count == 0)
    return scanresults;

  for (GCController* controller in controllerArray)
  {
    PERIPHERALS::PeripheralScanResult peripheralScanResult;
    peripheralScanResult.m_type = PERIPHERALS::PERIPHERAL_JOYSTICK;
    peripheralScanResult.m_strLocation =
        [cbmanager GetDeviceLocation:static_cast<int>(controller.playerIndex)];
    peripheralScanResult.m_iVendorId = 0;
    peripheralScanResult.m_iProductId = 0;
    peripheralScanResult.m_mappedType = PERIPHERALS::PERIPHERAL_JOYSTICK;

    if (controller.extendedGamepad)
      peripheralScanResult.m_strDeviceName = "Extended Gamepad";
    else if (controller.microGamepad)
      peripheralScanResult.m_strDeviceName = "Micro Gamepad";
    else
      peripheralScanResult.m_strDeviceName = "Unknown Gamepad";

    peripheralScanResult.m_busType = PERIPHERALS::PERIPHERAL_BUS_GCCONTROLLER;
    peripheralScanResult.m_mappedBusType = PERIPHERALS::PERIPHERAL_BUS_GCCONTROLLER;
    peripheralScanResult.m_iSequence = 0;
    scanresults.m_results.push_back(peripheralScanResult);
  }

  return scanresults;
}

- (GCCONTROLLER_TYPE)GetGCControllerType:(int)deviceID
{
  GCController* controller;
  for (GCController* aController in controllerArray)
  {
    if (controller.playerIndex != deviceID)
      continue;
    controller = aController;
    break;
  }
  if (controller.extendedGamepad)
    return GCCONTROLLER_TYPE::EXTENDED;
  else if (controller.microGamepad)
    return GCCONTROLLER_TYPE::MICRO;
  else
    return GCCONTROLLER_TYPE::NOTFOUND;
}

- (int)checkOptionalButtons:(int)deviceID
{
  int optionalButtonCount = 0;

  for (GCController* controller in controllerArray)
  {
    if (controller.playerIndex != deviceID)
      continue;

    if (!controller.extendedGamepad)
      continue;

    // Check if optional buttons exist on mapped controller
    // button object is nil if button doesn't exist
    if (@available(iOS 12.1, tvOS 12.1, macOS 10.14.1, *))
    {
      if (controller.extendedGamepad.leftThumbstickButton)
        ++optionalButtonCount;
      if (controller.extendedGamepad.rightThumbstickButton)
        ++optionalButtonCount;
    }
    if (@available(iOS 13.0, tvOS 13.0, macOS 10.15, *))
      if (controller.extendedGamepad.buttonOptions != nil)
        ++optionalButtonCount;
  }
  return optionalButtonCount;
}

- (instancetype)initWithName:(CBPeripheralBusGCControllerManager*)callbackManager
{
  self = [super init];
  if (!self)
    return nil;

  cbmanager = callbackManager;

  [self addModeSwitchObserver];

  controllerArray = [[NSMutableArray alloc] initWithCapacity:4];

  auto controllers = [GCController controllers];
  // Iterate through any pre-existing controller connections at startup to enable value handlers
  for (GCController* controller in controllers)
    [self controllerConnection:controller];

  return self;
}

- (NSString*)checkthumbstick:(GCControllerDirectionPad*)thumbstick
                   withEvent:(kodi::addon::PeripheralEvent*)event
                 withMessage:(NSString*)message
                    withAxis:(GCCONTROLLER_EXTENDED_GAMEPAD_AXIS)thumbstickside
             withplayerIndex:(int)playerIndex
{
  // thumbstick released completely - zero both axis
  if (!thumbstick.up.isPressed && !thumbstick.down.isPressed && !thumbstick.left.isPressed &&
      !thumbstick.right.isPressed)
  {
    if (thumbstickside == GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::RIGHT)
    {
      controllerState[playerIndex].RightThumbLeftPressed = NO;
      controllerState[playerIndex].RightThumbRightPressed = NO;
      controllerState[playerIndex].RightThumbUpPressed = NO;
      controllerState[playerIndex].RightThumbDownPressed = NO;

      // Thumbstick release event
      kodi::addon::PeripheralEvent releaseEvent;
      releaseEvent.SetPeripheralIndex(static_cast<unsigned int>(playerIndex));
      [self setAxisValue:0
               withEvent:&releaseEvent
                withAxis:GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::RIGHTTHUMB_X];

      [cbmanager SetAxisEvent:releaseEvent];

      [self setAxisValue:0
               withEvent:&releaseEvent
                withAxis:GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::RIGHTTHUMB_Y];

      message = [message stringByAppendingString:@" Released"];
      [cbmanager SetAxisEvent:releaseEvent];
    }
    else
    {
      controllerState[playerIndex].LeftThumbLeftPressed = NO;
      controllerState[playerIndex].LeftThumbRightPressed = NO;
      controllerState[playerIndex].LeftThumbUpPressed = NO;
      controllerState[playerIndex].LeftThumbDownPressed = NO;

      // Thumbstick release event
      kodi::addon::PeripheralEvent releaseEvent;
      releaseEvent.SetPeripheralIndex(static_cast<unsigned int>(playerIndex));
      [self setAxisValue:0
               withEvent:&releaseEvent
                withAxis:GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::LEFTTHUMB_X];

      [cbmanager SetAxisEvent:releaseEvent];

      [self setAxisValue:0
               withEvent:&releaseEvent
                withAxis:GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::LEFTTHUMB_Y];

      message = [message stringByAppendingString:@" Released"];
      [cbmanager SetAxisEvent:releaseEvent];
    }
  }
  else
  {

    if (thumbstick.up.isPressed || controllerState[playerIndex].RightThumbUpPressed ||
        controllerState[playerIndex].LeftThumbUpPressed)
    {
      // Thumbstick centered
      if (!thumbstick.up.isPressed)
      {
        if (controllerState[playerIndex].RightThumbUpPressed)
          controllerState[playerIndex].RightThumbUpPressed =
              !controllerState[playerIndex].RightThumbUpPressed;
        else if (controllerState[playerIndex].LeftThumbUpPressed)
          controllerState[playerIndex].LeftThumbUpPressed =
              !controllerState[playerIndex].LeftThumbUpPressed;

        // Thumbstick release event
        kodi::addon::PeripheralEvent newReleaseEvent;
        newReleaseEvent.SetPeripheralIndex(static_cast<unsigned int>(playerIndex));
        [self setAxisValue:0
                 withEvent:&newReleaseEvent
                  withAxis:(thumbstickside == GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::RIGHT
                                ? GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::RIGHTTHUMB_Y
                                : GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::LEFTTHUMB_Y)];

        message = [message stringByAppendingFormat:@" Up %f", 0.0];
        [cbmanager SetAxisEvent:newReleaseEvent];
      }
      else
      {
        controllerState[playerIndex].RightThumbUpPressed =
            (thumbstickside == GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::RIGHT
                 ? YES
                 : controllerState[playerIndex].RightThumbUpPressed);
        controllerState[playerIndex].LeftThumbUpPressed =
            (thumbstickside == GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::LEFT
                 ? YES
                 : controllerState[playerIndex].LeftThumbUpPressed);

        [self setAxisValue:thumbstick.yAxis
                 withEvent:event
                  withAxis:(thumbstickside == GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::RIGHT
                                ? GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::RIGHTTHUMB_Y
                                : GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::LEFTTHUMB_Y)];

        message = [message
            stringByAppendingFormat:@" Up %f", static_cast<double>(thumbstick.yAxis.value)];
        [cbmanager SetAxisEvent:*event];
      }
    }
    if (thumbstick.down.isPressed || controllerState[playerIndex].RightThumbDownPressed ||
        controllerState[playerIndex].LeftThumbDownPressed)
    {
      // Thumbstick centered
      if (!thumbstick.down.isPressed)
      {
        if (controllerState[playerIndex].RightThumbDownPressed)
          controllerState[playerIndex].RightThumbDownPressed =
              !controllerState[playerIndex].RightThumbDownPressed;
        else if (controllerState[playerIndex].LeftThumbDownPressed)
          controllerState[playerIndex].LeftThumbDownPressed =
              !controllerState[playerIndex].LeftThumbDownPressed;

        // Thumbstick release event
        kodi::addon::PeripheralEvent newReleaseEvent;
        newReleaseEvent.SetPeripheralIndex(static_cast<unsigned int>(playerIndex));
        [self setAxisValue:0
                 withEvent:&newReleaseEvent
                  withAxis:(thumbstickside == GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::RIGHT
                                ? GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::RIGHTTHUMB_Y
                                : GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::LEFTTHUMB_Y)];

        message = [message stringByAppendingFormat:@" Down %f", 0.0];
        [cbmanager SetAxisEvent:newReleaseEvent];
      }
      else
      {
        controllerState[playerIndex].RightThumbDownPressed =
            (thumbstickside == GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::RIGHT
                 ? YES
                 : controllerState[playerIndex].RightThumbDownPressed);
        controllerState[playerIndex].LeftThumbDownPressed =
            (thumbstickside == GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::LEFT
                 ? YES
                 : controllerState[playerIndex].LeftThumbDownPressed);

        [self setAxisValue:thumbstick.yAxis
                 withEvent:event
                  withAxis:(thumbstickside == GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::RIGHT
                                ? GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::RIGHTTHUMB_Y
                                : GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::LEFTTHUMB_Y)];

        message = [message
            stringByAppendingFormat:@" Down %f", static_cast<double>(thumbstick.yAxis.value)];
        [cbmanager SetAxisEvent:*event];
      }
    }
    if (thumbstick.left.isPressed || controllerState[playerIndex].RightThumbLeftPressed ||
        controllerState[playerIndex].LeftThumbLeftPressed)
    {
      // Thumbstick centered
      if (!thumbstick.left.isPressed)
      {
        if (controllerState[playerIndex].RightThumbLeftPressed)
          controllerState[playerIndex].RightThumbLeftPressed =
              !controllerState[playerIndex].RightThumbLeftPressed;
        else if (controllerState[playerIndex].LeftThumbLeftPressed)
          controllerState[playerIndex].LeftThumbLeftPressed =
              !controllerState[playerIndex].LeftThumbLeftPressed;

        // Thumbstick release event
        kodi::addon::PeripheralEvent newReleaseEvent;
        newReleaseEvent.SetPeripheralIndex(static_cast<unsigned int>(playerIndex));
        [self setAxisValue:0
                 withEvent:&newReleaseEvent
                  withAxis:(thumbstickside == GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::RIGHT
                                ? GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::RIGHTTHUMB_X
                                : GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::LEFTTHUMB_X)];

        message = [message stringByAppendingFormat:@" Left %f", 0.0];
        [cbmanager SetAxisEvent:newReleaseEvent];
      }
      else
      {
        controllerState[playerIndex].RightThumbLeftPressed =
            (thumbstickside == GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::RIGHT
                 ? YES
                 : controllerState[playerIndex].RightThumbLeftPressed);
        controllerState[playerIndex].LeftThumbLeftPressed =
            (thumbstickside == GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::LEFT
                 ? YES
                 : controllerState[playerIndex].LeftThumbLeftPressed);

        [self setAxisValue:thumbstick.xAxis
                 withEvent:event
                  withAxis:(thumbstickside == GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::RIGHT
                                ? GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::RIGHTTHUMB_X
                                : GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::LEFTTHUMB_X)];

        message = [message
            stringByAppendingFormat:@" Left %f", static_cast<double>(thumbstick.xAxis.value)];
        [cbmanager SetAxisEvent:*event];
      }
    }
    if (thumbstick.right.isPressed || controllerState[playerIndex].RightThumbRightPressed ||
        controllerState[playerIndex].LeftThumbRightPressed)
    {
      // Thumbstick centered
      if (!thumbstick.right.isPressed)
      {
        if (controllerState[playerIndex].RightThumbRightPressed)
          controllerState[playerIndex].RightThumbRightPressed =
              !controllerState[playerIndex].RightThumbRightPressed;
        else if (controllerState[playerIndex].LeftThumbRightPressed)
          controllerState[playerIndex].LeftThumbRightPressed =
              !controllerState[playerIndex].LeftThumbRightPressed;

        // Thumbstick release event
        kodi::addon::PeripheralEvent newReleaseEvent;
        newReleaseEvent.SetPeripheralIndex(static_cast<unsigned int>(playerIndex));
        [self setAxisValue:0
                 withEvent:&newReleaseEvent
                  withAxis:(thumbstickside == GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::RIGHT
                                ? GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::RIGHTTHUMB_X
                                : GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::LEFTTHUMB_X)];

        message = [message stringByAppendingFormat:@" Right %f", 0.0];
        [cbmanager SetAxisEvent:newReleaseEvent];
      }
      else
      {
        controllerState[playerIndex].RightThumbRightPressed =
            (thumbstickside == GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::RIGHT
                 ? YES
                 : controllerState[playerIndex].RightThumbRightPressed);
        controllerState[playerIndex].LeftThumbRightPressed =
            (thumbstickside == GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::LEFT
                 ? YES
                 : controllerState[playerIndex].LeftThumbRightPressed);

        [self setAxisValue:thumbstick.xAxis
                 withEvent:event
                  withAxis:(thumbstickside == GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::RIGHT
                                ? GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::RIGHTTHUMB_X
                                : GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::LEFTTHUMB_X)];

        message = [message
            stringByAppendingFormat:@" Right %f", static_cast<double>(thumbstick.xAxis.value)];
        [cbmanager SetAxisEvent:*event];
      }
    }
  }
  return message;
}

- (NSString*)checkdpad:(GCControllerDirectionPad*)dpad
             withEvent:(kodi::addon::PeripheralEvent*)event
         withInputInfo:(InputValueInfo)inputInfo
       withplayerIndex:(int)playerIndex
{
  NSString* message = nil;
  if ((dpad.up.isPressed && !controllerState[playerIndex].dpadUpPressed) ||
      (!dpad.up.isPressed && controllerState[playerIndex].dpadUpPressed))
  {
    message = @"D-Pad Up";

    if (inputInfo.controllerType == GCCONTROLLER_TYPE::EXTENDED)
      inputInfo.extendedButton = GCCONTROLLER_EXTENDED_GAMEPAD_BUTTON::UP;
    else if (inputInfo.controllerType == GCCONTROLLER_TYPE::MICRO)
      inputInfo.microButton = GCCONTROLLER_MICRO_GAMEPAD_BUTTON::UP;

    if (!controllerState[playerIndex].dpadUpPressed)
    {
      // Button Down event
      message = [self setButtonState:dpad.up
                           withEvent:event
                         withMessage:message
                       withInputInfo:inputInfo];
    }
    else
    {
      // Button Up event
      kodi::addon::PeripheralEvent newReleaseEvent;
      newReleaseEvent.SetPeripheralIndex(static_cast<unsigned int>(playerIndex));
      message = [self setButtonState:dpad.up
                           withEvent:&newReleaseEvent
                         withMessage:message
                       withInputInfo:inputInfo];
      [cbmanager SetDigitalEvent:newReleaseEvent];
    }
    controllerState[playerIndex].dpadUpPressed = !controllerState[playerIndex].dpadUpPressed;
  }
  if ((dpad.down.isPressed && !controllerState[playerIndex].dpadDownPressed) ||
      (!dpad.down.isPressed && controllerState[playerIndex].dpadDownPressed))
  {
    message = @"D-Pad Down";

    if (inputInfo.controllerType == GCCONTROLLER_TYPE::EXTENDED)
      inputInfo.extendedButton = GCCONTROLLER_EXTENDED_GAMEPAD_BUTTON::DOWN;
    else if (inputInfo.controllerType == GCCONTROLLER_TYPE::MICRO)
      inputInfo.microButton = GCCONTROLLER_MICRO_GAMEPAD_BUTTON::DOWN;

    if (!controllerState[playerIndex].dpadDownPressed)
    {
      // Button Down event
      message = [self setButtonState:dpad.down
                           withEvent:event
                         withMessage:message
                       withInputInfo:inputInfo];
    }
    else
    {
      // Button Up event
      kodi::addon::PeripheralEvent newReleaseEvent;
      newReleaseEvent.SetPeripheralIndex(static_cast<unsigned int>(playerIndex));
      message = [self setButtonState:dpad.down
                           withEvent:&newReleaseEvent
                         withMessage:message
                       withInputInfo:inputInfo];
      [cbmanager SetDigitalEvent:newReleaseEvent];
    }
    controllerState[playerIndex].dpadDownPressed = !controllerState[playerIndex].dpadDownPressed;
  }
  if ((dpad.left.isPressed && !controllerState[playerIndex].dpadLeftPressed) ||
      (!dpad.left.isPressed && controllerState[playerIndex].dpadLeftPressed))
  {
    message = @"D-Pad Left";

    if (inputInfo.controllerType == GCCONTROLLER_TYPE::EXTENDED)
      inputInfo.extendedButton = GCCONTROLLER_EXTENDED_GAMEPAD_BUTTON::LEFT;
    else if (inputInfo.controllerType == GCCONTROLLER_TYPE::MICRO)
      inputInfo.microButton = GCCONTROLLER_MICRO_GAMEPAD_BUTTON::LEFT;

    if (!controllerState[playerIndex].dpadLeftPressed)
    {
      // Button Down event
      message = [self setButtonState:dpad.left
                           withEvent:event
                         withMessage:message
                       withInputInfo:inputInfo];
    }
    else
    {
      // Button Up event
      kodi::addon::PeripheralEvent newReleaseEvent;
      newReleaseEvent.SetPeripheralIndex(static_cast<unsigned int>(playerIndex));
      message = [self setButtonState:dpad.left
                           withEvent:&newReleaseEvent
                         withMessage:message
                       withInputInfo:inputInfo];
      [cbmanager SetDigitalEvent:newReleaseEvent];
    }
    controllerState[playerIndex].dpadLeftPressed = !controllerState[playerIndex].dpadLeftPressed;
  }
  if ((dpad.right.isPressed && !controllerState[playerIndex].dpadRightPressed) ||
      (!dpad.right.isPressed && controllerState[playerIndex].dpadRightPressed))
  {
    message = @"D-Pad Right";

    if (inputInfo.controllerType == GCCONTROLLER_TYPE::EXTENDED)
      inputInfo.extendedButton = GCCONTROLLER_EXTENDED_GAMEPAD_BUTTON::RIGHT;
    else if (inputInfo.controllerType == GCCONTROLLER_TYPE::MICRO)
      inputInfo.microButton = GCCONTROLLER_MICRO_GAMEPAD_BUTTON::RIGHT;

    if (!controllerState[playerIndex].dpadRightPressed)
    {
      // Button Down event
      message = [self setButtonState:dpad.right
                           withEvent:event
                         withMessage:message
                       withInputInfo:inputInfo];
    }
    else
    {
      // Button Up event
      kodi::addon::PeripheralEvent newReleaseEvent;
      newReleaseEvent.SetPeripheralIndex(static_cast<unsigned int>(playerIndex));
      message = [self setButtonState:dpad.right
                           withEvent:&newReleaseEvent
                         withMessage:message
                       withInputInfo:InputValueInfo{GCCONTROLLER_TYPE::EXTENDED,
                                                    GCCONTROLLER_EXTENDED_GAMEPAD_BUTTON::RIGHT}];
      [cbmanager SetDigitalEvent:newReleaseEvent];
    }
    controllerState[playerIndex].dpadRightPressed = !controllerState[playerIndex].dpadRightPressed;
  }
  return message;
}

@end
