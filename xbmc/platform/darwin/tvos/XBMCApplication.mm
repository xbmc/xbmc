/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "platform/darwin/tvos/XBMCApplication.h"

#import "platform/darwin/NSLogDebugHelpers.h"
#import "platform/darwin/tvos/PreflightHandler.h"
#import "platform/darwin/tvos/TVOSTopShelf.h"
#import "platform/darwin/tvos/XBMCController.h"

#import <AVFoundation/AVFoundation.h>

@implementation XBMCApplicationDelegate

- (XBMCController*)xbmcController
{
  return static_cast<XBMCController*>(self.window.rootViewController);
}

#pragma mark - Shutdown Procedures

- (void)applicationWillResignActive:(UIApplication*)application
{
  // Occurs when Kodi is interrupted by something
  // (e.g. Siri triggered by user, Control center opened by user, Mutlitask opened by user ...)
}

- (void)applicationDidEnterBackground:(UIApplication*)application
{
  // Occurs when Kodi has been backgrounded
  // (e.g. when user uses remote to go to tvOS homescreen)
  // applicationWillResignActive() will always be called before this method
  if (application.applicationState == UIApplicationStateBackground)
  {
    // the app is turn into background, not in by screen lock which has app state inactive.
    [self.xbmcController pauseAnimation];
    [self.xbmcController enterBackground];
  }
}

- (void)applicationWillTerminate:(UIApplication*)application
{
  [self.xbmcController stopAnimation];
}

#pragma mark - Startup Procedures

- (void)applicationDidBecomeActive:(UIApplication*)application
{
  // This function occurs:
  //  * on the first start of Kodi
  //  * when Kodi has been activated after being suspended by applicationWillResignActive()
  //  * when Kodi has been foregrounded after applicationDidEnterBackground()
}

- (void)applicationWillEnterForeground:(UIApplication*)application
{
  // Occurs only after an applicationDidEnterBackground()
  [self.xbmcController resumeAnimation];
  [self.xbmcController enterForeground];
}

- (BOOL)application:(UIApplication*)application
    didFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
  // check if apple removed our Cache folder first
  // this will trigger the restore if there is a backup available
  CPreflightHandler::CheckForRemovedCacheFolder();

  // This needs to run before anything does any CLog::Log calls
  // as they will directly cause guisetting to get accessed/created
  // via debug log settings.
  CPreflightHandler::MigrateUserdataXMLToNSUserDefaults();

  // UI setup
  self.window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
  self.window.rootViewController = [XBMCController new];
  [self.window makeKeyAndVisible];
  [self.xbmcController startAnimation];

  // audio session setup
  auto audioSession = AVAudioSession.sharedInstance;
  NSError* err = nil;
  if (![audioSession setCategory:AVAudioSessionCategoryPlayback error:&err])
    NSLog(@"audioSession setCategory failed: %@", err);

  err = nil;
  if (![audioSession setMode:AVAudioSessionModeMoviePlayback error:&err])
    NSLog(@"audioSession setMode failed: %@", err);

  err = nil;
  if (![audioSession setActive:YES error:&err])
    NSLog(@"audioSession setActive failed: %@", err);

  return YES;
}

- (BOOL)application:(UIApplication*)app
            openURL:(NSURL*)url
            options:(NSDictionary<NSString*, id>*)options
{
  NSArray* urlComponents = [url.absoluteString componentsSeparatedByString:@"/"];
  NSString* action = urlComponents[2];
  if ([action isEqualToString:@"display"] || [action isEqualToString:@"play"])
    CTVOSTopShelf::GetInstance().HandleTopShelfUrl(url.absoluteString.UTF8String, true);
  return YES;
}
@end

static void SigPipeHandler(int s)
{
  NSLog(@"We Got a Pipe Signal: %d____________", s);
}

int main(int argc, char* argv[])
{
  @autoreleasepool
  {
    signal(SIGPIPE, SigPipeHandler);

    int retVal = 0;
    @try
    {
      retVal =
          UIApplicationMain(argc, argv, nil, NSStringFromClass([XBMCApplicationDelegate class]));
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
