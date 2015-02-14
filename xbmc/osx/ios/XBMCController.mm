/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#include "system.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "MusicInfoTag.h"
#include "SpecialProtocol.h"
#include "PlayList.h"
#include "ApplicationMessenger.h"
#include "Application.h"
#include "interfaces/AnnouncementManager.h"
#include "input/touch/generic/GenericTouchActionHandler.h"
#include "guilib/GUIControl.h"
#include "input/Key.h"
#include "windowing/WindowingFactory.h"
#include "video/VideoReferenceClock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "utils/Variant.h"
#include "Util.h"
#include "threads/Event.h"
#define id _id
#include "TextureCache.h"
#undef id
#include <math.h>
#include "osx/DarwinUtils.h"

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795028842
#endif
#define RADIANS_TO_DEGREES(radians) ((radians) * (180.0 / M_PI))

#undef BOOL

#import <AVFoundation/AVAudioSession.h>
#import <MediaPlayer/MPMediaItem.h>
#ifdef __IPHONE_5_0
#import <MediaPlayer/MPNowPlayingInfoCenter.h>
#else
const NSString *MPNowPlayingInfoPropertyElapsedPlaybackTime = @"MPNowPlayingInfoPropertyElapsedPlaybackTime";
const NSString *MPNowPlayingInfoPropertyPlaybackRate = @"MPNowPlayingInfoPropertyPlaybackRate";
const NSString *MPNowPlayingInfoPropertyPlaybackQueueIndex = @"MPNowPlayingInfoPropertyPlaybackQueueIndex";
const NSString *MPNowPlayingInfoPropertyPlaybackQueueCount = @"MPNowPlayingInfoPropertyPlaybackQueueCount";
#endif
#import "IOSEAGLView.h"

#import "XBMCController.h"
#import "IOSScreenManager.h"
#import "XBMCApplication.h"
#import "XBMCDebugHelpers.h"
#import "AutoPool.h"

XBMCController *g_xbmcController;
static CEvent screenChangeEvent;


// notification messages
extern NSString* kBRScreenSaverActivated;
extern NSString* kBRScreenSaverDismissed;

id objectFromVariant(const CVariant &data);

NSArray *arrayFromVariantArray(const CVariant &data)
{
  if (!data.isArray())
    return nil;
  NSMutableArray *array = [[[NSMutableArray alloc] initWithCapacity:data.size()] autorelease];
  for (CVariant::const_iterator_array itr = data.begin_array(); itr != data.end_array(); ++itr)
  {
    [array addObject:objectFromVariant(*itr)];
  }
  return array;
}

NSDictionary *dictionaryFromVariantMap(const CVariant &data)
{
  if (!data.isObject())
    return nil;
  NSMutableDictionary *dict = [[[NSMutableDictionary alloc] initWithCapacity:data.size()] autorelease];
  for (CVariant::const_iterator_map itr = data.begin_map(); itr != data.end_map(); ++itr)
  {
    [dict setValue:objectFromVariant(itr->second) forKey:[NSString stringWithUTF8String:itr->first.c_str()]];
  }
  return dict;
}

id objectFromVariant(const CVariant &data)
{
  if (data.isNull())
    return nil;
  if (data.isString())
    return [NSString stringWithUTF8String:data.asString().c_str()];
  if (data.isWideString())
    return [NSString stringWithCString:(const char *)data.asWideString().c_str() encoding:NSUnicodeStringEncoding];
  if (data.isInteger())
    return [NSNumber numberWithLongLong:data.asInteger()];
  if (data.isUnsignedInteger())
    return [NSNumber numberWithUnsignedLongLong:data.asUnsignedInteger()];
  if (data.isBoolean())
    return [NSNumber numberWithInt:data.asBoolean()?1:0];
  if (data.isDouble())
    return [NSNumber numberWithDouble:data.asDouble()];
  if (data.isArray())
    return arrayFromVariantArray(data);
  if (data.isObject())
    return dictionaryFromVariantMap(data);
  return nil;
}

void AnnounceBridge(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  LOG(@"AnnounceBridge: [%s], [%s], [%s]", ANNOUNCEMENT::AnnouncementFlagToString(flag), sender, message);
  NSDictionary *dict = dictionaryFromVariantMap(data);
  LOG(@"data: %@", dict.description);
  const std::string msg(message);
  if (msg == "OnPlay")
  {
    NSDictionary *item = [dict valueForKey:@"item"];
    NSDictionary *player = [dict valueForKey:@"player"];
    [item setValue:[player valueForKey:@"speed"] forKey:@"speed"];
    std::string thumb = g_application.CurrentFileItem().GetArt("thumb");
    if (!thumb.empty())
    {
      bool needsRecaching;
      std::string cachedThumb(CTextureCache::Get().CheckCachedImage(thumb, false, needsRecaching));
      LOG("thumb: %s, %s", thumb.c_str(), cachedThumb.c_str());
      if (!cachedThumb.empty())
      {
        std::string thumbRealPath = CSpecialProtocol::TranslatePath(cachedThumb);
        [item setValue:[NSString stringWithUTF8String:thumbRealPath.c_str()] forKey:@"thumb"];
      }
    }
    double duration = g_application.GetTotalTime();
    if (duration > 0)
      [item setValue:[NSNumber numberWithDouble:duration] forKey:@"duration"];
    [item setValue:[NSNumber numberWithDouble:g_application.GetTime()] forKey:@"elapsed"];
    int current = g_playlistPlayer.GetCurrentSong();
    if (current >= 0)
    {
      [item setValue:[NSNumber numberWithInt:current] forKey:@"current"];
      [item setValue:[NSNumber numberWithInt:g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist()).size()] forKey:@"total"];
    }
    if (g_application.CurrentFileItem().HasMusicInfoTag())
    {
      const std::vector<std::string> &genre = g_application.CurrentFileItem().GetMusicInfoTag()->GetGenre();
      if (!genre.empty())
      {
        NSMutableArray *genreArray = [[NSMutableArray alloc] initWithCapacity:genre.size()];
        for(std::vector<std::string>::const_iterator it = genre.begin(); it != genre.end(); ++it)
        {
          [genreArray addObject:[NSString stringWithUTF8String:it->c_str()]];
        }
        [item setValue:genreArray forKey:@"genre"];
      }
    }
    LOG(@"item: %@", item.description);
    [g_xbmcController performSelectorOnMainThread:@selector(onPlay:) withObject:item  waitUntilDone:NO];
  }
  else if (msg == "OnSpeedChanged" || msg == "OnPause")
  {
    NSDictionary *item = [dict valueForKey:@"item"];
    NSDictionary *player = [dict valueForKey:@"player"];
    [item setValue:[player valueForKey:@"speed"] forKey:@"speed"];
    [item setValue:[NSNumber numberWithDouble:g_application.GetTime()] forKey:@"elapsed"];
    LOG(@"item: %@", item.description);
    [g_xbmcController performSelectorOnMainThread:@selector(OnSpeedChanged:) withObject:item  waitUntilDone:NO];
    if (msg == "OnPause")
      [g_xbmcController performSelectorOnMainThread:@selector(onPause:) withObject:[dict valueForKey:@"item"]  waitUntilDone:NO];
  }
  else if (msg == "OnStop")
  {
    [g_xbmcController performSelectorOnMainThread:@selector(onStop:) withObject:[dict valueForKey:@"item"]  waitUntilDone:NO];
  }
}

class AnnounceReceiver : public ANNOUNCEMENT::IAnnouncer
{
public:
  virtual void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
  {
    // not all Announce called from xbmc main thread, we need an auto poll here.
    CCocoaAutoPool pool;
    AnnounceBridge(flag, sender, message, data);
  }
  virtual ~AnnounceReceiver() {}
  static void init()
  {
    if (NULL==g_announceReceiver) {
      g_announceReceiver = new AnnounceReceiver();
      ANNOUNCEMENT::CAnnouncementManager::Get().AddAnnouncer(g_announceReceiver);
    }
  }
  static void dealloc()
  {
    ANNOUNCEMENT::CAnnouncementManager::Get().RemoveAnnouncer(g_announceReceiver);
    delete g_announceReceiver;
  }
private:
  AnnounceReceiver() {}
  static AnnounceReceiver *g_announceReceiver;
};

AnnounceReceiver *AnnounceReceiver::g_announceReceiver = NULL;

//--------------------------------------------------------------
//

@interface XBMCController ()
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
@synthesize nowPlayingInfo;
//--------------------------------------------------------------
- (void) sendKeypressEvent: (XBMC_Event) event
{
  event.type = XBMC_KEYDOWN;
  CWinEvents::MessagePush(&event);

  event.type = XBMC_KEYUP;
  CWinEvents::MessagePush(&event);
}

// START OF UIKeyInput protocol
- (BOOL)hasText
{
  return NO;
}

- (void)insertText:(NSString *)text
{
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

// - iOS6 rotation API - will be called on iOS7 runtime!--------
- (NSUInteger)supportedInterfaceOrientations
{
  //mask defines available as of ios6 sdk
  //return UIInterfaceOrientationMaskLandscape;
  return (1 << UIInterfaceOrientationLandscapeLeft) | (1 << UIInterfaceOrientationLandscapeRight);
}
// - old rotation API will be called on iOS6 and lower - removed in iOS7
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
#if __IPHONE_8_0
  if (CDarwinUtils::GetIOSVersion() < 8.0)
#endif
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
      case UIInterfaceOrientationUnknown:
        break;//just leave the rect as is
    }
    m_glView.frame = rect;
  }
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
- (void)createGestureRecognizers 
{
  //1 finger single tab
  UITapGestureRecognizer *singleFingerSingleTap = [[UITapGestureRecognizer alloc]
                                                   initWithTarget:self action:@selector(handleSingleFingerSingleTap:)];

  singleFingerSingleTap.delaysTouchesBegan = NO;
  singleFingerSingleTap.numberOfTapsRequired = 1;
  singleFingerSingleTap.numberOfTouchesRequired = 1;

  [m_glView addGestureRecognizer:singleFingerSingleTap];
  [singleFingerSingleTap release];

  //2 finger single tab - right mouse
  //single finger double tab delays single finger single tab - so we
  //go for 2 fingers here - so single finger single tap is instant
  UITapGestureRecognizer *doubleFingerSingleTap = [[UITapGestureRecognizer alloc]
    initWithTarget:self action:@selector(handleDoubleFingerSingleTap:)];  

  doubleFingerSingleTap.delaysTouchesBegan = NO;
  doubleFingerSingleTap.numberOfTapsRequired = 1;
  doubleFingerSingleTap.numberOfTouchesRequired = 2;
  [m_glView addGestureRecognizer:doubleFingerSingleTap];
  [doubleFingerSingleTap release];

  //1 finger single long tab - right mouse - alernative
  UILongPressGestureRecognizer *singleFingerSingleLongTap = [[UILongPressGestureRecognizer alloc]
    initWithTarget:self action:@selector(handleSingleFingerSingleLongTap:)];  

  singleFingerSingleLongTap.delaysTouchesBegan = NO;
  singleFingerSingleLongTap.delaysTouchesEnded = NO;
  [m_glView addGestureRecognizer:singleFingerSingleLongTap];
  [singleFingerSingleLongTap release];

  //double finger swipe left for backspace ... i like this fast backspace feature ;)
  UISwipeGestureRecognizer *swipeLeft2 = [[UISwipeGestureRecognizer alloc]
                                            initWithTarget:self action:@selector(handleSwipe:)];

  swipeLeft2.delaysTouchesBegan = NO;
  swipeLeft2.numberOfTouchesRequired = 2;
  swipeLeft2.direction = UISwipeGestureRecognizerDirectionLeft;
  swipeLeft2.delegate = self;
  [m_glView addGestureRecognizer:swipeLeft2];
  [swipeLeft2 release];

  //single finger swipe left
  UISwipeGestureRecognizer *swipeLeft = [[UISwipeGestureRecognizer alloc]
                                          initWithTarget:self action:@selector(handleSwipe:)];

  swipeLeft.delaysTouchesBegan = NO;
  swipeLeft.numberOfTouchesRequired = 1;
  swipeLeft.direction = UISwipeGestureRecognizerDirectionLeft;
  swipeLeft.delegate = self;
  [m_glView addGestureRecognizer:swipeLeft];
  [swipeLeft release];
  
  //single finger swipe right
  UISwipeGestureRecognizer *swipeRight = [[UISwipeGestureRecognizer alloc]
                                         initWithTarget:self action:@selector(handleSwipe:)];
  
  swipeRight.delaysTouchesBegan = NO;
  swipeRight.numberOfTouchesRequired = 1;
  swipeRight.direction = UISwipeGestureRecognizerDirectionRight;
  swipeRight.delegate = self;
  [m_glView addGestureRecognizer:swipeRight];
  [swipeRight release];
  
  //single finger swipe up
  UISwipeGestureRecognizer *swipeUp = [[UISwipeGestureRecognizer alloc]
                                         initWithTarget:self action:@selector(handleSwipe:)];
  
  swipeUp.delaysTouchesBegan = NO;
  swipeUp.numberOfTouchesRequired = 1;
  swipeUp.direction = UISwipeGestureRecognizerDirectionUp;
  swipeUp.delegate = self;
  [m_glView addGestureRecognizer:swipeUp];
  [swipeUp release];

  //single finger swipe down
  UISwipeGestureRecognizer *swipeDown = [[UISwipeGestureRecognizer alloc]
                                         initWithTarget:self action:@selector(handleSwipe:)];
  
  swipeDown.delaysTouchesBegan = NO;
  swipeDown.numberOfTouchesRequired = 1;
  swipeDown.direction = UISwipeGestureRecognizerDirectionDown;
  swipeDown.delegate = self;
  [m_glView addGestureRecognizer:swipeDown];
  [swipeDown release];
  
  //for pan gestures with one finger
  UIPanGestureRecognizer *pan = [[UIPanGestureRecognizer alloc]
    initWithTarget:self action:@selector(handlePan:)];

  pan.delaysTouchesBegan = NO;
  pan.maximumNumberOfTouches = 1;
  [m_glView addGestureRecognizer:pan];
  [pan release];

  //for zoom gesture
  UIPinchGestureRecognizer *pinch = [[UIPinchGestureRecognizer alloc]
    initWithTarget:self action:@selector(handlePinch:)];

  pinch.delaysTouchesBegan = NO;
  pinch.delegate = self;
  [m_glView addGestureRecognizer:pinch];
  [pinch release];

  //for rotate gesture
  UIRotationGestureRecognizer *rotate = [[UIRotationGestureRecognizer alloc]
                                         initWithTarget:self action:@selector(handleRotate:)];

  rotate.delaysTouchesBegan = NO;
  rotate.delegate = self;
  [m_glView addGestureRecognizer:rotate];
  [rotate release];
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
-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
  if( [m_glView isXBMCAlive] )//NO GESTURES BEFORE WE ARE UP AND RUNNING
  {
    UITouch *touch = (UITouch *)[[touches allObjects] objectAtIndex:0];
    CGPoint point = [touch locationInView:m_glView];
    point.x *= screenScale;
    point.y *= screenScale;
    CGenericTouchActionHandler::Get().OnSingleTouchStart(point.x, point.y);
  }
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
        CGenericTouchActionHandler::Get().OnTouchGestureStart(point.x, point.y);
        break;
      case UIGestureRecognizerStateChanged:
        CGenericTouchActionHandler::Get().OnZoomPinch(point.x, point.y, [sender scale]);
        break;
      case UIGestureRecognizerStateEnded:
      case UIGestureRecognizerStateCancelled:
        CGenericTouchActionHandler::Get().OnTouchGestureEnd(point.x, point.y, 0, 0, 0, 0);
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
        CGenericTouchActionHandler::Get().OnTouchGestureStart(point.x, point.y);
        break;
      case UIGestureRecognizerStateChanged:
        CGenericTouchActionHandler::Get().OnRotate(point.x, point.y, RADIANS_TO_DEGREES([sender rotation]));
        break;
      case UIGestureRecognizerStateEnded:
        CGenericTouchActionHandler::Get().OnTouchGestureEnd(point.x, point.y, 0, 0, 0, 0);
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
    CGPoint velocity = [sender velocityInView:m_glView];

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
          CGenericTouchActionHandler::Get().OnTouchGestureStart((float)point.x, (float)point.y);
          touchBeginSignaled = true;
        }

        CGenericTouchActionHandler::Get().OnTouchGesturePan((float)point.x, (float)point.y,
                                                            (float)xMovement, (float)yMovement, 
                                                            (float)velocity.x, (float)velocity.y);
        lastGesturePoint = point;
      }
    }
    
    if( touchBeginSignaled && ([sender state] == UIGestureRecognizerStateEnded || [sender state] == UIGestureRecognizerStateCancelled))
    {
      //signal end of pan - this will start inertial scrolling with deacceleration in CApplication
      CGenericTouchActionHandler::Get().OnTouchGestureEnd((float)lastGesturePoint.x, (float)lastGesturePoint.y,
                                                             (float)0.0, (float)0.0, 
                                                             (float)velocity.x, (float)velocity.y);

      touchBeginSignaled = false;
    }
  }
}
//--------------------------------------------------------------
- (IBAction)handleSwipe:(UISwipeGestureRecognizer *)sender
{
  if( [m_glView isXBMCAlive] )//NO GESTURES BEFORE WE ARE UP AND RUNNING
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
      CGenericTouchActionHandler::Get().OnSwipe(direction,
                                                0.0, 0.0,
                                                point.x, point.y, 0, 0,
                                                [sender numberOfTouches]);
    }
  }
}
//--------------------------------------------------------------
- (IBAction)handleSingleFingerSingleTap:(UIGestureRecognizer *)sender 
{
  //Allow the tap gesture during init
  //(for allowing the user to tap away any messagboxes during init)
  if( [m_glView isReadyToRun] )
  {
    CGPoint point = [sender locationOfTouch:0 inView:m_glView];
    point.x *= screenScale;
    point.y *= screenScale;
    //NSLog(@"%s singleTap", __PRETTY_FUNCTION__);
    CGenericTouchActionHandler::Get().OnTap((float)point.x, (float)point.y, [sender numberOfTouches]);
  }
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
    CGenericTouchActionHandler::Get().OnTap((float)point.x, (float)point.y, [sender numberOfTouches]);
  }
}
//--------------------------------------------------------------
- (IBAction)handleSingleFingerSingleLongTap:(UIGestureRecognizer *)sender
{
  if( [m_glView isXBMCAlive] )//NO GESTURES BEFORE WE ARE UP AND RUNNING
  {
    CGPoint point = [sender locationOfTouch:0 inView:m_glView];
    point.x *= screenScale;
    point.y *= screenScale;

    if (sender.state == UIGestureRecognizerStateBegan)
    {
      lastGesturePoint = point;
      // mark the control
      //CGenericTouchActionHandler::Get().OnSingleTouchStart((float)point.x, (float)point.y);
    }

    if (sender.state == UIGestureRecognizerStateEnded)
    {
      CGenericTouchActionHandler::Get().OnSingleTouchMove((float)point.x, (float)point.y, point.x - lastGesturePoint.x, point.y - lastGesturePoint.y, 0, 0);
    }
    
    if (sender.state == UIGestureRecognizerStateEnded)
    {	
      CGenericTouchActionHandler::Get().OnLongPress((float)point.x, (float)point.y);
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

  m_isPlayingBeforeInactive = NO;
  m_bgTask = UIBackgroundTaskInvalid;
  m_playbackState = IOS_PLAYBACK_STOPPED;

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

#if __IPHONE_8_0
  if (CDarwinUtils::GetIOSVersion() < 8.0)
#endif
  {
    /* We start in landscape mode */
    CGRect srect = frame;
    // in ios sdks older then 8.0 the landscape mode is 90 degrees
    // rotated
    srect.size = CGSizeMake( frame.size.height, frame.size.width );
  
    m_glView = [[IOSEAGLView alloc] initWithFrame: srect withScreen:screen];
    [[IOSScreenManager sharedInstance] setView:m_glView];
    [m_glView setMultipleTouchEnabled:YES];
  
    /* Check if screen is Retina */
    screenScale = [m_glView getScreenScale:screen];

    [self.view addSubview: m_glView];
  
    [self createGestureRecognizers];
    [m_window addSubview: self.view];
  }

  [m_window makeKeyAndVisible];
  g_xbmcController = self;  
  
  AnnounceReceiver::init();

  return self;
}
//--------------------------------------------------------------
#if __IPHONE_8_0
- (void)loadView
{
  [super loadView];
  if (CDarwinUtils::GetIOSVersion() >= 8.0)
  {
    self.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.view.autoresizesSubviews = YES;
  
    m_glView = [[IOSEAGLView alloc] initWithFrame:self.view.bounds withScreen:[UIScreen mainScreen]];
    [[IOSScreenManager sharedInstance] setView:m_glView];
    [m_glView setMultipleTouchEnabled:YES];
  
    /* Check if screen is Retina */
    screenScale = [m_glView getScreenScale:[UIScreen mainScreen]];
  
    [self.view addSubview: m_glView];
  
    [self createGestureRecognizers];
  }
}
#endif
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

  AnnounceReceiver::dealloc();
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
  return [[[UIView alloc] initWithFrame:CGRectZero] autorelease];
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
#if __IPHONE_8_0
  view.layer.bounds = view.bounds;
#else
  [view setFrame:m_window.frame];
#endif
  m_window.screen = screen;
#if __IPHONE_8_0
  [view setFrame:m_window.frame];
#endif
}
//--------------------------------------------------------------
- (void) remoteControlReceivedWithEvent: (UIEvent *) receivedEvent {
  LOG(@"%s: type %d, subtype: %d", __PRETTY_FUNCTION__, receivedEvent.type, receivedEvent.subtype);
  if (receivedEvent.type == UIEventTypeRemoteControl)
  {
    [self disableNetworkAutoSuspend];
    switch (receivedEvent.subtype)
    {
      case UIEventSubtypeRemoteControlTogglePlayPause:
        CApplicationMessenger::Get().SendAction(ACTION_PLAYER_PLAYPAUSE);
        break;
      case UIEventSubtypeRemoteControlPlay:
        CApplicationMessenger::Get().SendAction(ACTION_PLAYER_PLAY);
        break;
      case UIEventSubtypeRemoteControlPause:
        // ACTION_PAUSE sometimes cause unpause, use MediaPauseIfPlaying to make sure pause only
        CApplicationMessenger::Get().MediaPauseIfPlaying();
        break;
      case UIEventSubtypeRemoteControlNextTrack:
        CApplicationMessenger::Get().SendAction(ACTION_NEXT_ITEM);
        break;
      case UIEventSubtypeRemoteControlPreviousTrack:
        CApplicationMessenger::Get().SendAction(ACTION_PREV_ITEM);
        break;
      case UIEventSubtypeRemoteControlBeginSeekingForward:
        // use 4X speed forward.
        CApplicationMessenger::Get().SendAction(ACTION_PLAYER_FORWARD);
        CApplicationMessenger::Get().SendAction(ACTION_PLAYER_FORWARD);
        break;
      case UIEventSubtypeRemoteControlBeginSeekingBackward:
        // use 4X speed rewind.
        CApplicationMessenger::Get().SendAction(ACTION_PLAYER_REWIND);
        CApplicationMessenger::Get().SendAction(ACTION_PLAYER_REWIND);
        break;
      case UIEventSubtypeRemoteControlEndSeekingForward:
      case UIEventSubtypeRemoteControlEndSeekingBackward:
        // restore to normal playback speed.
        if (g_application.m_pPlayer->IsPlaying() && !g_application.m_pPlayer->IsPaused())
          CApplicationMessenger::Get().SendAction(ACTION_PLAYER_PLAY);
        break;
      default:
        LOG(@"unhandled subtype: %d", receivedEvent.subtype);
        break;
    }
    [self rescheduleNetworkAutoSuspend];
  }
}
//--------------------------------------------------------------
- (void)enterBackground
{
  PRINT_SIGNATURE();
  if (g_application.m_pPlayer->IsPlaying() && !g_application.m_pPlayer->IsPaused())
  {
    m_isPlayingBeforeInactive = YES;
    CApplicationMessenger::Get().MediaPauseIfPlaying();
  }
  g_Windowing.OnAppFocusChange(false);
}

- (void)enterForeground
{
  PRINT_SIGNATURE();
  g_Windowing.OnAppFocusChange(true);
  // when we come back, restore playing if we were.
  if (m_isPlayingBeforeInactive)
  {
    CApplicationMessenger::Get().MediaUnPause();
    m_isPlayingBeforeInactive = NO;
  }
}

- (void)becomeInactive
{
  // if we were interrupted, already paused here
  // else if user background us or lock screen, only pause video here, audio keep playing.
  if (g_application.m_pPlayer->IsPlayingVideo() && !g_application.m_pPlayer->IsPaused())
  {
    m_isPlayingBeforeInactive = YES;
    CApplicationMessenger::Get().MediaPauseIfPlaying();
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
//--------------------------------------------------------------
- (void)setIOSNowPlayingInfo:(NSDictionary *)info
{
  self.nowPlayingInfo = info;
  // MPNowPlayingInfoCenter is an ios5+ class, following code will work on ios5 even if compiled by xcode3
  Class NowPlayingInfoCenter = NSClassFromString(@"MPNowPlayingInfoCenter");
  if (NowPlayingInfoCenter)
    [[NowPlayingInfoCenter defaultCenter] setNowPlayingInfo:self.nowPlayingInfo];
}
//--------------------------------------------------------------
- (void)onPlay:(NSDictionary *)item
{
  PRINT_SIGNATURE();
  NSMutableDictionary * dict = [[NSMutableDictionary alloc] init];

  NSString *title = [item objectForKey:@"title"];
  if (title && title.length > 0)
    [dict setObject:title forKey:MPMediaItemPropertyTitle];
  NSString *album = [item objectForKey:@"album"];
  if (album && album.length > 0)
    [dict setObject:album forKey:MPMediaItemPropertyAlbumTitle];
  NSArray *artists = [item objectForKey:@"artist"];
  if (artists && artists.count > 0)
    [dict setObject:[artists componentsJoinedByString:@" "] forKey:MPMediaItemPropertyArtist];
  NSNumber *track = [item objectForKey:@"track"];
  if (track)
    [dict setObject:track forKey:MPMediaItemPropertyAlbumTrackNumber];
  NSNumber *duration = [item objectForKey:@"duration"];
  if (duration)
    [dict setObject:duration forKey:MPMediaItemPropertyPlaybackDuration];
  NSArray *genres = [item objectForKey:@"genre"];
  if (genres && genres.count > 0)
    [dict setObject:[genres componentsJoinedByString:@" "] forKey:MPMediaItemPropertyGenre];

  if (NSClassFromString(@"MPNowPlayingInfoCenter"))
  {
    NSString *thumb = [item objectForKey:@"thumb"];
    if (thumb && thumb.length > 0)
    {
      UIImage *image = [UIImage imageWithContentsOfFile:thumb];
      if (image)
      {
        MPMediaItemArtwork *mArt = [[MPMediaItemArtwork alloc] initWithImage:image];
        if (mArt)
        {
          [dict setObject:mArt forKey:MPMediaItemPropertyArtwork];
          [mArt release];
        }
      }
    }
    // these proprity keys are ios5+ only
    NSNumber *elapsed = [item objectForKey:@"elapsed"];
    if (elapsed)
      [dict setObject:elapsed forKey:MPNowPlayingInfoPropertyElapsedPlaybackTime];
    NSNumber *speed = [item objectForKey:@"speed"];
    if (speed)
      [dict setObject:speed forKey:MPNowPlayingInfoPropertyPlaybackRate];
    NSNumber *current = [item objectForKey:@"current"];
    if (current)
      [dict setObject:current forKey:MPNowPlayingInfoPropertyPlaybackQueueIndex];
    NSNumber *total = [item objectForKey:@"total"];
    if (total)
      [dict setObject:total forKey:MPNowPlayingInfoPropertyPlaybackQueueCount];
  }
  /*
   other properities can be set:
   MPMediaItemPropertyAlbumTrackCount
   MPMediaItemPropertyComposer
   MPMediaItemPropertyDiscCount
   MPMediaItemPropertyDiscNumber
   MPMediaItemPropertyPersistentID

   Additional metadata properties:
   MPNowPlayingInfoPropertyChapterNumber;
   MPNowPlayingInfoPropertyChapterCount;
   */

  [self setIOSNowPlayingInfo:dict];
  [dict release];

  m_playbackState = IOS_PLAYBACK_PLAYING;
  [self disableNetworkAutoSuspend];
}
//--------------------------------------------------------------
- (void)OnSpeedChanged:(NSDictionary *)item
{
  PRINT_SIGNATURE();
  if (NSClassFromString(@"MPNowPlayingInfoCenter"))
  {
    NSMutableDictionary *info = [self.nowPlayingInfo mutableCopy];
    NSNumber *elapsed = [item objectForKey:@"elapsed"];
    if (elapsed)
      [info setObject:elapsed forKey:MPNowPlayingInfoPropertyElapsedPlaybackTime];
    NSNumber *speed = [item objectForKey:@"speed"];
    if (speed)
      [info setObject:speed forKey:MPNowPlayingInfoPropertyPlaybackRate];

    [self setIOSNowPlayingInfo:info];
  }
}
//--------------------------------------------------------------
- (void)onPause:(NSDictionary *)item
{
  PRINT_SIGNATURE();
  m_playbackState = IOS_PLAYBACK_PAUSED;
  // schedule set network auto suspend state for save power if idle.
  [self rescheduleNetworkAutoSuspend];
}
//--------------------------------------------------------------
- (void)onStop:(NSDictionary *)item
{
  PRINT_SIGNATURE();
  [self setIOSNowPlayingInfo:nil];

  m_playbackState = IOS_PLAYBACK_STOPPED;
  // delay set network auto suspend state in case we are switching playing item.
  [self rescheduleNetworkAutoSuspend];
}
//--------------------------------------------------------------
- (void)rescheduleNetworkAutoSuspend
{
  LOG(@"%s: playback state: %d", __PRETTY_FUNCTION__,  m_playbackState);
  if (m_playbackState == IOS_PLAYBACK_PLAYING)
  {
    [self disableNetworkAutoSuspend];
    return;
  }
  if (m_networkAutoSuspendTimer)
    [m_networkAutoSuspendTimer invalidate];

  int delay = m_playbackState == IOS_PLAYBACK_PAUSED ? 60 : 30;  // wait longer if paused than stopped
  self.m_networkAutoSuspendTimer = [NSTimer scheduledTimerWithTimeInterval:delay target:self selector:@selector(enableNetworkAutoSuspend:) userInfo:nil repeats:NO];
}

#pragma mark -
#pragma mark private helper methods
//
- (void)observeDefaultCenterStuff: (NSNotification *) notification
{
//  LOG(@"default: %@", [notification name]);
//  LOG(@"userInfo: %@", [notification userInfo]);
}

@end
