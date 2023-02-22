/*
 *  XBMCApplication.mm - main entry point for our Cocoa-ized app
 *  Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
 *  Non-NIB-Code & other changes: Max Horn <max@quendi.de>
 *
 *  SPDX-License-Identifier: Unlicense
 *  See LICENSES/README.md for more information.
 */

#import "XBMCApplication.h"

#include "ServiceBroker.h"
#include "application/AppEnvironment.h"
#include "application/AppInboundProtocol.h"
#include "application/AppParamParser.h"
#include "platform/xbmc.h"
#include "utils/log.h"
#import "windowing/osx/WinSystemOSX.h"

#import "platform/darwin/osx/storage/OSXStorageProvider.h"

#import <AppKit/AppKit.h> /* for NSApplicationMain() */

static int gArgc;
static const char** gArgv;
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

- (void)applicationDidChangeOcclusionState:(NSNotification*)notification
{
  bool occluded = true;
  if (NSApp.occlusionState & NSApplicationOcclusionStateVisible)
    occluded = false;

  CWinSystemOSX* winSystem = dynamic_cast<CWinSystemOSX*>(CServiceBroker::GetWinSystem());
  if (winSystem)
    winSystem->SetOcclusionState(occluded); // SHH method body is commented out
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

  // kick our mainloop into a separate thread

  auto xbmcThread = [[NSThread alloc] initWithBlock:^{
    CAppParamParser appParamParser;
    appParamParser.Parse(gArgv, gArgc);
    CAppEnvironment::SetUp(appParamParser.GetAppParams());
    XBMC_Run(true);
    CAppEnvironment::TearDown();
  }];

  auto __weak notifier = [NSNotificationCenter defaultCenter];
  id __block obs = [notifier addObserverForName:NSThreadWillExitNotification
                                         object:xbmcThread
                                          queue:[NSOperationQueue mainQueue]
                                     usingBlock:^(NSNotification* note) {
                                       [NSApp replyToApplicationShouldTerminate:YES];
                                       [NSApp terminate:nil];
                                       [notifier removeObserver:obs];
                                     }];

  xbmcThread.name = @"XBMCMainThread";
  [xbmcThread start];
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender
{
  auto appPort = CServiceBroker::GetAppPort();
  if (appPort == nullptr)
    return NSTerminateNow;

  XBMC_Event quitEvent{.type = XBMC_QUIT};
  appPort->OnEvent(quitEvent);

  return NSTerminateLater;
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
  const char** newargv;

  // app has started, ignore this document.
  if (gCalledAppMainline)
    return FALSE;

  temparg = [filename UTF8String];
  arglen = strlen(temparg) + 1;
  arg = (char*)malloc(arglen);
  if (arg == nullptr)
    return FALSE;

  newargv = static_cast<const char**>(realloc(gArgv, sizeof(char*) * (gArgc + 2)));
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
    [sender setState:NSControlStateValueOff];
  }
  else
  {
    [window setLevel:NSFloatingWindowLevel];
    [sender setState:NSControlStateValueOn];
  }
}

- (NSMenu*)applicationDockMenu:(NSApplication*)sender
{
  return setupWindowMenu();
}

@end

int main(int argc, const char* argv[])
{
#if defined(_DEBUG) && 0 // SHH enable as needed
  struct rlimit rlim;
  rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
  if (setrlimit(RLIMIT_CORE, &rlim) == -1)
    CLog::Log(LOGDEBUG, "Failed to set core size limit ({})", strerror(errno));
#endif

  @autoreleasepool
  {
    /* Copy the arguments into a global variable */
    /* This is passed if we are launched by double-clicking */
    if (argc >= 2 && strncmp(argv[1], "-psn", 4) == 0)
    {
      gArgv = static_cast<const char**>(malloc(sizeof(char*) * 2));
      gArgv[0] = argv[0];
      gArgv[1] = nullptr;
      gArgc = 1;
    }
    else
    {
      gArgc = argc;
      gArgv = static_cast<const char**>(malloc(sizeof(char*) * (argc + 1)));
      for (int i = 0; i <= argc; i++)
        gArgv[i] = argv[i];
    }

    // Make sure we call applicationWillTerminate on SIGTERM
    [NSProcessInfo.processInfo disableSuddenTermination];

    auto appDelegate = [XBMCDelegate new];
    [NSApplication sharedApplication].delegate = appDelegate;

#if defined(_DEBUG)
    // HACK: for unbundled apps (e.g. executing kodi.bin directly from xcode) the
    // default policy is NSApplicationActivationPolicyProhibited (no dock and may not create windows)
    // since Kodi does not currently support any headless mode force it to regular (the default for bundled)
    // TODO: If headless is supported in the future or if setting NSApplicationActivationPolicyProhibited is
    // intentional (e.g. from Info.Plist) this might need to be revisited later.
    if ([NSApp activationPolicy] == NSApplicationActivationPolicyProhibited)
    {
      [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    }
#endif
    return NSApplicationMain(argc, argv);
  }
}
