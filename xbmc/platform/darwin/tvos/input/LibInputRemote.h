/*
 *  Copyright (C) 2019- Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import <Foundation/NSTimer.h>
#import <UIKit/UIEvent.h>

@interface TVOSLibInputRemote : NSObject
{
  NSTimer* m_pressAutoRepeatTimer;
  NSTimer* m_siriRemoteIdleTimer;
}

@property(nonatomic) bool siriRemoteIdleState;

- (void)startSiriRemoteIdleTimer;
- (void)stopSiriRemoteIdleTimer;
- (void)setSiriRemoteIdleState;
- (void)startKeyPressTimer:(int)keyId;
- (void)startKeyPressTimer:(int)keyId clickTime:(NSTimeInterval)interval;
- (void)stopKeyPressTimer;
- (void)keyPressTimerCallback:(NSTimer*)theTimer;
- (void)remoteControlEvent:(UIEvent*)receivedEvent;

@end
