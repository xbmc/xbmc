/*
 *  Copyright (C) 2019- Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "LibInputHandler.h"

#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPowerHandling.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/InputManager.h"
#include "input/keymaps/remote/CustomControllerTranslator.h"
#include "utils/log.h"

#import "platform/darwin/tvos/input/LibInputRemote.h"
#import "platform/darwin/tvos/input/LibInputSettings.h"
#import "platform/darwin/tvos/input/LibInputTouch.h"

@implementation TVOSLibInputHandler

@synthesize inputRemote;
@synthesize inputSettings;
@synthesize inputTouch;

#pragma mark - internal key press methods

//! @Todo: factor out siriremote customcontroller to a setting?
// allow to select multiple customcontrollers via setting list?
- (void)sendButtonPressed:(int)buttonId
{
  int actionID;
  std::string actionName;

  // Translate using custom controller translator.
  if (CServiceBroker::GetInputManager().TranslateCustomControllerString(
          CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog(), "SiriRemote",
          buttonId, actionID, actionName))
  {
    // break screensaver
    auto& components = CServiceBroker::GetAppComponents();
    const auto appPower = components.GetComponent<CApplicationPowerHandling>();
    appPower->ResetSystemIdleTimer();
    appPower->ResetScreenSaver();

    // in case we wokeup the screensaver or screen - eat that action...
    if (appPower->WakeUpScreenSaverAndDPMS())
      return;
    CServiceBroker::GetInputManager().QueueAction(CAction(actionID, 1.0f, 0.0f, actionName));
  }
  else
  {
    CLog::Log(LOGDEBUG, "ERROR mapping customcontroller action. CustomController: {} {}",
              "SiriRemote", buttonId);
  }
}

- (instancetype)init
{
  self = [super init];
  if (!self)
    return nil;

  inputRemote = [TVOSLibInputRemote new];
  inputSettings = [TVOSLibInputSettings new];
  inputTouch = [TVOSLibInputTouch new];

  return self;
}

@end
