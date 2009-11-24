/*   SDLMain.m - main entry point for our Cocoa-ized SDL app
       Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
       Non-NIB-Code & other changes: Max Horn <max@quendi.de>

    Feel free to customize this file to suit your needs
*/
/*
  SDLMain.m and SDLMain.h are licensed under the terms of GNU Lesser General
  Public License, version 2.
*/

#import "SDL/SDL.h"
#import "SDLMain.h"
#import <sys/param.h> /* for MAXPATHLEN */
#import <unistd.h>

#import "CocoaInterface.h"
//hack around problem with xbmc's typedef int BOOL
// and obj-c's typedef unsigned char BOOL
#define BOOL XBMC_BOOL 
#import "StdString.h"
#import "PlatformDefs.h"
#import "ApplicationMessenger.h"
#import "DarwinStorageProvider.h"
#import "WindowingFactory.h"
#undef BOOL

/* For some reaon, Apple removed setAppleMenu from the headers in 10.4,
 but the method still is there and works. To avoid warnings, we declare
 it ourselves here. */
@interface NSApplication(SDL_Missing_Methods)
- (void)setAppleMenu:(NSMenu *)menu;
@end

/* Use this flag to determine whether we use CPS (docking) or not */
#define		SDL_USE_CPS		1
#ifdef SDL_USE_CPS
/* Portions of CPS.h */
typedef struct CPSProcessSerNum
{
	UInt32		lo;
	UInt32		hi;
} CPSProcessSerNum;

extern "C" {
extern OSErr	CPSGetCurrentProcess(CPSProcessSerNum *psn);
extern OSErr 	CPSEnableForegroundOperation(CPSProcessSerNum *psn, UInt32 _arg2, UInt32 _arg3, UInt32 _arg4, UInt32 _arg5);
extern OSErr	CPSSetFrontProcess(CPSProcessSerNum *psn);
}
#endif /* SDL_USE_CPS */

static int    gArgc;
static char  **gArgv;
static BOOL   gFinderLaunch;
static BOOL   gCalledAppMainline = FALSE;

static NSString *getApplicationName(void)
{
    NSDictionary *dict;
    NSString *appName = 0;

    /* Determine the application name */
    dict = (NSDictionary *)CFBundleGetInfoDictionary(CFBundleGetMainBundle());
    if (dict)
        appName = [dict objectForKey: @"CFBundleName"];
    
    if (![appName length])
        appName = [[NSProcessInfo processInfo] processName];

    return appName;
}
static void setApplicationMenu(void)
{
    /* warning: this code is very odd */
    NSMenu *appleMenu;
    NSMenuItem *menuItem;
    NSString *title;
    NSString *appName;
    
    appName = getApplicationName();
    appleMenu = [[NSMenu alloc] initWithTitle:@""];
    
    /* Add menu items */
    title = [@"About " stringByAppendingString:appName];
    [appleMenu addItemWithTitle:title action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];

    [appleMenu addItem:[NSMenuItem separatorItem]];

    title = [@"Hide " stringByAppendingString:appName];
    [appleMenu addItemWithTitle:title action:@selector(hide:) keyEquivalent:@"h"];

    menuItem = (NSMenuItem *)[appleMenu addItemWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h"];
    [menuItem setKeyEquivalentModifierMask:(NSAlternateKeyMask|NSCommandKeyMask)];

    [appleMenu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];

    [appleMenu addItem:[NSMenuItem separatorItem]];

    title = [@"Quit " stringByAppendingString:appName];
    [appleMenu addItemWithTitle:title action:@selector(terminate:) keyEquivalent:@"q"];

    
    /* Put menu into the menubar */
    menuItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    [menuItem setSubmenu:appleMenu];
    [[NSApp mainMenu] addItem:menuItem];

    /* Tell the application object that this is now the application menu */
    [NSApp setAppleMenu:appleMenu];

    /* Finally give up our references to the objects */
    [appleMenu release];
    [menuItem release];
}

/* Create a window menu */
static void setupWindowMenu(void)
{
    NSMenu      *windowMenu;
    NSMenuItem  *windowMenuItem;
    NSMenuItem  *menuItem;

    windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];
    
    /* "Full/Windowed Toggle" item */
    menuItem = [[NSMenuItem alloc] initWithTitle:@"Full/Windowed Toggle" action:@selector(fullScreenToggle:) keyEquivalent:@"f"];
    [windowMenu addItem:menuItem];
    [menuItem release];

    /* "Full/Windowed Toggle" item */
    menuItem = [[NSMenuItem alloc] initWithTitle:@"Float on Top" action:@selector(floatOnTopToggle:) keyEquivalent:@"t"];
    [windowMenu addItem:menuItem];
    [menuItem release];

    /* "Minimize" item */
    menuItem = [[NSMenuItem alloc] initWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@"m"];
    [windowMenu addItem:menuItem];
    [menuItem release];
    
    /* Put menu into the menubar */
    windowMenuItem = [[NSMenuItem alloc] initWithTitle:@"Window" action:nil keyEquivalent:@""];
    [windowMenuItem setSubmenu:windowMenu];
    [[NSApp mainMenu] addItem:windowMenuItem];
    
    /* Tell the application object that this is now the window menu */
    [NSApp setWindowsMenu:windowMenu];

    /* Finally give up our references to the objects */
    [windowMenu release];
    [windowMenuItem release];
}

@interface SDLApplication : NSApplication
@end

@implementation SDLApplication

// Called before the internal event loop has started running.
- (void) finishLaunching
{
  [super finishLaunching];
}

// Invoked from the Quit menu item
- (void)terminate:(id)sender
{
    // remove any notification handlers
    [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:self];
    [[NSNotificationCenter defaultCenter] removeObserver:self];

    /* Post a SDL_QUIT event */
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
}

- (void)fullScreenToggle:(id)sender
{
  // Post an toggle full-screen event to the application thread.
  SDL_Event event;
  memset(&event, 0, sizeof(event));
  event.type = SDL_USEREVENT;
  event.user.code = TMSG_TOGGLEFULLSCREEN;
  SDL_PushEvent(&event);
}

- (void)floatOnTopToggle:(id)sender
{
  NSWindow* window = [[[NSOpenGLContext currentContext] view] window];
  if ([window level] == NSFloatingWindowLevel)
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

// The main class of the application, the application's delegate
@implementation SDLMain

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
- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename
{
    const char *temparg;
    size_t arglen;
    char *arg;
    char **newargv;

    if (!gFinderLaunch)  /* MacOS is passing command line args. */
        return FALSE;

    if (gCalledAppMainline)  /* app has started, ignore this document. */
        return FALSE;

    temparg = [filename UTF8String];
    arglen = SDL_strlen(temparg) + 1;
    arg = (char *) SDL_malloc(arglen);
    if (arg == NULL)
        return FALSE;

    newargv = (char **) realloc(gArgv, sizeof (char *) * (gArgc + 2));
    if (newargv == NULL)
    {
        SDL_free(arg);
        return FALSE;
    }
    gArgv = newargv;

    SDL_strlcpy(arg, temparg, arglen);
    gArgv[gArgc++] = arg;
    gArgv[gArgc] = NULL;
    return TRUE;
}

/* Set the working directory to the .app's parent directory */
- (void) setupWorkingDirectory:(BOOL)shouldChdir
{
    if (shouldChdir)
    {
        char parentdir[MAXPATHLEN];
        CFURLRef url = CFBundleCopyBundleURL(CFBundleGetMainBundle());
        CFURLRef url2 = CFURLCreateCopyDeletingLastPathComponent(0, url);
        if (CFURLGetFileSystemRepresentation(url2, true, (UInt8 *)parentdir, MAXPATHLEN)) {
            assert( chdir (parentdir) == 0 );   /* chdir to the binary app's parent */
		}
		CFRelease(url);
		CFRelease(url2);
	}

}

// Called after the internal event loop has started running.
- (void) applicationDidFinishLaunching: (NSNotification *) note
{
    /* Set the working directory to the .app's parent directory */
    [self setupWorkingDirectory:gFinderLaunch];

    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
      selector:@selector(workspaceDidWake:)
      name:NSWorkspaceDidWakeNotification
      object:nil];
      
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
      selector:@selector(workspaceWillSleep:)
      name:NSWorkspaceWillSleepNotification
      object:nil];
      
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
      selector:@selector(deviceDidMount:)
      name:NSWorkspaceDidMountNotification
      object:nil];

    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
      selector:@selector(deviceDidUnMount:)
      name:NSWorkspaceDidUnmountNotification
      object:nil];
		
    [[NSNotificationCenter defaultCenter] addObserver:self
      selector:@selector(windowDidMove:)
      name:NSWindowDidMoveNotification
      object:nil];

    [[NSNotificationCenter defaultCenter] addObserver:self
      selector:@selector(windowDidReSize:)
      name:NSWindowDidResizeNotification
      object:nil];
      
    // We're going to manually manage the screensaver.
    setenv("SDL_VIDEO_ALLOW_SCREENSAVER", "1", true);

    /* Hand off to main application code */
    gCalledAppMainline = TRUE;
    int status = SDL_main(gArgc, gArgv);

    /* We're done, thank you for playing */
    exit(status);
}

- (void) applicationWillTerminate: (NSNotification *) note
{
}

- (void) applicationWillResignActive:(NSNotification *) note
{
}

- (void) applicationWillBecomeActive:(NSNotification *) note
{
}

- (void) workspaceDidWake:(NSNotification *) note
{
  if (g_Windowing.IsFullScreen())
  {
    // Find which display we are on
    CGDirectDisplayID display_id = kCGDirectMainDisplay;
  
    NSOpenGLContext* context = [NSOpenGLContext currentContext];
    if (context)
    {
      NSView* view;

      view = [context view];
      if (view)
      {
        NSWindow* window;
        window = [view window];
        if (window)
        {
          NSDictionary* screenInfo = [[window screen] deviceDescription];
          NSNumber* screenID = [screenInfo objectForKey:@"NSScreenNumber"];
          if (kCGDirectMainDisplay == (CGDirectDisplayID)[screenID longValue])
          {
            CStdString tmp_str;

            // keep the dock hidden using applescriptif on main screen with the dock.
            tmp_str = "tell application \"System Events\" \n";
            tmp_str += "keystroke \"d\" using {command down, option down} \n";
            tmp_str += "end tell \n";
            
            Cocoa_DoAppleScript( tmp_str.c_str() );
          }
        }
      }
    }
  }
}

- (void) workspaceWillSleep:(NSNotification *) note
{
}

- (void) deviceDidMount:(NSNotification *) note 
{
  // calling into c++ code, need to use autorelease pools
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

  CDarwinStorageProvider::SetEvent();
  [pool release];
}

- (void) deviceDidUnMount:(NSNotification *) note 
{
  // calling into c++ code, need to use autorelease pools
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

  CDarwinStorageProvider::SetEvent();
  [pool release];
}

- (void) windowDidMove:(NSNotification*) note
{
  Cocoa_CVDisplayLinkUpdate();
}

- (void) windowDidReSize:(NSNotification*) note
{
  /* 
  // SDL_PushEvent is not working but
  // this is how one would pass update events in response to grow events.
  NSOpenGLContext* context = [NSOpenGLContext currentContext];
  if (context)
  {
    NSView *view = [context view];
    if (view)
    {
      XBMC_Event event;
      memset(&event, 0, sizeof(event));
      event.resize.type = XBMC_VIDEORESIZE;
      event.resize.w = [view frame].size.width;
      event.resize.h = [view frame].size.height;
      SDL_PushEvent(&event);
    }
  }
  */
}

@end

#ifdef main
#  undef main
#endif
/* Main entry point to executable - should *not* be SDL_main! */
int main(int argc, char *argv[])
{
    NSAutoreleasePool	*pool = [[NSAutoreleasePool alloc] init];
    SDLMain				*sdlMain;

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
    if ( argc >= 2 && strncmp (argv[1], "-psn", 4) == 0 ) {
        gArgv = (char **) SDL_malloc(sizeof (char *) * 2);
        gArgv[0] = argv[0];
        gArgv[1] = NULL;
        gArgc = 1;
        gFinderLaunch = YES;
    } else {
        int i;
        gArgc = argc;
        gArgv = (char **) SDL_malloc(sizeof (char *) * (argc+1));
        for (i = 0; i <= argc; i++)
            gArgv[i] = argv[i];
        gFinderLaunch = NO;
    }

    /* Ensure the application object is initialised */
    [SDLApplication sharedApplication];
    
#ifdef SDL_USE_CPS
    {
        CPSProcessSerNum PSN;
        /* Tell the dock about us */
        if (!CPSGetCurrentProcess(&PSN))
            if (!CPSEnableForegroundOperation(&PSN,0x03,0x3C,0x2C,0x1103))
                if (!CPSSetFrontProcess(&PSN))
                    [SDLApplication sharedApplication];
    }
#endif /* SDL_USE_CPS */

    /* Set up the menubar */
    [NSApp setMainMenu:[[NSMenu alloc] init]];
    setApplicationMenu();
    setupWindowMenu();

    /* Create SDLMain and make it the app delegate */
    sdlMain = [[SDLMain alloc] init];
    [NSApp setDelegate:sdlMain];
    
    /* Start the main event loop */
    [NSApp run];
    
    [sdlMain release];
    [pool release];
    return 0;
}

