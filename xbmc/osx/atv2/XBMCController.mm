/*
 *      Copyright (C) 2010-2013 Team XBMC
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


/* HowTo code in this file:
 * Since AppleTV/iOS6.x (atv2 version 5.2) Apple removed the AppleTV.framework and put all those classes into the
 * AppleTV.app. So we can't use standard obj-c coding here anymore. Instead we need to use the obj-c runtime
 * functions for subclassing and adding methods to our instances during runtime (hooking).
 * 
 * 1. For implementing a method of a base class:
 *  a) declare it in the form <XBMCController$nameOfMethod> like the others 
 *  b) these methods need to be static and have XBMCController* self, SEL _cmd (replace XBMCAppliance with the class the method gets implemented for) as minimum params. 
 *  c) add the method to the XBMCController.h for getting rid of the compiler warnings of unresponsive selectors (declare the method like done in the baseclass).
 *  d) in initControllerRuntimeClasses exchange the base class implementation with ours by calling MSHookMessageEx
 *  e) if we need to call the base class implementation as well we have to save the original implementation (see brEventAction$Orig for reference)
 *
 * 2. For implementing a new method which is not part of the base class:
 *  a) same as 1.a
 *  b) same as 1.b
 *  c) same as 1.c
 *  d) in initControllerRuntimeClasses add the method to our class via class_addMethod
 *
 * 3. Never access any BackRow classes directly - but always get the class via objc_getClass - if the class is used in multiple places 
 *    save it as static (see BRWindowCls)
 * 
 * 4. Keep the structure of this file based on the section comments (marked with // SECTIONCOMMENT).
 * 5. really - obey 4.!
 *
 * 6. for adding class members use associated objects - see timerKey
 *
 * For further reference see https://developer.apple.com/library/mac/#documentation/Cocoa/Reference/ObjCRuntimeRef/Reference/reference.html
 */

//hack around problem with xbmc's typedef int BOOL
// and obj-c's typedef unsigned char BOOL
#define BOOL XBMC_BOOL 
#import "WinEventsIOS.h"
#import "XBMC_events.h"
#include "utils/log.h"
#include "osx/DarwinUtils.h"
#include "threads/Event.h"
#include "Application.h"
#undef BOOL

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import "XBMCController.h"
#import "XBMCDebugHelpers.h"

#import "IOSEAGLView.h"
#import "IOSSCreenManager.h"
#include "XBMC_keysym.h"
#include "substrate.h"

//start repeating after 0.5s
#define REPEATED_KEYPRESS_DELAY_S 0.5
//pause 0.01s (10ms) between keypresses
#define REPEATED_KEYPRESS_PAUSE_S 0.01

typedef enum {

  ATV_BUTTON_UP                 = 1,
  ATV_BUTTON_DOWN               = 2,
  ATV_BUTTON_LEFT               = 3,
  ATV_BUTTON_RIGHT              = 4,
  ATV_BUTTON_PLAY               = 5,
  ATV_BUTTON_MENU               = 6,
  ATV_BUTTON_PLAY_H             = 7,
  ATV_BUTTON_MENU_H             = 8,
  ATV_BUTTON_LEFT_H             = 9,
  ATV_BUTTON_RIGHT_H            = 10,

  //new aluminium remote buttons
  ATV_ALUMINIUM_PLAY            = 12,
  ATV_ALUMINIUM_PLAY_H          = 11,

  //newly added remote buttons
  ATV_BUTTON_PAGEUP             = 13,
  ATV_BUTTON_PAGEDOWN           = 14,
  ATV_BUTTON_PAUSE              = 15,
  ATV_BUTTON_PLAY2              = 16,
  ATV_BUTTON_STOP               = 17,
  ATV_BUTTON_STOP_RELEASE       = 17,
  ATV_BUTTON_FASTFWD            = 18,
  ATV_BUTTON_FASTFWD_RELEASE    = 18,
  ATV_BUTTON_REWIND             = 19,
  ATV_BUTTON_REWIND_RELEASE     = 19,
  ATV_BUTTON_SKIPFWD            = 20,
  ATV_BUTTON_SKIPBACK           = 21,

  //learned remote buttons
  ATV_LEARNED_PLAY              = 70,
  ATV_LEARNED_PAUSE             = 71,
  ATV_LEARNED_STOP              = 72,
  ATV_LEARNED_PREVIOUS          = 73,
  ATV_LEARNED_NEXT              = 74,
  ATV_LEARNED_REWIND            = 75,
  ATV_LEARNED_REWIND_RELEASE    = 75,
  ATV_LEARNED_FORWARD           = 76,
  ATV_LEARNED_FORWARD_RELEASE   = 76,
  ATV_LEARNED_RETURN            = 77,
  ATV_LEARNED_ENTER             = 78,

  //gestures
  ATV_GESTURE_SWIPE_LEFT        = 80,
  ATV_GESTURE_SWIPE_RIGHT       = 81,
  ATV_GESTURE_SWIPE_UP          = 82,
  ATV_GESTURE_SWIPE_DOWN        = 83,

  ATV_GESTURE_FLICK_LEFT        = 85,
  ATV_GESTURE_FLICK_RIGHT       = 86,
  ATV_GESTURE_FLICK_UP          = 87,
  ATV_GESTURE_FLICK_DOWN        = 88,
  ATV_GESTURE_TOUCHHOLD         = 89,

  ATV_BTKEYPRESS                = 84,

  ATV_INVALID_BUTTON
} eATVClientEvent;


typedef enum {
  // for originator kBREventOriginatorRemote
  kBREventRemoteActionMenu      = 1,
  kBREventRemoteActionMenuHold  = 2,
  kBREventRemoteActionUp        = 3,
  kBREventRemoteActionDown      = 4,
  kBREventRemoteActionPlay      = 5,
  kBREventRemoteActionLeft      = 6,
  kBREventRemoteActionRight     = 7,

  kBREventRemoteActionALPlay    = 10,

  kBREventRemoteActionPageUp    = 13,
  kBREventRemoteActionPageDown  = 14,
  kBREventRemoteActionPause     = 15,
  kBREventRemoteActionPlay2     = 16,
  kBREventRemoteActionStop      = 17,
  kBREventRemoteActionFastFwd   = 18,
  kBREventRemoteActionRewind    = 19,
  kBREventRemoteActionSkipFwd   = 20,
  kBREventRemoteActionSkipBack  = 21,


  kBREventRemoteActionPlayHold  = 22,
  kBREventRemoteActionCenterHold,
  kBREventRemoteActionCenterHold42,

  // Gestures, for originator kBREventOriginatorGesture
  kBREventRemoteActionTouchBegin= 31,
  kBREventRemoteActionTouchMove = 32,
  kBREventRemoteActionTouchEnd  = 33,

  kBREventRemoteActionSwipeLeft = 34,
  kBREventRemoteActionSwipeRight= 35,
  kBREventRemoteActionSwipeUp   = 36,
  kBREventRemoteActionSwipeDown = 37,

  kBREventRemoteActionFlickLeft = 38,
  kBREventRemoteActionFlickRight= 39,
  kBREventRemoteActionFlickUp   = 40,
  kBREventRemoteActionFlickDown = 41,

  kBREventRemoteActionTouchHold = 46,

  kBREventRemoteActionKeyPress  = 47,
  kBREventRemoteActionKeyPress42,
  

  // Custom remote actions for old remote actions
  kBREventRemoteActionHoldLeft = 0xfeed0001,
  kBREventRemoteActionHoldRight,
  kBREventRemoteActionHoldUp,
  kBREventRemoteActionHoldDown,
} BREventRemoteAction;


XBMCController *g_xbmcController;

//--------------------------------------------------------------
// so we don't have to include AppleTV.frameworks/PrivateHeaders/ATVSettingsFacade.h
@interface XBMCSettingsFacade : NSObject
-(int)screenSaverTimeout;
-(void)setScreenSaverTimeout:(int) f_timeout;
-(void)setSleepTimeout:(int)timeout;
-(int)sleepTimeout;
-(void)flushDiskChanges;
@end

// notification messages
extern NSString* kBRScreenSaverActivated;
extern NSString* kBRScreenSaverDismissed;

//--------------------------------------------------------------
//--------------------------------------------------------------
// SECTIONCOMMENT
// orig method handlers we wanna call in hooked methods ([super method])
static BOOL (*XBMCController$brEventAction$Orig)(XBMCController*, SEL, BREvent*);
static id (*XBMCController$init$Orig)(XBMCController*, SEL);
static void (*XBMCController$dealloc$Orig)(XBMCController*, SEL);
static void (*XBMCController$controlWasActivated$Orig)(XBMCController*, SEL);
static void (*XBMCController$controlWasDeactivated$Orig)(XBMCController*, SEL);

// SECTIONCOMMENT
// classes we need multiple times
static Class BRWindowCls;

int padding[16];//obsolete? - was commented with "credit is due here to SapphireCompatibilityClasses!!"
  
//--------------------------------------------------------------
//--------------------------------------------------------------
// SECTIONCOMMENT
// since we can't inject ivars we need to use associated objects
// these are the keys for XBMCController
static char timerKey;
static char glviewKey;
static char screensaverKey;
static char systemsleepKey;

//
//
// SECTIONCOMMENT
//implementation XBMCController
 
static id XBMCController$keyTimer(XBMCController* self, SEL _cmd) 
{ 
  return objc_getAssociatedObject(self, &timerKey);
}

static void XBMCController$setKeyTimer(XBMCController* self, SEL _cmd, id timer) 
{ 
  objc_setAssociatedObject(self, &timerKey, timer, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

static id XBMCController$glView(XBMCController* self, SEL _cmd) 
{ 
  return objc_getAssociatedObject(self, &glviewKey);
}

static void XBMCController$setGlView(XBMCController* self, SEL _cmd, id view) 
{ 
  objc_setAssociatedObject(self, &glviewKey, view, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

static id XBMCController$systemScreenSaverTimeout(XBMCController* self, SEL _cmd) 
{ 
  return objc_getAssociatedObject(self, &screensaverKey);
}

static void XBMCController$setSystemScreenSaverTimeout(XBMCController* self, SEL _cmd, id timeout) 
{ 
  objc_setAssociatedObject(self, &screensaverKey, timeout, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

static id XBMCController$systemSleepTimeout(XBMCController* self, SEL _cmd) 
{ 
  return objc_getAssociatedObject(self, &systemsleepKey);
}

static void XBMCController$setSystemSleepTimeout(XBMCController* self, SEL _cmd, id timeout) 
{ 
  objc_setAssociatedObject(self, &systemsleepKey, timeout, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

static void XBMCController$applicationDidExit(XBMCController* self, SEL _cmd) 
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);

  [[self glView] stopAnimation];
  [self enableScreenSaver];
  [self enableSystemSleep];
  [[self stack] popController];
}

static void XBMCController$initDisplayLink(XBMCController* self, SEL _cmd) 
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);

  [[self glView] initDisplayLink];
}

static void XBMCController$deinitDisplayLink(XBMCController* self, SEL _cmd) 
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);

  [[self glView] deinitDisplayLink];
}

static double XBMCController$getDisplayLinkFPS(XBMCController* self, SEL _cmd) 
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);

  return [[self glView] getDisplayLinkFPS];
}

static void XBMCController$setFramebuffer(XBMCController* self, SEL _cmd) 
{   
  [[self glView] setFramebuffer];
}

static bool XBMCController$presentFramebuffer(XBMCController* self, SEL _cmd) 
{    
  return [[self glView] presentFramebuffer];
}

static CGSize XBMCController$getScreenSize(XBMCController* self, SEL _cmd) 
{
  CGSize screensize;
  screensize.width  = [BRWindowCls interfaceFrame].size.width;
  screensize.height = [BRWindowCls interfaceFrame].size.height;
  //NSLog(@"%s UpdateResolutions width=%f, height=%f", 
  //__PRETTY_FUNCTION__, screensize.width, screensize.height);
  return screensize;
}

static void XBMCController$sendKey(XBMCController* self, SEL _cmd, XBMCKey key) 
{
  //empty because its not used here. Only implemented for getting rid
  //of "may not respond to selector" compile warnings in IOSExternalTouchController
}

static id XBMCController$init(XBMCController* self, SEL _cmd) 
{
  if((self = XBMCController$init$Orig(self, _cmd)) != nil)  
  {
    //NSLog(@"%s", __PRETTY_FUNCTION__);

    NSNotificationCenter *center;
    // first the default notification center, which is all
    // notifications that only happen inside of our program
    center = [NSNotificationCenter defaultCenter];
    [center addObserver: self
      selector: @selector(observeDefaultCenterStuff:)
      name: nil
      object: nil];

    IOSEAGLView *view = [[IOSEAGLView alloc] initWithFrame:[BRWindowCls interfaceFrame] withScreen:[UIScreen mainScreen]];
    [self setGlView:view];
 
    [[IOSScreenManager sharedInstance] setView:[self glView]];

    g_xbmcController = self;
  }
  return self;
}

static void XBMCController$dealloc(XBMCController* self, SEL _cmd) 
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);
  [[self glView] stopAnimation];
  [[self glView] release];

  NSNotificationCenter *center;
  // take us off the default center for our app
  center = [NSNotificationCenter defaultCenter];
  [center removeObserver: self];

  XBMCController$dealloc$Orig(self, _cmd);
}

static void XBMCController$controlWasActivated(XBMCController* self, SEL _cmd) 
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);

  XBMCController$controlWasActivated$Orig(self, _cmd);

  [self disableSystemSleep];
  [self disableScreenSaver];
  
  IOSEAGLView *view = [self glView];
  //inject our gles layer into the backrow root layer
  [[BRWindowCls rootLayer] addSublayer:view.layer];

  [[self glView] startAnimation];
}

static void XBMCController$controlWasDeactivated(XBMCController* self, SEL _cmd) 
{
  NSLog(@"XBMC was forced by FrontRow to exit via controlWasDeactivated");

  [[self glView] stopAnimation];
  [[[self glView] layer] removeFromSuperlayer];

  [self enableScreenSaver];
  [self enableSystemSleep];

  XBMCController$controlWasDeactivated$Orig(self, _cmd);
}

static BOOL XBMCController$recreateOnReselect(XBMCController* self, SEL _cmd) 
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);
  return YES;
}

static void XBMCController$ATVClientEventFromBREvent(XBMCController* self, SEL _cmd, BREvent* f_event, bool * isRepeatable, bool * isPressed, int * result) 
{
  if(f_event == nil)// paranoia
    return;

  int remoteAction = [f_event remoteAction];
  CLog::Log(LOGDEBUG,"XBMCPureController: Button press remoteAction = %i", remoteAction);
  *isRepeatable = false;
  *isPressed = false;

  switch (remoteAction)
  {
    // tap up
    case kBREventRemoteActionUp:
    case 65676:
      *isRepeatable = true;
      if([f_event value] == 1)
        *isPressed = true;
      *result = ATV_BUTTON_UP;
      return;

    // tap down
    case kBREventRemoteActionDown:
    case 65677:
      *isRepeatable = true;
      if([f_event value] == 1)
        *isPressed = true;
      *result = ATV_BUTTON_DOWN;
      return;
    
    // tap left
    case kBREventRemoteActionLeft:
    case 65675:
      *isRepeatable = true;
      if([f_event value] == 1)
        *isPressed = true;
      *result = ATV_BUTTON_LEFT;
      return;
    
    // hold left
    case 786612:
      if([f_event value] == 1)
        *result = ATV_LEARNED_REWIND;
      else
        *result = ATV_INVALID_BUTTON;
      return;
    
    // tap right
    case kBREventRemoteActionRight:
    case 65674:
      *isRepeatable = true;
      if ([f_event value] == 1)
        *isPressed = true;
      *result = ATV_BUTTON_RIGHT;
      return ;
    
    // hold right
    case 786611:
      if ([f_event value] == 1)
        *result = ATV_LEARNED_FORWARD;
      else
        *result = ATV_INVALID_BUTTON;
      return ;
   
    // tap play
    case kBREventRemoteActionPlay:
    case 65673:
      *result = ATV_BUTTON_PLAY;
      return ;
    
    // hold play
    case kBREventRemoteActionPlayHold:
    case kBREventRemoteActionCenterHold:
    case kBREventRemoteActionCenterHold42:
    case 65668:
      *result = ATV_BUTTON_PLAY_H;
      return ;
    
    // menu
    case kBREventRemoteActionMenu:
    case 65670:
      *result = ATV_BUTTON_MENU;
      return ;
    
    // hold menu
    case kBREventRemoteActionMenuHold:
    case 786496:
      *result = ATV_BUTTON_MENU_H;
      return ;
    
    // learned play
    case 786608:
      *result = ATV_LEARNED_PLAY;
      return ;
    
    // learned pause
    case 786609:
      *result = ATV_LEARNED_PAUSE;
      return ;
    
    // learned stop
    case 786615:
      *result = ATV_LEARNED_STOP;
      return ;
    
    // learned next
    case 786613:
      *result = ATV_LEARNED_NEXT;
      return ;
    
    // learned previous
    case 786614:
      *result = ATV_LEARNED_PREVIOUS;
      return ;
    
    // learned enter, like go into something
    case 786630:
      *result = ATV_LEARNED_ENTER;
      return ;
    
    // learned return, like go back
    case 786631:
      *result = ATV_LEARNED_RETURN;
      return ;
    
    // tap play on new Al IR remote
    case kBREventRemoteActionALPlay:
    case 786637:
      *result = ATV_ALUMINIUM_PLAY;
      return ;

    case kBREventRemoteActionKeyPress:
    case kBREventRemoteActionKeyPress42:
      *result = ATV_BTKEYPRESS;
      return ;
    
    // PageUp
    case kBREventRemoteActionPageUp:
      *result = ATV_BUTTON_PAGEUP;
      return ;
    
    // PageDown
    case kBREventRemoteActionPageDown:
      *result = ATV_BUTTON_PAGEDOWN;
      return ;
    
    // Pause
    case kBREventRemoteActionPause:
      *result = ATV_BUTTON_PAUSE;
      return ;
    
    // Play2
    case kBREventRemoteActionPlay2:
      *result = ATV_BUTTON_PLAY2;
      return ;
    
    // Stop
    case kBREventRemoteActionStop:
      *result = ATV_BUTTON_STOP;
      return ;
    
    // Fast Forward
    case kBREventRemoteActionFastFwd:
      *result = ATV_BUTTON_FASTFWD;
      return ;
    
    // Rewind
    case kBREventRemoteActionRewind:
      *result = ATV_BUTTON_REWIND;
      return ;

    // Skip Forward
    case kBREventRemoteActionSkipFwd:
      *result = ATV_BUTTON_SKIPFWD;
      return ;

    // Skip Back      
    case kBREventRemoteActionSkipBack:
      *result = ATV_BUTTON_SKIPBACK;
      return ;
    
    // Gesture Swipe Left
    case kBREventRemoteActionSwipeLeft:
      if ([f_event value] == 1)
        *result = ATV_GESTURE_SWIPE_LEFT;
      else
        *result = ATV_INVALID_BUTTON;
      return ;
    
    // Gesture Swipe Right
    case kBREventRemoteActionSwipeRight:
      if ([f_event value] == 1)
        *result = ATV_GESTURE_SWIPE_RIGHT;
      else
        *result = ATV_INVALID_BUTTON;
      return ;

    // Gesture Swipe Up
    case kBREventRemoteActionSwipeUp:
      if ([f_event value] == 1)
        *result = ATV_GESTURE_SWIPE_UP;
      else
        *result = ATV_INVALID_BUTTON;
      return ;
    
    // Gesture Swipe Down
    case kBREventRemoteActionSwipeDown:
      if ([f_event value] == 1)
        *result = ATV_GESTURE_SWIPE_DOWN;
      else
        *result = ATV_INVALID_BUTTON;
      return;

    // Gesture Flick Left
    case kBREventRemoteActionFlickLeft:
      if ([f_event value] == 1)
        *result = ATV_GESTURE_FLICK_LEFT;
      else
        *result = ATV_INVALID_BUTTON;
      return;
    
    // Gesture Flick Right
    case kBREventRemoteActionFlickRight:
      if ([f_event value] == 1)
        *result = ATV_GESTURE_FLICK_RIGHT;
      else
        *result = ATV_INVALID_BUTTON;
      return;
    
    // Gesture Flick Up
    case kBREventRemoteActionFlickUp:
      if ([f_event value] == 1)
        *result = ATV_GESTURE_FLICK_UP;
      else
        *result = ATV_INVALID_BUTTON;
      return;
    
    // Gesture Flick Down
    case kBREventRemoteActionFlickDown:
      if ([f_event value] == 1)
        *result = ATV_GESTURE_FLICK_DOWN;
      else
        *result = ATV_INVALID_BUTTON;
      return;

    default:
      ELOG(@"XBMCPureController: Unknown button press remoteAction = %i", remoteAction);
      *result = ATV_INVALID_BUTTON;
  }
}

static void XBMCController$setUserEvent(XBMCController* self, SEL _cmd, int eventId, unsigned int holdTime) 
{
  
  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));

  newEvent.type = XBMC_USEREVENT;
  newEvent.jbutton.which = eventId;
  newEvent.jbutton.holdTime = holdTime;
  CWinEventsIOS::MessagePush(&newEvent);
}


static BOOL XBMCController$brEventAction(XBMCController* self, SEL _cmd, BREvent* event) 
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);

  if ([[self glView] isAnimating])
  {
    BOOL is_handled = NO;
    bool isRepeatable = false;
    bool isPressed = false;
    int xbmc_ir_key = ATV_INVALID_BUTTON;
    [self ATVClientEventFromBREvent:event 
                                        Repeatable:&isRepeatable
                                        ButtonState:&isPressed
                                        Result:&xbmc_ir_key];

    if ( xbmc_ir_key != ATV_INVALID_BUTTON )
    {
      if (xbmc_ir_key == ATV_BTKEYPRESS && [event value] == 1)
      {
        XBMC_Event newEvent;
        memset(&newEvent, 0, sizeof(newEvent));

        NSDictionary *dict = [event eventDictionary];
        NSString *key_nsstring = [dict objectForKey:@"kBRKeyEventCharactersKey"];
        
        if (key_nsstring != nil && [key_nsstring length] == 1)
        {
          //ns_string contains the letter you want to input
          //unichar c = [key_nsstring characterAtIndex:0];
          //keyEvent = translateCocoaToXBMCEvent(c);
          const char* wstr = [key_nsstring cStringUsingEncoding:NSUTF16StringEncoding];
          //NSLog(@"%s, key: wstr[0] = %d, wstr[1] = %d", __PRETTY_FUNCTION__, wstr[0], wstr[1]);

          if (wstr[0] != 92) 
          {
            if (wstr[0] == 62 && wstr[1] == -9)
            {
              // stupid delete key
              newEvent.key.keysym.sym = (XBMCKey)8;
              newEvent.key.keysym.unicode = 8;
            }
            else
            {
              newEvent.key.keysym.sym = (XBMCKey)wstr[0];
              newEvent.key.keysym.unicode = wstr[0] | (wstr[1] << 8);
            }
            newEvent.type = XBMC_KEYDOWN;
            CWinEventsIOS::MessagePush(&newEvent);

            newEvent.type = XBMC_KEYUP;
            CWinEventsIOS::MessagePush(&newEvent);
            is_handled = TRUE;
          }
        }
      }
      else
      {
        if(isRepeatable)
        {
          if(isPressed)
          {
            [self setUserEvent:xbmc_ir_key withHoldTime:0];
            [self startKeyPressTimer:xbmc_ir_key];
          }
          else
          {
            //stop the timer
            [self stopKeyPressTimer];
          }
        }
        else
        {
          [self setUserEvent:xbmc_ir_key withHoldTime:0];
        }
        is_handled = TRUE;
      }
    }
    return is_handled;
  }
  else
  {
    return XBMCController$brEventAction$Orig(self, _cmd, event);
  }
}

#pragma mark -
#pragma mark private helper methods
static void XBMCController$startKeyPressTimer(XBMCController* self, SEL _cmd, int keyId) 
{ 
  NSNumber *number = [NSNumber numberWithInt:keyId];
  NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:[NSDate date], @"StartDate", 
                                                                  number, @"keyId", nil];
  
  NSDate *fireDate = [NSDate dateWithTimeIntervalSinceNow:REPEATED_KEYPRESS_DELAY_S];
  [self stopKeyPressTimer];
  
  //schedule repeated timer which starts after REPEATED_KEYPRESS_DELAY_S and fires
  //every REPEATED_KEYPRESS_PAUSE_S
  NSTimer *timer       = [[NSTimer alloc] initWithFireDate:fireDate 
                                      interval:REPEATED_KEYPRESS_PAUSE_S 
                                      target:self 
                                      selector:@selector(keyPressTimerCallback:) 
                                      userInfo:dict 
                                      repeats:YES];
 
  //schedule the timer to the runloop
  NSRunLoop *runLoop = [NSRunLoop currentRunLoop];
  [runLoop addTimer:timer forMode:NSDefaultRunLoopMode];
  [self setKeyTimer:timer];
} 

static void XBMCController$stopKeyPressTimer(XBMCController* self, SEL _cmd) 
{
  if([self keyTimer] != nil)
  {
    [[self keyTimer] invalidate];
    [[self keyTimer] release];
    [self setKeyTimer:nil];
  }
}

static void XBMCController$keyPressTimerCallback(XBMCController* self, SEL _cmd, NSTimer* theTimer)  
{ 
  //if queue is empty - skip this timer event
  //for letting it process
  if(CWinEventsIOS::GetQueueSize())
    return;

  NSDate *startDate = [[theTimer userInfo] objectForKey:@"StartDate"];
  int keyId = [[[theTimer userInfo] objectForKey:@"keyId"] intValue];
  //calc the holdTime - timeIntervalSinceNow gives the
  //passed time since startDate in seconds as negative number
  //so multiply with -1000 for getting the positive ms
  NSTimeInterval holdTime = [startDate timeIntervalSinceNow] * -1000.0f;
  [self setUserEvent:keyId withHoldTime:(unsigned int)holdTime];
} 

static void XBMCController$observeDefaultCenterStuff(XBMCController* self, SEL _cmd, NSNotification * notification) 
{
  //NSLog(@"default: %@", [notification name]);

  if ([notification name] == UIApplicationDidReceiveMemoryWarningNotification)
    NSLog(@"XBMC: %@", [notification name]);
  
  //if ([notification name] == kBRScreenSaverActivated)
  //  [m_glView stopAnimation];
  
  //if ([notification name] == kBRScreenSaverDismissed)
  //  [m_glView startAnimation];
}

static void XBMCController$disableSystemSleep(XBMCController* self, SEL _cmd) 
{
  Class ATVSettingsFacadeCls = objc_getClass("ATVSettingsFacade");
  XBMCSettingsFacade *single = (XBMCSettingsFacade *)[ATVSettingsFacadeCls singleton];

  int tmpTimeout = [single sleepTimeout];
  NSNumber *timeout = [NSNumber numberWithInt:tmpTimeout];
  [self setSystemSleepTimeout:timeout];
  [single setSleepTimeout: -1];
  [single flushDiskChanges];
}

static void XBMCController$enableSystemSleep(XBMCController* self, SEL _cmd) 
{
  Class ATVSettingsFacadeCls = objc_getClass("ATVSettingsFacade");
  int timeoutInt = [[self systemSleepTimeout] intValue];
  [[ATVSettingsFacadeCls singleton] setSleepTimeout:timeoutInt];
  [[ATVSettingsFacadeCls singleton] flushDiskChanges];
}

static void XBMCController$disableScreenSaver(XBMCController* self, SEL _cmd) 
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);
  //store screen saver state and disable it

  Class ATVSettingsFacadeCls = objc_getClass("ATVSettingsFacade");
  XBMCSettingsFacade *single = (XBMCSettingsFacade *)[ATVSettingsFacadeCls singleton];

  int tmpTimeout = [single screenSaverTimeout];
  NSNumber *timeout = [NSNumber numberWithInt:tmpTimeout];
  [self setSystemScreenSaverTimeout:timeout];
  [single setScreenSaverTimeout: -1];
  [single flushDiskChanges];

  // breaks in 4.2.1 [[BRBackgroundTaskManager singleton] holdOffBackgroundTasks];
}

static void XBMCController$enableScreenSaver(XBMCController* self, SEL _cmd) 
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);
  //reset screen saver to user settings
  Class ATVSettingsFacadeCls = objc_getClass("ATVSettingsFacade");

  int timeoutInt = [[self systemScreenSaverTimeout] intValue];
  [[ATVSettingsFacadeCls singleton] setScreenSaverTimeout:timeoutInt];
  [[ATVSettingsFacadeCls singleton] flushDiskChanges];

  // breaks in 4.2.1 [[BRBackgroundTaskManager singleton] okToDoBackgroundProcessing];
}

/*
- (XBMC_Event) translateCocoaToXBMCEvent: (unichar) c
{
  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));
  
   switch (c)
   {
   // Alt
   case NSMenuFunctionKey: 
   return "Alt";
   
   // "Apps"
   // "BrowserBack"
   // "BrowserForward"
   // "BrowserHome"
   // "BrowserRefresh"
   // "BrowserSearch"
   // "BrowserStop"
   // "CapsLock"
   
   // "Clear"
   case NSClearLineFunctionKey:
   return "Clear";
   
   // "CodeInput"
   // "Compose"
   // "Control"
   // "Crsel"
   // "Convert"
   // "Copy"
   // "Cut"
   
   // "Down"
   case NSDownArrowFunctionKey:
   return "Down";
   // "End"
   case NSEndFunctionKey:
   return "End";
   // "Enter"
   case 0x3: case 0xA: case 0xD: // Macintosh calls the one on the main keyboard Return, but Windows calls it Enter, so we'll do the same for the DOM
   return "Enter";
   
   // "EraseEof"
   
   // "Execute"
   case NSExecuteFunctionKey:
   return "Execute";
   
   // "Exsel"
   
   // "F1"
   case NSF1FunctionKey:
   return "F1";
   // "F2"
   case NSF2FunctionKey:
   return "F2";
   // "F3"
   case NSF3FunctionKey:
   return "F3";
   // "F4"
   case NSF4FunctionKey:
   return "F4";
   // "F5"
   case NSF5FunctionKey:
   return "F5";
   // "F6"
   case NSF6FunctionKey:
   return "F6";
   // "F7"
   case NSF7FunctionKey:
   return "F7";
   // "F8"
   case NSF8FunctionKey:
   return "F8";
   // "F9"
   case NSF9FunctionKey:
   return "F9";
   // "F10"
   case NSF10FunctionKey:
   return "F10";
   // "F11"
   case NSF11FunctionKey:
   return "F11";
   // "F12"
   case NSF12FunctionKey:
   return "F12";
   // "F13"
   case NSF13FunctionKey:
   return "F13";
   // "F14"
   case NSF14FunctionKey:
   return "F14";
   // "F15"
   case NSF15FunctionKey:
   return "F15";
   // "F16"
   case NSF16FunctionKey:
   return "F16";
   // "F17"
   case NSF17FunctionKey:
   return "F17";
   // "F18"
   case NSF18FunctionKey:
   return "F18";
   // "F19"
   case NSF19FunctionKey:
   return "F19";
   // "F20"
   case NSF20FunctionKey:
   return "F20";
   // "F21"
   case NSF21FunctionKey:
   return "F21";
   // "F22"
   case NSF22FunctionKey:
   return "F22";
   // "F23"
   case NSF23FunctionKey:
   return "F23";
   // "F24"
   case NSF24FunctionKey:
   return "F24";
   
   // "FinalMode"
   
   // "Find"
   case NSFindFunctionKey:
   return "Find";
   
   // "FullWidth"
   // "HalfWidth"
   // "HangulMode"
   // "HanjaMode"
   
   // "Help"
   case NSHelpFunctionKey:
   return "Help";
   
   // "Hiragana"
   
   // "Home"
   case NSHomeFunctionKey:
   return "Home";
   // "Insert"
   case NSInsertFunctionKey:
   return "Insert";
   
   // "JapaneseHiragana"
   // "JapaneseKatakana"
   // "JapaneseRomaji"
   // "JunjaMode"
   // "KanaMode"
   // "KanjiMode"
   // "Katakana"
   // "LaunchApplication1"
   // "LaunchApplication2"
   // "LaunchMail"
   
   // "Left"
   case NSLeftArrowFunctionKey:
   return "Left";
   
   // "Meta"
   // "MediaNextTrack"
   // "MediaPlayPause"
   // "MediaPreviousTrack"
   // "MediaStop"
   
   // "ModeChange"
   case NSModeSwitchFunctionKey:
   return "ModeChange";
   
   // "Nonconvert"
   // "NumLock"
   
   // "PageDown"
   case NSPageDownFunctionKey:
   return "PageDown";
   // "PageUp"
   case NSPageUpFunctionKey:
   return "PageUp";
   
   // "Paste"
   
   // "Pause"
   case NSPauseFunctionKey:
   return "Pause";
   
   // "Play"
   // "PreviousCandidate"
   
   // "PrintScreen"
   case NSPrintScreenFunctionKey:
   return "PrintScreen";
   
   // "Process"
   // "Props"
   
   // "Right"
   case NSRightArrowFunctionKey:
   return "Right";
   
   // "RomanCharacters"
   
   // "Scroll"
   case NSScrollLockFunctionKey:
   return "Scroll";
   // "Select"
   case NSSelectFunctionKey:
   return "Select";
   
   // "SelectMedia"
   // "Shift"
   
   // "Stop"
   case NSStopFunctionKey:
   return "Stop";
   // "Up"
   case NSUpArrowFunctionKey:
   return "Up";
   // "Undo"
   case NSUndoFunctionKey:
   return "Undo";
   
   // "VolumeDown"
   // "VolumeMute"
   // "VolumeUp"
   // "Win"
   // "Zoom"
   
   // More function keys, not in the key identifier specification.
   case NSF25FunctionKey:
   return "F25";
   case NSF26FunctionKey:
   return "F26";
   case NSF27FunctionKey:
   return "F27";
   case NSF28FunctionKey:
   return "F28";
   case NSF29FunctionKey:
   return "F29";
   case NSF30FunctionKey:
   return "F30";
   case NSF31FunctionKey:
   return "F31";
   case NSF32FunctionKey:
   return "F32";
   case NSF33FunctionKey:
   return "F33";
   case NSF34FunctionKey:
   return "F34";
   case NSF35FunctionKey:
   return "F35";
   
   // Turn 0x7F into 0x08, because backspace needs to always be 0x08.
   case 0x7F:
   XBMCK_BACKSPACE
   // Standard says that DEL becomes U+007F.
   case NSDeleteFunctionKey:
   XBMCK_DELETE;
   
   // Always use 0x09 for tab instead of AppKit's backtab character.
   case NSBackTabCharacter:
   return "U+0009";
   
   case NSBeginFunctionKey:
   case NSBreakFunctionKey:
   case NSClearDisplayFunctionKey:
   case NSDeleteCharFunctionKey:
   case NSDeleteLineFunctionKey:
   case NSInsertCharFunctionKey:
   case NSInsertLineFunctionKey:
   case NSNextFunctionKey:
   case NSPrevFunctionKey:
   case NSPrintFunctionKey:
   case NSRedoFunctionKey:
   case NSResetFunctionKey:
   case NSSysReqFunctionKey:
   case NSSystemFunctionKey:
   case NSUserFunctionKey:
          // FIXME: We should use something other than the vendor-area Unicode values for the above keys.
          // For now, just fall through to the default.
      default:
          return String::format("U+%04X", toASCIIUpper(c));
  }
  return newEvent;
}*/

//--------------------------------------------------------------
static void XBMCController$pauseAnimation(XBMCController* self, SEL _cmd) 
{
  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(XBMC_Event));
 
  newEvent.appcommand.type = XBMC_APPCOMMAND;
  newEvent.appcommand.action = ACTION_PLAYER_PLAYPAUSE;
  CWinEventsIOS::MessagePush(&newEvent);
  
  Sleep(2000); 
  [[self glView] pauseAnimation];
}
//--------------------------------------------------------------
static void XBMCController$resumeAnimation(XBMCController* self, SEL _cmd) 
{  
  NSLog(@"%s", __PRETTY_FUNCTION__);

  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(XBMC_Event));

  newEvent.appcommand.type = XBMC_APPCOMMAND;
  newEvent.appcommand.action = ACTION_PLAYER_PLAY;
  CWinEventsIOS::MessagePush(&newEvent);
 
  [[self glView] resumeAnimation];
}
//--------------------------------------------------------------
static void XBMCController$startAnimation(XBMCController* self, SEL _cmd) 
{
  NSLog(@"%s", __PRETTY_FUNCTION__);

  [[self glView] startAnimation];
}
//--------------------------------------------------------------
static void XBMCController$stopAnimation(XBMCController* self, SEL _cmd) 
{
  NSLog(@"%s", __PRETTY_FUNCTION__);

  [[self glView] stopAnimation];
}
//--------------------------------------------------------------
static bool XBMCController$changeScreen(XBMCController* self, SEL _cmd, unsigned int screenIdx, UIScreenMode * mode) 
{
  return [[IOSScreenManager sharedInstance] changeScreen: screenIdx withMode: mode];
}
//--------------------------------------------------------------
static void XBMCController$activateScreen(XBMCController* self, SEL _cmd, UIScreen * screen) 
{
}

// SECTIONCOMMENT
// c'tor - this sets up our class at runtime by 
// 1. subclassing from the base classes
// 2. adding new methods to our class
// 3. exchanging (hooking) base class methods with ours
// 4. register the classes to the objc runtime system
static __attribute__((constructor)) void initControllerRuntimeClasses() 
{
  char _typeEncoding[1024];
  unsigned int i = 0;

  // subclass BRController into XBMCController
  Class XBMCControllerCls = objc_allocateClassPair(objc_getClass("BRController"), "XBMCController", 0);
  // add our custom methods which are not part of the baseclass
  // XBMCController::keyTimer
  class_addMethod(XBMCControllerCls, @selector(keyTimer), (IMP)&XBMCController$keyTimer, "@@:");
  // XBMCController::setKeyTimer
  class_addMethod(XBMCControllerCls, @selector(setKeyTimer:), (IMP)&XBMCController$setKeyTimer, "v@:@");
  // XBMCController::glView
  class_addMethod(XBMCControllerCls, @selector(glView), (IMP)&XBMCController$glView, "@@:");
  // XBMCController::setGlView
  class_addMethod(XBMCControllerCls, @selector(setGlView:), (IMP)&XBMCController$setGlView, "v@:@");
  // XBMCController::systemScreenSaverTimeout
  class_addMethod(XBMCControllerCls, @selector(systemScreenSaverTimeout), (IMP)&XBMCController$systemScreenSaverTimeout, "@@:");
  // XBMCController::setSystemScreenSaverTimeout
  class_addMethod(XBMCControllerCls, @selector(setSystemScreenSaverTimeout:), (IMP)&XBMCController$setSystemScreenSaverTimeout, "v@:@");
  // XBMCController::systemSleepTimeout
  class_addMethod(XBMCControllerCls, @selector(systemSleepTimeout), (IMP)&XBMCController$systemSleepTimeout, "@@:");
  // XBMCController::setSystemSleepTimeout
  class_addMethod(XBMCControllerCls, @selector(setSystemSleepTimeout:), (IMP)&XBMCController$setSystemSleepTimeout, "v@:@");
  // XBMCController::applicationDidExit
  class_addMethod(XBMCControllerCls, @selector(applicationDidExit), (IMP)&XBMCController$applicationDidExit, "v@:");
  // XBMCController::initDisplayLink
  class_addMethod(XBMCControllerCls, @selector(initDisplayLink), (IMP)&XBMCController$initDisplayLink, "v@:");
  // XBMCController::deinitDisplayLink
  class_addMethod(XBMCControllerCls, @selector(deinitDisplayLink), (IMP)&XBMCController$deinitDisplayLink, "v@:");
  // XBMCController::getDisplayLinkFPS
  class_addMethod(XBMCControllerCls, @selector(getDisplayLinkFPS), (IMP)&XBMCController$getDisplayLinkFPS, "d@:");
  // XBMCController::setFramebuffer
  class_addMethod(XBMCControllerCls, @selector(setFramebuffer), (IMP)&XBMCController$setFramebuffer, "v@:");
  // XBMCController::presentFramebuffer
  class_addMethod(XBMCControllerCls, @selector(presentFramebuffer), (IMP)&XBMCController$presentFramebuffer, "B@:");
  // XBMCController::setUserEvent
  class_addMethod(XBMCControllerCls, @selector(setUserEvent:withHoldTime:), (IMP)&XBMCController$setUserEvent, "v@:iI");  
  // XBMCController::startKeyPressTimer
  class_addMethod(XBMCControllerCls, @selector(startKeyPressTimer:), (IMP)&XBMCController$startKeyPressTimer, "v@:i");  
  // XBMCController::stopKeyPressTimer
  class_addMethod(XBMCControllerCls, @selector(stopKeyPressTimer), (IMP)&XBMCController$stopKeyPressTimer, "v@:");
  // XBMCController::disableSystemSleep
  class_addMethod(XBMCControllerCls, @selector(disableSystemSleep), (IMP)&XBMCController$disableSystemSleep, "v@:");
  // XBMCController__enableSystemSleep
  class_addMethod(XBMCControllerCls, @selector(enableSystemSleep), (IMP)&XBMCController$enableSystemSleep, "v@:");
  // XBMCController::disableScreenSaver
  class_addMethod(XBMCControllerCls, @selector(disableScreenSaver), (IMP)&XBMCController$disableScreenSaver, "v@:");
  // XBMCController::enableScreenSaver
  class_addMethod(XBMCControllerCls, @selector(enableScreenSaver), (IMP)&XBMCController$enableScreenSaver, "v@:");
  // XBMCController::pauseAnimation
  class_addMethod(XBMCControllerCls, @selector(pauseAnimation), (IMP)&XBMCController$pauseAnimation, "v@:");
  // XBMCController::resumeAnimation
  class_addMethod(XBMCControllerCls, @selector(resumeAnimation), (IMP)&XBMCController$resumeAnimation, "v@:");
  // XBMCController::startAnimation
  class_addMethod(XBMCControllerCls, @selector(startAnimation), (IMP)&XBMCController$startAnimation, "v@:");
  // XBMCController::stopAnimation
  class_addMethod(XBMCControllerCls, @selector(stopAnimation), (IMP)&XBMCController$stopAnimation, "v@:");

  i = 0;
  memcpy(_typeEncoding + i, @encode(CGSize), strlen(@encode(CGSize)));
  i += strlen(@encode(CGSize));
  _typeEncoding[i] = '@';
  i += 1;
  _typeEncoding[i] = ':';
  i += 1;
  _typeEncoding[i] = '\0';
  // XBMCController::getScreenSize
  class_addMethod(XBMCControllerCls, @selector(getScreenSize), (IMP)&XBMCController$getScreenSize, _typeEncoding);

  i = 0;
  _typeEncoding[i] = 'v';
  i += 1;
  _typeEncoding[i] = '@';
  i += 1;
  _typeEncoding[i] = ':';
  i += 1;
  memcpy(_typeEncoding + i, @encode(XBMCKey), strlen(@encode(XBMCKey)));
  i += strlen(@encode(XBMCKey));
  _typeEncoding[i] = '\0';
  // XBMCController::sendKey
  class_addMethod(XBMCControllerCls, @selector(sendKey:), (IMP)&XBMCController$sendKey, _typeEncoding);
 
  i = 0;
  _typeEncoding[i] = 'v';
  i += 1;
  _typeEncoding[i] = '@';
  i += 1;
  _typeEncoding[i] = ':';
  i += 1;
  memcpy(_typeEncoding + i, @encode(BREvent*), strlen(@encode(BREvent*)));
  i += strlen(@encode(BREvent*));
  _typeEncoding[i] = '^';
  _typeEncoding[i + 1] = 'B';
  i += 2;
  _typeEncoding[i] = '^';
  _typeEncoding[i + 1] = 'B';
  i += 2;
  _typeEncoding[i] = '^';
  _typeEncoding[i + 1] = 'i';
  i += 2;
  _typeEncoding[i] = '\0';
  // XBMCController::ATVClientEventFromBREvent
  class_addMethod(XBMCControllerCls, @selector(ATVClientEventFromBREvent:Repeatable:ButtonState:Result:), (IMP)&XBMCController$ATVClientEventFromBREvent, _typeEncoding);

  i = 0;
  _typeEncoding[i] = 'v';
  i += 1;
  _typeEncoding[i] = '@';
  i += 1;
  _typeEncoding[i] = ':';
  i += 1;
  memcpy(_typeEncoding + i, @encode(NSTimer*), strlen(@encode(NSTimer*)));
  i += strlen(@encode(NSTimer*));
  _typeEncoding[i] = '\0';
  // XBMCController::keyPressTimerCallback
  class_addMethod(XBMCControllerCls, @selector(keyPressTimerCallback:), (IMP)&XBMCController$keyPressTimerCallback, _typeEncoding);
 
  i = 0;
  _typeEncoding[i] = 'v';
  i += 1;
  _typeEncoding[i] = '@';
  i += 1;
  _typeEncoding[i] = ':';
  i += 1;
  memcpy(_typeEncoding + i, @encode(NSNotification *), strlen(@encode(NSNotification *)));
  i += strlen(@encode(NSNotification *));
  _typeEncoding[i] = '\0';
  // XBMCController:observeDefaultCenterStuff
  class_addMethod(XBMCControllerCls, @selector(observeDefaultCenterStuff:), (IMP)&XBMCController$observeDefaultCenterStuff, _typeEncoding);

  i = 0;
  _typeEncoding[i] = 'B';
  i += 1;
  _typeEncoding[i] = '@';
  i += 1;
  _typeEncoding[i] = ':';
  i += 1;
  _typeEncoding[i] = 'I';
  i += 1;
  memcpy(_typeEncoding + i, @encode(UIScreenMode *), strlen(@encode(UIScreenMode *)));
  i += strlen(@encode(UIScreenMode *));
  _typeEncoding[i] = '\0';
  // XBMCController::changeScreen
  class_addMethod(XBMCControllerCls, @selector(changeScreen:withMode:), (IMP)&XBMCController$changeScreen, _typeEncoding);

  i = 0;
  _typeEncoding[i] = 'v';
  i += 1;
  _typeEncoding[i] = '@';
  i += 1;
  _typeEncoding[i] = ':';
  i += 1;
  memcpy(_typeEncoding + i, @encode(UIScreen *), strlen(@encode(UIScreen *)));
  i += strlen(@encode(UIScreen *));
  _typeEncoding[i] = '\0';
  // XBMCController::activateScreen$
  class_addMethod(XBMCControllerCls, @selector(activateScreen:), (IMP)&XBMCController$activateScreen, _typeEncoding);

  // and hook up our methods (implementation of the base class methods)
  // XBMCController::brEventAction
  MSHookMessageEx(XBMCControllerCls, @selector(brEventAction:), (IMP)&XBMCController$brEventAction, (IMP*)&XBMCController$brEventAction$Orig); 
  // XBMCController::init
  MSHookMessageEx(XBMCControllerCls, @selector(init), (IMP)&XBMCController$init, (IMP*)&XBMCController$init$Orig);
  // XBMCController::dealloc
  MSHookMessageEx(XBMCControllerCls, @selector(dealloc), (IMP)&XBMCController$dealloc, (IMP*)&XBMCController$dealloc$Orig);
  // XBMCController::controlWasActivated
  MSHookMessageEx(XBMCControllerCls, @selector(controlWasActivated), (IMP)&XBMCController$controlWasActivated, (IMP*)&XBMCController$controlWasActivated$Orig);
  // XBMCController::controlWasDeactivated
  MSHookMessageEx(XBMCControllerCls, @selector(controlWasDeactivated), (IMP)&XBMCController$controlWasDeactivated, (IMP*)&XBMCController$controlWasDeactivated$Orig);
  // XBMCController::recreateOnReselect
  MSHookMessageEx(XBMCControllerCls, @selector(recreateOnReselect), (IMP)&XBMCController$recreateOnReselect, nil);

  // and register the class to the runtime
  objc_registerClassPair(XBMCControllerCls);
  
  // save this as static for referencing it in multiple methods
  BRWindowCls = objc_getClass("BRWindow");
}
