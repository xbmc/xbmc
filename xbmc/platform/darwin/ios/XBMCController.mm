/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "XBMCController.h"

#include "AppInboundProtocol.h"
#include "Application.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIControl.h"
#include "input/Key.h"
#include "input/touch/generic/GenericTouchActionHandler.h"
#include "messaging/ApplicationMessenger.h"
#include "music/tags/MusicInfoTag.h"
#include "playlists/PlayList.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/Event.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "windowing/XBMC_events.h"
#import "windowing/ios/WinSystemIOS.h"

#import "platform/darwin/NSLogDebugHelpers.h"
#import "platform/darwin/ios-common/DarwinEmbedNowPlayingInfoManager.h"
#import "platform/darwin/ios/IOSEAGLView.h"
#import "platform/darwin/ios/IOSScreenManager.h"
#import "platform/darwin/ios/IOSSettingsHandler.h"
#import "platform/darwin/ios/XBMCApplication.h"

#define id _id
#include "TextureCache.h"
#undef id

#include <math.h>
#include <signal.h>

#import <AVFoundation/AVAudioSession.h>
#include <sys/resource.h>

using namespace KODI::MESSAGING;

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795028842
#endif
#define RADIANS_TO_DEGREES(radians) ((radians) * (180.0 / M_PI))

XBMCController *g_xbmcController;

//--------------------------------------------------------------
//

@interface XBMCController ()
{
  std::unique_ptr<IOSSettingsHandler> m_settingsHandler;
}
- (void)rescheduleNetworkAutoSuspend;
@end

@interface UIApplication (extended)
-(void) terminateWithSuccess;
@end

@implementation XBMCController
@synthesize animating;
@synthesize lastGesturePoint;
@synthesize screenScale;
@synthesize touchBeginSignaled;
@synthesize m_screenIdx;
@synthesize screensize;
@synthesize m_networkAutoSuspendTimer;
@synthesize MPNPInfoManager;
@synthesize nativeKeyboardActive;
//--------------------------------------------------------------
- (void) sendKeypressEvent: (XBMC_Event) event
{
  std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();

  if (appPort)
  {
    event.type = XBMC_KEYDOWN;
    appPort->OnEvent(event);

    event.type = XBMC_KEYUP;
    appPort->OnEvent(event);
  }
}

// START OF UIKeyInput protocol
- (BOOL)hasText
{
  return NO;
}

- (void)insertText:(NSString *)text
{
  // in case the native touch keyboard is active
  // don't do anything here
  // we are only supposed to be called when
  // using an external bt keyboard...
  if (nativeKeyboardActive)
  {
    return;
  }

  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));
  unichar currentKey = [text characterAtIndex:0];

  // handle upper case letters
  if (currentKey >= 'A' && currentKey <= 'Z')
  {
    newEvent.key.keysym.mod = XBMCKMOD_LSHIFT;
    currentKey += 0x20;// convert to lower case
  }

  // handle return
  if (currentKey == '\n' || currentKey == '\r')
    currentKey = XBMCK_RETURN;

  newEvent.key.keysym.sym = (XBMCKey)currentKey;
  newEvent.key.keysym.unicode = currentKey;

  [self sendKeypressEvent:newEvent];
}

- (void)deleteBackward
{
  [self sendKey:XBMCK_BACKSPACE];
}
// END OF UIKeyInput protocol

- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
  return UIInterfaceOrientationMaskLandscape;
}
//--------------------------------------------------------------
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
  [self sendKeypressEvent:newEvent];

}
//--------------------------------------------------------------
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer
{
  if ([gestureRecognizer isKindOfClass:[UIRotationGestureRecognizer class]] && [otherGestureRecognizer isKindOfClass:[UIPinchGestureRecognizer class]]) {
    return YES;
  }

  if ([gestureRecognizer isKindOfClass:[UISwipeGestureRecognizer class]] && [otherGestureRecognizer isKindOfClass:[UIPanGestureRecognizer class]]) {
    return YES;
  }


  return NO;
}
//--------------------------------------------------------------
- (void)addSwipeGesture:(UISwipeGestureRecognizerDirection)direction numTouches : (NSUInteger)numTouches
{
  UISwipeGestureRecognizer *swipe = [[UISwipeGestureRecognizer alloc]
                                          initWithTarget:self action:@selector(handleSwipe:)];

  swipe.delaysTouchesBegan = NO;
  swipe.numberOfTouchesRequired = numTouches;
  swipe.direction = direction;
  swipe.delegate = self;
  [m_glView addGestureRecognizer:swipe];
}
//--------------------------------------------------------------
- (void)addTapGesture:(NSUInteger)numTouches
{
  UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc]
                                                   initWithTarget:self action:@selector(handleTap:)];

  tapGesture.delaysTouchesBegan = NO;
  tapGesture.numberOfTapsRequired = 1;
  tapGesture.numberOfTouchesRequired = numTouches;

  [m_glView addGestureRecognizer:tapGesture];
}
//--------------------------------------------------------------
- (void)createGestureRecognizers
{
  //1 finger single tap
  [self addTapGesture:1];

  //2 finger single tap - right mouse
  //single finger double tap delays single finger single tap - so we
  //go for 2 fingers here - so single finger single tap is instant
  [self addTapGesture:2];

  //3 finger single tap
  [self addTapGesture:3];

  //1 finger single long tap - right mouse - alternative
  UILongPressGestureRecognizer *singleFingerSingleLongTap = [[UILongPressGestureRecognizer alloc]
    initWithTarget:self action:@selector(handleSingleFingerSingleLongTap:)];

  singleFingerSingleLongTap.delaysTouchesBegan = NO;
  singleFingerSingleLongTap.delaysTouchesEnded = NO;
  [m_glView addGestureRecognizer:singleFingerSingleLongTap];

  //triple finger swipe left
  [self addSwipeGesture:UISwipeGestureRecognizerDirectionLeft numTouches:3];

  //double finger swipe left for backspace ... i like this fast backspace feature ;)
  [self addSwipeGesture:UISwipeGestureRecognizerDirectionLeft numTouches:2];

  //single finger swipe left
  [self addSwipeGesture:UISwipeGestureRecognizerDirectionLeft numTouches:1];

  //triple finger swipe right
  [self addSwipeGesture:UISwipeGestureRecognizerDirectionRight numTouches:3];

  //double finger swipe right
  [self addSwipeGesture:UISwipeGestureRecognizerDirectionRight numTouches:2];

  //single finger swipe right
  [self addSwipeGesture:UISwipeGestureRecognizerDirectionRight numTouches:1];

  //triple finger swipe up
  [self addSwipeGesture:UISwipeGestureRecognizerDirectionUp numTouches:3];

  //double finger swipe up
  [self addSwipeGesture:UISwipeGestureRecognizerDirectionUp numTouches:2];

  //single finger swipe up
  [self addSwipeGesture:UISwipeGestureRecognizerDirectionUp numTouches:1];

  //triple finger swipe down
  [self addSwipeGesture:UISwipeGestureRecognizerDirectionDown numTouches:3];

  //double finger swipe down
  [self addSwipeGesture:UISwipeGestureRecognizerDirectionDown numTouches:2];

  //single finger swipe down
  [self addSwipeGesture:UISwipeGestureRecognizerDirectionDown numTouches:1];

  //for pan gestures with one finger
  UIPanGestureRecognizer *pan = [[UIPanGestureRecognizer alloc]
    initWithTarget:self action:@selector(handlePan:)];

  pan.delaysTouchesBegan = NO;
  pan.maximumNumberOfTouches = 1;
  [m_glView addGestureRecognizer:pan];

  //for zoom gesture
  UIPinchGestureRecognizer *pinch = [[UIPinchGestureRecognizer alloc]
    initWithTarget:self action:@selector(handlePinch:)];

  pinch.delaysTouchesBegan = NO;
  pinch.delegate = self;
  [m_glView addGestureRecognizer:pinch];

  //for rotate gesture
  UIRotationGestureRecognizer *rotate = [[UIRotationGestureRecognizer alloc]
                                         initWithTarget:self action:@selector(handleRotate:)];

  rotate.delaysTouchesBegan = NO;
  rotate.delegate = self;
  [m_glView addGestureRecognizer:rotate];
}
//--------------------------------------------------------------
- (void) activateKeyboard:(UIView *)view
{
  [self.view addSubview:view];
  m_glView.userInteractionEnabled = NO;
}
//--------------------------------------------------------------
- (void) deactivateKeyboard:(UIView *)view
{
  [view removeFromSuperview];
  m_glView.userInteractionEnabled = YES;
  [self becomeFirstResponder];
}
//--------------------------------------------------------------
- (void) nativeKeyboardActive: (bool)active
{
  nativeKeyboardActive = active;
}
//--------------------------------------------------------------
-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
  if( m_glView && [m_glView isXBMCAlive] )//NO GESTURES BEFORE WE ARE UP AND RUNNING
  {
    UITouch *touch = (UITouch *)[[touches allObjects] objectAtIndex:0];
    CGPoint point = [touch locationInView:m_glView];
    point.x *= screenScale;
    point.y *= screenScale;
    CGenericTouchActionHandler::GetInstance().OnSingleTouchStart(point.x, point.y);
  }
}
//--------------------------------------------------------------
-(void)handlePinch:(UIPinchGestureRecognizer*)sender
{
  if( m_glView && [m_glView isXBMCAlive] && sender.numberOfTouches )//NO GESTURES BEFORE WE ARE UP AND RUNNING
  {
    CGPoint point = [sender locationOfTouch:0 inView:m_glView];
    point.x *= screenScale;
    point.y *= screenScale;

    switch(sender.state)
    {
      case UIGestureRecognizerStateBegan:
        CGenericTouchActionHandler::GetInstance().OnTouchGestureStart(point.x, point.y);
        break;
      case UIGestureRecognizerStateChanged:
        CGenericTouchActionHandler::GetInstance().OnZoomPinch(point.x, point.y, [sender scale]);
        break;
      case UIGestureRecognizerStateEnded:
      case UIGestureRecognizerStateCancelled:
        CGenericTouchActionHandler::GetInstance().OnTouchGestureEnd(point.x, point.y, 0, 0, 0, 0);
        break;
      default:
        break;
    }
  }
}
//--------------------------------------------------------------
-(void)handleRotate:(UIRotationGestureRecognizer*)sender
{
  if( m_glView && [m_glView isXBMCAlive] && sender.numberOfTouches )//NO GESTURES BEFORE WE ARE UP AND RUNNING
  {
    CGPoint point = [sender locationOfTouch:0 inView:m_glView];
    point.x *= screenScale;
    point.y *= screenScale;

    switch(sender.state)
    {
      case UIGestureRecognizerStateBegan:
        CGenericTouchActionHandler::GetInstance().OnTouchGestureStart(point.x, point.y);
        break;
      case UIGestureRecognizerStateChanged:
        CGenericTouchActionHandler::GetInstance().OnRotate(point.x, point.y, RADIANS_TO_DEGREES([sender rotation]));
        break;
      case UIGestureRecognizerStateEnded:
        CGenericTouchActionHandler::GetInstance().OnTouchGestureEnd(point.x, point.y, 0, 0, 0, 0);
        break;
      default:
        break;
    }
  }
}
//--------------------------------------------------------------
- (IBAction)handlePan:(UIPanGestureRecognizer *)sender
{
  if( m_glView && [m_glView isXBMCAlive] )//NO GESTURES BEFORE WE ARE UP AND RUNNING
  {
    CGPoint velocity = [sender velocityInView:m_glView];

    if( [sender state] == UIGestureRecognizerStateBegan && sender.numberOfTouches )
    {
      CGPoint point = [sender locationOfTouch:0 inView:m_glView];
      point.x *= screenScale;
      point.y *= screenScale;
      touchBeginSignaled = false;
      lastGesturePoint = point;
    }

    if( [sender state] == UIGestureRecognizerStateChanged && sender.numberOfTouches )
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
          CGenericTouchActionHandler::GetInstance().OnTouchGestureStart((float)point.x, (float)point.y);
          touchBeginSignaled = true;
        }

        CGenericTouchActionHandler::GetInstance().OnTouchGesturePan((float)point.x, (float)point.y,
                                                            (float)xMovement, (float)yMovement,
                                                            (float)velocity.x, (float)velocity.y);
        lastGesturePoint = point;
      }
    }

    if( touchBeginSignaled && ([sender state] == UIGestureRecognizerStateEnded || [sender state] == UIGestureRecognizerStateCancelled))
    {
      //signal end of pan - this will start inertial scrolling with deacceleration in CApplication
      CGenericTouchActionHandler::GetInstance().OnTouchGestureEnd((float)lastGesturePoint.x, (float)lastGesturePoint.y,
                                                             (float)0.0, (float)0.0,
                                                             (float)velocity.x, (float)velocity.y);

      touchBeginSignaled = false;
    }
  }
}
//--------------------------------------------------------------
- (IBAction)handleSwipe:(UISwipeGestureRecognizer *)sender
{
  if( m_glView && [m_glView isXBMCAlive] && sender.numberOfTouches )//NO GESTURES BEFORE WE ARE UP AND RUNNING
  {


    if (sender.state == UIGestureRecognizerStateRecognized)
    {
      CGPoint point = [sender locationOfTouch:0 inView:m_glView];
      point.x *= screenScale;
      point.y *= screenScale;

      TouchMoveDirection direction = TouchMoveDirectionNone;
      switch ([sender direction])
      {
        case UISwipeGestureRecognizerDirectionRight:
          direction = TouchMoveDirectionRight;
          break;
        case UISwipeGestureRecognizerDirectionLeft:
          direction = TouchMoveDirectionLeft;
          break;
        case UISwipeGestureRecognizerDirectionUp:
          direction = TouchMoveDirectionUp;
          break;
        case UISwipeGestureRecognizerDirectionDown:
          direction = TouchMoveDirectionDown;
          break;
      }
      CGenericTouchActionHandler::GetInstance().OnSwipe(direction,
                                                0.0, 0.0,
                                                point.x, point.y, 0, 0,
                                                [sender numberOfTouches]);
    }
  }
}
//--------------------------------------------------------------
- (IBAction)handleTap:(UIGestureRecognizer *)sender
{
  //Allow the tap gesture during init
  //(for allowing the user to tap away any messageboxes during init)
  if( ([m_glView isReadyToRun] && [sender numberOfTouches] == 1) || [m_glView isXBMCAlive])
  {
    CGPoint point = [sender locationOfTouch:0 inView:m_glView];
    point.x *= screenScale;
    point.y *= screenScale;
    //NSLog(@"%s singleTap", __PRETTY_FUNCTION__);
    CGenericTouchActionHandler::GetInstance().OnTap((float)point.x, (float)point.y, [sender numberOfTouches]);
  }
}
//--------------------------------------------------------------
- (IBAction)handleSingleFingerSingleLongTap:(UIGestureRecognizer *)sender
{
  if( m_glView && [m_glView isXBMCAlive] && sender.numberOfTouches)//NO GESTURES BEFORE WE ARE UP AND RUNNING
  {
    CGPoint point = [sender locationOfTouch:0 inView:m_glView];
    point.x *= screenScale;
    point.y *= screenScale;

    if (sender.state == UIGestureRecognizerStateBegan)
    {
      lastGesturePoint = point;
      // mark the control
      //CGenericTouchActionHandler::GetInstance().OnSingleTouchStart((float)point.x, (float)point.y);
    }

    if (sender.state == UIGestureRecognizerStateEnded)
    {
      CGenericTouchActionHandler::GetInstance().OnSingleTouchMove((float)point.x, (float)point.y, point.x - lastGesturePoint.x, point.y - lastGesturePoint.y, 0, 0);
    }

    if (sender.state == UIGestureRecognizerStateEnded)
    {
      CGenericTouchActionHandler::GetInstance().OnLongPress((float)point.x, (float)point.y);
    }
  }
}
//--------------------------------------------------------------
- (id)initWithFrame:(CGRect)frame withScreen:(UIScreen *)screen
{
  PRINT_SIGNATURE();
  m_screenIdx = 0;
  self = [super init];
  if ( !self )
    return ( nil );

  m_glView = NULL;

  m_isPlayingBeforeInactive = NO;
  m_bgTask = UIBackgroundTaskInvalid;

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

  orientation = UIInterfaceOrientationLandscapeLeft;

  [m_window makeKeyAndVisible];
  g_xbmcController = self;
  MPNPInfoManager = [DarwinEmbedNowPlayingInfoManager new];

  return self;
}
//--------------------------------------------------------------
- (void)loadView
{
  [super loadView];
  self.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  self.view.autoresizesSubviews = YES;

  auto frame = [NSUserDefaults.standardUserDefaults boolForKey:DisableSafeAreaDefaultsKey]
                   ? self.view.bounds
                   : [self fullscreenSubviewFrame];
  m_glView = [[IOSEAGLView alloc] initWithFrame:frame withScreen:UIScreen.mainScreen];
  [[IOSScreenManager sharedInstance] setView:m_glView];
  [m_glView setMultipleTouchEnabled:YES];

  /* Check if screen is Retina */
  screenScale = [m_glView getScreenScale:[UIScreen mainScreen]];

  [self.view addSubview: m_glView];

  [self createGestureRecognizers];
}
//--------------------------------------------------------------
-(void)viewDidLoad
{
  [super viewDidLoad];
}
//--------------------------------------------------------------
- (void)dealloc
{
  // stop background task
  [m_networkAutoSuspendTimer invalidate];
  [self enableNetworkAutoSuspend:nil];

  [m_glView stopAnimation];

  NSNotificationCenter *center;
  // take us off the default center for our app
  center = [NSNotificationCenter defaultCenter];
  [center removeObserver: self];
}
//--------------------------------------------------------------
- (void)viewWillAppear:(BOOL)animated
{
  PRINT_SIGNATURE();

  // move this later into CocoaPowerSyscall
  [[UIApplication sharedApplication] setIdleTimerDisabled:YES];

  [self resumeAnimation];

  [super viewWillAppear:animated];
}
//--------------------------------------------------------------
-(void) viewDidAppear:(BOOL)animated
{
  [super viewDidAppear:animated];

  [self becomeFirstResponder];
  [[UIApplication sharedApplication] beginReceivingRemoteControlEvents];
  // Notifies UIKit that our view controller updated its preference
  // regarding the visual indicator
  // this should make ios call prefersHomeIndicatorAutoHidden and
  // hide the home indicator on iPhoneX and other devices without
  // home button
  if ([self respondsToSelector:@selector(setNeedsUpdateOfHomeIndicatorAutoHidden)]) {
    [self performSelector:@selector(setNeedsUpdateOfHomeIndicatorAutoHidden)];
  }
}
//--------------------------------------------------------------
- (BOOL)prefersHomeIndicatorAutoHidden
{
  return YES;
}
//--------------------------------------------------------------
- (void)viewWillDisappear:(BOOL)animated
{
  PRINT_SIGNATURE();

  [self pauseAnimation];

  // move this later into CocoaPowerSyscall
  [[UIApplication sharedApplication] setIdleTimerDisabled:NO];

  [super viewWillDisappear:animated];
}
//--------------------------------------------------------------
-(UIView *)inputView
{
  // override our input view to an empty view
  // this prevents the on screen keyboard
  // which would be shown whenever this UIResponder
  // becomes the first responder (which is always the case!)
  // caused by implementing the UIKeyInput protocol
  return [[UIView alloc] initWithFrame:CGRectZero];
}
//--------------------------------------------------------------
- (BOOL) canBecomeFirstResponder
{
  return YES;
}
//--------------------------------------------------------------
- (void)viewDidUnload
{
  [[UIApplication sharedApplication] endReceivingRemoteControlEvents];
  [self resignFirstResponder];

	[super viewDidUnload];
}
//--------------------------------------------------------------
- (CGRect)fullscreenSubviewFrame
{
  auto rect = self.view.bounds;
  if (@available(ios 11.0, *))
    return UIEdgeInsetsInsetRect(rect, m_window.safeAreaInsets);
  else
    return rect;
}
//--------------------------------------------------------------
- (void)onXbmcAlive
{
  m_settingsHandler = std::make_unique<IOSSettingsHandler>();
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
  __block CGSize tmp;
  if ([NSThread isMainThread])
  {
    tmp.width  = m_glView.bounds.size.width * screenScale;
    tmp.height = m_glView.bounds.size.height * screenScale;
  }
  else
  {
    dispatch_sync(dispatch_get_main_queue(), ^{
      tmp.width  = m_glView.bounds.size.width * screenScale;
      tmp.height = m_glView.bounds.size.height * screenScale;
    });
  }
  screensize = tmp;
  return screensize;
}
//--------------------------------------------------------------
- (BOOL) recreateOnReselect
{
  PRINT_SIGNATURE();
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
- (void)disableNetworkAutoSuspend
{
  PRINT_SIGNATURE();
  if (m_bgTask != UIBackgroundTaskInvalid)
  {
    [[UIApplication sharedApplication] endBackgroundTask: m_bgTask];
    m_bgTask = UIBackgroundTaskInvalid;
  }
  // we have to alloc the background task for keep network working after screen lock and dark.
  UIBackgroundTaskIdentifier newTask = [[UIApplication sharedApplication] beginBackgroundTaskWithExpirationHandler:nil];
  m_bgTask = newTask;

  if (m_networkAutoSuspendTimer)
  {
    [m_networkAutoSuspendTimer invalidate];
    self.m_networkAutoSuspendTimer = nil;
  }
}
//--------------------------------------------------------------
- (void)enableNetworkAutoSuspend:(id)obj
{
  PRINT_SIGNATURE();
  if (m_bgTask != UIBackgroundTaskInvalid)
  {
    [[UIApplication sharedApplication] endBackgroundTask: m_bgTask];
    m_bgTask = UIBackgroundTaskInvalid;
  }
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
- (void) activateScreen: (UIScreen *)screen  withOrientation:(UIInterfaceOrientation)newOrientation
{
  // Since ios7 we have to handle the orientation manually
  // it differs by 90 degree between internal and external screen
  float   angle = 0;
  UIView *view = [m_window.subviews objectAtIndex:0];
  switch(newOrientation)
  {
    case UIInterfaceOrientationUnknown:
    case UIInterfaceOrientationPortrait:
      angle = 0;
      break;
    case UIInterfaceOrientationPortraitUpsideDown:
      angle = M_PI;
      break;
    case UIInterfaceOrientationLandscapeLeft:
      angle = -M_PI_2;
      break;
    case UIInterfaceOrientationLandscapeRight:
      angle = M_PI_2;
      break;
  }
  // reset the rotation of the view
  view.layer.transform = CATransform3DMakeRotation(angle, 0, 0.0, 1.0);
  view.layer.bounds = view.bounds;
  m_window.screen = screen;
  [view setFrame:m_window.frame];
}
//--------------------------------------------------------------
- (void) remoteControlReceivedWithEvent: (UIEvent *) receivedEvent {
  LOG(@"%s: type %zd, subtype: %zd", __PRETTY_FUNCTION__, receivedEvent.type, receivedEvent.subtype);
  if (receivedEvent.type == UIEventTypeRemoteControl)
  {
    [self disableNetworkAutoSuspend];
    switch (receivedEvent.subtype)
    {
      case UIEventSubtypeRemoteControlTogglePlayPause:
        CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_PLAYER_PLAYPAUSE)));
        break;
      case UIEventSubtypeRemoteControlPlay:
	    CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_PLAYER_PLAY)));
        break;
      case UIEventSubtypeRemoteControlPause:
        // ACTION_PAUSE sometimes cause unpause, use MediaPauseIfPlaying to make sure pause only
        CApplicationMessenger::GetInstance().SendMsg(TMSG_MEDIA_PAUSE_IF_PLAYING);
        break;
      case UIEventSubtypeRemoteControlNextTrack:
	    CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_NEXT_ITEM)));
        break;
      case UIEventSubtypeRemoteControlPreviousTrack:
	    CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_PREV_ITEM)));
        break;
      case UIEventSubtypeRemoteControlBeginSeekingForward:
        // use 4X speed forward.
		CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_PLAYER_FORWARD)));
		CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_PLAYER_FORWARD)));
        break;
      case UIEventSubtypeRemoteControlBeginSeekingBackward:
        // use 4X speed rewind.
		CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_PLAYER_REWIND)));
		CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_PLAYER_REWIND)));
        break;
      case UIEventSubtypeRemoteControlEndSeekingForward:
      case UIEventSubtypeRemoteControlEndSeekingBackward:
        // restore to normal playback speed.
        if (g_application.GetAppPlayer().IsPlaying() && !g_application.GetAppPlayer().IsPaused())
		  CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_PLAYER_PLAY)));
        break;
      default:
        LOG(@"unhandled subtype: %zd", receivedEvent.subtype);
        break;
    }
    [self rescheduleNetworkAutoSuspend];
  }
}
//--------------------------------------------------------------
- (void)enterBackground
{
  PRINT_SIGNATURE();
  if (g_application.GetAppPlayer().IsPlaying() && !g_application.GetAppPlayer().IsPaused())
  {
    m_isPlayingBeforeInactive = YES;
    CApplicationMessenger::GetInstance().SendMsg(TMSG_MEDIA_PAUSE_IF_PLAYING);
  }
  CWinSystemIOS* winSystem = dynamic_cast<CWinSystemIOS*>(CServiceBroker::GetWinSystem());
  winSystem->OnAppFocusChange(false);
}

- (void)enterForeground
{
  PRINT_SIGNATURE();
  CWinSystemIOS* winSystem = dynamic_cast<CWinSystemIOS*>(CServiceBroker::GetWinSystem());
  if (winSystem)
    winSystem->OnAppFocusChange(true);
  // when we come back, restore playing if we were.
  if (m_isPlayingBeforeInactive)
  {
    CApplicationMessenger::GetInstance().SendMsg(TMSG_MEDIA_UNPAUSE);
    m_isPlayingBeforeInactive = NO;
  }
}

- (void)becomeInactive
{
  // if we were interrupted, already paused here
  // else if user background us or lock screen, only pause video here, audio keep playing.
  if (g_application.GetAppPlayer().IsPlayingVideo() && !g_application.GetAppPlayer().IsPaused())
  {
    m_isPlayingBeforeInactive = YES;
    CApplicationMessenger::GetInstance().SendMsg(TMSG_MEDIA_PAUSE_IF_PLAYING);
  }
  // check whether we need disable network auto suspend.
  [self rescheduleNetworkAutoSuspend];
}
//--------------------------------------------------------------
- (void)pauseAnimation
{
  PRINT_SIGNATURE();

  [m_glView pauseAnimation];
}
//--------------------------------------------------------------
- (void)resumeAnimation
{
  PRINT_SIGNATURE();

  [m_glView resumeAnimation];
}
//--------------------------------------------------------------
- (void)startAnimation
{
  PRINT_SIGNATURE();

  [m_glView startAnimation];
}
//--------------------------------------------------------------
- (void)stopAnimation
{
  PRINT_SIGNATURE();

  [m_glView stopAnimation];
}

- (void)rescheduleNetworkAutoSuspend
{
  LOG(@"%s: playback state: %d", __PRETTY_FUNCTION__, MPNPInfoManager.playbackState);
  if (MPNPInfoManager.playbackState == DARWINEMBED_PLAYBACK_PLAYING)
  {
    [self disableNetworkAutoSuspend];
    return;
  }
  if (m_networkAutoSuspendTimer)
    [m_networkAutoSuspendTimer invalidate];

  // wait longer if paused than stopped
  int delay = MPNPInfoManager.playbackState == DARWINEMBED_PLAYBACK_PAUSED ? 60 : 30;
  self.m_networkAutoSuspendTimer =
      [NSTimer scheduledTimerWithTimeInterval:delay
                                       target:self
                                     selector:@selector(enableNetworkAutoSuspend:)
                                     userInfo:nil
                                      repeats:NO];
}

#pragma mark -
#pragma mark private helper methods
//
- (void)observeDefaultCenterStuff: (NSNotification *) notification
{
//  LOG(@"default: %@", [notification name]);
//  LOG(@"userInfo: %@", [notification userInfo]);
}

- (CVEAGLContext)getEAGLContextObj
{
  return [m_glView getCurrentEAGLContext];
}

@end
