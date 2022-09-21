/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "IOSScreenManager.h"

#import "IOSEAGLView.h"
#import "IOSExternalTouchController.h"
#include "ServiceBroker.h"
#import "XBMCController.h"
#include "application/Application.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "settings/DisplaySettings.h"
#include "threads/Event.h"
#include "utils/log.h"
#include "windowing/WinSystem.h"
#include "windowing/ios/WinSystemIOS.h"

#include <signal.h>

#import <Foundation/Foundation.h>
#include <objc/runtime.h>
#include <sys/resource.h>

using namespace std::chrono_literals;

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

    [g_xbmcController activateScreen:newScreen withOrientation:UIInterfaceOrientationPortrait];// will attach the screen to xbmc mainwindow

    if(toExternal)//changing the external screen might need some time ...
    {
      //deactivate any overscan compensation when switching to external screens
      //tvout has a default overscan compensation and property
      //we need to switch it off here so that the tv can handle any
      //needed overscan compensation (else on tvs without "just scan" option
      //we might end up with black borders.
      [newScreen setOverscanCompensation:UIScreenOverscanCompensationNone];
      CLog::Log(LOGDEBUG, "[IOSScreenManager] Disabling overscancompensation.");

      [[IOSScreenManager sharedInstance] fadeFromBlack:timeSwitchingToExternalSecs];
    }
    else
    {
      [[IOSScreenManager sharedInstance] fadeFromBlack:timeSwitchingToInternalSecs];
    }
    [g_xbmcController setGUIInsetsFromMainThread:YES];

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
    _lastTouchControllerOrientation = [[UIApplication sharedApplication] statusBarOrientation];
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


  CLog::Log(LOGINFO, "Changing screen to {} with {:f} x {:f}", screenIdx, [mode size].width,
            [mode size].height);
  //ensure that the screen change is done in the mainthread
  if([NSThread currentThread] != [NSThread mainThread])
  {
    [self performSelectorOnMainThread:@selector(changeScreenSelector:) withObject:dict  waitUntilDone:YES];
    screenChangeEvent.Wait(30000ms);
  }
  else
  {
    [self changeScreenSelector:dict];
  }

  // re-enumerate audio devices in that case too
  // as we might gain passthrough capabilities via HDMI
  CServiceBroker::GetActiveAE()->DeviceChange();
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
- (void) screenDisconnect
{
  //if we are on external screen and he was disconnected
  //change back to internal screen
  if (_screenIdx != 0)
  {
    CWinSystemIOS *winSystem = (CWinSystemIOS *)CServiceBroker::GetWinSystem();
    if (winSystem != nullptr)
    {
      winSystem->MoveToTouchscreen();
    }
  }
}
//--------------------------------------------------------------
+ (void) updateResolutions
{
  CWinSystemBase *winSystem = CServiceBroker::GetWinSystem();
  if (winSystem != nullptr)
  {
    winSystem->UpdateResolutions();
  }
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
