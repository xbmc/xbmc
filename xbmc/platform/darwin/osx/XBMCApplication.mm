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

#import <sys/param.h> /* for MAXPATHLEN */
#import <unistd.h>

// For some reason, Apple removed setAppleMenu from the headers in 10.4,
// but the method still is there and works. To avoid warnings, we declare
// it ourselves here.
@interface NSApplication (Missing_Methods)
- (void)setAppleMenu:(NSMenu*)menu;
@end

// Portions of CPS.h
typedef struct CPSProcessSerNum
{
  UInt32 lo;
  UInt32 hi;
} CPSProcessSerNum;

extern "C"
{
  extern OSErr CPSGetCurrentProcess(CPSProcessSerNum* psn);
  extern OSErr CPSEnableForegroundOperation(
      CPSProcessSerNum* psn, UInt32 _arg2, UInt32 _arg3, UInt32 _arg4, UInt32 _arg5);
  extern OSErr CPSSetFrontProcess(CPSProcessSerNum* psn);
}

static int gArgc;
static char** gArgv;
static BOOL gCalledAppMainline = FALSE;

static NSString* getApplicationName(void)
{
  NSString* appName = 0;

  // Determine the application name
  NSDictionary* dict = (NSDictionary*)CFBundleGetInfoDictionary(CFBundleGetMainBundle());
  if (dict)
    appName = [dict objectForKey:@"CFBundleName"];

  if (![appName length])
    appName = NSProcessInfo.processInfo.processName;

  return appName;
}
static void setupApplicationMenu(void)
{
  // warning: this code is very odd
  NSString* appName = getApplicationName();
  NSMenu* appleMenu = [[NSMenu alloc] initWithTitle:@""];

  // Add menu items
  NSString* title = [@"About " stringByAppendingString:appName];
  [appleMenu addItemWithTitle:title
                       action:@selector(orderFrontStandardAboutPanel:)
                keyEquivalent:@""];

  [appleMenu addItem:[NSMenuItem separatorItem]];

  title = [@"Quit " stringByAppendingString:appName];
  [appleMenu addItemWithTitle:title action:@selector(terminate:) keyEquivalent:@"q"];

  // Put menu into the menubar
  NSMenuItem* menuItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
  [menuItem setSubmenu:appleMenu];
  [NSApplication.sharedApplication.mainMenu addItem:menuItem];

  // Tell the application object that this is now the application menu
  [NSApplication.sharedApplication setAppleMenu:appleMenu];
}

// Create a window menu
static void setupWindowMenu(void)
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
  menuItem.keyEquivalentModifierMask = NSEventModifierFlagCommand | NSEventModifierFlagControl;
  [windowMenu addItem:menuItem];

  // "Minimize" item
  menuItem = [[NSMenuItem alloc] initWithTitle:@"Minimize"
                                        action:@selector(performMiniaturize:)
                                 keyEquivalent:@"m"];
  menuItem.keyEquivalentModifierMask = NSEventModifierFlagCommand | NSEventModifierFlagControl;
  [windowMenu addItem:menuItem];

  // Put menu into the menubar
  NSMenuItem* windowMenuItem = [[NSMenuItem alloc] initWithTitle:@"Window"
                                                          action:nil
                                                   keyEquivalent:@""];
  [windowMenuItem setSubmenu:windowMenu];
  [NSApplication.sharedApplication.mainMenu addItem:windowMenuItem];

  // Tell the application object that this is now the window menu
  [NSApplication.sharedApplication setWindowsMenu:windowMenu];
}

// The main class of the application, the application's delegate
@implementation XBMCDelegate

// Set the working directory to the .app's parent directory
- (void)setupWorkingDirectory
{
  char parentdir[MAXPATHLEN];
  CFURLRef url = CFBundleCopyBundleURL(CFBundleGetMainBundle());
  CFURLRef url2 = CFURLCreateCopyDeletingLastPathComponent(0, url);
  if (CFURLGetFileSystemRepresentation(url2, true, (UInt8*)parentdir, MAXPATHLEN))
  {
    assert(chdir(parentdir) == 0); /* chdir to the binary app's parent */
  }
  CFRelease(url);
  CFRelease(url2);
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
    CLog::Log(LOGDEBUG, "Failed to set core size limit (%s)", strerror(errno));
#endif

  setlocale(LC_NUMERIC, "C");

  CAppParamParser appParamParser;
  appParamParser.Parse((const char**)gArgv, (int)gArgc);

  XBMC_Run(true, appParamParser);

#ifdef _DEBUG
  CServiceBroker::GetLogging().SetLogLevel(LOG_LEVEL_DEBUG);
#else
  //  CServiceBroker::GetLogging().SetLogLevel(LOG_LEVEL_NORMAL);
  CServiceBroker::GetLogging().SetLogLevel(LOG_LEVEL_DEBUG);
#endif

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
  winSystem->SetOcclusionState(occluded);
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
  if (arg == NULL)
    return FALSE;

  newargv = (char**)realloc(gArgv, sizeof(char*) * (gArgc + 2));
  if (newargv == NULL)
  {
    free(arg);
    return FALSE;
  }
  gArgv = newargv;

  strlcpy(arg, temparg, arglen);
  gArgv[gArgc++] = arg;
  gArgv[gArgc] = NULL;

  return TRUE;
}

- (void)deviceDidMountNotification:(NSNotification*)note
{
  // calling into c++ code, need to use autorelease pools
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
  // calling into c++ code, need to use autorelease pools
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

@end

int main(int argc, char* argv[])
{
  @autoreleasepool
  {
    XBMCDelegate* xbmc_delegate;

    // Block SIGPIPE
    // SIGPIPE repeatably kills us, turn it off
    {
      sigset_t set;
      sigemptyset(&set);
      sigaddset(&set, SIGPIPE);
      sigprocmask(SIG_BLOCK, &set, NULL);
    }

    /* Copy the arguments into a global variable */
    /* This is passed if we are launched by double-clicking */
    if (argc >= 2 && strncmp(argv[1], "-psn", 4) == 0)
    {
      gArgv = (char**)malloc(sizeof(char*) * 2);
      gArgv[0] = argv[0];
      gArgv[1] = NULL;
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

    CPSProcessSerNum PSN;
    /* Tell the dock about us */
    if (!CPSGetCurrentProcess(&PSN))
      if (!CPSEnableForegroundOperation(&PSN, 0x03, 0x3C, 0x2C, 0x1103))
        if (!CPSSetFrontProcess(&PSN))
          [NSApplication sharedApplication];

    // Set up the menubars
    [NSApplication.sharedApplication setMainMenu:[[NSMenu alloc] init]];
    setupApplicationMenu();
    setupWindowMenu();

    // Create XBMCDelegate and make it the app delegate
    xbmc_delegate = [[XBMCDelegate alloc] init];
    [NSApplication.sharedApplication setDelegate:xbmc_delegate];

    // Start the main event loop
    [NSApplication.sharedApplication run];

    return 1;
  }
}
