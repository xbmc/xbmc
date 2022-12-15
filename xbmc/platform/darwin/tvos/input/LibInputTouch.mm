/*
 *  Copyright (C) 2019- Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "LibInputTouch.h"

#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

#import "platform/darwin/tvos/TVOSEAGLView.h"
#import "platform/darwin/tvos/XBMCController.h"
#import "platform/darwin/tvos/input/LibInputHandler.h"
#import "platform/darwin/tvos/input/LibInputRemote.h"
#import "platform/darwin/tvos/input/LibInputSettings.h"

#include <tuple>

#import <UIKit/UIKit.h>

@class XBMCController;

@implementation TVOSLibInputTouch

#pragma mark - gesture methods

// called before any press or touch event
- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
       shouldReceiveEvent:(nonnull UIEvent*)event
{
  // allow press or touch event only if we are up and running
  if (g_xbmcController.appAlive)
    return YES;
  return NO;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer*)otherGestureRecognizer
{
  // an high speed move in specific direction should trigger pan AND swipe gesture
  if (([gestureRecognizer isKindOfClass:[UISwipeGestureRecognizer class]] &&
       [otherGestureRecognizer isKindOfClass:[UIPanGestureRecognizer class]]) ||
      ([gestureRecognizer isKindOfClass:[UIPanGestureRecognizer class]] &&
       [otherGestureRecognizer isKindOfClass:[UISwipeGestureRecognizer class]]))
  {
    return YES;
  }
  return NO;
}

// called before pressesBegan:withEvent: is called on the gesture recognizer
// for a new press. return NO to prevent the gesture recognizer from seeing this press
- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer shouldReceivePress:(UIPress*)press
{
  BOOL handled = YES;
  switch (press.type)
  {
    // single press key, but also detect hold and back to tvos.
    case UIPressTypeMenu:
    {
      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      // menu is special.
      //  a) if at our home view, should return to atv home screen.
      //  b) if not, let it pass to us.
      if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_HOME &&
          !CServiceBroker::GetGUI()->GetWindowManager().HasVisibleModalDialog() &&
          !appPlayer->IsPlaying())
        handled = NO;
      break;
    }

    // single press keys
    case UIPressTypeSelect:
    case UIPressTypePlayPause:
    case UIPressTypePageUp:
    case UIPressTypePageDown:
      break;

    // auto-repeat keys
    case UIPressTypeUpArrow:
    case UIPressTypeDownArrow:
    case UIPressTypeLeftArrow:
    case UIPressTypeRightArrow:
      break;

    default:
      handled = NO;
  }

  return handled;
}

- (void)createSwipeGestureRecognizers
{
  for (auto swipeDirection :
       {UISwipeGestureRecognizerDirectionLeft, UISwipeGestureRecognizerDirectionRight,
        UISwipeGestureRecognizerDirectionUp, UISwipeGestureRecognizerDirectionDown})
  {
    auto swipeRecognizer =
        [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(handleSwipe:)];
    swipeRecognizer.delaysTouchesBegan = NO;
    swipeRecognizer.direction = swipeDirection;
    swipeRecognizer.delegate = self;
    [g_xbmcController.glView addGestureRecognizer:swipeRecognizer];
  }
}

- (void)createPanGestureRecognizers
{
  // for pan gestures with one finger
  auto pan = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handlePan:)];
  pan.delegate = self;
  [g_xbmcController.glView addGestureRecognizer:pan];
}

- (void)createTapGesturecognizers
{
  // tap side of siri remote pad
  for (auto t : {
         std::make_tuple(UIPressTypeUpArrow, @selector(tapUpArrowPressed:),
                         @selector(IRRemoteUpArrowPressed:)),
             std::make_tuple(UIPressTypeDownArrow, @selector(tapDownArrowPressed:),
                             @selector(IRRemoteDownArrowPressed:)),
             std::make_tuple(UIPressTypeLeftArrow, @selector(tapLeftArrowPressed:),
                             @selector(IRRemoteLeftArrowPressed:)),
             std::make_tuple(UIPressTypeRightArrow, @selector(tapRightArrowPressed:),
                             @selector(IRRemoteRightArrowPressed:))
       })
  {
    auto allowedPressTypes = @[ @(std::get<0>(t)) ];

    auto arrowRecognizer =
        [[UITapGestureRecognizer alloc] initWithTarget:self action:std::get<1>(t)];
    arrowRecognizer.allowedPressTypes = allowedPressTypes;
    arrowRecognizer.delegate = self;
    [g_xbmcController.glView addGestureRecognizer:arrowRecognizer];

    // @todo doesn't seem to work
    // we need UILongPressGestureRecognizer here because it will give
    // UIGestureRecognizerStateBegan AND UIGestureRecognizerStateEnded
    // even if we hold down for a long time. UITapGestureRecognizer
    // will eat the ending on long holds and we never see it.
    auto longArrowRecognizer =
        [[UILongPressGestureRecognizer alloc] initWithTarget:self action:std::get<2>(t)];
    longArrowRecognizer.allowedPressTypes = allowedPressTypes;
    longArrowRecognizer.minimumPressDuration = 0.01;
    longArrowRecognizer.delegate = self;
    [g_xbmcController.glView addGestureRecognizer:longArrowRecognizer];
  }
}

- (void)createPressGesturecognizers
{
  auto menuRecognizer =
      [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(menuPressed:)];
  menuRecognizer.allowedPressTypes = @[ @(UIPressTypeMenu) ];
  menuRecognizer.delegate = self;
  [g_xbmcController.glView addGestureRecognizer:menuRecognizer];

  if (@available(tvOS 14.3, *)) {
    auto pageUpTypes = @[ @(UIPressTypePageUp) ];
    auto pageUpRecognizer =
        [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(pageUpPressed:)];
    pageUpRecognizer.allowedPressTypes = pageUpTypes;
    pageUpRecognizer.delegate = self;
    [g_xbmcController.glView addGestureRecognizer:pageUpRecognizer];

    auto pageDownTypes = @[ @(UIPressTypePageDown) ];
    auto pageDownRecognizer =
        [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(pageDownPressed:)];
    pageDownRecognizer.allowedPressTypes = pageDownTypes;
    pageDownRecognizer.delegate = self;
    [g_xbmcController.glView addGestureRecognizer:pageDownRecognizer];
  }


  auto playPauseTypes = @[ @(UIPressTypePlayPause) ];
  auto playPauseRecognizer =
      [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(playPausePressed:)];
  playPauseRecognizer.allowedPressTypes = playPauseTypes;
  playPauseRecognizer.delegate = self;
  [g_xbmcController.glView addGestureRecognizer:playPauseRecognizer];

  auto doublePlayPauseRecognizer =
      [[UITapGestureRecognizer alloc] initWithTarget:self
                                              action:@selector(doublePlayPausePressed:)];
  doublePlayPauseRecognizer.allowedPressTypes = playPauseTypes;
  doublePlayPauseRecognizer.numberOfTapsRequired = 2;
  doublePlayPauseRecognizer.delegate = self;
  [g_xbmcController.glView.gestureRecognizers.lastObject
      requireGestureRecognizerToFail:doublePlayPauseRecognizer];
  [g_xbmcController.glView addGestureRecognizer:doublePlayPauseRecognizer];

  auto longPlayPauseRecognizer =
      [[UILongPressGestureRecognizer alloc] initWithTarget:self
                                                    action:@selector(longPlayPausePressed:)];
  longPlayPauseRecognizer.allowedPressTypes = playPauseTypes;
  longPlayPauseRecognizer.delegate = self;
  [g_xbmcController.glView addGestureRecognizer:longPlayPauseRecognizer];

  auto selectTypes = @[ @(UIPressTypeSelect) ];
  auto longSelectRecognizer =
      [[UILongPressGestureRecognizer alloc] initWithTarget:self
                                                    action:@selector(SiriLongSelectHandler:)];
  longSelectRecognizer.allowedPressTypes = selectTypes;
  longSelectRecognizer.minimumPressDuration = 0.001;
  longSelectRecognizer.delegate = self;
  [g_xbmcController.glView addGestureRecognizer:longSelectRecognizer];

  auto selectRecognizer =
      [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(SiriSelectHandler:)];
  selectRecognizer.allowedPressTypes = selectTypes;
  selectRecognizer.delegate = self;
  [longSelectRecognizer requireGestureRecognizerToFail:selectRecognizer];
  [g_xbmcController.glView addGestureRecognizer:selectRecognizer];

  auto doubleSelectRecognizer =
      [[UITapGestureRecognizer alloc] initWithTarget:self
                                              action:@selector(SiriDoubleSelectHandler:)];
  doubleSelectRecognizer.allowedPressTypes = selectTypes;
  doubleSelectRecognizer.numberOfTapsRequired = 2;
  doubleSelectRecognizer.delegate = self;
  [longSelectRecognizer requireGestureRecognizerToFail:doubleSelectRecognizer];
  [g_xbmcController.glView.gestureRecognizers.lastObject
      requireGestureRecognizerToFail:doubleSelectRecognizer];
  [g_xbmcController.glView addGestureRecognizer:doubleSelectRecognizer];
}

- (void)menuPressed:(UITapGestureRecognizer*)sender
{
  switch (sender.state)
  {
    case UIGestureRecognizerStateEnded:
      CLog::Log(LOGDEBUG, "Input: Siri remote menu press (id: 6)");
      [g_xbmcController.inputHandler sendButtonPressed:6];
      [g_xbmcController.inputHandler.inputRemote startSiriRemoteIdleTimer];
      break;
    default:
      break;
  }
}

- (void)SiriLongSelectHandler:(UIGestureRecognizer*)sender
{
  switch (sender.state)
  {
    case UIGestureRecognizerStateBegan:
      CLog::Log(LOGDEBUG, "Input: Siri remote select long press (id: 7)");
      [g_xbmcController.inputHandler sendButtonPressed:7];
      [g_xbmcController.inputHandler.inputRemote startSiriRemoteIdleTimer];
      break;
    default:
      break;
  }
}

- (void)SiriSelectHandler:(UITapGestureRecognizer*)sender
{
  switch (sender.state)
  {
    case UIGestureRecognizerStateEnded:
      CLog::Log(LOGDEBUG, "Input: Siri remote select press (id: 5)");
      [g_xbmcController.inputHandler sendButtonPressed:5];
      [g_xbmcController.inputHandler.inputRemote startSiriRemoteIdleTimer];
      break;
    default:
      break;
  }
}

- (void)pageUpPressed:(UITapGestureRecognizer*)sender
{
  switch (sender.state)
  {
    case UIGestureRecognizerStateEnded:
      CLog::Log(LOGDEBUG, "Input: Siri remote page up press (id: 27)");
      [g_xbmcController.inputHandler sendButtonPressed:27];
      [g_xbmcController.inputHandler.inputRemote startSiriRemoteIdleTimer];
      break;
    default:
      break;
  }
}

- (void)pageDownPressed:(UITapGestureRecognizer*)sender
{
  switch (sender.state)
  {
    case UIGestureRecognizerStateEnded:
      CLog::Log(LOGDEBUG, "Input: Siri remote page down press (id: 28)");
      [g_xbmcController.inputHandler sendButtonPressed:28];
      [g_xbmcController.inputHandler.inputRemote startSiriRemoteIdleTimer];
      break;
    default:
      break;
  }
}

- (void)playPausePressed:(UITapGestureRecognizer*)sender
{
  switch (sender.state)
  {
    case UIGestureRecognizerStateEnded:
      CLog::Log(LOGDEBUG, "Input: Siri remote play/pause press (id: 12)");
      [g_xbmcController.inputHandler sendButtonPressed:12];
      [g_xbmcController.inputHandler.inputRemote startSiriRemoteIdleTimer];
      break;
    default:
      break;
  }
}

- (void)longPlayPausePressed:(UILongPressGestureRecognizer*)sender
{
  switch (sender.state)
  {
    case UIGestureRecognizerStateBegan:
      CLog::Log(LOGDEBUG, "Input: Siri remote play/pause long press (id: 20)");
      [g_xbmcController.inputHandler sendButtonPressed:20];
      [g_xbmcController.inputHandler.inputRemote startSiriRemoteIdleTimer];
      break;
    default:
      break;
  }
}

- (void)doublePlayPausePressed:(UITapGestureRecognizer*)sender
{
  switch (sender.state)
  {
    case UIGestureRecognizerStateEnded:
      CLog::Log(LOGDEBUG, "Input: Siri remote play/pause double press (id: 21)");
      [g_xbmcController.inputHandler sendButtonPressed:21];
      [g_xbmcController.inputHandler.inputRemote startSiriRemoteIdleTimer];
      break;
    default:
      break;
  }
}

- (void)SiriDoubleSelectHandler:(UITapGestureRecognizer*)sender
{
  switch (sender.state)
  {
    case UIGestureRecognizerStateEnded:
      CLog::Log(LOGDEBUG, "Input: Siri remote select double press (id: 22)");
      [g_xbmcController.inputHandler sendButtonPressed:22];
      [g_xbmcController.inputHandler.inputRemote startSiriRemoteIdleTimer];
      break;
    default:
      break;
  }
}

#pragma mark - IR Arrows Pressed

- (IBAction)IRRemoteUpArrowPressed:(UIGestureRecognizer*)sender
{
  switch (sender.state)
  {
    case UIGestureRecognizerStateBegan:
      CLog::Log(LOGDEBUG, "Input: IR remote up press (id: 1)");
      [g_xbmcController.inputHandler.inputRemote startKeyPressTimer:1];
      break;
    case UIGestureRecognizerStateChanged:
      break;
    case UIGestureRecognizerStateEnded:
      [g_xbmcController.inputHandler.inputRemote stopKeyPressTimer];
      [g_xbmcController.inputHandler.inputRemote startSiriRemoteIdleTimer];
      break;
    default:
      break;
  }
}

- (IBAction)IRRemoteDownArrowPressed:(UIGestureRecognizer*)sender
{
  switch (sender.state)
  {
    case UIGestureRecognizerStateBegan:
      CLog::Log(LOGDEBUG, "Input: IR remote down press (id: 2)");
      [g_xbmcController.inputHandler.inputRemote startKeyPressTimer:2];
      break;
    case UIGestureRecognizerStateChanged:
      break;
    case UIGestureRecognizerStateEnded:
      [g_xbmcController.inputHandler.inputRemote stopKeyPressTimer];
      [g_xbmcController.inputHandler.inputRemote startSiriRemoteIdleTimer];
      break;
    default:
      break;
  }
}

- (IBAction)IRRemoteLeftArrowPressed:(UIGestureRecognizer*)sender
{
  switch (sender.state)
  {
    case UIGestureRecognizerStateBegan:
      CLog::Log(LOGDEBUG, "Input: IR remote left press (id: 3)");
      [g_xbmcController.inputHandler.inputRemote startKeyPressTimer:3];
      break;
    case UIGestureRecognizerStateChanged:
      break;
    case UIGestureRecognizerStateEnded:
      [g_xbmcController.inputHandler.inputRemote stopKeyPressTimer];
      [g_xbmcController.inputHandler.inputRemote startSiriRemoteIdleTimer];
      break;
    default:
      break;
  }
}

- (IBAction)IRRemoteRightArrowPressed:(UIGestureRecognizer*)sender
{
  switch (sender.state)
  {
    case UIGestureRecognizerStateBegan:
      CLog::Log(LOGDEBUG, "Input: IR remote right press (id: 4)");
      [g_xbmcController.inputHandler.inputRemote startKeyPressTimer:4];
      break;
    case UIGestureRecognizerStateChanged:
      break;
    case UIGestureRecognizerStateEnded:
      [g_xbmcController.inputHandler.inputRemote stopKeyPressTimer];
      [g_xbmcController.inputHandler.inputRemote startSiriRemoteIdleTimer];
      break;
    default:
      break;
  }
}

#pragma mark - Tap Arrows

- (IBAction)tapUpArrowPressed:(UIGestureRecognizer*)sender
{
  CLog::Log(LOGDEBUG, "Input: Siri remote tap up (id: 1)");
  if (!g_xbmcController.inputHandler.inputRemote.siriRemoteIdleState)
    [g_xbmcController.inputHandler sendButtonPressed:1];

  [g_xbmcController.inputHandler.inputRemote startSiriRemoteIdleTimer];
}

- (IBAction)tapDownArrowPressed:(UIGestureRecognizer*)sender
{
  CLog::Log(LOGDEBUG, "Input: Siri remote tap down (id: 2)");
  if (!g_xbmcController.inputHandler.inputRemote.siriRemoteIdleState)
    [g_xbmcController.inputHandler sendButtonPressed:2];

  [g_xbmcController.inputHandler.inputRemote startSiriRemoteIdleTimer];
}

- (IBAction)tapLeftArrowPressed:(UIGestureRecognizer*)sender
{
  CLog::Log(LOGDEBUG, "Input: Siri remote tap left (id: 3)");
  if (!g_xbmcController.inputHandler.inputRemote.siriRemoteIdleState)
    [g_xbmcController.inputHandler sendButtonPressed:3];

  [g_xbmcController.inputHandler.inputRemote startSiriRemoteIdleTimer];
}

- (IBAction)tapRightArrowPressed:(UIGestureRecognizer*)sender
{
  CLog::Log(LOGDEBUG, "Input: Siri remote tap right (id: 4)");
  if (!g_xbmcController.inputHandler.inputRemote.siriRemoteIdleState)
    [g_xbmcController.inputHandler sendButtonPressed:4];

  [g_xbmcController.inputHandler.inputRemote startSiriRemoteIdleTimer];
}

#pragma mark - Pan

- (IBAction)handlePan:(UIPanGestureRecognizer*)sender
{
  if (g_xbmcController.inputHandler.inputRemote.siriRemoteIdleState)
  {
    [g_xbmcController.inputHandler.inputRemote startSiriRemoteIdleTimer];
    return;
  }

  auto translation = [sender translationInView:sender.view];
  auto velocity = [sender velocityInView:sender.view];
  auto direction = [self getPanDirection:velocity];
  const auto maxSensitivity = 1500;

  switch (sender.state)
  {
    case UIGestureRecognizerStateBegan:
    {
      m_lastGesturePoint = translation;
      break;
    }
    case UIGestureRecognizerStateChanged:
    {
      int keyId = 0;
      switch (direction)
      {
        case UIPanGestureRecognizerDirectionUp:
        {
          if (fabs(m_lastGesturePoint.y - translation.y) >
              maxSensitivity - g_xbmcController.inputHandler.inputSettings.siriRemoteVerticalSensitivity)
          {
            CLog::Log(LOGDEBUG, "Input: Siri remote pan up (id: 23)");
            keyId = 23;
          }
          break;
        }
        case UIPanGestureRecognizerDirectionDown:
        {
          if (fabs(m_lastGesturePoint.y - translation.y) >
              maxSensitivity - g_xbmcController.inputHandler.inputSettings.siriRemoteVerticalSensitivity)
          {
            CLog::Log(LOGDEBUG, "Input: Siri remote pan down (id: 24)");
            keyId = 24;
          }
          break;
        }
        case UIPanGestureRecognizerDirectionLeft:
        {
          if (fabs(m_lastGesturePoint.x - translation.x) >
              maxSensitivity - g_xbmcController.inputHandler.inputSettings.siriRemoteHorizontalSensitivity)
          {
            CLog::Log(LOGDEBUG, "Input: Siri remote pan left (id: 25)");
            keyId = 25;
          }
          break;
        }
        case UIPanGestureRecognizerDirectionRight:
        {
          if (fabs(m_lastGesturePoint.x - translation.x) >
              maxSensitivity - g_xbmcController.inputHandler.inputSettings.siriRemoteHorizontalSensitivity)
          {
            CLog::Log(LOGDEBUG, "Input: Siri remote pan right (id: 26)");
            keyId = 26;
          }
          break;
        }
        default:
        {
          break;
        }
      }
      if (keyId != 0)
      {
        m_lastGesturePoint = translation;
        [g_xbmcController.inputHandler sendButtonPressed:keyId];
      }
      break;
    }
    default:
      break;
  }
  [g_xbmcController.inputHandler.inputRemote startSiriRemoteIdleTimer];
}

- (IBAction)handleSwipe:(UISwipeGestureRecognizer*)sender
{
  if (g_xbmcController.inputHandler.inputRemote.siriRemoteIdleState)
  {
    [g_xbmcController.inputHandler.inputRemote startSiriRemoteIdleTimer];
    return;
  }

  int keyId = 0;
  switch (sender.direction)
  {
    case UISwipeGestureRecognizerDirectionUp:
    {
      CLog::Log(LOGDEBUG, "Input: Siri remote swipe up (id: 8)");
      keyId = 8;
      break;
    }
    case UISwipeGestureRecognizerDirectionDown:
    {
      CLog::Log(LOGDEBUG, "Input: Siri remote swipe down (id: 9)");
      keyId = 9;
      break;
    }
    case UISwipeGestureRecognizerDirectionLeft:
    {
      CLog::Log(LOGDEBUG, "Input: Siri remote swipe left (id: 10)");
      keyId = 10;
      break;
    }
    case UISwipeGestureRecognizerDirectionRight:
    {
      CLog::Log(LOGDEBUG, "Input: Siri remote swipe right (id: 11)");
      keyId = 11;
      break;
    }
    default:
      break;
  }
  [g_xbmcController.inputHandler sendButtonPressed:keyId];
  [g_xbmcController.inputHandler.inputRemote startSiriRemoteIdleTimer];
}

- (UIPanGestureRecognizerDirection)getPanDirection:(CGPoint)velocity
{
  bool isVerticalGesture = fabs(velocity.y) > fabs(velocity.x);

  if (isVerticalGesture)
  {
    if (velocity.y > 0)
      return UIPanGestureRecognizerDirectionDown;
    else
      return UIPanGestureRecognizerDirectionUp;
  }
  else
  {
    if (velocity.x > 0)
      return UIPanGestureRecognizerDirectionRight;
    else
      return UIPanGestureRecognizerDirectionLeft;
  }
}

@end
