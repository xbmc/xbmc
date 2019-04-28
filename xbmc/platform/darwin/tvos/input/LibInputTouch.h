/*
 *  Copyright (C) 2019- Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import <UIKit/UIKit.h>

typedef NS_ENUM(NSUInteger, UIPanGestureRecognizerDirection) {
  UIPanGestureRecognizerDirectionUndefined,
  UIPanGestureRecognizerDirectionUp,
  UIPanGestureRecognizerDirectionDown,
  UIPanGestureRecognizerDirectionLeft,
  UIPanGestureRecognizerDirectionRight
};

@interface TVOSLibInputTouch : NSObject <UIGestureRecognizerDelegate>
{
  UIPanGestureRecognizerDirection m_direction;
  BOOL m_directionOverride;
  CGPoint m_lastGesturePoint;
  unsigned long m_touchDirection;
  BOOL m_touchBeginSignaled;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer*)otherGestureRecognizer;
- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
       shouldReceivePress:(UIPress*)press;
- (void)createSwipeGestureRecognizers;
- (void)createPanGestureRecognizers;
- (void)createTapGesturecognizers;
- (void)createPressGesturecognizers;
- (void)menuPressed:(UITapGestureRecognizer*)sender;
- (void)SiriLongSelectHandler:(UIGestureRecognizer*)sender;
- (void)SiriSelectHandler:(UITapGestureRecognizer*)sender;
- (void)playPausePressed:(UITapGestureRecognizer*)sender;
- (void)longPlayPausePressed:(UILongPressGestureRecognizer*)sender;
- (void)doublePlayPausePressed:(UITapGestureRecognizer*)sender;
- (void)SiriDoubleSelectHandler:(UITapGestureRecognizer*)sender;
- (IBAction)IRRemoteUpArrowPressed:(UIGestureRecognizer*)sender;
- (IBAction)IRRemoteDownArrowPressed:(UIGestureRecognizer*)sender;
- (IBAction)IRRemoteLeftArrowPressed:(UIGestureRecognizer*)sender;
- (IBAction)IRRemoteRightArrowPressed:(UIGestureRecognizer*)sender;
- (IBAction)tapUpArrowPressed:(UIGestureRecognizer*)sender;
- (IBAction)tapDownArrowPressed:(UIGestureRecognizer*)sender;
- (IBAction)tapLeftArrowPressed:(UIGestureRecognizer*)sender;
- (IBAction)tapRightArrowPressed:(UIGestureRecognizer*)sender;
- (IBAction)handlePan:(UIPanGestureRecognizer*)sender;
- (IBAction)handleSwipe:(UISwipeGestureRecognizer*)sender;
- (UIPanGestureRecognizerDirection)getPanDirection:(CGPoint)translation;
- (BOOL)shouldFastScroll;

@end
