/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import <UIKit/UIKit.h>


@interface IOSExternalTouchController : UIViewController
{
  UIWindow      *_internalWindow;
  UIView        *_touchView;
  NSTimer       *_sleepTimer;
}
- (id)init;
- (void)createGestureRecognizers;
- (void)fadeToBlack;
- (void)fadeFromBlack;
- (void)startSleepTimer;
@end
