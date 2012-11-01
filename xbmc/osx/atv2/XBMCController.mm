/*
 *      Copyright (C) 2010-2012 Team XBMC
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
#import <BackRow/BackRow.h>

#import "XBMCController.h"
#import "XBMCDebugHelpers.h"

//start repeating after 0.5s
#define REPEATED_KEYPRESS_DELAY_S     0.5
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
@interface ATVSettingsFacade : BRSettingsFacade {}
-(int)screenSaverTimeout;
-(void)setScreenSaverTimeout:(int) f_timeout;
-(void)setSleepTimeout:(int)timeout;
-(int)sleepTimeout;
@end

// notification messages
extern NSString* kBRScreenSaverActivated;
extern NSString* kBRScreenSaverDismissed;
//--------------------------------------------------------------
//--------------------------------------------------------------
@interface XBMCController (PrivateMethods)
- (void) observeDefaultCenterStuff: (NSNotification *) notification;
- (void) keyPressTimerCallback: (NSTimer*)theTimer;
- (void) startKeyPressTimer: (int) keyId;
- (void) stopKeyPressTimer;
- (void) setUserEvent:(int) id withHoldTime:(unsigned int) holdTime;
@end
//
//
@implementation XBMCController

/*
+ (XBMCController*) sharedInstance
{
  // the instance of this class is stored here
  static XBMCController *myInstance = nil;

  // check to see if an instance already exists
  if (nil == myInstance)
    myInstance  = [[[[self class] alloc] init] autorelease];

  // return the instance of this class
  return myInstance;
}
*/

- (void) applicationDidExit
{
  [m_glView stopAnimation];

  [self enableScreenSaver];
  [self enableSystemSleep];

  [[self stack] popController];
}
- (void) initDisplayLink
{
  [m_glView initDisplayLink];
}
- (void) deinitDisplayLink
{
  [m_glView deinitDisplayLink];
}
- (double) getDisplayLinkFPS
{
  return [m_glView getDisplayLinkFPS];
}
- (void) setFramebuffer
{
  [m_glView setFramebuffer];
}
- (bool) presentFramebuffer
{
  return [m_glView presentFramebuffer];
}
- (CGSize) getScreenSize
{
  CGSize screensize;

  screensize.width  = [BRWindow interfaceFrame].size.width;
  screensize.height = [BRWindow interfaceFrame].size.height;

  //NSLog(@"%s UpdateResolutions width=%f, height=%f", 
	//	__PRETTY_FUNCTION__, screensize.width, screensize.height);

  return screensize;
}
- (void) sendKey: (XBMCKey) key
{
  //empty because its not used here. Only implemented for getting rid
  //of "may not respond to selector" compile warnings in IOSExternalTouchController
}


- (id) init
{  
  //NSLog(@"%s", __PRETTY_FUNCTION__);

  self = [super init];
  if ( !self )
    return ( nil );

  NSNotificationCenter *center;
  // first the default notification center, which is all
  // notifications that only happen inside of our program
  center = [NSNotificationCenter defaultCenter];
  [center addObserver: self
    selector: @selector(observeDefaultCenterStuff:)
    name: nil
    object: nil];

  m_glView = [[IOSEAGLView alloc] initWithFrame:[BRWindow interfaceFrame] withScreen:[UIScreen mainScreen]];
  [[IOSScreenManager sharedInstance] setView:m_glView];

  g_xbmcController = self;

  return self;
}

- (void)dealloc
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);
  [m_glView stopAnimation];
  [m_glView release];


  NSNotificationCenter *center;
  // take us off the default center for our app
  center = [NSNotificationCenter defaultCenter];
  [center removeObserver: self];

  [super dealloc];
}

- (void)controlWasActivated
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);
  
  [super controlWasActivated];

  [self disableSystemSleep];
  [self disableScreenSaver];

  //inject our gles layer into the backrow root layer
  [[BRWindow rootLayer] addSublayer:m_glView.layer];

  [m_glView startAnimation];
}

- (void)controlWasDeactivated
{
  NSLog(@"XBMC was forced by FrontRow to exit via controlWasDeactivated");

  [m_glView stopAnimation];
  [m_glView.layer removeFromSuperlayer];

  [self enableScreenSaver];
  [self enableSystemSleep];

  [super controlWasDeactivated];
}

- (BOOL) recreateOnReselect
{ 
  //NSLog(@"%s", __PRETTY_FUNCTION__);
  return YES;
}

- (eATVClientEvent) ATVClientEventFromBREvent:(BREvent*) f_event 
                    Repeatable:(bool &) isRepeatable
                    ButtonState:(bool &) isPressed
{
  int remoteAction = [f_event remoteAction];
  CLog::Log(LOGDEBUG,"XBMCPureController: Button press remoteAction = %i", remoteAction);
  isRepeatable = false;
  isPressed = false;

  switch (remoteAction)
  {
    // tap up
    case kBREventRemoteActionUp:
    case 65676:
      isRepeatable = true;
      if([f_event value] == 1)
        isPressed = true;
      return ATV_BUTTON_UP;

    // tap down
    case kBREventRemoteActionDown:
    case 65677:
      isRepeatable = true;
      if([f_event value] == 1)
        isPressed = true;
      return ATV_BUTTON_DOWN;

    // tap left
    case kBREventRemoteActionLeft:
    case 65675:
      isRepeatable = true;
      if([f_event value] == 1)
        isPressed = true;
      return ATV_BUTTON_LEFT;

    // hold left
    case 786612:
      if([f_event value] == 1)
        return ATV_LEARNED_REWIND;
      else
        return ATV_INVALID_BUTTON;

    // tap right
    case kBREventRemoteActionRight:
    case 65674:
      isRepeatable = true;
      if ([f_event value] == 1)
        isPressed = true;
      return ATV_BUTTON_RIGHT;

    // hold right
    case 786611:
      if ([f_event value] == 1)
        return ATV_LEARNED_FORWARD;
      else
        return ATV_INVALID_BUTTON;

    // tap play
    case kBREventRemoteActionPlay:
    case 65673:
      return ATV_BUTTON_PLAY;

    // hold play
    case kBREventRemoteActionPlayHold:
    case kBREventRemoteActionCenterHold:
    case kBREventRemoteActionCenterHold42:
    case 65668:
      return ATV_BUTTON_PLAY_H;

    // menu
    case kBREventRemoteActionMenu:
    case 65670:
      return ATV_BUTTON_MENU;

    // hold menu
    case kBREventRemoteActionMenuHold:
    case 786496:
      return ATV_BUTTON_MENU_H;

    // learned play
    case 786608:
      return ATV_LEARNED_PLAY;

    // learned pause
    case 786609:
      return ATV_LEARNED_PAUSE;

    // learned stop
    case 786615:
      return ATV_LEARNED_STOP;

    // learned next
    case 786613:
      return ATV_LEARNED_NEXT;

    // learned previous
    case 786614:
      return ATV_LEARNED_PREVIOUS;

    // learned enter, like go into something
    case 786630:
      return ATV_LEARNED_ENTER;

    // learned return, like go back
    case 786631:
      return ATV_LEARNED_RETURN;

    // tap play on new Al IR remote
    case kBREventRemoteActionALPlay:
    case 786637:
      return ATV_ALUMINIUM_PLAY;

    case kBREventRemoteActionKeyPress:
    case kBREventRemoteActionKeyPress42:
      return ATV_BTKEYPRESS;

    // PageUp
    case kBREventRemoteActionPageUp:
      return ATV_BUTTON_PAGEUP;

    // PageDown
    case kBREventRemoteActionPageDown:
      return ATV_BUTTON_PAGEDOWN;

    // Pause
    case kBREventRemoteActionPause:
      return ATV_BUTTON_PAUSE;

    // Play2
    case kBREventRemoteActionPlay2:
      return ATV_BUTTON_PLAY2;

    // Stop
    case kBREventRemoteActionStop:
      return ATV_BUTTON_STOP;

    // Fast Forward
    case kBREventRemoteActionFastFwd:
      return ATV_BUTTON_FASTFWD;

    // Rewind
    case kBREventRemoteActionRewind:
      return ATV_BUTTON_REWIND;

    // Skip Forward
    case kBREventRemoteActionSkipFwd:
      return ATV_BUTTON_SKIPFWD;

    // Skip Back
    case kBREventRemoteActionSkipBack:
      return ATV_BUTTON_SKIPBACK;

    // Gesture Swipe Left
    case kBREventRemoteActionSwipeLeft:
      if ([f_event value] == 1)
        return ATV_GESTURE_SWIPE_LEFT;
      else
        return ATV_INVALID_BUTTON;

    // Gesture Swipe Right
    case kBREventRemoteActionSwipeRight:
      if ([f_event value] == 1)
        return ATV_GESTURE_SWIPE_RIGHT;
      else
        return ATV_INVALID_BUTTON;

    // Gesture Swipe Up
    case kBREventRemoteActionSwipeUp:
      if ([f_event value] == 1)
        return ATV_GESTURE_SWIPE_UP;
      else
        return ATV_INVALID_BUTTON;

    // Gesture Swipe Down
    case kBREventRemoteActionSwipeDown:
      if ([f_event value] == 1)
        return ATV_GESTURE_SWIPE_DOWN;
      else
        return ATV_INVALID_BUTTON;

    // Gesture Flick Left
    case kBREventRemoteActionFlickLeft:
      if ([f_event value] == 1)
        return ATV_GESTURE_FLICK_LEFT;
      else
        return ATV_INVALID_BUTTON;

    // Gesture Flick Right
    case kBREventRemoteActionFlickRight:
      if ([f_event value] == 1)
        return ATV_GESTURE_FLICK_RIGHT;
      else
        return ATV_INVALID_BUTTON;

    // Gesture Flick Up
    case kBREventRemoteActionFlickUp:
      if ([f_event value] == 1)
        return ATV_GESTURE_FLICK_UP;
      else
        return ATV_INVALID_BUTTON;

    // Gesture Flick Down
    case kBREventRemoteActionFlickDown:
      if ([f_event value] == 1)
        return ATV_GESTURE_FLICK_DOWN;
      else
        return ATV_INVALID_BUTTON;



    default:
      ELOG(@"XBMCPureController: Unknown button press remoteAction = %i", remoteAction);
      return ATV_INVALID_BUTTON;
  }
}

- (void)setUserEvent:(int) id withHoldTime:(unsigned int) holdTime
{
  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));

  newEvent.type = XBMC_USEREVENT;
  newEvent.jbutton.which = id;
  newEvent.jbutton.holdTime = holdTime;
  CWinEventsIOS::MessagePush(&newEvent);
}

- (BOOL)brEventAction:(BREvent*)event
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);

	if ([m_glView isAnimating])
  {
    BOOL is_handled = NO;
    bool isRepeatable = false;
    bool isPressed = false;
    eATVClientEvent xbmc_ir_key = [self ATVClientEventFromBREvent:event 
                                        Repeatable:isRepeatable
                                        ButtonState:isPressed];
    
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

          if (wstr[0] != 92) // trap out "\" which toggle fullscreen/windowed
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
            [self setUserEvent:xbmc_ir_key withHoldTime:0]; //fire event
            [self startKeyPressTimer:xbmc_ir_key];//start repeat timer
          }
          else
          {
            //stop the timer
            [self stopKeyPressTimer];
          }
        }
        else
        {
          [self setUserEvent:xbmc_ir_key  withHoldTime:0];
        }
        is_handled = TRUE;
      }
    }
    return is_handled;
	}
  else
  {
		return [super brEventAction:event];
	}
}

#pragma mark -
#pragma mark private helper methods
- (void)startKeyPressTimer: (int) keyId
{ 
  NSNumber *number = [NSNumber numberWithInt:keyId]; 
  NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:[NSDate date], @"StartDate", 
                                                                  number, @"keyId", nil];
  
  NSDate *fireDate = [NSDate dateWithTimeIntervalSinceNow:REPEATED_KEYPRESS_DELAY_S]; 

  [self stopKeyPressTimer];

  //schedule repeated timer which starts after REPEATED_KEYPRESS_DELAY_S and fires
  //every REPEATED_KEYPRESS_PAUSE_S
  m_keyTimer       = [[NSTimer alloc] initWithFireDate:fireDate 
                                      interval:REPEATED_KEYPRESS_PAUSE_S 
                                      target:self 
                                      selector:@selector(keyPressTimerCallback:) 
                                      userInfo:dict 
                                      repeats:YES]; 
  //schedule the timer to the runloop
  NSRunLoop *runLoop = [NSRunLoop currentRunLoop]; 
  [runLoop addTimer:m_keyTimer forMode:NSDefaultRunLoopMode]; 
} 

- (void)stopKeyPressTimer
{
  if(m_keyTimer != nil)
  {
    [m_keyTimer invalidate];
    [m_keyTimer release];
    m_keyTimer = nil;
  }
}

- (void)keyPressTimerCallback:(NSTimer*)theTimer 
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

//
- (void)observeDefaultCenterStuff: (NSNotification *) notification
{
  //NSLog(@"default: %@", [notification name]);

  if ([notification name] == UIApplicationDidReceiveMemoryWarningNotification)
    NSLog(@"XBMC: %@", [notification name]);

  //if ([notification name] == kBRScreenSaverActivated)
  //  [m_glView stopAnimation];
  
  //if ([notification name] == kBRScreenSaverDismissed)
  //  [m_glView startAnimation];
}

- (void) disableSystemSleep
{
  m_systemsleepTimeout = [[ATVSettingsFacade singleton] sleepTimeout];
  [[ATVSettingsFacade singleton] setSleepTimeout: -1];
  [[ATVSettingsFacade singleton] flushDiskChanges];
}

- (void) enableSystemSleep
{
  [[ATVSettingsFacade singleton] setSleepTimeout: m_systemsleepTimeout];
  [[ATVSettingsFacade singleton] flushDiskChanges];
}

- (void) disableScreenSaver
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);
  //store screen saver state and disable it

  m_screensaverTimeout = [[ATVSettingsFacade singleton] screenSaverTimeout];
  [[ATVSettingsFacade singleton] setScreenSaverTimeout: -1];
  [[ATVSettingsFacade singleton] flushDiskChanges];

  // breaks in 4.2.1 [[BRBackgroundTaskManager singleton] holdOffBackgroundTasks];
}

- (void) enableScreenSaver
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);
  //reset screen saver to user settings

  [[ATVSettingsFacade singleton] setScreenSaverTimeout: m_screensaverTimeout];
  [[ATVSettingsFacade singleton] flushDiskChanges];

  // breaks in 4.2.1 [[BRBackgroundTaskManager singleton] okToDoBackgroundProcessing];
}

- (XBMC_Event) translateCocoaToXBMCEvent: (unichar) c
{
  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));
/*
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
*/
  return newEvent;
}

//--------------------------------------------------------------
- (void)pauseAnimation
{
  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(XBMC_Event));
  
  newEvent.appcommand.type = XBMC_APPCOMMAND;
  newEvent.appcommand.action = ACTION_PLAYER_PLAYPAUSE;
  CWinEventsIOS::MessagePush(&newEvent);
  
  /* Give player time to pause */
  Sleep(2000);
  //NSLog(@"%s", __PRETTY_FUNCTION__);
  
  [m_glView pauseAnimation];
  
}
//--------------------------------------------------------------
- (void)resumeAnimation
{  
  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(XBMC_Event));
  
  newEvent.appcommand.type = XBMC_APPCOMMAND;
  newEvent.appcommand.action = ACTION_PLAYER_PLAY;
  CWinEventsIOS::MessagePush(&newEvent);    
  
  [m_glView resumeAnimation];
}
//--------------------------------------------------------------
- (void)startAnimation
{
  [m_glView startAnimation];
}
//--------------------------------------------------------------
- (void)stopAnimation
{
  [m_glView stopAnimation];
}
//--------------------------------------------------------------
- (bool) changeScreen: (unsigned int)screenIdx withMode:(UIScreenMode *)mode
{
  return [[IOSScreenManager sharedInstance] changeScreen: screenIdx withMode: mode];
}
//--------------------------------------------------------------
- (void) activateScreen: (UIScreen *)screen
{
}
@end

