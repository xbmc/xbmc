/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "input/keyboard/XBMC_keysym.h"
#include "windowing/XBMC_events.h"

#import "platform/darwin/ios-common/DarwinEmbedNowPlayingInfoManager.h"

#import <AudioToolbox/AudioToolbox.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/gl.h>
#import <UIKit/UIKit.h>

@class IOSEAGLView;

@interface XBMCController : UIViewController <UIGestureRecognizerDelegate, UIKeyInput>
{
  UIWindow *m_window;
  IOSEAGLView  *m_glView;
  int m_screensaverTimeout;

  /* Touch handling */
  CGSize screensize;
  CGPoint lastGesturePoint;
  CGFloat screenScale;
  bool touchBeginSignaled;
  int  m_screenIdx;

  UIInterfaceOrientation orientation;

  bool m_isPlayingBeforeInactive;
  UIBackgroundTaskIdentifier m_bgTask;
  NSTimer *m_networkAutoSuspendTimer;
  bool nativeKeyboardActive;
}
@property (readonly, nonatomic, getter=isAnimating) BOOL animating;
@property CGPoint lastGesturePoint;
@property CGFloat screenScale;
@property bool touchBeginSignaled;
@property int  m_screenIdx;
@property CGSize screensize;
@property(nonatomic, strong) NSTimer* m_networkAutoSuspendTimer;
@property(nonatomic, strong) DarwinEmbedNowPlayingInfoManager* MPNPInfoManager;
@property bool nativeKeyboardActive;

// message from which our instance is obtained
- (void) pauseAnimation;
- (void) resumeAnimation;
- (void) startAnimation;
- (void) stopAnimation;
- (void) enterBackground;
- (void) enterForeground;
- (void) becomeInactive;
- (void) sendKey: (XBMCKey) key;
- (void) observeDefaultCenterStuff: (NSNotification *) notification;
- (CGRect)fullscreenSubviewFrame;
- (void)onXbmcAlive;
- (void)setGUIInsetsFromMainThread:(BOOL)isMainThread;
- (void) setFramebuffer;
- (bool) presentFramebuffer;
- (CGSize) getScreenSize;
- (UIInterfaceOrientation) getOrientation;
- (void) createGestureRecognizers;
- (void) activateKeyboard:(UIView *)view;
- (void) deactivateKeyboard:(UIView *)view;
- (void) nativeKeyboardActive: (bool)active;

- (void) disableNetworkAutoSuspend;
- (void) enableNetworkAutoSuspend:(id)obj;
- (void)rescheduleNetworkAutoSuspend;
- (void) disableSystemSleep;
- (void) enableSystemSleep;
- (void) disableScreenSaver;
- (void) enableScreenSaver;
- (bool) changeScreen: (unsigned int)screenIdx withMode:(UIScreenMode *)mode;
- (void) activateScreen: (UIScreen *)screen withOrientation:(UIInterfaceOrientation)newOrientation;
- (id)   initWithFrame:(CGRect)frame withScreen:(UIScreen *)screen;
- (CVEAGLContext)getEAGLContextObj;
@end

extern XBMCController *g_xbmcController;
