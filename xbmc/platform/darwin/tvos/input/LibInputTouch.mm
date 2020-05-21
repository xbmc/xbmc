/*
 *  Copyright (C) 2019- Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "LibInputTouch.h"

#include "Application.h"
#include "ServiceBroker.h"
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

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer*)otherGestureRecognizer
{
  if ([gestureRecognizer isKindOfClass:[UISwipeGestureRecognizer class]] &&
      [otherGestureRecognizer isKindOfClass:[UIPanGestureRecognizer class]])
  {
    return YES;
  }
  if ([gestureRecognizer isKindOfClass:[UIPanGestureRecognizer class]] &&
      [otherGestureRecognizer isKindOfClass:[UILongPressGestureRecognizer class]])
  {
    return YES;
  }
  if ([gestureRecognizer isKindOfClass:[UITapGestureRecognizer class]] &&
      [otherGestureRecognizer isKindOfClass:[UIPanGestureRecognizer class]])
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
      // menu is special.
      //  a) if at our home view, should return to atv home screen.
      //  b) if not, let it pass to us.
      if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_HOME &&
          !CServiceBroker::GetGUI()->GetWindowManager().HasVisibleModalDialog() &&
          !g_application.GetAppPlayer().IsPlaying())
        handled = NO;
      break;

    // single press keys
    case UIPressTypeSelect:
    case UIPressTypePlayPause:
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
    case UIGestureRecognizerStateBegan:
      break;
    case UIGestureRecognizerStateChanged:
      break;
    case UIGestureRecognizerStateEnded:
      [g_xbmcController.inputHandler sendButtonPressed:6];

      // start remote timeout
      [g_xbmcController.inputHandler.inputRemote startRemoteTimer];
      break;
    default:
      break;
  }
}

- (void)SiriLongSelectHandler:(UIGestureRecognizer*)sender
{
  if (sender.state == UIGestureRecognizerStateBegan)
  {
    [g_xbmcController.inputHandler sendButtonPressed:7];
    [g_xbmcController.inputHandler.inputRemote startRemoteTimer];
  }
}

- (void)SiriSelectHandler:(UITapGestureRecognizer*)sender
{
  CLog::Log(LOGDEBUG, "SiriSelectHandler");
  switch (sender.state)
  {
    case UIGestureRecognizerStateEnded:
      [g_xbmcController.inputHandler sendButtonPressed:5];
      break;
    default:
      break;
  }
}

- (void)playPausePressed:(UITapGestureRecognizer*)sender
{
  switch (sender.state)
  {
    case UIGestureRecognizerStateBegan:
      break;
    case UIGestureRecognizerStateChanged:
      break;
    case UIGestureRecognizerStateEnded:
      [g_xbmcController.inputHandler sendButtonPressed:12];
      // start remote timeout
      [g_xbmcController.inputHandler.inputRemote startRemoteTimer];
      break;
    default:
      break;
  }
}

- (void)longPlayPausePressed:(UILongPressGestureRecognizer*)sender
{
  CLog::Log(LOGDEBUG, "Input: play/pause long press, state: %ld", static_cast<long>(sender.state));
}

- (void)doublePlayPausePressed:(UITapGestureRecognizer*)sender
{
  // state is only UIGestureRecognizerStateBegan and UIGestureRecognizerStateEnded
  CLog::Log(LOGDEBUG, "Input: play/pause double press");
}

- (void)SiriDoubleSelectHandler:(UITapGestureRecognizer*)sender
{
  CLog::Log(LOGDEBUG, "Input: select double press");
}

#pragma mark - IR Arrows Pressed

- (IBAction)IRRemoteUpArrowPressed:(UIGestureRecognizer*)sender
{
  switch (sender.state)
  {
    case UIGestureRecognizerStateBegan:
      [g_xbmcController.inputHandler.inputRemote startKeyPressTimer:1];
      break;
    case UIGestureRecognizerStateChanged:
      break;
    case UIGestureRecognizerStateEnded:
      [g_xbmcController.inputHandler.inputRemote stopKeyPressTimer];
      // start remote timeout
      [g_xbmcController.inputHandler.inputRemote startRemoteTimer];
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
      [g_xbmcController.inputHandler.inputRemote startKeyPressTimer:2];
      break;
    case UIGestureRecognizerStateChanged:
      break;
    case UIGestureRecognizerStateEnded:
      [g_xbmcController.inputHandler.inputRemote stopKeyPressTimer];
      // start remote timeout
      [g_xbmcController.inputHandler.inputRemote startRemoteTimer];
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
      [g_xbmcController.inputHandler.inputRemote startKeyPressTimer:3];
      break;
    case UIGestureRecognizerStateChanged:
      break;
    case UIGestureRecognizerStateEnded:
      [g_xbmcController.inputHandler.inputRemote stopKeyPressTimer];
      // start remote timeout
      [g_xbmcController.inputHandler.inputRemote startRemoteTimer];
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
      [g_xbmcController.inputHandler.inputRemote startKeyPressTimer:4];
      break;
    case UIGestureRecognizerStateChanged:
      break;
    case UIGestureRecognizerStateEnded:
      [g_xbmcController.inputHandler.inputRemote stopKeyPressTimer];
      // start remote timeout
      [g_xbmcController.inputHandler.inputRemote startRemoteTimer];
      break;
    default:
      break;
  }
}

#pragma mark - Tap Arrows

- (IBAction)tapUpArrowPressed:(UIGestureRecognizer*)sender
{
  if (!g_xbmcController.inputHandler.inputRemote.remoteIdleState)
    [g_xbmcController.inputHandler sendButtonPressed:1];

  [g_xbmcController.inputHandler.inputRemote startRemoteTimer];
}

- (IBAction)tapDownArrowPressed:(UIGestureRecognizer*)sender
{
  if (!g_xbmcController.inputHandler.inputRemote.remoteIdleState)
    [g_xbmcController.inputHandler sendButtonPressed:2];

  [g_xbmcController.inputHandler.inputRemote startRemoteTimer];
}

- (IBAction)tapLeftArrowPressed:(UIGestureRecognizer*)sender
{
  if (!g_xbmcController.inputHandler.inputRemote.remoteIdleState)
    [g_xbmcController.inputHandler sendButtonPressed:3];

  [g_xbmcController.inputHandler.inputRemote startRemoteTimer];
}

- (IBAction)tapRightArrowPressed:(UIGestureRecognizer*)sender
{
  if (!g_xbmcController.inputHandler.inputRemote.remoteIdleState)
    [g_xbmcController.inputHandler sendButtonPressed:4];

  [g_xbmcController.inputHandler.inputRemote startRemoteTimer];
}

#pragma mark - Pan

- (IBAction)handlePan:(UIPanGestureRecognizer*)sender
{
  if (g_xbmcController.inputHandler.inputRemote.remoteIdleState)
    return;

  if (!g_xbmcController.appAlive) //NO GESTURES BEFORE WE ARE UP AND RUNNING
    return;

  if ([g_xbmcController.inputHandler.inputSettings useSiriRemote])
  {
    static UIPanGestureRecognizerDirection direction = UIPanGestureRecognizerDirectionUndefined;
    // speed       == how many clicks full swipe will give us(1000x1000px)
    // minVelocity == min velocity to trigger fast scroll, add this to settings?
    float speed = 240.0f;
    float minVelocity = 1300.0f;
    switch (sender.state)
    {

      case UIGestureRecognizerStateBegan:
      {

        if (direction == UIPanGestureRecognizerDirectionUndefined)
        {
          m_lastGesturePoint = [sender translationInView:sender.view];
          m_lastGesturePoint.x = m_lastGesturePoint.x / 1.92;
          m_lastGesturePoint.y = m_lastGesturePoint.y / 1.08;

          m_direction = [self getPanDirection:m_lastGesturePoint];
          m_directionOverride = false;
        }
        break;
      }
      case UIGestureRecognizerStateChanged:
      {
        CGPoint gesturePoint = [sender translationInView:sender.view];
        gesturePoint.x = gesturePoint.x / 1.92;
        gesturePoint.y = gesturePoint.y / 1.08;

        CGPoint gestureMovement;
        gestureMovement.x = gesturePoint.x - m_lastGesturePoint.x;
        gestureMovement.y = gesturePoint.y - m_lastGesturePoint.y;
        direction = [self getPanDirection:gestureMovement];

        CGPoint velocity = [sender velocityInView:sender.view];
        CGFloat velocityX = (0.2 * velocity.x);
        CGFloat velocityY = (0.2 * velocity.y);

        if (ABS(velocityY) > minVelocity || ABS(velocityX) > minVelocity || m_directionOverride)
        {
          direction = m_direction;
          // Override direction to correct swipe errors
          m_directionOverride = true;
        }
        switch (direction)
        {
          case UIPanGestureRecognizerDirectionUp:
          {
            if ((ABS(m_lastGesturePoint.y - gesturePoint.y) > speed) ||
                ABS(velocityY) > minVelocity)
            {
              [g_xbmcController.inputHandler sendButtonPressed:8];
              if (ABS(velocityY) > minVelocity && [self shouldFastScroll])
                [g_xbmcController.inputHandler sendButtonPressed:8];

              m_lastGesturePoint = gesturePoint;
            }
            break;
          }
          case UIPanGestureRecognizerDirectionDown:
          {
            if ((ABS(m_lastGesturePoint.y - gesturePoint.y) > speed) ||
                ABS(velocityY) > minVelocity)
            {
              [g_xbmcController.inputHandler sendButtonPressed:9];
              if (ABS(velocityY) > minVelocity && [self shouldFastScroll])
                [g_xbmcController.inputHandler sendButtonPressed:9];

              m_lastGesturePoint = gesturePoint;
            }
            break;
          }
          case UIPanGestureRecognizerDirectionLeft:
          {
            // add 80 px to slow left/right swipes, it matched up down better
            if ((ABS(m_lastGesturePoint.x - gesturePoint.x) > speed + 80) ||
                ABS(velocityX) > minVelocity)
            {
              [g_xbmcController.inputHandler sendButtonPressed:10];
              if (ABS(velocityX) > minVelocity && [self shouldFastScroll])
                [g_xbmcController.inputHandler sendButtonPressed:10];

              m_lastGesturePoint = gesturePoint;
            }
            break;
          }
          case UIPanGestureRecognizerDirectionRight:
          {
            // add 80 px to slow left/right swipes, it matched up down better
            if ((ABS(m_lastGesturePoint.x - gesturePoint.x) > speed + 80) ||
                ABS(velocityX) > minVelocity)
            {
              [g_xbmcController.inputHandler sendButtonPressed:11];
              if (ABS(velocityX) > minVelocity && [self shouldFastScroll])
                [g_xbmcController.inputHandler sendButtonPressed:11];

              m_lastGesturePoint = gesturePoint;
            }
            break;
          }
          default:
            break;
        }
      }
      case UIGestureRecognizerStateEnded:
      {
        direction = UIPanGestureRecognizerDirectionUndefined;
        // start remote idle timer
        [g_xbmcController.inputHandler.inputRemote startRemoteTimer];
        break;
      }
      default:
        break;
    }
  }
  else // dont mimic apple siri remote
  {
    switch (sender.state)
    {
      case UIGestureRecognizerStateBegan:
      {
        m_touchBeginSignaled = false;
        break;
      }
      case UIGestureRecognizerStateChanged:
      {
        int keyId = 0;
        if (!m_touchBeginSignaled && m_touchDirection)
        {
          switch (m_touchDirection)
          {
            case UISwipeGestureRecognizerDirectionRight:
            {
              keyId = 11;
              break;
            }
            case UISwipeGestureRecognizerDirectionLeft:
            {
              keyId = 10;
              break;
            }
            case UISwipeGestureRecognizerDirectionUp:
            {
              keyId = 8;
              break;
            }
            case UISwipeGestureRecognizerDirectionDown:
            {
              keyId = 9;
              break;
            }
            default:
              break;
          }
          m_touchBeginSignaled = true;
          [g_xbmcController.inputHandler.inputRemote startKeyPressTimer:keyId];
        }
        break;
      }
      case UIGestureRecognizerStateEnded:
      case UIGestureRecognizerStateCancelled:
      {
        if (m_touchBeginSignaled)
        {
          m_touchBeginSignaled = false;
          m_touchDirection = NULL;
          [g_xbmcController.inputHandler.inputRemote stopKeyPressTimer];
        }
        // start remote idle timer
        [g_xbmcController.inputHandler.inputRemote startRemoteTimer];
        break;
      }
      default:
        break;
    }
  }
}

- (IBAction)handleSwipe:(UISwipeGestureRecognizer*)sender
{
  if (!g_xbmcController.inputHandler.inputRemote.remoteIdleState)
    m_touchDirection = sender.direction;

  // start remote idle timer
  [g_xbmcController.inputHandler.inputRemote startRemoteTimer];
}

- (UIPanGestureRecognizerDirection)getPanDirection:(CGPoint)translation
{
  int x = static_cast<int>(translation.x);
  int y = static_cast<int>(translation.y);
  int absX = x;
  int absY = y;

  if (absX < 0)
    absX *= -1;

  if (absY < 0)
    absY *= -1;

  bool horizontal, veritical;
  horizontal = (absX > absY);
  veritical = !horizontal;

  // Determine up, down, right, or left:
  bool swipe_up, swipe_down, swipe_left, swipe_right;
  swipe_left = (horizontal && x < 0);
  swipe_right = (horizontal && x >= 0);
  swipe_up = (veritical && y < 0);
  swipe_down = (veritical && y >= 0);

  if (swipe_down)
    return UIPanGestureRecognizerDirectionDown;
  if (swipe_up)
    return UIPanGestureRecognizerDirectionUp;
  if (swipe_left)
    return UIPanGestureRecognizerDirectionLeft;
  if (swipe_right)
    return UIPanGestureRecognizerDirectionRight;

  return UIPanGestureRecognizerDirectionUndefined;
}

#pragma mark - Private Functions

- (BOOL)shouldFastScroll
{
  // we dont want fast scroll in below windows, no point in going 15 places in home screen
  int window = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();

  if (window == WINDOW_HOME || window == WINDOW_FULLSCREEN_LIVETV ||
      window == WINDOW_FULLSCREEN_VIDEO || window == WINDOW_FULLSCREEN_RADIO ||
      (window >= WINDOW_SETTINGS_START && window <= WINDOW_SETTINGS_SERVICE))
    return NO;

  return YES;
}

@end
