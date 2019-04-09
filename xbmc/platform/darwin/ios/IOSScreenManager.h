/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

/*Class for managing the UIScreens/resolutions of an iOS device*/

#import <UIKit/UIKit.h>

@class IOSEAGLView;
@class XBMCController;
@class IOSExternalTouchController;

@interface IOSScreenManager : NSObject {

  unsigned int  _screenIdx;
  bool _externalScreen;
  IOSEAGLView *_glView;
  IOSExternalTouchController *_externalTouchController;
#ifdef TARGET_DARWIN_IOS
  UIInterfaceOrientation _lastTouchControllerOrientation;
#endif
}
@property (readonly, getter=GetScreenIdx)unsigned int  _screenIdx;
@property (readonly, getter=isExternalScreen)bool _externalScreen;
@property (assign, setter=setView:) IOSEAGLView *_glView;
#ifdef TARGET_DARWIN_IOS
@property UIInterfaceOrientation _lastTouchControllerOrientation;
#endif

// init the screenmanager with our eaglview
//- (id)      initWithView:(IOSEAGLView *)view;
// change to screen with the given mode (might also change only the mode on the same screen)
- (bool)    changeScreen: (unsigned int)screenIdx withMode:(UIScreenMode *)mode;
// called when app detects disconnection of external screen - will move xbmc to the internal screen then
- (void)    screenDisconnect;
// wrapper for g_Windowing.UpdateResolutions();
+ (void)    updateResolutions;
// fades the screen from black back to full alpha after delaySecs seconds
- (void)    fadeFromBlack:(CGFloat) delaySecs;
// returns true if switching to screenIdx will change from internal to external screen
- (bool)    willSwitchToExternal:(unsigned int) screenIdx;
// returns true if switching to screenIdx will change from external to internal screen
- (bool)    willSwitchToInternal:(unsigned int) screenIdx;
// singleton access
+ (id)      sharedInstance;
@end
