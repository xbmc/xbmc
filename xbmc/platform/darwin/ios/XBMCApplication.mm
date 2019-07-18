/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "XBMCApplication.h"

#import "IOSScreenManager.h"
#import "XBMCController.h"

#import "platform/darwin/NSLogDebugHelpers.h"

#import <AVFoundation/AVAudioSession.h>

@implementation XBMCApplicationDelegate
{
  XBMCController* m_xbmcController;
}

// - iOS6 rotation API - will be called on iOS7 runtime!--------
// - on iOS7 first application is asked for supported orientation
// - then the controller of the current view is asked for supported orientation
// - if both say OK - rotation is allowed
- (NSUInteger)application:(UIApplication *)application supportedInterfaceOrientationsForWindow:(UIWindow *)window
{
  return [[window rootViewController] supportedInterfaceOrientations];
}

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
  [[IOSScreenManager sharedInstance] screenDisconnect];
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
}

- (NSArray<UIKeyCommand*>*)keyCommands
{
  @autoreleasepool
  {
    return @[
      [UIKeyCommand keyCommandWithInput:UIKeyInputUpArrow
                          modifierFlags:kNilOptions
                                 action:@selector(upPressed)],
      [UIKeyCommand keyCommandWithInput:UIKeyInputDownArrow
                          modifierFlags:kNilOptions
                                 action:@selector(downPressed)],
      [UIKeyCommand keyCommandWithInput:UIKeyInputLeftArrow
                          modifierFlags:kNilOptions
                                 action:@selector(leftPressed)],
      [UIKeyCommand keyCommandWithInput:UIKeyInputRightArrow
                          modifierFlags:kNilOptions
                                 action:@selector(rightPressed)]
    ];
  }
}

- (void)upPressed
{
  [g_xbmcController sendKey:XBMCK_UP];
}

- (void)downPressed
{
  [g_xbmcController sendKey:XBMCK_DOWN];
}

- (void)leftPressed
{
  [g_xbmcController sendKey:XBMCK_LEFT];
}

- (void)rightPressed
{
  [g_xbmcController sendKey:XBMCK_RIGHT];
}

@end


static void SigPipeHandler(int s)
{
  NSLog(@"We Got a Pipe Single :%d____________", s);
}

int main(int argc, char *argv[]) {
  @autoreleasepool
  {
    int retVal = 0;

    signal(SIGPIPE, SigPipeHandler);

    @try
    {
      retVal = UIApplicationMain(argc, argv, nil, NSStringFromClass(XBMCApplicationDelegate.class));
    }
    @catch (id theException)
    {
      ELOG(@"%@", theException);
    }
    @finally
    {
      ILOG(@"This always happens.");
    }

    return retVal;
  }
}
