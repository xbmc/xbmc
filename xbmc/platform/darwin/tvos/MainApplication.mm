/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "platform/darwin/tvos/MainApplication.h"

#import "platform/darwin/NSLogDebugHelpers.h"
#import "platform/darwin/tvos/MainController.h"
#import "platform/darwin/tvos/TVOSTopShelf.h"

#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>
#import <objc/runtime.h>

@implementation MainApplicationDelegate
MainController* m_xbmcController;

- (void)applicationWillResignActive:(UIApplication*)application
{
//  PRINT_SIGNATURE();

  [m_xbmcController pauseAnimation];
  [m_xbmcController becomeInactive];
}

- (void)applicationDidBecomeActive:(UIApplication*)application
{
  //PRINT_SIGNATURE();

  [m_xbmcController resumeAnimation];
  [m_xbmcController enterForeground];
}

- (void)applicationDidEnterBackground:(UIApplication*)application
{
//  PRINT_SIGNATURE();

  if (application.applicationState == UIApplicationStateBackground)
  {
    // the app is turn into background, not in by screen lock which has app state inactive.
    [m_xbmcController enterBackground];
  }
}

- (void)applicationWillTerminate:(UIApplication*)application
{
//  PRINT_SIGNATURE();

  [m_xbmcController stopAnimation];
}

- (void)applicationWillEnterForeground:(UIApplication*)application
{
//  PRINT_SIGNATURE();
}

- (void)applicationDidFinishLaunching:(UIApplication*)application
{
  //PRINT_SIGNATURE();
  NSError* err = nullptr;
  if (![[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback error:&err])
  {
    NSLog(@"AVAudioSession setCategory failed: %ld", static_cast<long>(err.code));
  }
  err = nil;
  if (![[AVAudioSession sharedInstance] setMode:AVAudioSessionModeMoviePlayback error:&err])
  {
    NSLog(@"AVAudioSession setMode failed: %ld", static_cast<long>(err.code));
  }
  err = nil;
  if (![[AVAudioSession sharedInstance] setActive: YES error: &err])
  {
    NSLog(@"AVAudioSession setActive YES failed: %ld", static_cast<long>(err.code));
  }



  UIScreen* currentScreen = [UIScreen mainScreen];
  m_xbmcController = [[MainController alloc] initWithFrame: [currentScreen bounds] withScreen:currentScreen];
  [m_xbmcController startAnimation];
}

- (BOOL)application:(UIApplication*)app
  openURL:(NSURL*)url options:(NSDictionary<NSString*, id>*)options
{
  NSArray* urlComponents = [[url absoluteString] componentsSeparatedByString:@"/"];
  NSString* action = urlComponents[2];
  if ([action isEqualToString:@"display"] || [action isEqualToString:@"play"])
  {
    std::string cleanURL = *new std::string([[url absoluteString] UTF8String]);
    CTVOSTopShelf::GetInstance().HandleTopShelfUrl(cleanURL,true);
  }
  return YES;
}

- (void)dealloc
{
  [m_xbmcController stopAnimation];
  [m_xbmcController release];

  [super dealloc];
}
@end

static void SigPipeHandler(int s)
{
  NSLog(@"We Got a Pipe Signal: %d____________", s);
}

int main(int argc, char* argv[])
{
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

  signal(SIGPIPE, SigPipeHandler);

  int retVal = 0;
  @try
  {
    retVal = UIApplicationMain(argc, argv, nil, NSStringFromClass([MainApplicationDelegate class]));
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
