/*
 *      Copyright (C) 2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

//hack around problem with xbmc's typedef int BOOL
// and obj-c's typedef unsigned char BOOL
#define BOOL XBMC_BOOL 
#include <sys/resource.h>
#include <signal.h>

#include "system.h"
#include "AdvancedSettings.h"
#include "FileItem.h"
#include "Application.h"
#include "MouseStat.h"
#include "WindowingFactory.h"
#include "VideoReferenceClock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "Util.h"
#include "WinEventsIOS.h"
#undef BOOL

#import "XBMCEAGLView.h"

#import "XBMCController.h"
#import "XBMCApplication.h"
#import "XBMCDebugHelpers.h"

XBMCController *g_xbmcController;

// notification messages
extern NSString* kBRScreenSaverActivated;
extern NSString* kBRScreenSaverDismissed;

//--------------------------------------------------------------
//

@interface XBMCController ()
XBMCEAGLView  *m_glView;
@end

@interface UIApplication (extended)
-(void) terminateWithSuccess;
@end

@implementation XBMCController
@synthesize firstTouch;
@synthesize lastTouch;
@synthesize lastEvent;
@synthesize screensize;

//--------------------------------------------------------------
-(BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  if(interfaceOrientation == UIInterfaceOrientationLandscapeLeft) 
  {
    return YES;
  }
  else if(interfaceOrientation == UIInterfaceOrientationLandscapeRight)
  {
    return YES;
  }
  else
  {
    return NO;
  }
}
//--------------------------------------------------------------
- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
  orientation = toInterfaceOrientation;

  CGRect rect;
  CGRect srect = [[UIScreen mainScreen] bounds];
  
	if(toInterfaceOrientation == UIInterfaceOrientationPortrait || toInterfaceOrientation == UIInterfaceOrientationPortraitUpsideDown) {
    rect = srect;
	} else if(toInterfaceOrientation == UIInterfaceOrientationLandscapeLeft || toInterfaceOrientation == UIInterfaceOrientationLandscapeRight) {
    rect.size = CGSizeMake( srect.size.height, srect.size.width );
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
- (void)createGestureRecognizers 
{
  
  UITapGestureRecognizer *singleFingerDTap = [[UITapGestureRecognizer alloc]
                                              initWithTarget:self action:@selector(handleSingleDoubleTap:)];  
  singleFingerDTap.delaysTouchesBegan = YES;
  singleFingerDTap.numberOfTapsRequired = 2;
  [self.view addGestureRecognizer:singleFingerDTap];
  [singleFingerDTap release];

  UISwipeGestureRecognizer *swipeLeft = [[UISwipeGestureRecognizer alloc]
                                              initWithTarget:self action:@selector(handleSwipeLeft:)];
  swipeLeft.direction = UISwipeGestureRecognizerDirectionLeft;
  swipeLeft.delaysTouchesBegan = YES;
  [self.view addGestureRecognizer:swipeLeft];
  [swipeLeft release];
  
  UISwipeGestureRecognizer *swipeRight = [[UISwipeGestureRecognizer alloc]
                                         initWithTarget:self action:@selector(handleSwipeRight:)];
  swipeRight.direction = UISwipeGestureRecognizerDirectionRight;
  swipeRight.delaysTouchesBegan = YES;
  [self.view addGestureRecognizer:swipeRight];
  [swipeRight release];

}
//--------------------------------------------------------------
- (IBAction)handleSwipeLeft:(UISwipeGestureRecognizer *)sender 
{
  //NSLog(@"%s swipeLeft", __PRETTY_FUNCTION__);
  [self sendKey:XBMCK_BACKSPACE];
}
//--------------------------------------------------------------
- (IBAction)handleSwipeRight:(UISwipeGestureRecognizer *)sender 
{
  //NSLog(@"%s swipeRight", __PRETTY_FUNCTION__);
  [self sendKey:XBMCK_TAB];
}
//--------------------------------------------------------------
- (IBAction)handleSingleDoubleTap:(UIGestureRecognizer *)sender 
{
  firstTouch = [sender locationOfTouch:0 inView:m_glView];
  lastTouch = [sender locationOfTouch:0 inView:m_glView];
  
  //NSLog(@"%s toubleTap", __PRETTY_FUNCTION__);
  
  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));
  
  newEvent.type = XBMC_MOUSEBUTTONDOWN;
  newEvent.button.type = XBMC_MOUSEBUTTONDOWN;
  newEvent.button.button = XBMC_BUTTON_RIGHT;
  newEvent.button.x = lastTouch.x;
  newEvent.button.y = lastTouch.y;
  
  CWinEventsIOS::MessagePush(&newEvent);    

  newEvent.type = XBMC_MOUSEBUTTONUP;
  newEvent.button.type = XBMC_MOUSEBUTTONUP;
  CWinEventsIOS::MessagePush(&newEvent);    
  
  memset(&lastEvent, 0x0, sizeof(XBMC_Event));         
  
}
//--------------------------------------------------------------
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event 
{
  UITouch *touch = [touches anyObject];
  firstTouch = [touch locationInView:m_glView];
  lastTouch = [touch locationInView:m_glView];
  
  //NSLog(@"%s touchesBegan x=%f, y=%f count=%d", __PRETTY_FUNCTION__, lastTouch.x, lastTouch.y, [touch tapCount]);

  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));

  newEvent.type = XBMC_MOUSEBUTTONDOWN;
  newEvent.button.type = XBMC_MOUSEBUTTONDOWN;
  newEvent.button.x = lastTouch.x;
  newEvent.button.y = lastTouch.y;  
  newEvent.button.button = XBMC_BUTTON_LEFT;
  CWinEventsIOS::MessagePush(&newEvent);
  
  /* Store the tap action for later */
  memcpy(&lastEvent, &newEvent, sizeof(XBMC_Event));
}
//--------------------------------------------------------------
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event 
{
  UITouch *touch = [touches anyObject];
  lastTouch = [touch locationInView:m_glView];

  //NSLog(@"%s touchesMoved x=%f, y=%f count=%d", __PRETTY_FUNCTION__, lastTouch.x, lastTouch.y, touch.tapCount);

  static int nCount = 0;
  
  /* Only process move when we are not in right click state */
  if(nCount == 4) {
    
    XBMC_Event newEvent;
    memcpy(&newEvent, &lastEvent, sizeof(XBMC_Event));
    
    newEvent.motion.x = lastTouch.x;
    newEvent.motion.y = lastTouch.y;
    
    CWinEventsIOS::MessagePush(&newEvent);
    
    nCount = 0;
    
  } else {
    
    nCount++;
    
  }
}
//--------------------------------------------------------------
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event 
{
  UITouch *touch = [touches anyObject];
  lastTouch = [touch locationInView:m_glView];
  
  //NSLog(@"%s touchesEnded x=%f, y=%f count=%d", __PRETTY_FUNCTION__, lastTouch.x, lastTouch.y, [touch tapCount]);

  XBMC_Event newEvent;
  memcpy(&newEvent, &lastEvent, sizeof(XBMC_Event));

  newEvent.type = XBMC_MOUSEBUTTONUP;
  newEvent.button.type = XBMC_MOUSEBUTTONUP;
  newEvent.button.x = lastTouch.x;
  newEvent.button.y = lastTouch.y;
  CWinEventsIOS::MessagePush(&newEvent);    
  
  memset(&lastEvent, 0x0, sizeof(XBMC_Event));         
  
}
//--------------------------------------------------------------
- (id)initWithFrame:(CGRect)frame
{ 
  //NSLog(@"%s", __PRETTY_FUNCTION__);

  self = [super init];
  if ( !self )
    return ( nil );

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
  
  m_glView = [[XBMCEAGLView alloc] initWithFrame: srect];
  [m_glView setMultipleTouchEnabled:YES];

  //[self setView: m_glView];

  [self.view addSubview: m_glView];
  
  [self createGestureRecognizers];

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
  screensize.width  = m_glView.bounds.size.width;
  screensize.height = m_glView.bounds.size.height;  
  return screensize;
}
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
//--------------------------------------------------------------

#pragma mark -
#pragma mark private helper methods
//
- (void)observeDefaultCenterStuff: (NSNotification *) notification
{
  //NSLog(@"default: %@", [notification name]);
}

@end
