/*
 *  Copyright (C) 2019- Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "LibInputSettings.h"

#import "platform/darwin/tvos/XBMCController.h"
#import "platform/darwin/tvos/input/LibInputHandler.h"
#import "platform/darwin/tvos/input/LibInputRemote.h"

#import <Foundation/Foundation.h>

@implementation TVOSLibInputSettings

@synthesize siriRemoteIdleTimerEnabled = m_siriRemoteIdleTimerEnabled;
@synthesize siriRemoteIdleTime = m_siriRemoteIdleTime;
@synthesize siriRemoteHorizontalSensitivity = m_siriRemoteHorizontalSensitivity;
@synthesize siriRemoteVerticalSensitivity = m_siriRemoteVerticalSensitivity;

- (void)setSiriRemoteIdleTimer:(bool)idle
{
  if (m_siriRemoteIdleTimerEnabled != idle)
  {
    m_siriRemoteIdleTimerEnabled = idle;
    if (m_siriRemoteIdleTimerEnabled)
      [g_xbmcController.inputHandler.inputRemote startSiriRemoteIdleTimer];
  }
}

- (void)setSiriRemoteIdleTime:(int)time
{
  m_siriRemoteIdleTime = time;
  [g_xbmcController.inputHandler.inputRemote startSiriRemoteIdleTimer];
}

@end
