/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/gl.h>
#import <AudioToolbox/AudioToolbox.h>

#import "XBMC_events.h"
#include "XBMC_keysym.h"

@class IOSEAGLView;

typedef enum
{
  IOS_PLAYBACK_STOPPED,
  IOS_PLAYBACK_PAUSED,
  IOS_PLAYBACK_PLAYING
} IOSPlaybackState;

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
  IOSPlaybackState m_playbackState;
  NSDictionary *nowPlayingInfo;
}
@property (readonly, nonatomic, getter=isAnimating) BOOL animating;
@property CGPoint lastGesturePoint;
@property CGFloat screenScale;
@property bool touchBeginSignaled;
@property int  m_screenIdx;
@property CGSize screensize;
@property (nonatomic, retain) NSTimer *m_networkAutoSuspendTimer;
@property (nonatomic, retain) NSDictionary *nowPlayingInfo;

// message from which our instance is obtained
- (void) pauseAnimation;
- (void) resumeAnimation;
- (void) startAnimation;
- (void) stopAnimation;
- (void) enterBackground;
- (void) enterForeground;
- (void) becomeInactive;
- (void) setIOSNowPlayingInfo:(NSDictionary *)info;
- (void) sendKey: (XBMCKey) key;
- (void) observeDefaultCenterStuff: (NSNotification *) notification;
- (void) setFramebuffer;
- (bool) presentFramebuffer;
- (CGSize) getScreenSize;
- (CGFloat) getScreenScale:(UIScreen *)screen;
- (UIInterfaceOrientation) getOrientation;
- (void) createGestureRecognizers;
- (void) activateKeyboard:(UIView *)view;
- (void) deactivateKeyboard:(UIView *)view;

- (void) disableNetworkAutoSuspend;
- (void) enableNetworkAutoSuspend:(id)obj;
- (void) disableSystemSleep;
- (void) enableSystemSleep;
- (void) disableScreenSaver;
- (void) enableScreenSaver;
- (bool) changeScreen: (unsigned int)screenIdx withMode:(UIScreenMode *)mode;
- (void) activateScreen: (UIScreen *)screen withOrientation:(UIInterfaceOrientation)newOrientation;
- (id)   initWithFrame:(CGRect)frame withScreen:(UIScreen *)screen;
@end

extern XBMCController *g_xbmcController;
