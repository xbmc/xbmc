/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "windowing/XBMC_events.h"

#import <UIKit/UIKit.h>

typedef enum
{
  IOS_PLAYBACK_STOPPED,
  IOS_PLAYBACK_PAUSED,
  IOS_PLAYBACK_PLAYING
} IOSPlaybackState;

typedef NS_ENUM(NSUInteger, UIPanGestureRecognizerDirection)
{
  UIPanGestureRecognizerDirectionUndefined,
  UIPanGestureRecognizerDirectionUp,
  UIPanGestureRecognizerDirectionDown,
  UIPanGestureRecognizerDirectionLeft,
  UIPanGestureRecognizerDirectionRight
};

@class MainEAGLView;

@interface MainController : UIViewController <UIGestureRecognizerDelegate>
{
@private
  UIWindow* m_window;
  MainEAGLView* m_glView;
  // Touch handling
  CGSize m_screensize;
  CGPoint m_lastGesturePoint;
  CGFloat m_screenScale;
  int m_screenIdx;
  int m_currentClick;

  bool m_isPlayingBeforeInactive;
  UIBackgroundTaskIdentifier m_bgTask;
  IOSPlaybackState m_playbackState;
  NSDictionary* m_nowPlayingInfo;

  BOOL m_pause;
  BOOL m_appAlive;
  BOOL m_animating;
  BOOL m_disableIdleTimer;
  NSConditionLock* m_animationThreadLock;
  NSThread* m_animationThread;
  BOOL m_directionOverride;
  BOOL m_mimicAppleSiri;
  XBMCKey m_currentKey;
  BOOL m_clickResetPan;
  BOOL m_remoteIdleState;
  CGFloat m_remoteIdleTimeout;
  BOOL m_shouldRemoteIdle;
  BOOL m_RemoteOSDSwipes;
  unsigned long m_touchDirection;
  bool m_touchBeginSignaled;
  UIPanGestureRecognizerDirection m_direction;
}
// why are these properties ?
@property (nonatomic, strong) NSTimer* m_holdTimer;
@property (nonatomic, retain) NSDictionary* m_nowPlayingInfo;
@property int m_holdCounter;
@property CGPoint m_lastGesturePoint;
@property CGFloat m_screenScale;
@property XBMCKey m_currentKey;
@property int m_screenIdx;
@property CGSize m_screensize;
@property BOOL m_directionOverride;
@property BOOL m_mimicAppleSiri;
@property BOOL m_clickResetPan;
@property BOOL m_remoteIdleState;
@property CGFloat m_remoteIdleTimeout;
@property BOOL m_shouldRemoteIdle;
@property BOOL m_RemoteOSDSwipes;
@property unsigned long m_touchDirection;
@property bool m_touchBeginSignaled;
@property UIPanGestureRecognizerDirection m_direction;

- (void) pauseAnimation;
- (void) resumeAnimation;
- (void) startAnimation;
- (void) stopAnimation;

- (void) enterBackground;
- (void) enterForeground;
- (void) becomeInactive;
- (void) sendKeyDownUp:(XBMCKey)key;
- (void) setFramebuffer;
- (bool) presentFramebuffer;
- (CGSize) getScreenSize;
- (void) activateKeyboard:(UIView*)view;
- (void) deactivateKeyboard:(UIView*)view;

- (void) enableBackGroundTask;
- (void) disableBackGroundTask;

- (void) disableSystemSleep;
- (void) enableSystemSleep;
- (void) disableScreenSaver;
- (void) enableScreenSaver;
- (bool) resetSystemIdleTimer;
- (void) setSiriRemote:(BOOL)enable;
- (void) setRemoteIdleTimeout:(int)timeout;
- (void) setShouldRemoteIdle:(BOOL)idle;

- (NSArray<UIScreenMode*>*) availableScreenModes:(UIScreen*) screen;
- (UIScreenMode*) preferredScreenMode:(UIScreen*) screen;
- (bool) changeScreen: (unsigned int)screenIdx withMode:(UIScreenMode*)mode;
  // message from which our instance is obtained
- (id) initWithFrame:(CGRect)frame withScreen:(UIScreen*)screen;
- (void) insertVideoView:(UIView*)view;
- (void) removeVideoView:(UIView*)view;
- (float) getDisplayRate;
- (void) displayRateSwitch:(float)refreshRate withDynamicRange:(int)dynamicRange;
- (void) displayRateReset;
- (void*) getEAGLContextObj;
@end

extern MainController* g_xbmcController;
