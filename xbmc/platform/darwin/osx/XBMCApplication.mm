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
#include "messaging/ApplicationMessenger.h"
#include "platform/xbmc.h"
#include "utils/log.h"

#import "platform/darwin/osx/storage/OSXStorageProvider.h"

#import <AppKit/AppKit.h> /* for NSApplicationMain() */

static int gArgc;
static const char** gArgv;
static BOOL gCalledAppMainline = FALSE;

//! @TODO We should use standard XIB files for better management of the menubar.
static NSDictionary* _appMenu = @{
  @"title" : @"Kodi",
  @"items" : @[
    @{@"title" : @"About", @"sel" : @"orderFrontStandardAboutPanel:"},
    @{@"title" : @"-"},
    @{@"title" : @"Hide", @"sel" : @"hide:", @"key" : @"h"},
    @{
      @"title" : @"Hide Others",
      @"sel" : @"hideOtherApplications:",
      @"mod" : @(NSEventModifierFlagOption),
      @"key" : @"h"
    },
    @{@"title" : @"Show All", @"sel" : @"unhideAllApplications:"},
    @{@"title" : @"-"},
    @{@"title" : @"Quit", @"sel" : @"terminate:", @"key" : @"q"},
  ]
};

static NSDictionary* _windowMenu = @{
  @"title" : @"Window",
  @"items" : @[
    @{@"title" : @"Minimize", @"sel" : @"performMiniaturize:", @"key" : @"m"},
    @{@"title" : @"Zoom", @"sel" : @"performZoom:"},
    @{
      @"title" : @"Float on Top",
      @"sel" : @"floatOnTopToggle:",
      @"mod" : @(NSEventModifierFlagOption),
      @"key" : @"t"
    },
    @{@"title" : @"-"},
    @{
      @"title" : @"Toggle Full Screen",
      @"sel" : @"fullScreenToggle:",
      @"mod" : @(NSEventModifierFlagControl),
      @"key" : @"f"
    },
  ]
};

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

- (void)setupMenuBar
{
  NSMenu* menubar = [NSMenu new];
  NSMenu* windowsMenu = nil;

  for (NSDictionary* menuDict in @[ _appMenu, _windowMenu ])
  {
    auto item = [menubar addItemWithTitle:@"" action:nil keyEquivalent:@""];
    auto submenu = [self makeMenu:menuDict];
    [menubar setSubmenu:submenu forItem:item];
  }
  NSApp.mainMenu = menubar;
  NSApp.windowsMenu = windowsMenu;
}

- (NSMenu*)makeMenu:(NSDictionary*)menuDict
{
  auto menu = [[NSMenu alloc] initWithTitle:menuDict[@"title"]];
  for (NSDictionary* items in menuDict[@"items"])
  {
    NSString* title = items[@"title"];
    if ([title isEqualToString:@"-"])
    {
      [menu addItem:[NSMenuItem separatorItem]];
      continue;
    }
    auto item = [[NSMenuItem alloc] initWithTitle:title action:nil keyEquivalent:@""];
    if (NSString* sel = items[@"sel"])
      item.action = NSSelectorFromString(sel);
    if (NSString* key = items[@"key"])
      item.keyEquivalent = key;
    if (NSNumber* mask = items[@"mod"])
      item.keyEquivalentModifierMask |= [mask intValue];
    [menu addItem:item];
  }
  return menu;
}

- (void)applicationWillFinishLaunching:(NSNotification*)notification
{
  [self setupMenuBar];
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
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_TOGGLEFULLSCREEN);
}

- (void)floatOnTopToggle:(id)sender
{
  auto mainWindow = [NSApplication sharedApplication].mainWindow;
  if (!mainWindow)
  {
    return;
  }

  if (mainWindow.level == NSNormalWindowLevel)
  {
    [mainWindow setLevel:NSFloatingWindowLevel];
    [sender setState:NSControlStateValueOn];
  }
  else
  {
    [mainWindow setLevel:NSNormalWindowLevel];
    [sender setState:NSControlStateValueOff];
  }
}

- (NSMenu*)applicationDockMenu:(NSApplication*)sender
{
  return [self makeMenu:_windowMenu];
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

    return NSApplicationMain(argc, argv);
  }
}
