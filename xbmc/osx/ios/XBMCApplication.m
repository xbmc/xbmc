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
 
#import <UIKit/UIKit.h>

#import "XBMCApplication.h"
#import "XBMCController.h"

@implementation XBMCApplicationDelegate
XBMCController *m_xbmcController;  
UIWindow *m_window;

- (void)applicationWillResignActive:(UIApplication *)application
{
  [m_xbmcController pauseAnimation];
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
  [m_xbmcController resumeAnimation];
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
  [m_xbmcController pauseAnimation];
}

- (void)applicationWillTerminate:(UIApplication *)application
{
  [m_xbmcController pauseAnimation];
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
  [m_xbmcController resumeAnimation];
}

- (void)applicationDidFinishLaunching:(UIApplication *)application 
{
  [[UIDevice currentDevice] setBatteryMonitoringEnabled:YES];
  
  m_window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];

  /* Turn off autoresizing */
  m_window.autoresizingMask = 0;
  m_window.autoresizesSubviews = NO;

  m_xbmcController = [[XBMCController alloc] initWithFrame: [m_window bounds]];  
  m_xbmcController.wantsFullScreenLayout = YES;
  
  //m_window.rootViewController = m_xbmcController;
  
  [m_window addSubview: m_xbmcController.view];
  [m_window makeKeyAndVisible];
  
  [m_xbmcController startAnimation];
}

- (void)dealloc
{
  [m_xbmcController stopAnimation];
  [m_xbmcController release];
  [m_window release];
	
  [super dealloc];
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
    retVal = UIApplicationMain(argc,argv,@"UIApplication",@"XBMCApplicationDelegate");
    //UIApplicationMain(argc, argv, nil, nil);
  } 
  @catch (id theException) 
  {
    NSLog(@"%@", theException);
  }
  @finally 
  {
    NSLog(@"This always happens.");
  }
    
  [pool release];
	
  return retVal;

}
