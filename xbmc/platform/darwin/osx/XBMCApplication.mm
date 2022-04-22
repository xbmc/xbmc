/*
 *  SDLMain.mm - main entry point for our Cocoa-ized SDL app
 *  Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
 *  Non-NIB-Code & other changes: Max Horn <max@quendi.de>
 *
 *  SPDX-License-Identifier: Unlicense
 *  See LICENSES/README.md for more information.
 */

#import "XBMCApplication.h"

#include "AppInboundProtocol.h"
#include "AppParamParser.h"
#include "ServiceBroker.h"
#include "messaging/ApplicationMessenger.h"
#include "platform/xbmc.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"
#import "windowing/osx/WinSystemOSX.h"

#import "platform/darwin/osx/storage/OSXStorageProvider.h"

#import <Foundation/Foundation.h>
#import <sys/param.h> /* for MAXPATHLEN */
#import <unistd.h>

static int gArgc;
static char** gArgv;
static BOOL gCalledAppMainline = FALSE;

// Create a window menu
static NSMenu* setupWindowMenu()
{
  NSMenu* windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];

  // "Full/Windowed Toggle" item
  NSMenuItem* menuItem = [[NSMenuItem alloc] initWithTitle:@"Full/Windowed Toggle"
                                                    action:@selector(fullScreenToggle:)
                                             keyEquivalent:@"f"];
  // this is just for display purposes, key handling is in CWinEventsOSX::ProcessOSXShortcuts()
  menuItem.keyEquivalentModifierMask = NSEventModifierFlagCommand | NSEventModifierFlagControl;
  [windowMenu addItem:menuItem];

  // "Full/Windowed Toggle" item
  menuItem = [[NSMenuItem alloc] initWithTitle:@"Float on Top"
                                        action:@selector(floatOnTopToggle:)
                                 keyEquivalent:@"t"];
  menuItem.keyEquivalentModifierMask = NSEventModifierFlagCommand;
  [windowMenu addItem:menuItem];

  // "Minimize" item
  menuItem = [[NSMenuItem alloc] initWithTitle:@"Minimize"
                                        action:@selector(performMiniaturize:)
                                 keyEquivalent:@"m"];
  menuItem.keyEquivalentModifierMask = NSEventModifierFlagCommand;
  [windowMenu addItem:menuItem];

  return windowMenu;
}

// The main class of the application, the application's delegate
@implementation XBMCDelegate

// Set the working directory to the .app's parent directory
//! @todo Whats this for, is it required?
- (void)setupWorkingDirectory
{
  auto parentPath = NSBundle.mainBundle.bundlePath.stringByDeletingLastPathComponent;
  NSAssert([NSFileManager.defaultManager changeCurrentDirectoryPath:parentPath],
           @"SetupWorkingDirectory Failed to cwd");
}

- (void)stopRunLoop
{
  // to get applicationShouldTerminate and
  // applicationWillTerminate notifications.
  [NSApplication.sharedApplication terminate:nil];
  // to flag a stop on next event.
  [NSApplication.sharedApplication stop:nil];

  //post a NOP event, so the run loop actually stops
  //see http://www.cocoabuilder.com/archive/cocoa/219842-nsapp-stop.html
  NSEvent* event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                      location:NSMakePoint(0, 0)
                                 modifierFlags:0
                                     timestamp:0.0
                                  windowNumber:0
                                       context:nil
                                       subtype:0
                                         data1:0
                                         data2:0];

  [NSApplication.sharedApplication postEvent:event atStart:true];
}

// To use Cocoa on secondary POSIX threads, your application must first detach
// at least one NSThread object, which can immediately exit. Some info says this
// is not required anymore, who knows ?
- (void)kickstartMultiThreaded:(id)arg
{
  @autoreleasepool
  {
    // empty
  }
}

- (void)mainLoopThread:(id)arg
{

#if defined(DEBUG)
  struct rlimit rlim;
  rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
  if (setrlimit(RLIMIT_CORE, &rlim) == -1)
    CLog::Log(LOGDEBUG, "Failed to set core size limit ({})", strerror(errno));
#endif

  setlocale(LC_NUMERIC, "C");

  CAppParamParser appParamParser;
  appParamParser.Parse((const char**)gArgv, (int)gArgc);

  XBMC_Run(true, appParamParser.GetAppParams());

  std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
  if (appPort)
    appPort->SetRenderGUI(false);

  [self performSelectorOnMainThread:@selector(stopRunLoop) withObject:nil waitUntilDone:false];
}

- (void)applicationDidChangeOcclusionState:(NSNotification*)notification
{
  bool occluded = true;
  if (NSApp.occlusionState & NSApplicationOcclusionStateVisible)
    occluded = false;

  CWinSystemOSX* winSystem = dynamic_cast<CWinSystemOSX*>(CServiceBroker::GetWinSystem());
  if (winSystem)
    winSystem->SetOcclusionState(occluded);
}

- (void)applicationWillFinishLaunching:(NSNotification*)notification
{
  NSMenu* menubar = [NSMenu new];
  NSMenuItem* menuBarItem = [NSMenuItem new];
  [menubar addItem:menuBarItem];
  [NSApp setMainMenu:menubar];

  // Main menu
  NSMenu* appMenu = [NSMenu new];
  NSMenuItem* quitMenuItem = [[NSMenuItem alloc] initWithTitle:@"Quit"
                                                        action:@selector(terminate:)
                                                 keyEquivalent:@"q"];
  [appMenu addItem:quitMenuItem];
  [menuBarItem setSubmenu:appMenu];

  // Window Menu
  NSMenuItem* windowMenuItem = [menubar addItemWithTitle:@"" action:nil keyEquivalent:@""];
  NSMenu* windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];
  [menubar setSubmenu:windowMenu forItem:windowMenuItem];
  NSMenuItem* fullscreenMenuItem = [[NSMenuItem alloc] initWithTitle:@"Full/Windowed Toggle"
                                                              action:@selector(fullScreenToggle:)
                                                       keyEquivalent:@"f"];
  fullscreenMenuItem.keyEquivalentModifierMask =
      NSEventModifierFlagCommand | NSEventModifierFlagControl;
  [windowMenu addItem:fullscreenMenuItem];
  [windowMenu addItemWithTitle:@"Float on Top"
                        action:@selector(floatOnTopToggle:)
                 keyEquivalent:@"t"];
  [windowMenu addItemWithTitle:@"Minimize"
                        action:@selector(performMiniaturize:)
                 keyEquivalent:@"m"];
}

// Called after the internal event loop has started running.
- (void)applicationDidFinishLaunching:(NSNotification*)note
{
  // enable multithreading, we should NOT have to do this but as we are mixing NSThreads/pthreads...
  if (!NSThread.isMultiThreaded)
    [NSThread detachNewThreadSelector:@selector(kickstartMultiThreaded:)
                             toTarget:self
                           withObject:nil];

  // Set the working directory to the .app's parent directory
  [self setupWorkingDirectory];

  [NSWorkspace.sharedWorkspace.notificationCenter addObserver:self
                                                     selector:@selector(deviceDidMountNotification:)
                                                         name:NSWorkspaceDidMountNotification
                                                       object:nil];

  [NSWorkspace.sharedWorkspace.notificationCenter
      addObserver:self
         selector:@selector(deviceDidUnMountNotification:)
             name:NSWorkspaceDidUnmountNotification
           object:nil];

  // Hand off to main application code
  gCalledAppMainline = TRUE;

  //window.acceptsMouseMovedEvents = TRUE;
  [NSApp activateIgnoringOtherApps:YES];
  // kick our mainloop into an extra thread
  [NSThread detachNewThreadSelector:@selector(mainLoopThread:) toTarget:self withObject:nil];
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender
{
  return NSTerminateNow;
}

- (void)applicationWillTerminate:(NSNotification*)note
{
  [NSWorkspace.sharedWorkspace.notificationCenter removeObserver:self
                                                            name:NSWorkspaceDidMountNotification
                                                          object:nil];

  [NSWorkspace.sharedWorkspace.notificationCenter removeObserver:self
                                                            name:NSWorkspaceDidUnmountNotification
                                                          object:nil];
}

- (void)applicationWillResignActive:(NSNotification*)note
{
  // when app moves to background
}

- (void)applicationWillBecomeActive:(NSNotification*)note
{
  // when app moves to front
}

/*
 * Catch document open requests...this lets us notice files when the app
 *  was launched by double-clicking a document, or when a document was
 *  dragged/dropped on the app's icon. You need to have a
 *  CFBundleDocumentsType section in your Info.plist to get this message,
 *  apparently.
 *
 * Files are added to gArgv, so to the app, they'll look like command line
 *  arguments. Previously, apps launched from the finder had nothing but
 *  an argv[0].
 *
 * This message may be received multiple times to open several docs on launch.
 *
 * This message is ignored once the app's mainline has been called.
 */

//! @Todo Transition to application: openURLs:
- (BOOL)application:(NSApplication*)theApplication openFile:(NSString*)filename
{
  const char* temparg;
  size_t arglen;
  char* arg;
  char** newargv;

  // app has started, ignore this document.
  if (gCalledAppMainline)
    return FALSE;

  temparg = [filename UTF8String];
  arglen = strlen(temparg) + 1;
  arg = (char*)malloc(arglen);
  if (arg == nullptr)
    return FALSE;

  newargv = (char**)realloc(gArgv, sizeof(char*) * (gArgc + 2));
  if (newargv == nullptr)
  {
    free(arg);
    return FALSE;
  }
  gArgv = newargv;

  strlcpy(arg, temparg, arglen);
  gArgv[gArgc++] = arg;
  gArgv[gArgc] = nullptr;

  return TRUE;
}

- (void)deviceDidMountNotification:(NSNotification*)note
{
  @autoreleasepool
  {
    NSString* volumeLabel = [note.userInfo objectForKey:@"NSWorkspaceVolumeLocalizedNameKey"];
    const char* label = volumeLabel.UTF8String;

    NSString* volumePath = [note.userInfo objectForKey:@"NSDevicePath"];
    const char* path = volumePath.UTF8String;

    COSXStorageProvider::VolumeMountNotification(label, path);
  }
}

- (void)deviceDidUnMountNotification:(NSNotification*)note
{
  @autoreleasepool
  {
    NSString* volumeLabel = [note.userInfo objectForKey:@"NSWorkspaceVolumeLocalizedNameKey"];
    const char* label = [volumeLabel UTF8String];

    NSString* volumePath = [note.userInfo objectForKey:@"NSDevicePath"];
    const char* path = [volumePath UTF8String];

    COSXStorageProvider::VolumeUnmountNotification(label, path);
  }
}

// Invoked from the Quit menu item
- (void)terminate:(id)sender
{
  // remove any notification handlers
  [NSWorkspace.sharedWorkspace.notificationCenter removeObserver:self];
  [NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)fullScreenToggle:(id)sender
{
}

- (void)floatOnTopToggle:(id)sender
{
  // ToDo!: non functional, test further
  NSWindow* window = NSOpenGLContext.currentContext.view.window;
  if (window.level == NSFloatingWindowLevel)
  {
    [window setLevel:NSNormalWindowLevel];
    [sender setState:NSOffState];
  }
  else
  {
    [window setLevel:NSFloatingWindowLevel];
    [sender setState:NSOnState];
  }
}

- (NSMenu*)applicationDockMenu:(NSApplication*)sender
{
  return setupWindowMenu();
}

@end

int main(int argc, char* argv[])
{
  @autoreleasepool
  {
    /* Copy the arguments into a global variable */
    /* This is passed if we are launched by double-clicking */
    if (argc >= 2 && strncmp(argv[1], "-psn", 4) == 0)
    {
      gArgv = (char**)malloc(sizeof(char*) * 2);
      gArgv[0] = argv[0];
      gArgv[1] = nullptr;
      gArgc = 1;
    }
    else
    {
      gArgc = argc;
      gArgv = (char**)malloc(sizeof(char*) * (argc + 1));
      for (int i = 0; i <= argc; i++)
        gArgv[i] = argv[i];
    }

    // Ensure the application object is initialised
    [NSApplication sharedApplication];

    // Make sure we call applicationWillTerminate on SIGTERM
    [NSProcessInfo.processInfo disableSuddenTermination];

    // Set App Delegate
    auto appDelegate = [XBMCDelegate new];
    NSApp.delegate = appDelegate;

    // Set NSApp to show in dock
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    // Start the main event loop
    [NSApp run];

    return 1;
  }
}
