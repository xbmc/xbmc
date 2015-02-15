/*
 *      Copyright (C) 2012-2013 Team XBMC
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

//hack around problem with xbmc's typedef int BOOL
// and obj-c's typedef unsigned char BOOL
#define BOOL XBMC_BOOL
#include <sys/resource.h>
#include <signal.h>
#include "utils/log.h"
#include "settings/DisplaySettings.h"
#include "threads/Event.h"
#include "Application.h"
#include "WindowingFactory.h"
#include "settings/DisplaySettings.h"
#include "cores/AudioEngine/AEFactory.h"
#include "osx/DarwinUtils.h"
#undef BOOL

#import <Foundation/Foundation.h>
#include <objc/runtime.h>

#import "IOSScreenManager.h"
#if defined(TARGET_DARWIN_IOS_ATV2)
#import "xbmc/osx/atv2/KodiController.h"
#elif defined(TARGET_DARWIN_IOS)
#import "xbmc/osx/ios/XBMCController.h"
#endif
#import "IOSExternalTouchController.h"
#import "IOSEAGLView.h"

const CGFloat timeSwitchingToExternalSecs = 6.0;
const CGFloat timeSwitchingToInternalSecs = 2.0;
const CGFloat timeFadeSecs                = 2.0;

static CEvent screenChangeEvent;

@implementation IOSScreenManager
@synthesize _screenIdx;
@synthesize _externalScreen;
@synthesize _glView;
@synthesize _lastTouchControllerOrientation;

//--------------------------------------------------------------
- (void) fadeFromBlack:(CGFloat) delaySecs
{
  if([_glView alpha] != 1.0)
  {
    [UIView animateWithDuration:timeFadeSecs delay:delaySecs options:UIViewAnimationOptionCurveEaseInOut animations:^{
      [_glView setAlpha:1.0];
    }
    completion:^(BOOL finished){   screenChangeEvent.Set(); }];
  }
}
//--------------------------------------------------------------
// the real screen/mode change method
- (void) setScreen:(unsigned int) screenIdx withMode:(UIScreenMode *)mode
{
    UIScreen *newScreen = [[UIScreen screens] objectAtIndex:screenIdx];
    bool toExternal = false;

    // current screen is main screen and new screen
    // is different
    if (_screenIdx == 0 && _screenIdx != screenIdx)
      toExternal = true;

    // current screen is not main screen
    // and new screen is the same as current
    // this means we are external already but
    // for example resolution gets changed
    // treat this as toExternal for proper rotation...
    if (_screenIdx != 0 && _screenIdx == screenIdx)
      toExternal = true;

    //set new screen mode
    [newScreen setCurrentMode:mode];

    //mode couldn't be applied to external screen
    //wonkey screen!
    if([newScreen currentMode] != mode)
    {
      NSLog(@"Error setting screen mode!");
      screenChangeEvent.Set();
      return;
    }
    _screenIdx = screenIdx;

    //inform the other layers
    _externalScreen = screenIdx != 0;

    [_glView setScreen:newScreen withFrameBufferResize:TRUE];//will also resize the framebuffer

    if (toExternal)
    {
      // portrait on external screen means its landscape for xbmc
#if __IPHONE_8_0
      if (CDarwinUtils::GetIOSVersion() >= 8.0)
        [g_xbmcController activateScreen:newScreen withOrientation:UIInterfaceOrientationLandscapeLeft];// will attach the screen to xbmc mainwindow
      else
#endif
        [g_xbmcController activateScreen:newScreen withOrientation:UIInterfaceOrientationPortrait];// will attach the screen to xbmc mainwindow
    }
    else
    {
#if __IPHONE_8_0
      if (CDarwinUtils::GetIOSVersion() >= 8.0)
        [g_xbmcController activateScreen:newScreen withOrientation:UIInterfaceOrientationPortrait];// will attach the screen to xbmc mainwindow
      else
#endif
      // switching back to internal - use same orientation as we used for the touch controller
      [g_xbmcController activateScreen:newScreen withOrientation:_lastTouchControllerOrientation];// will attach the screen to xbmc mainwindow
    }

    if(toExternal)//changing the external screen might need some time ...
    {
      //deactivate any overscan compensation when switching to external screens
      if([newScreen respondsToSelector:@selector(overscanCompensation)])
      {
        //since iOS5.0 tvout has an default overscan compensation and property
        //we need to switch it off here so that the tv can handle any
        //needed overscan compensation (else on tvs without "just scan" option
        //we might end up with black borders.
        //Beside that in Apples documentation to setOverscanCompensation
        //the parameter enum is lacking the UIScreenOverscanCompensationNone value.
        //Someone on stackoverflow figured out that value 3 is for turning it off
        //(though there is no enum value for it).
#ifdef __IPHONE_5_0
        [newScreen setOverscanCompensation:(UIScreenOverscanCompensation)3];
#else
        [newScreen setOverscanCompensation:3];
#endif
        CLog::Log(LOGDEBUG, "[IOSScreenManager] Disabling overscancompensation.");
      }
      else
      {
        CLog::Log(LOGDEBUG, "[IOSScreenManager] Disabling overscancompensation not supported on this iOS version.");
      }

      [[IOSScreenManager sharedInstance] fadeFromBlack:timeSwitchingToExternalSecs];
    }
    else
    {
      [[IOSScreenManager sharedInstance] fadeFromBlack:timeSwitchingToInternalSecs];
    }

    int w = [[newScreen currentMode] size].width;
    int h = [[newScreen currentMode] size].height;
    NSLog(@"Switched to screen %i with %i x %i",screenIdx, w ,h);
}
//--------------------------------------------------------------
// - will fade current screen to black
// - change mode and screen
// - optionally activate external touchscreen controller when
// switching to external screen
// - fade back from black
- (void) changeScreenSelector:(NSDictionary *)dict
{
  bool activateExternalTouchController = false;
  int screenIdx = [[dict objectForKey:@"screenIdx"] intValue];
  UIScreenMode *mode = [dict objectForKey:@"screenMode"];

  if([self willSwitchToInternal:screenIdx] && _externalTouchController != nil)
  {
    _lastTouchControllerOrientation = [_externalTouchController interfaceOrientation];
    [_externalTouchController release];
    _externalTouchController = nil;
  }

  if([self willSwitchToExternal:screenIdx])
  {
    activateExternalTouchController = true;
  }


  [UIView animateWithDuration:timeFadeSecs delay:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
    [_glView setAlpha:0.0];
  }
  completion:^(BOOL finished)
  {
    [self setScreen:screenIdx withMode:mode];
    if(activateExternalTouchController)
    {
      _externalTouchController = [[IOSExternalTouchController alloc] init];
    }
  }];
}
//--------------------------------------------------------------
- (bool) changeScreen: (unsigned int)screenIdx withMode:(UIScreenMode *)mode
{
  //screen has changed - get the new screen
  if(screenIdx >= [[UIScreen screens] count])
    return false;

  //if we are about to switch to current screen
  //with current mode - don't do anything
  if(screenIdx == _screenIdx &&
    mode == (UIScreenMode *)[[[UIScreen screens] objectAtIndex:screenIdx] currentMode])
    return true;

  //put the params into a dict
  NSNumber *idx = [NSNumber numberWithInt:screenIdx];
  NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:mode, @"screenMode",
                                                                  idx,  @"screenIdx", nil];


  CLog::Log(LOGINFO, "Changing screen to %d with %f x %f",screenIdx,[mode size].width, [mode size].height);
  //ensure that the screen change is done in the mainthread
  if([NSThread currentThread] != [NSThread mainThread])
  {
    [self performSelectorOnMainThread:@selector(changeScreenSelector:) withObject:dict  waitUntilDone:YES];
    screenChangeEvent.WaitMSec(30000);
  }
  else
  {
    [self changeScreenSelector:dict];
  }

  // re-enumerate audio devices in that case too
  // as we might gain passthrough capabilities via HDMI
  CAEFactory::DeviceChange();
  return true;
}
//--------------------------------------------------------------
- (bool) willSwitchToExternal:(unsigned int) screenIdx
{
  if(_screenIdx == 0 && screenIdx != _screenIdx)
  {
    return true;
  }
  return false;
}
//--------------------------------------------------------------
- (bool) willSwitchToInternal:(unsigned int) screenIdx
{
  if(_screenIdx != 0 && screenIdx == 0)
  {
    return true;
  }
  return false;
}
//--------------------------------------------------------------
+ (CGRect) getLandscapeResolution:(UIScreen *)screen
{
  CGRect res = [screen bounds];
#ifdef TARGET_DARWIN_IOS_ATV2
  //because bounds returns f00bar on atv2 - we return the preferred resolution (which mostly is the
  //right resolution
#if __IPHONE_OS_VERSION_MIN_REQUIRED > __IPHONE_4_2
  res.size = screen.preferredMode.size;
#else
  Class brwin = objc_getClass("BRWindow");
  res.size = [brwin interfaceFrame].size;
#endif
#else
  #if __IPHONE_8_0
  if (CDarwinUtils::GetIOSVersion() < 8.0)
  #endif
  {
    //main screen is in portrait mode (physically) so exchange height and width
    //at least when compiled with ios sdk < 8.0 (seems to be fixed in later sdks)
    if(screen == [UIScreen mainScreen])
    {
      CGRect frame = res;
      res.size = CGSizeMake(frame.size.height, frame.size.width);
    }
  }
#endif
  return res;
}
//--------------------------------------------------------------
- (void) screenDisconnect
{
  //if we are on external screen and he was disconnected
  //change back to internal screen
  if([[UIScreen screens] count] == 1 && _screenIdx != 0)
  {
    RESOLUTION_INFO res = CDisplaySettings::Get().GetResolutionInfo(RES_DESKTOP);//internal screen default res
    g_Windowing.SetFullScreen(true, res, false);
  }
}
//--------------------------------------------------------------
+ (void) updateResolutions
{
  g_Windowing.UpdateResolutions();
}
//--------------------------------------------------------------
- (void) dealloc
{
  if(_externalTouchController != nil )
  {
    [_externalTouchController release];
  }
  [super dealloc];
}
//--------------------------------------------------------------
+ (id) sharedInstance
{
	static IOSScreenManager* sharedManager = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
   sharedManager = [[self alloc] init];
	});
	return sharedManager;
}
@end
