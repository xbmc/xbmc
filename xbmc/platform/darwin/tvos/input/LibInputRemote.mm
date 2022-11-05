/*
 *  Copyright (C) 2019- Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "LibInputRemote.h"

#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"
#import "windowing/tvos/WinSystemTVOS.h"

#import "platform/darwin/tvos/XBMCController.h"
#import "platform/darwin/tvos/input/LibInputHandler.h"
#import "platform/darwin/tvos/input/LibInputSettings.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIEvent.h>

@implementation TVOSLibInputRemote

@synthesize siriRemoteIdleState = m_siriRemoteIdleState;

// Default Timer values (seconds)
NSTimeInterval REPEATED_KEYPRESS_DELAY_S = 0.50;
NSTimeInterval REPEATED_KEYPRESS_PAUSE_S = 0.05;

#pragma mark - Siri remote idle timer

- (void)startSiriRemoteIdleTimer
{
  m_siriRemoteIdleState = false;

  if (m_siriRemoteIdleTimer != nil)
    [self stopSiriRemoteIdleTimer];
  if (g_xbmcController.inputHandler.inputSettings.siriRemoteIdleTimerEnabled)
  {
    auto fireDate = [NSDate dateWithTimeIntervalSinceNow:g_xbmcController.inputHandler.inputSettings
                                                             .siriRemoteIdleTime];
    auto timer = [[NSTimer alloc] initWithFireDate:fireDate
                                          interval:0.0
                                            target:self
                                          selector:@selector(setSiriRemoteIdleState)
                                          userInfo:nil
                                           repeats:NO];

    [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSDefaultRunLoopMode];
    m_siriRemoteIdleTimer = timer;
  }
}

- (void)stopSiriRemoteIdleTimer
{
  [m_siriRemoteIdleTimer invalidate];
  m_siriRemoteIdleTimer = nil;
  m_siriRemoteIdleState = false;
}

- (void)setSiriRemoteIdleState
{
  m_siriRemoteIdleState = true;
}

#pragma mark - key press auto-repeat methods

- (void)startKeyPressTimer:(int)keyId
{
  [self startKeyPressTimer:keyId clickTime:REPEATED_KEYPRESS_PAUSE_S];
}

- (void)startKeyPressTimer:(int)keyId clickTime:(NSTimeInterval)interval
{
  if (m_pressAutoRepeatTimer != nil)
    [self stopKeyPressTimer];

  [g_xbmcController.inputHandler sendButtonPressed:keyId];

  NSNumber* number = @(keyId);
  auto fireDate = [NSDate dateWithTimeIntervalSinceNow:REPEATED_KEYPRESS_DELAY_S];

  // schedule repeated timer which starts after REPEATED_KEYPRESS_DELAY_S
  // and fires every REPEATED_KEYPRESS_PAUSE_S
  auto timer = [[NSTimer alloc] initWithFireDate:fireDate
                                        interval:interval
                                          target:self
                                        selector:@selector(keyPressTimerCallback:)
                                        userInfo:number
                                         repeats:YES];

  // schedule the timer to the runloop
  [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSDefaultRunLoopMode];
  m_pressAutoRepeatTimer = timer;
}

- (void)stopKeyPressTimer
{
  [m_pressAutoRepeatTimer invalidate];
  m_pressAutoRepeatTimer = nil;
}

- (void)keyPressTimerCallback:(NSTimer*)theTimer
{
  // if queue is empty - skip this timer event before letting it process
  CWinSystemTVOS* winSystem(dynamic_cast<CWinSystemTVOS*>(CServiceBroker::GetWinSystem()));
  if (!winSystem->GetQueueSize())
    [g_xbmcController.inputHandler sendButtonPressed:[theTimer.userInfo intValue]];
}

#pragma mark - remoteControlEventwith

- (void)remoteControlEvent:(UIEvent*)receivedEvent
{
  switch (receivedEvent.subtype)
  {
    case UIEventSubtypeRemoteControlTogglePlayPause:
      CServiceBroker::GetAppMessenger()->PostMsg(
          TMSG_GUI_ACTION, WINDOW_INVALID, -1,
          static_cast<void*>(new CAction(ACTION_PLAYER_PLAYPAUSE)));
      break;
    case UIEventSubtypeRemoteControlPlay:
      [g_xbmcController.inputHandler sendButtonPressed:13];
      break;
    case UIEventSubtypeRemoteControlPause:
      [g_xbmcController.inputHandler sendButtonPressed:14];
      break;
    case UIEventSubtypeRemoteControlStop:
      [g_xbmcController.inputHandler sendButtonPressed:15];
      break;
    case UIEventSubtypeRemoteControlNextTrack:
      [g_xbmcController.inputHandler sendButtonPressed:16];
      break;
    case UIEventSubtypeRemoteControlPreviousTrack:
      [g_xbmcController.inputHandler sendButtonPressed:17];
      break;
    case UIEventSubtypeRemoteControlBeginSeekingForward:
      [g_xbmcController.inputHandler sendButtonPressed:18];
      break;
    case UIEventSubtypeRemoteControlBeginSeekingBackward:
      [g_xbmcController.inputHandler sendButtonPressed:19];
      break;
    case UIEventSubtypeRemoteControlEndSeekingForward:
    case UIEventSubtypeRemoteControlEndSeekingBackward:
    {
      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      // restore to normal playback speed.
      if (appPlayer->IsPlaying() && !appPlayer->IsPaused())
        CServiceBroker::GetAppMessenger()->PostMsg(
            TMSG_GUI_ACTION, WINDOW_INVALID, -1,
            static_cast<void*>(new CAction(ACTION_PLAYER_PLAY)));
      break;
    }
    default:
      break;
  }
  // start remote timeout
  [self startSiriRemoteIdleTimer];
}

@end
