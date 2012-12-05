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

//hack around problem with xbmc's typedef int BOOL
// and obj-c's typedef unsigned char BOOL
#define BOOL XBMC_BOOL 
#include "MouseStat.h"
#include "WindowingFactory.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/LocalizeStrings.h"
#undef BOOL

#import "IOSExternalTouchController.h"
#import "XBMCController.h"

//dim the touchscreen after 15 secs without touch event
const CGFloat touchScreenDimTimeoutSecs       = 15.0;
const CGFloat timeFadeSecs                    = 2.0;

@implementation IOSExternalTouchController
//--------------------------------------------------------------
- (id)init
{
  CGRect frame = [[UIScreen mainScreen] bounds];
  
  self = [super init];
  if (self) 
  {
    UIImage       *xbmcLogo;
    UIImageView   *xbmcLogoView;
    UILabel       *descriptionLabel;
  
    _internalWindow = [[UIWindow alloc] initWithFrame:frame];
    _touchView = [[UIView alloc] initWithFrame:frame];
    /* Turn on autoresizing for the whole hirarchy*/
    [_touchView setAutoresizesSubviews:YES];
    [_touchView setAutoresizingMask:UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight];
    [_touchView setAlpha:0.0];//start with alpha 0 and fade in with animation below
    [_touchView setMultipleTouchEnabled:YES];
    [_touchView setContentMode:UIViewContentModeCenter];
    
    
    CGRect labelRect = frame;
    labelRect.size.height/=2;
    //uilabel with the description
    descriptionLabel = [[UILabel alloc] initWithFrame:labelRect];
    //gray textcolor on transparent background
    [descriptionLabel setTextColor:[UIColor grayColor]];
    [descriptionLabel setBackgroundColor:[UIColor clearColor]];
    //text is aligned left in its frame
    [descriptionLabel setTextAlignment:UITextAlignmentCenter];
    [descriptionLabel setContentMode:UIViewContentModeCenter];
    //setup multiline behaviour
    [descriptionLabel setLineBreakMode:UILineBreakModeTailTruncation];

    [descriptionLabel setNumberOfLines:5];
    CStdString descText    = g_localizeStrings.Get(34404) + "\n";
    descText              += g_localizeStrings.Get(34405) + "\n";
    descText              += g_localizeStrings.Get(34406) + "\n";
    descText              += g_localizeStrings.Get(34407) + "\n";
    descText              += g_localizeStrings.Get(34408) + "\n";

    NSString *stringFromUTFString = [[NSString alloc] initWithUTF8String:descText];
    
    [descriptionLabel setText:stringFromUTFString];
    [stringFromUTFString release];

    //resize it to full view
    [descriptionLabel setAutoresizingMask:UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight];
    [descriptionLabel setAutoresizesSubviews:YES];
    [_touchView addSubview:descriptionLabel];
    [descriptionLabel release];

    //load the splash image
    CStdString strUserSplash = CSpecialProtocol::TranslatePath("special://xbmc/media/Splash.png");
    xbmcLogo = [UIImage imageWithContentsOfFile:[NSString stringWithUTF8String:strUserSplash.c_str()]];
    
    //make a view with the image
    xbmcLogoView = [[UIImageView alloc] initWithImage:xbmcLogo];
    //center the image and add it to the view
    [xbmcLogoView setFrame:frame];
    [xbmcLogoView setContentMode:UIViewContentModeCenter];
    //autoresize the image frame
    [xbmcLogoView setAutoresizingMask:UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight];
    [xbmcLogoView setAutoresizesSubviews:YES];
    [_touchView addSubview:xbmcLogoView];
    //send the image to the background
    [_touchView sendSubviewToBack:xbmcLogoView];
    [xbmcLogoView release];
  
    [[self view] addSubview: _touchView];

    [self createGestureRecognizers];

    [_internalWindow addSubview:[self view]];
    [_internalWindow setBackgroundColor:[UIColor blackColor]];
    [_internalWindow setScreen:[UIScreen mainScreen]];
    [_internalWindow makeKeyAndVisible];

    [self setWantsFullScreenLayout:YES];

    [self startSleepTimer];//will fade from black too
  }
  return self;
}
//--------------------------------------------------------------
- (void)startSleepTimer
{
  NSDate *fireDate = [NSDate dateWithTimeIntervalSinceNow:touchScreenDimTimeoutSecs]; 

  //schedule sleep timer to fire once in touchScreenDimTimeoutSecs if not reset 
  _sleepTimer       = [[NSTimer alloc] initWithFireDate:fireDate 
                                      interval:1 
                                      target:self 
                                      selector:@selector(sleepTimerCallback:) 
                                      userInfo:nil
                                      repeats:NO]; 
  //schedule the timer to the runloop
  NSRunLoop *runLoop = [NSRunLoop currentRunLoop]; 
  [self fadeFromBlack];
  [runLoop addTimer:_sleepTimer forMode:NSDefaultRunLoopMode]; 
} 
//--------------------------------------------------------------
- (void)stopSleepTimer
{
  if(_sleepTimer != nil)
  {
    [_sleepTimer invalidate];
    [_sleepTimer release];
    _sleepTimer = nil;
  }
}
//--------------------------------------------------------------
- (void)sleepTimerCallback:(NSTimer*)theTimer 
{ 
  [self fadeToBlack];
  [self stopSleepTimer];
} 
//--------------------------------------------------------------
- (bool)wakeUpFromSleep//returns false if we where dimmed, true if not
{
  if(_sleepTimer == nil)
  {
    [self startSleepTimer];
    return false;
  }
  else
  {
    NSDate *fireDate = [NSDate dateWithTimeIntervalSinceNow:touchScreenDimTimeoutSecs]; 
    [_sleepTimer setFireDate:fireDate];
    return true;
  }
}
//--------------------------------------------------------------
- (void)fadeToBlack
{
    [UIView animateWithDuration:timeFadeSecs delay:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
    [_touchView setAlpha:0.01];//fade to black (don't fade to 0.0 else we don't get gesture callbacks)
    }
    completion:^(BOOL finished){}];
}
//--------------------------------------------------------------
- (void)fadeFromBlack
{
    [UIView animateWithDuration:timeFadeSecs delay:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
    [_touchView setAlpha:1.0];//fade in to full alpha
    }
    completion:^(BOOL finished){}];
}
//--------------------------------------------------------------
- (void)createGestureRecognizers
{
  //2 finger single tab - right mouse
  //single finger double tab delays single finger single tab - so we
  //go for 2 fingers here - so single finger single tap is instant
  UITapGestureRecognizer *doubleFingerSingleTap = [[UITapGestureRecognizer alloc]
                                                    initWithTarget:self action:@selector(handleDoubleFingerSingleTap:)];  
  [doubleFingerSingleTap setNumberOfTapsRequired:1];
  [doubleFingerSingleTap setNumberOfTouchesRequired:2];
  [[self view] addGestureRecognizer:doubleFingerSingleTap];
  [doubleFingerSingleTap release];
  
  //1 finger single long tab - right mouse - alernative
  UITapGestureRecognizer *singleFingerSingleLongTap = (UITapGestureRecognizer*)[[UILongPressGestureRecognizer alloc]
                                                        initWithTarget:self action:@selector(handleSingleFingerSingleLongTap:)];  
  singleFingerSingleLongTap.delaysTouchesBegan = YES;
  singleFingerSingleLongTap.delaysTouchesEnded = YES;  
  singleFingerSingleLongTap.numberOfTouchesRequired = 1;
  [self.view addGestureRecognizer:singleFingerSingleLongTap];
  [singleFingerSingleLongTap release];
  
  //1 finger single tab - left mouse
  UITapGestureRecognizer *singleFingerSingleTap = [[UITapGestureRecognizer alloc]
                                                    initWithTarget:self action:@selector(handleSingleFingerSingleTap:)];  
  [singleFingerSingleTap setDelaysTouchesBegan:NO];
  [[self view] addGestureRecognizer:singleFingerSingleTap];
  [singleFingerSingleTap release];
  
  //double finger swipe left for backspace ... i like this fast backspace feature ;)
  UISwipeGestureRecognizer *swipeDoubleLeft = [[UISwipeGestureRecognizer alloc]
                                          initWithTarget:self action:@selector(handleDoubleSwipeLeft:)];
  [swipeDoubleLeft setNumberOfTouchesRequired:2];
  [swipeDoubleLeft setDirection:UISwipeGestureRecognizerDirectionLeft];
  [[self view] addGestureRecognizer:swipeDoubleLeft];
  [swipeDoubleLeft release];
  
  //single finger swipe left for left
  UISwipeGestureRecognizer *swipeLeft = [[UISwipeGestureRecognizer alloc]
                                      initWithTarget:self action:@selector(handleSwipeLeft:)];
  [swipeLeft setNumberOfTouchesRequired:1];
  [swipeLeft setDirection:UISwipeGestureRecognizerDirectionLeft];
  [[self view] addGestureRecognizer:swipeLeft];
  [swipeLeft release];

  //single finger swipe right for right
  UISwipeGestureRecognizer *swipeRight = [[UISwipeGestureRecognizer alloc]
                                      initWithTarget:self action:@selector(handleSwipeRight:)];
  [swipeRight setNumberOfTouchesRequired:1];
  [swipeRight setDirection:UISwipeGestureRecognizerDirectionRight];
  [[self view] addGestureRecognizer:swipeRight];
  [swipeRight release];

  //single finger swipe up for up
  UISwipeGestureRecognizer *swipeUp = [[UISwipeGestureRecognizer alloc]
                                      initWithTarget:self action:@selector(handleSwipeUp:)];
  [swipeUp setNumberOfTouchesRequired:1];
  [swipeUp setDirection:UISwipeGestureRecognizerDirectionUp];
  [[self view] addGestureRecognizer:swipeUp];  
  [swipeUp release];

  //single finger swipe down for down
  UISwipeGestureRecognizer *swipeDown = [[UISwipeGestureRecognizer alloc]
                                      initWithTarget:self action:@selector(handleSwipeDown:)];
  [swipeDown setNumberOfTouchesRequired:1];
  [swipeDown setDirection:UISwipeGestureRecognizerDirectionDown];
  [[self view] addGestureRecognizer:swipeDown];
  [swipeDown release];
  
}
//--------------------------------------------------------------
- (IBAction)handleDoubleSwipeLeft:(UISwipeGestureRecognizer *)sender 
{
  if([self wakeUpFromSleep])
  {
    [g_xbmcController sendKey:XBMCK_BACKSPACE];
  }
}
//--------------------------------------------------------------
- (IBAction)handleSwipeLeft:(UISwipeGestureRecognizer *)sender 
{
  if([self wakeUpFromSleep])
  {
    [g_xbmcController sendKey:XBMCK_LEFT];
  }
}
//--------------------------------------------------------------
- (IBAction)handleSwipeRight:(UISwipeGestureRecognizer *)sender 
{
  if([self wakeUpFromSleep])
  {
    [g_xbmcController sendKey:XBMCK_RIGHT];
  }
}
//--------------------------------------------------------------
- (IBAction)handleSwipeUp:(UISwipeGestureRecognizer *)sender 
{
  if([self wakeUpFromSleep])
  {
    [g_xbmcController sendKey:XBMCK_UP];
  }
}
//--------------------------------------------------------------
- (IBAction)handleSwipeDown:(UISwipeGestureRecognizer *)sender 
{
  if([self wakeUpFromSleep])
  {
    [g_xbmcController sendKey:XBMCK_DOWN];
  }
}
//--------------------------------------------------------------
- (IBAction)handleDoubleFingerSingleTap:(UIGestureRecognizer *)sender 
{
  if([self wakeUpFromSleep])
  {
    [g_xbmcController sendKey:XBMCK_c];
  }
}
//--------------------------------------------------------------
- (IBAction)handleSingleFingerSingleLongTap:(UIGestureRecognizer *)sender
{
  if([self wakeUpFromSleep])
  {
    if (sender.state == UIGestureRecognizerStateEnded)
    {
      [self handleDoubleFingerSingleTap:sender];
    }
  }
}
//--------------------------------------------------------------
- (IBAction)handleSingleFingerSingleTap:(UIGestureRecognizer *)sender 
{
  if([self wakeUpFromSleep])
  {
    [g_xbmcController sendKey:XBMCK_RETURN];
  }
}
//--------------------------------------------------------------
- (void)viewWillAppear:(BOOL)animated
{
  _startup = true;
  [super viewWillAppear:animated];
}
//--------------------------------------------------------------
- (void)dealloc
{
  [self stopSleepTimer];
  [_touchView release];  
  [_internalWindow release];
  [super dealloc];  
}
//--------------------------------------------------------------
-(BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{ 
  if(_startup)
  {
    //start with landscape
    switch(interfaceOrientation)
    {
      case UIInterfaceOrientationLandscapeLeft:
      case UIInterfaceOrientationLandscapeRight:
        return YES;
      default:
        return FALSE;
    }
  }
  else
  {
    return YES;//we allow all rotations after startup...
  }
}
- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
  if(_startup)
  {
    //start with landscape
    switch(toInterfaceOrientation)
    {
      case UIInterfaceOrientationLandscapeLeft:
      case UIInterfaceOrientationLandscapeRight:
        _startup = false;//allow all orientations after initial landscape rotation
        break;
      default:
        break;
    }
  }
}
@end
