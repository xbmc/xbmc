/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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

/*Class for managing the UIScreens/resolutions of an iOS device*/

#ifdef TARGET_DARWIN_IOS_ATV2
#import <BackRow/BackRow.h>
#else
#endif
#import <UIKit/UIKit.h>

@class IOSEAGLView;
@class XBMCController;
@class IOSExternalTouchController;

@interface IOSScreenManager : NSObject {

  int  _screenIdx;
  bool _externalScreen;
  IOSEAGLView *_glView;
  IOSExternalTouchController *_externalTouchController;
}
@property int  _screenIdx;
@property (readonly, getter=isExternalScreen)bool _externalScreen;
@property (assign, setter=setView:) IOSEAGLView *_glView;

// init the screenmanager with our eaglview
//- (id)      initWithView:(IOSEAGLView *)view;
// change to screen with the given mode (might also change only the mode on the same screen)
- (bool)    changeScreen: (unsigned int)screenIdx withMode:(UIScreenMode *)mode;
// called when app detects disconnection of external screen - will move xbmc to the internal screen then
- (void)    screenDisconnect;
// wrapper for g_Windowing.UpdateResolutions();
+ (void)    updateResolutions;
// returns the landscape resolution for the given screen
+ (CGRect)  getLandscapeResolution:(UIScreen *)screen;
// fades the screen from black back to full alpha after delaySecs seconds
- (void)    fadeFromBlack:(CGFloat) delaySecs;
// returns true if switching to screenIdx will change from internal to external screen
- (bool)    willSwitchToExternal:(unsigned int) screenIdx;
// returns true if switching to screenIdx will change from external to internal screen
- (bool)    willSwitchToInternal:(unsigned int) screenIdx;
// singleton access
+ (id)      sharedInstance;
@end