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
 
#import <UIKit/UIKit.h>
#import <AVFoundation/AVAudioSession.h>

#import "XBMCApplication.h"
#import "XBMCController.h"
#import "IOSScreenManager.h"
#import "XBMCDebugHelpers.h"

@implementation XBMCApplicationDelegate
XBMCController *m_xbmcController;  

- (void)applicationWillResignActive:(UIApplication *)application
{
  PRINT_SIGNATURE();

  [m_xbmcController pauseAnimation];
  [m_xbmcController becomeInactive];
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
  PRINT_SIGNATURE();

  [m_xbmcController resumeAnimation];
  [m_xbmcController enterForeground];
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
  PRINT_SIGNATURE();

  if (application.applicationState == UIApplicationStateBackground)
  {
    // the app is turn into background, not in by screen lock which has app state inactive.
    [m_xbmcController enterBackground];
  }
}

- (void)applicationWillTerminate:(UIApplication *)application
{
  PRINT_SIGNATURE();

  [m_xbmcController stopAnimation];
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
  PRINT_SIGNATURE();
}

- (void)screenDidConnect:(NSNotification *)aNotification
{
  [IOSScreenManager updateResolutions];
}

- (void)screenDidDisconnect:(NSNotification *)aNotification
{
  [IOSScreenManager updateResolutions];
}

- (void)registerScreenNotifications:(BOOL)bRegister
{
  NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];  
  
  if( bRegister )
  {
    //register to screen notifications
    [nc addObserver:self selector:@selector(screenDidConnect:) name:UIScreenDidConnectNotification object:nil]; 
    [nc addObserver:self selector:@selector(screenDidDisconnect:) name:UIScreenDidDisconnectNotification object:nil]; 
  }
  else
  {
    //deregister from screen notifications
    [nc removeObserver:self name:UIScreenDidConnectNotification object:nil];
    [nc removeObserver:self name:UIScreenDidDisconnectNotification object:nil];
  }
}

- (void)applicationDidFinishLaunching:(UIApplication *)application 
{
  PRINT_SIGNATURE();

  [[UIDevice currentDevice] setBatteryMonitoringEnabled:YES];
  UIScreen *currentScreen = [UIScreen mainScreen];

  m_xbmcController = [[XBMCController alloc] initWithFrame: [currentScreen bounds] withScreen:currentScreen];  
  m_xbmcController.wantsFullScreenLayout = YES;  
  [m_xbmcController startAnimation];
  [self registerScreenNotifications:YES];

  NSError *err = nil;
  if (![[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback error:&err])
  {
    ELOG(@"AVAudioSession setCategory failed: %@", err);
  }
  err = nil;
  if (![[AVAudioSession sharedInstance] setActive: YES error: &err])
  {
    ELOG(@"AVAudioSession setActive failed: %@", err);
  }
  [[AVAudioSession sharedInstance] setDelegate:self];
}

- (void)beginInterruption
{
  PRINT_SIGNATURE();
  [m_xbmcController beginInterruption];
}
- (void)endInterruptionWithFlags:(NSUInteger)flags
{
  LOG(@"%s: %d", __PRETTY_FUNCTION__, flags);
  if (flags & AVAudioSessionInterruptionFlags_ShouldResume)
  {
    NSError *err = nil;
    if (![[AVAudioSession sharedInstance] setActive: YES error: &err])
    {
      ELOG(@"AVAudioSession::endInterruption setActive failed: %@", err);
    }
    [m_xbmcController endInterruption];
  }
}

- (void)dealloc
{
  [self registerScreenNotifications:NO];
  [m_xbmcController stopAnimation];
  [m_xbmcController release];

  [super dealloc];
}
@end

@interface XBMCApplication : UIApplication{
}
@end

@implementation XBMCApplication
#define GSEVENT_TYPE 2
#define GSEVENT_FLAGS 12
#define GSEVENTKEY_KEYCODE 15
#define GSEVENT_TYPE_KEYUP 11

- (void)sendEvent:(UIEvent *)event
{ 
  [super sendEvent:event];

  if ([event respondsToSelector:@selector(_gsEvent)]) 
  {
    // Key events come in form of UIInternalEvents.
    // They contain a GSEvent object which contains 
    // a GSEventRecord among other things
    int *eventMem;
    eventMem = (int *)[event performSelector:@selector(_gsEvent)];
    if (eventMem) 
    {
      // So far we got a GSEvent :)
      int eventType = eventMem[GSEVENT_TYPE];
      if (eventType == GSEVENT_TYPE_KEYUP) 
      {
        // Now we got a GSEventKey!
        
        // Read flags from GSEvent
        // for modifier keys if we want to use them somehow at a later time
        //int eventFlags = eventMem[GSEVENT_FLAGS];
        // Read keycode from GSEventKey
        UniChar tmp = (UniChar)eventMem[GSEVENTKEY_KEYCODE];
        XBMCKey key = XBMCK_UNKNOWN;
        switch (tmp)
        {
          case 0x4f:
            // right
            key = XBMCK_RIGHT;
            break;
          case 0x50:
            // left
            key = XBMCK_LEFT;
            break;
          case 0x51:
            // down
            key = XBMCK_DOWN;
            break;
          case 0x52:
            // up
            key = XBMCK_UP;
            break;
          default:
            return; // not supported by us - return...
        }
        [g_xbmcController sendKey:key];
      }
    }
  }
}
@end

int main(int argc, char *argv[]) {
  NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];	
  int retVal = 0;
  
  // Block SIGPIPE
  // SIGPIPE repeatably kills us, turn it off
  {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    sigprocmask(SIG_BLOCK, &set, NULL);
  }
  
  @try
  {
    retVal = UIApplicationMain(argc,argv,@"XBMCApplication",@"XBMCApplicationDelegate");
    //UIApplicationMain(argc, argv, nil, nil);
  } 
  @catch (id theException) 
  {
    ELOG(@"%@", theException);
  }
  @finally 
  {
    ILOG(@"This always happens.");
  }
    
  [pool release];
	
  return retVal;

}
