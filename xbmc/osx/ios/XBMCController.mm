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
#include <sys/resource.h>
#include <signal.h>

#include "system.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "ApplicationMessenger.h"
#include "input/MouseStat.h"
#include "windowing/WindowingFactory.h"
#include "video/VideoReferenceClock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "Util.h"
#include "threads/Event.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795028842
#endif
#define RADIANS_TO_DEGREES(radians) ((radians) * (180.0 / M_PI))

#undef BOOL

#import "IOSEAGLView.h"

#import "XBMCController.h"
#import "IOSScreenManager.h"
#import "XBMCApplication.h"
#import "XBMCDebugHelpers.h"

XBMCController *g_xbmcController;
static CEvent screenChangeEvent;


// notification messages
extern NSString* kBRScreenSaverActivated;
extern NSString* kBRScreenSaverDismissed;

//--------------------------------------------------------------
//

@interface XBMCController ()

@end

@interface UIApplication (extended)
-(void) terminateWithSuccess;
@end

@implementation XBMCController
@synthesize animating;
@synthesize lastGesturePoint;
@synthesize screenScale;
@synthesize lastEvent;
@synthesize touchBeginSignaled;
@synthesize m_screenIdx;
@synthesize screensize;
//--------------------------------------------------------------
-(BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{  
  //on external screens somehow the logic is rotated by 90°
  //so we have to do this with our supported orientations then aswell
  if([[IOSScreenManager sharedInstance] isExternalScreen])
  {
    if(interfaceOrientation == UIInterfaceOrientationPortrait) 
    {
      return YES;
    }
  }
  else//internal screen
  {
    if(interfaceOrientation == UIInterfaceOrientationLandscapeLeft) 
    {
      return YES;
    }
    else if(interfaceOrientation == UIInterfaceOrientationLandscapeRight)
    {
      return YES;
    }
  }
  return NO;
}
//--------------------------------------------------------------
- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
  orientation = toInterfaceOrientation;
  CGRect srect = [IOSScreenManager getLandscapeResolution: [m_glView getCurrentScreen]];
  CGRect rect = srect;;
  

  switch(toInterfaceOrientation)
  {
    case UIInterfaceOrientationPortrait:  
    case UIInterfaceOrientationPortraitUpsideDown:
      if(![[IOSScreenManager sharedInstance] isExternalScreen]) 
      {
        rect.size = CGSizeMake( srect.size.height, srect.size.width );    
      }
      break;
    case UIInterfaceOrientationLandscapeLeft:
    case UIInterfaceOrientationLandscapeRight:
      break;//just leave the rect as is
  }  
	m_glView.frame = rect;
}

- (UIInterfaceOrientation) getOrientation
{
	return orientation;
}

-(void)sendKey:(XBMCKey) key
{
  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));
  
  //newEvent.key.keysym.unicode = key;
  newEvent.key.keysym.sym = key;
  
  newEvent.type = XBMC_KEYDOWN;
  CWinEventsIOS::MessagePush(&newEvent);
  
  newEvent.type = XBMC_KEYUP;
  CWinEventsIOS::MessagePush(&newEvent);
}
//--------------------------------------------------------------
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer
{
  if ([gestureRecognizer isKindOfClass:[UIRotationGestureRecognizer class]] && [otherGestureRecognizer isKindOfClass:[UIPinchGestureRecognizer class]]) {
    return YES;
  }

  return NO;
}
//--------------------------------------------------------------
- (void)createGestureRecognizers 
{
  //2 finger single tab - right mouse
  //single finger double tab delays single finger single tab - so we
  //go for 2 fingers here - so single finger single tap is instant
  UITapGestureRecognizer *doubleFingerSingleTap = [[UITapGestureRecognizer alloc]
    initWithTarget:self action:@selector(handleDoubleFingerSingleTap:)];  

  doubleFingerSingleTap.delaysTouchesBegan = YES;
  doubleFingerSingleTap.numberOfTapsRequired = 1;
  doubleFingerSingleTap.numberOfTouchesRequired = 2;
  [self.view addGestureRecognizer:doubleFingerSingleTap];
  [doubleFingerSingleTap release];

  //1 finger single long tab - right mouse - alernative
  UILongPressGestureRecognizer *singleFingerSingleLongTap = [[UILongPressGestureRecognizer alloc]
    initWithTarget:self action:@selector(handleSingleFingerSingleLongTap:)];  

  singleFingerSingleLongTap.delaysTouchesBegan = YES;
  singleFingerSingleLongTap.delaysTouchesEnded = YES;
  [self.view addGestureRecognizer:singleFingerSingleLongTap];
  [singleFingerSingleLongTap release];

  //double finger swipe left for backspace ... i like this fast backspace feature ;)
  UISwipeGestureRecognizer *swipeLeft = [[UISwipeGestureRecognizer alloc]
    initWithTarget:self action:@selector(handleSwipeLeft:)];

  swipeLeft.delaysTouchesBegan = YES;
  swipeLeft.numberOfTouchesRequired = 2;
  swipeLeft.direction = UISwipeGestureRecognizerDirectionLeft;
  [self.view addGestureRecognizer:swipeLeft];
  [swipeLeft release];

  //for pan gestures with one finger
  UIPanGestureRecognizer *pan = [[UIPanGestureRecognizer alloc]
    initWithTarget:self action:@selector(handlePan:)];

  pan.delaysTouchesBegan = YES;
  pan.maximumNumberOfTouches = 1;
  [self.view addGestureRecognizer:pan];
  [pan release];

  //for zoom gesture
  UIPinchGestureRecognizer *pinch = [[UIPinchGestureRecognizer alloc]
    initWithTarget:self action:@selector(handlePinch:)];

  pinch.delaysTouchesBegan = YES;
  pinch.delegate = self;
  [self.view addGestureRecognizer:pinch];
  [pinch release];

  //for rotate gesture
  UIRotationGestureRecognizer *rotate = [[UIRotationGestureRecognizer alloc]
                                         initWithTarget:self action:@selector(handleRotate:)];

  rotate.delaysTouchesBegan = YES;
  rotate.delegate = self;
  [self.view addGestureRecognizer:rotate];
  [rotate release];
}
//--------------------------------------------------------------
- (void) activateKeyboard:(UIView *)view
{
  [self.view addSubview:view];
}
//--------------------------------------------------------------
- (void) deactivateKeyboard:(UIView *)view
{
  [view removeFromSuperview];
}
//--------------------------------------------------------------
-(void)handlePinch:(UIPinchGestureRecognizer*)sender 
{
  if( [m_glView isXBMCAlive] )//NO GESTURES BEFORE WE ARE UP AND RUNNING
  {
    CGPoint point = [sender locationOfTouch:0 inView:m_glView];  
    point.x *= screenScale;
    point.y *= screenScale;
  
    switch(sender.state)
    {
      case UIGestureRecognizerStateBegan:  
        CApplicationMessenger::Get().SendAction(CAction(ACTION_GESTURE_BEGIN, 0, (float)point.x, (float)point.y,
                                                        0, 0), WINDOW_INVALID,false);
        break;
      case UIGestureRecognizerStateChanged:
        CApplicationMessenger::Get().SendAction(CAction(ACTION_GESTURE_ZOOM, 0, (float)point.x, (float)point.y, 
                                                                   [sender scale], 0), WINDOW_INVALID,false);
        break;
      case UIGestureRecognizerStateEnded:
        CApplicationMessenger::Get().SendAction(CAction(ACTION_GESTURE_END, 0, 0, 0,
                                                        0, 0), WINDOW_INVALID,false);
        break;
      default:
        break;
    }
  }
}
//--------------------------------------------------------------
-(void)handleRotate:(UIRotationGestureRecognizer*)sender
{
  if( [m_glView isXBMCAlive] )//NO GESTURES BEFORE WE ARE UP AND RUNNING
  {
    CGPoint point = [sender locationOfTouch:0 inView:m_glView];
    point.x *= screenScale;
    point.y *= screenScale;

    switch(sender.state)
    {
      case UIGestureRecognizerStateBegan:
        CApplicationMessenger::Get().SendAction(CAction(ACTION_GESTURE_BEGIN, 0, (float)point.x, (float)point.y,
                                                        0, 0), WINDOW_INVALID,false);
        break;
      case UIGestureRecognizerStateChanged:
        CApplicationMessenger::Get().SendAction(CAction(ACTION_GESTURE_ROTATE, 0, (float)point.x, (float)point.y,
                                                        RADIANS_TO_DEGREES([sender rotation]), 0), WINDOW_INVALID,false);
        break;
      case UIGestureRecognizerStateEnded:
        break;
      default:
        break;
    }
  }
}
//--------------------------------------------------------------
- (IBAction)handlePan:(UIPanGestureRecognizer *)sender 
{
  if( [m_glView isXBMCAlive] )//NO GESTURES BEFORE WE ARE UP AND RUNNING
  { 
    if( [sender state] == UIGestureRecognizerStateBegan )
    {
      CGPoint point = [sender locationOfTouch:0 inView:m_glView];  
      point.x *= screenScale;
      point.y *= screenScale;
      touchBeginSignaled = false;
      lastGesturePoint = point;
    }
    
    if( [sender state] == UIGestureRecognizerStateChanged )
    {
      CGPoint point = [sender locationOfTouch:0 inView:m_glView];    
      point.x *= screenScale;
      point.y *= screenScale;
      bool bNotify = false;
      CGFloat yMovement=point.y - lastGesturePoint.y;
      CGFloat xMovement=point.x - lastGesturePoint.x;
      
      if( xMovement )
      {
        bNotify = true;
      }
      
      if( yMovement )
      {
        bNotify = true;
      }
      
      if( bNotify )
      {
        if( !touchBeginSignaled )
        {
          CApplicationMessenger::Get().SendAction(CAction(ACTION_GESTURE_BEGIN, 0, (float)point.x, (float)point.y, 
                                                            0, 0), WINDOW_INVALID,false);
          touchBeginSignaled = true;
        }    
        
        CApplicationMessenger::Get().SendAction(CAction(ACTION_GESTURE_PAN, 0, (float)point.x, (float)point.y,
                                                          xMovement, yMovement), WINDOW_INVALID,false);
        lastGesturePoint = point;
      }
    }
    
    if( touchBeginSignaled && [sender state] == UIGestureRecognizerStateEnded )
    {
      CGPoint velocity = [sender velocityInView:m_glView];
      //signal end of pan - this will start inertial scrolling with deacceleration in CApplication
      CApplicationMessenger::Get().SendAction(CAction(ACTION_GESTURE_END, 0, (float)velocity.x, (float)velocity.y, (int)lastGesturePoint.x, (int)lastGesturePoint.y),WINDOW_INVALID,false);
      touchBeginSignaled = false;
    }
  }
}
//--------------------------------------------------------------
- (IBAction)handleSwipeLeft:(UISwipeGestureRecognizer *)sender 
{
  if( [m_glView isXBMCAlive] )//NO GESTURES BEFORE WE ARE UP AND RUNNING
  {
    [self sendKey:XBMCK_BACKSPACE];
  }
}
//--------------------------------------------------------------
- (void)postMouseMotionEvent:(CGPoint)point
{
  XBMC_Event newEvent;

  point.x *= screenScale;
  point.y *= screenScale;

  memset(&newEvent, 0, sizeof(newEvent));

  newEvent.type = XBMC_MOUSEMOTION;
  newEvent.motion.type = XBMC_MOUSEMOTION;
  newEvent.motion.which = 0;
  newEvent.motion.state = 0;
  newEvent.motion.x = point.x;
  newEvent.motion.y = point.y;
  newEvent.motion.xrel = 0;
  newEvent.motion.yrel = 0;
  CWinEventsIOS::MessagePush(&newEvent);
}
//--------------------------------------------------------------
- (IBAction)handleDoubleFingerSingleTap:(UIGestureRecognizer *)sender 
{
  if( [m_glView isXBMCAlive] )//NO GESTURES BEFORE WE ARE UP AND RUNNING
  {
    CGPoint point = [sender locationOfTouch:0 inView:m_glView];
    point.x *= screenScale;
    point.y *= screenScale;
    //NSLog(@"%s toubleTap", __PRETTY_FUNCTION__);

    [self postMouseMotionEvent:point];

    XBMC_Event newEvent;
    memset(&newEvent, 0, sizeof(newEvent));
    
    newEvent.type = XBMC_MOUSEBUTTONDOWN;
    newEvent.button.type = XBMC_MOUSEBUTTONDOWN;
    newEvent.button.button = XBMC_BUTTON_RIGHT;
    newEvent.button.x = point.x;
    newEvent.button.y = point.y;
    
    CWinEventsIOS::MessagePush(&newEvent);    
    
    newEvent.type = XBMC_MOUSEBUTTONUP;
    newEvent.button.type = XBMC_MOUSEBUTTONUP;
    CWinEventsIOS::MessagePush(&newEvent);    
    
    memset(&lastEvent, 0x0, sizeof(XBMC_Event));         
  }
}
//--------------------------------------------------------------
- (IBAction)handleSingleFingerSingleLongTap:(UIGestureRecognizer *)sender
{
  if( [m_glView isXBMCAlive] )//NO GESTURES BEFORE WE ARE UP AND RUNNING
  {
    if (sender.state == UIGestureRecognizerStateBegan)
    {
      CGPoint point = [sender locationOfTouch:0 inView:m_glView];
      [self postMouseMotionEvent:point];//selects the current control
    }

    if (sender.state == UIGestureRecognizerStateEnded)
    {
      [self handleDoubleFingerSingleTap:sender];
    }
  }
}
//--------------------------------------------------------------
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event 
{
  if( [m_glView isXBMCAlive] )//NO GESTURES BEFORE WE ARE UP AND RUNNING
  {
    UITouch *touch = [touches anyObject];
    
    if( [touches count] == 1 && [touch tapCount] == 1)
    {
      lastGesturePoint = [touch locationInView:m_glView];    
      [self postMouseMotionEvent:lastGesturePoint];//selects the current control

      lastGesturePoint.x *= screenScale;
      lastGesturePoint.y *= screenScale;  
      XBMC_Event newEvent;
      memset(&newEvent, 0, sizeof(newEvent));
      
      newEvent.type = XBMC_MOUSEBUTTONDOWN;
      newEvent.button.type = XBMC_MOUSEBUTTONDOWN;
      newEvent.button.button = XBMC_BUTTON_LEFT;
      newEvent.button.x = lastGesturePoint.x;
      newEvent.button.y = lastGesturePoint.y;  
      CWinEventsIOS::MessagePush(&newEvent);    
      
      /* Store the tap action for later */
      lastEvent = newEvent;
    }
  }
}
//--------------------------------------------------------------
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event 
{
  
}
//--------------------------------------------------------------
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event 
{
  if( [m_glView isXBMCAlive] )//NO GESTURES BEFORE WE ARE UP AND RUNNING
  {
    UITouch *touch = [touches anyObject];
    
    if( [touches count] == 1 && [touch tapCount] == 1 )
    {
      XBMC_Event newEvent = lastEvent;
      
      newEvent.type = XBMC_MOUSEBUTTONUP;
      newEvent.button.type = XBMC_MOUSEBUTTONUP;
      newEvent.button.button = XBMC_BUTTON_LEFT;
      newEvent.button.x = lastGesturePoint.x;
      newEvent.button.y = lastGesturePoint.y;
      CWinEventsIOS::MessagePush(&newEvent);
      
      memset(&lastEvent, 0x0, sizeof(XBMC_Event));     
    }
  }
}
//--------------------------------------------------------------
- (id)initWithFrame:(CGRect)frame withScreen:(UIScreen *)screen
{ 
  //NSLog(@"%s", __PRETTY_FUNCTION__);
  m_screenIdx = 0;
  self = [super init];
  if ( !self )
    return ( nil );

  m_window = [[UIWindow alloc] initWithFrame:frame];
  [m_window setRootViewController:self];  
  m_window.screen = screen;
  /* Turn off autoresizing */
  m_window.autoresizingMask = 0;
  m_window.autoresizesSubviews = NO;
  
  NSNotificationCenter *center;
  center = [NSNotificationCenter defaultCenter];
  [center addObserver: self
             selector: @selector(observeDefaultCenterStuff:)
                 name: nil
               object: nil];

  /* We start in landscape mode */
  CGRect srect = frame;
  srect.size = CGSizeMake( frame.size.height, frame.size.width );
  orientation = UIInterfaceOrientationLandscapeLeft;
  
  m_glView = [[IOSEAGLView alloc] initWithFrame: srect withScreen:screen];
  [[IOSScreenManager sharedInstance] setView:m_glView];  
  [m_glView setMultipleTouchEnabled:YES];
  
  /* Check if screen is Retina */
  screenScale = [m_glView getScreenScale:screen];

  [self.view addSubview: m_glView];
  
  [self createGestureRecognizers];
  [m_window addSubview: self.view];
  [m_window makeKeyAndVisible];
  g_xbmcController = self;  
  
  return self;
}
//--------------------------------------------------------------
-(void)viewDidLoad
{
  [super viewDidLoad];
}
//--------------------------------------------------------------
- (void)dealloc
{
  [m_glView stopAnimation];
  [m_glView release];
  [m_window release];

  NSNotificationCenter *center;
  // take us off the default center for our app
  center = [NSNotificationCenter defaultCenter];
  [center removeObserver: self];
  
  [super dealloc];
}
//--------------------------------------------------------------
- (void)viewWillAppear:(BOOL)animated
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);
  
  // move this later into CocoaPowerSyscall
  [[UIApplication sharedApplication] setIdleTimerDisabled:YES];
  
  [self resumeAnimation];
  
  [super viewWillAppear:animated];
}
//--------------------------------------------------------------
- (void)viewWillDisappear:(BOOL)animated
{  
  //NSLog(@"%s", __PRETTY_FUNCTION__);
  
  [self pauseAnimation];
  
  // move this later into CocoaPowerSyscall
  [[UIApplication sharedApplication] setIdleTimerDisabled:NO];
	
  [super viewWillDisappear:animated];
}
//--------------------------------------------------------------
- (void)viewDidUnload
{
	[super viewDidUnload];	
}
//--------------------------------------------------------------
- (void) initDisplayLink
{
	[m_glView initDisplayLink];
}
//--------------------------------------------------------------
- (void) deinitDisplayLink
{
  [m_glView deinitDisplayLink];
}
//--------------------------------------------------------------
- (double) getDisplayLinkFPS;
{
  return [m_glView getDisplayLinkFPS];
}
//--------------------------------------------------------------
- (void) setFramebuffer
{
  [m_glView setFramebuffer];
}
//--------------------------------------------------------------
- (bool) presentFramebuffer
{
  return [m_glView presentFramebuffer];
}
//--------------------------------------------------------------
- (CGSize) getScreenSize
{
  screensize.width  = m_glView.bounds.size.width * screenScale;
  screensize.height = m_glView.bounds.size.height * screenScale;  
  return screensize;
}
//--------------------------------------------------------------
- (CGFloat) getScreenScale:(UIScreen *)screen;
{
  return [m_glView getScreenScale:screen];
}
//--------------------------------------------------------------
//--------------------------------------------------------------
- (BOOL) recreateOnReselect
{ 
  //NSLog(@"%s", __PRETTY_FUNCTION__);
  return YES;
}
//--------------------------------------------------------------
- (void)didReceiveMemoryWarning
{
  // Releases the view if it doesn't have a superview.
  [super didReceiveMemoryWarning];
  
  // Release any cached data, images, etc. that aren't in use.
}
//--------------------------------------------------------------
- (void) disableSystemSleep
{
}
//--------------------------------------------------------------
- (void) enableSystemSleep
{
}
//--------------------------------------------------------------
- (void) disableScreenSaver
{
}
//--------------------------------------------------------------
- (void) enableScreenSaver
{
}
//--------------------------------------------------------------
- (bool) changeScreen: (unsigned int)screenIdx withMode:(UIScreenMode *)mode
{
  bool ret = false;

  ret = [[IOSScreenManager sharedInstance] changeScreen:screenIdx withMode:mode];

  return ret;
}
//--------------------------------------------------------------
- (void) activateScreen: (UIScreen *)screen
{
  //this is the only way for making ios call the
  //shouldAutorotateToInterfaceOrientation of the controller
  //this is needed because at least with my vga adapter
  //the orientation on the external screen is messed up by 90°
  //so we need to hard force the orientation to Portrait for
  //getting the correct display on external screen
  UIView *view = [m_window.subviews objectAtIndex:0];
  [view removeFromSuperview];
  [m_window addSubview:view];  
  m_window.screen = screen;
}
//--------------------------------------------------------------
- (void)pauseAnimation
{
  XBMC_Event newEvent;
  memcpy(&newEvent, &lastEvent, sizeof(XBMC_Event));
  
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
  memcpy(&newEvent, &lastEvent, sizeof(XBMC_Event));
  
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
#pragma mark -
#pragma mark private helper methods
//
- (void)observeDefaultCenterStuff: (NSNotification *) notification
{
  //NSLog(@"default: %@", [notification name]);
}

@end
