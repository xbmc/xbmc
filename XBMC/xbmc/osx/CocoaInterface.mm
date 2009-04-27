/*
 *      Copyright (C) 2005-2009 Team XBMC
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
#import <unistd.h>
#import <sys/mount.h>

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>

#import "CocoaInterface.h"

// hack for Cocoa_GL_ResizeWindow
extern "C" void SDL_SetWidthHeight(int w, int h);

#define MAX_DISPLAYS 32
static NSWindow* blankingWindows[MAX_DISPLAYS];

// Display Blanking
void Cocoa_GL_BlankOtherDisplays(int screen_index);
void Cocoa_GL_UnblankDisplays(void);

void* Cocoa_Create_AutoReleasePool()
{
  // Original Author: Elan Feingold
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
  return pool;
}

void Cocoa_Destroy_AutoReleasePool(void* aPool)
{
  // Original Author: Elan Feingold
  NSAutoreleasePool* pool = (NSAutoreleasePool* )aPool;
  [pool release];
}

void Cocoa_GL_MakeCurrentContext(void* theContext)
{
  // Original Author: Elan Feingold
  NSOpenGLContext* context = (NSOpenGLContext* )theContext;
  [ context makeCurrentContext ];
}

void Cocoa_GL_ReleaseContext(void* theContext)
{
  // Original Author: Elan Feingold
  NSOpenGLContext* context = (NSOpenGLContext* )theContext;
  [ NSOpenGLContext clearCurrentContext ];
  [ context clearDrawable ];
  [ context release ];
}

void Cocoa_GL_SwapBuffers(void* theContext)
{
  // Original Author: Elan Feingold
  [ (NSOpenGLContext*)theContext flushBuffer ];
}

int Cocoa_GetNumDisplays()
{
  return [[NSScreen screens] count];
}

int Cocoa_GetDisplayID(int screen_index)
{
  CGDirectDisplayID displayArray[MAX_DISPLAYS];
  CGDisplayCount    numDisplays;

  // Get the list of displays.
  CGGetActiveDisplayList(MAX_DISPLAYS, displayArray, &numDisplays);
  return( (int)displayArray[screen_index]);
}

CGDirectDisplayID Cocoa_GetDisplayIDFromScreen(NSScreen *screen)
{
  NSDictionary* screenInfo = [screen deviceDescription];
  NSNumber* screenID = [screenInfo objectForKey:@"NSScreenNumber"];
  
  return (CGDirectDisplayID)[screenID longValue];
}

int Cocoa_GetDisplayIndex(CGDirectDisplayID display)
{
  CGDirectDisplayID displayArray[MAX_DISPLAYS];
  CGDisplayCount    numDisplays;

  // Get the list of displays.
  CGGetActiveDisplayList(MAX_DISPLAYS, displayArray, &numDisplays);
  while (numDisplays > 0)
  {
    if (display == displayArray[--numDisplays])
	  return numDisplays;
  }
  return -1;
}

void Cocoa_GetScreenResolutionOfAnotherScreen(int screen, int* w, int* h)
{
  // Original Author: Elan Feingold
  CFDictionaryRef mode = CGDisplayCurrentMode( (CGDirectDisplayID)Cocoa_GetDisplayID(screen));
  CFNumberGetValue( (CFNumberRef)CFDictionaryGetValue(mode, kCGDisplayWidth), kCFNumberSInt32Type, w);
  CFNumberGetValue( (CFNumberRef)CFDictionaryGetValue(mode, kCGDisplayHeight), kCFNumberSInt32Type, h);
}

int Cocoa_GetScreenIndex(void)
{
  // return one based screen index
  int screen_index = 0;
  int numDisplays = [[NSScreen screens] count];
  NSScreen* current_Screen = nil;
  
  if (numDisplays > 1)
  {
    NSOpenGLContext* context = (NSOpenGLContext*)Cocoa_GL_GetCurrentContext();
    if (context)
    {
      NSView* view;
    
      view = [context view];
      if (view) {
        NSWindow* window;
        window = [view window];
        if (window)
        {
          // Get the screen we are using for display.
          current_Screen = [window screen];
        }
      }
    }
    
    for (screen_index = 0; screen_index < numDisplays; screen_index++)
    {
      if (current_Screen == [[NSScreen screens] objectAtIndex:screen_index])
      {
        break;
      }
    }
  }
  screen_index++;
  
  return(screen_index);
}

void Cocoa_GetScreenResolution(int* w, int* h)
{
  // Figure out the screen size. (default to main screen)
  CGDirectDisplayID display_id = kCGDirectMainDisplay;
  CFDictionaryRef mode  = CGDisplayCurrentMode(display_id);
  
  NSOpenGLContext* context = (NSOpenGLContext*)Cocoa_GL_GetCurrentContext();
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
        display_id = Cocoa_GetDisplayIDFromScreen( [window screen] );      
        mode  = CGDisplayCurrentMode(display_id);
      }
    }
  }
  
  CFNumberGetValue( (CFNumberRef)CFDictionaryGetValue(mode, kCGDisplayWidth), kCFNumberSInt32Type, w);
  CFNumberGetValue( (CFNumberRef)CFDictionaryGetValue(mode, kCGDisplayHeight), kCFNumberSInt32Type, h);
}

double Cocoa_GetScreenRefreshRate(int screen_index)
{
  // NOTE: The refresh rate will be REPORTED AS 0 for many DVI and notebook displays.
  CFDictionaryRef mode;
  double fps = 60.0;
  
  mode = CGDisplayCurrentMode((CGDirectDisplayID)Cocoa_GetDisplayID(screen_index));
  if (mode)
  {
    CFNumberGetValue( (CFNumberRef)CFDictionaryGetValue(mode, kCGDisplayRefreshRate), kCFNumberDoubleType, &fps);
    //fps = getDictDouble(mode, kCGDisplayRefreshRate);
    if (fps <= 0.0)
    {
      fps = 60.0;
    }
  }
  
  return(fps);
 }

void* Cocoa_GL_ResizeWindow(void *theContext, int w, int h)
{
  if (!theContext)
    return 0;
  
  NSOpenGLContext* context = (NSOpenGLContext*)Cocoa_GL_GetCurrentContext();
  NSView* view;
  NSWindow* window;
  
  view = [context view];
  if (view && w>0 && h>0)
  {
    window = [view window];
    if (window)
    {
      [window setContentSize:NSMakeSize(w,h)];
      [window update];
      [view setFrameSize:NSMakeSize(w, h)];
      [context update];
    }
  }

  // HACK: resize SDL's view manually so that mouse bounds are correctly updated.
  // there are two parts to this, the internal SDL (current_video->screen) and
  // the cocoa view ( handled in Cocoa_GL_SetFullScreen).
  SDL_SetWidthHeight(w, h);

  return context;
}

void Cocoa_GL_BlankOtherDisplays(int screen_index)
{
  int i;
  int numDisplays = [[NSScreen screens] count];
  
  // zero out blankingWindows for debugging
  for (i=0; i<MAX_DISPLAYS; i++)
  {
    blankingWindows[i] = 0;
  }
  
  // Blank.
  for (i=0; i<numDisplays; i++)
  {
    if (i != screen_index)
    {
      // Get the size.
      NSScreen* pScreen = [[NSScreen screens] objectAtIndex:i];
      NSRect    screenRect = [pScreen frame];
          
      // Build a blanking window.
      screenRect.origin = NSZeroPoint;
      blankingWindows[i] = [[NSWindow alloc] initWithContentRect:screenRect
        styleMask:NSBorderlessWindowMask
        backing:NSBackingStoreBuffered
        defer:NO 
        screen:pScreen];
                                            
      [blankingWindows[i] setBackgroundColor:[NSColor blackColor]];
      [blankingWindows[i] setLevel:CGShieldingWindowLevel()];
      [blankingWindows[i] makeKeyAndOrderFront:nil];
    }
  } 
}

void Cocoa_GL_UnblankDisplays(void)
{
  int numDisplays = [[NSScreen screens] count];
  int i = 0;

  for (i=0; i<numDisplays; i++)
  {
    if (blankingWindows[i] != 0)
    {
      // Get rid of the blanking windows we created.
      [blankingWindows[i] close];
      [blankingWindows[i] release];
      blankingWindows[i] = 0;
    }
  }
}

static NSOpenGLContext* lastOwnedContext = 0;

void Cocoa_GL_SetFullScreen(int width, int height, bool fs, bool blankOtherDisplays, bool gl_FullScreen)
{
  static NSView* lastView = NULL;
  static CGDirectDisplayID fullScreenDisplayID = 0;
  static NSScreen* lastScreen = NULL;
  static NSWindow* mainWindow = NULL;
  static NSPoint last_origin;
  static NSSize view_size;
  static NSPoint view_origin;
  int screen_index;

  // If we're already fullscreen then we must be moving to a different display.
  // Recurse to reset fullscreen mode and then continue.
  if (fs == true && lastScreen != NULL)
    Cocoa_GL_SetFullScreen(0, 0, false, blankOtherDisplays, gl_FullScreen);
  
  NSOpenGLContext* context = (NSOpenGLContext*)Cocoa_GL_GetCurrentContext();
  
  if (!context)
    return;
  
  if (fs)
  {
    // FullScreen Mode
    NSOpenGLContext* newContext = NULL;
  
    // Fade to black to hide resolution-switching flicker and garbage.
    CGDisplayFadeReservationToken fade_token = kCGDisplayFadeReservationInvalidToken;
    if (CGAcquireDisplayFadeReservation (5, &fade_token) == kCGErrorSuccess )
      CGDisplayFade(fade_token, 0.3, kCGDisplayBlendNormal, kCGDisplayBlendSolidColor, 0.0, 0.0, 0.0, TRUE);

    // Save and make sure the view is on the screen that we're activating (to hide it).
    lastView = [context view];
    lastScreen = [[lastView window] screen];
    screen_index = Cocoa_GetDisplayIndex( Cocoa_GetDisplayIDFromScreen(lastScreen) );
    
    if (gl_FullScreen)
    {
      // hide the window
      view_size = [lastView frame].size;
      view_origin = [lastView frame].origin;
      last_origin = [[lastView window] frame].origin;
      [[lastView window] setFrameOrigin:[lastScreen frame].origin];
      // expand the mouse bounds in SDL view to fullscreen
      [ lastView setFrameOrigin:NSMakePoint(0.0,0.0)];
      [ lastView setFrameSize:NSMakeSize(width,height) ];

      // This is OpenGL FullScreen Mode
      // obtain fullscreen pixel format
      NSOpenGLPixelFormat* pixFmt = (NSOpenGLPixelFormat*)Cocoa_GL_GetFullScreenPixelFormat(screen_index);
      if (!pixFmt)
        return;
      
      // create our new context (sharing with the current one)
      newContext = (NSOpenGLContext*)Cocoa_GL_CreateContext((void*) pixFmt, (void*)context);
      
      // release pixelformat
      [pixFmt release];
      pixFmt = nil;
      
      if (!newContext)
        return;
      
      // clear the current context
      [NSOpenGLContext clearCurrentContext];
              
      // set fullscreen
      [newContext setFullScreen];
      
      // Capture the display before going fullscreen.
      fullScreenDisplayID = (CGDirectDisplayID)Cocoa_GetDisplayID(screen_index);
      if (blankOtherDisplays == true)
        CGCaptureAllDisplays();
      else
        CGDisplayCapture(fullScreenDisplayID);

      // If we don't hide menu bar, it will get events and interrupt the program.
      if (fullScreenDisplayID == kCGDirectMainDisplay)
        HideMenuBar();
    }
    else
    {
      // This is Cocca Windowed FullScreen Mode
      // Get the screen rect of our current display
      NSScreen* pScreen = [[NSScreen screens] objectAtIndex:screen_index];
      NSRect    screenRect = [pScreen frame];
      
      // remove frame origin offset of orginal display
      screenRect.origin = NSZeroPoint;
      
      // make a new window to act as the windowedFullScreen
      mainWindow = [[NSWindow alloc] initWithContentRect:screenRect
        styleMask:NSBorderlessWindowMask
        backing:NSBackingStoreBuffered
        defer:NO 
        screen:pScreen];
                                              
      [mainWindow setBackgroundColor:[NSColor blackColor]];
      [mainWindow makeKeyAndOrderFront:nil];
      
      // Own'ed, Everything is below our window...
      [mainWindow setLevel:CGShieldingWindowLevel()];
      // Uncomment this to debug fullscreen on a one display system
      //[mainWindow setLevel:NSNormalWindowLevel];

      // ...and the original one beneath it and on the same screen.
      view_size = [lastView frame].size;
      view_origin = [lastView frame].origin;
      last_origin = [[lastView window] frame].origin;
      [[lastView window] setLevel:NSNormalWindowLevel];
      [[lastView window] setFrameOrigin:[pScreen frame].origin];
      // expand the mouse bounds in SDL view to fullscreen
      [ lastView setFrameOrigin:NSMakePoint(0.0,0.0)];
      [ lastView setFrameSize:NSMakeSize(width,height) ];
          
      NSView* blankView = [[NSView alloc] init];
      [mainWindow setContentView:blankView];
      [mainWindow setContentSize:NSMakeSize(width, height)];
      [mainWindow update];
      [blankView setFrameSize:NSMakeSize(width, height)];
      
      // Obtain windowed pixel format and create a new context.
      NSOpenGLPixelFormat* pixFmt = (NSOpenGLPixelFormat*)Cocoa_GL_GetWindowPixelFormat();
      newContext = (NSOpenGLContext*)Cocoa_GL_CreateContext((void* )pixFmt, (void* )context);
      [pixFmt release];
      
      [newContext setView:blankView];
      
      // Hide the menu bar.
      fullScreenDisplayID = (CGDirectDisplayID)Cocoa_GetDisplayID(screen_index);
      if (fullScreenDisplayID == kCGDirectMainDisplay)
        HideMenuBar();
          
      // Blank other displays if requested.
      if (blankOtherDisplays)
        Cocoa_GL_BlankOtherDisplays(screen_index);
    }

    // Hide the mouse.
    [NSCursor hide];
    
    // Release old context if we created it.
    if (lastOwnedContext == context)
      Cocoa_GL_ReleaseContext((void*)context);

    // activate context
    [newContext makeCurrentContext];
    lastOwnedContext = newContext;
    
    if (fade_token != kCGDisplayFadeReservationInvalidToken) 
    {
      CGDisplayFade(fade_token, 0.5, kCGDisplayBlendSolidColor, kCGDisplayBlendNormal, 0.0, 0.0, 0.0, FALSE);
      CGReleaseDisplayFadeReservation(fade_token);
    }
  }
  else
  {
    // Windowed Mode
  	// Fade to black to hide resolution-switching flicker and garbage.
  	CGDisplayFadeReservationToken fade_token = kCGDisplayFadeReservationInvalidToken;
    if (CGAcquireDisplayFadeReservation (5, &fade_token) == kCGErrorSuccess )
      CGDisplayFade(fade_token, 0.3, kCGDisplayBlendNormal, kCGDisplayBlendSolidColor, 0.0, 0.0, 0.0, TRUE);
    
    // exit fullscreen
    [context clearDrawable];
    
    [NSCursor unhide];
    
    // Show menubar.
    if (fullScreenDisplayID == kCGDirectMainDisplay)
      ShowMenuBar();

    if (gl_FullScreen)
    {
      // release displays
      CGReleaseAllDisplays();
    }
    else
    {
      // Get rid of the new window we created.
      [mainWindow close];
      [mainWindow release];
      
      // Unblank.
      if (blankOtherDisplays)
      {
        lastScreen = [[lastView window] screen];
        screen_index = Cocoa_GetDisplayIndex( Cocoa_GetDisplayIDFromScreen(lastScreen) );

        Cocoa_GL_UnblankDisplays();
      }
    }
    
    // obtain windowed pixel format
    NSOpenGLPixelFormat* pixFmt = (NSOpenGLPixelFormat*)Cocoa_GL_GetWindowPixelFormat();
    if (!pixFmt)
      return;
    
    // create our new context (sharing with the current one)
    NSOpenGLContext* newContext = (NSOpenGLContext*)Cocoa_GL_CreateContext((void* )pixFmt, (void* )context);
    
    // release pixelformat
    [pixFmt release];
    pixFmt = nil;
    
    if (!newContext)
      return;
    
    // Assign view from old context, move back to original screen.
    [newContext setView:lastView];
    [[lastView window] setFrameOrigin:last_origin];
    // return the mouse bounds in SDL view to prevous size
    [ lastView setFrameSize:view_size ];
    [ lastView setFrameOrigin:view_origin ];
    
    // Release the fullscreen context.
    if (lastOwnedContext == context)
      Cocoa_GL_ReleaseContext((void*)context);
    
    // Activate context.
    [newContext makeCurrentContext];
    lastOwnedContext = newContext;
    
    if (fade_token != kCGDisplayFadeReservationInvalidToken) 
    {
      CGDisplayFade(fade_token, 0.5, kCGDisplayBlendSolidColor, kCGDisplayBlendNormal, 0.0, 0.0, 0.0, FALSE);
      CGReleaseDisplayFadeReservation(fade_token);
    }
    
    // Reset.
    lastView = NULL;
    lastScreen = NULL;
    mainWindow = NULL;
    fullScreenDisplayID = 0;
  }
}

void Cocoa_GL_EnableVSync(bool enable)
{
  // OpenGL Flush synchronised with vertical retrace                       
  GLint swapInterval;
  
  swapInterval = enable ? 1 : 0;
  [[NSOpenGLContext currentContext] setValues:&swapInterval forParameter:NSOpenGLCPSwapInterval];
}

void* Cocoa_GL_GetWindowPixelFormat()
{
  NSOpenGLPixelFormatAttribute wattrs[] =
  {
    NSOpenGLPFADoubleBuffer,
    NSOpenGLPFAWindow,
    NSOpenGLPFANoRecovery,
    NSOpenGLPFAAccelerated,
    NSOpenGLPFADepthSize, (NSOpenGLPixelFormatAttribute)8,
    (NSOpenGLPixelFormatAttribute)0
  };
  NSOpenGLPixelFormat* pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:wattrs];
  return (void*)pixFmt;
}

void* Cocoa_GL_GetFullScreenPixelFormat(int screen)
{
  NSOpenGLPixelFormatAttribute fsattrs[] =
  {
    NSOpenGLPFADoubleBuffer,
    NSOpenGLPFAFullScreen,
    NSOpenGLPFANoRecovery,
    NSOpenGLPFAAccelerated,
    NSOpenGLPFADepthSize,  (NSOpenGLPixelFormatAttribute)8,
    NSOpenGLPFAScreenMask, (NSOpenGLPixelFormatAttribute)CGDisplayIDToOpenGLDisplayMask((CGDirectDisplayID)Cocoa_GetDisplayID(screen)),
    (NSOpenGLPixelFormatAttribute)0
  };
  NSOpenGLPixelFormat* pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:fsattrs];
  return (void*)pixFmt;
}

void* Cocoa_GL_GetCurrentContext()
{
  NSOpenGLContext* context = [NSOpenGLContext currentContext];
  return (void*)context;
}

void* Cocoa_GL_CreateContext(void* pixFmt, void* shareCtx)
{
  if (!pixFmt)
    return nil;
    
  NSOpenGLContext* newContext = [[NSOpenGLContext alloc] initWithFormat:(NSOpenGLPixelFormat*)pixFmt
    shareContext:(NSOpenGLContext*)shareCtx];
  return newContext;
}

void* Cocoa_GL_ReplaceSDLWindowContext()
{
  NSOpenGLContext* context = (NSOpenGLContext*)Cocoa_GL_GetCurrentContext();
  NSView* view = [context view];
  
  if (!view)
    return 0;
  
  // disassociate view from context
  [context clearDrawable];
  
  // release the context
  if (lastOwnedContext == context)
    Cocoa_GL_ReleaseContext((void*)context);
  
  // obtain window pixelformat
  NSOpenGLPixelFormat* pixFmt = (NSOpenGLPixelFormat*)Cocoa_GL_GetWindowPixelFormat();
  if (!pixFmt)
    return 0;
  
  NSOpenGLContext* newContext = (NSOpenGLContext*)Cocoa_GL_CreateContext((void*)pixFmt, nil);
  [pixFmt release];
  
  if (!newContext)
    return 0;
  
  // associate with current view
  [newContext setView:view];
  [newContext makeCurrentContext];
  lastOwnedContext = newContext;
  
  return newContext;
}

int Cocoa_IdleDisplays()
{
#if !defined(__POWERPC__)
  // http://lists.apple.com/archives/Cocoa-dev/2007/Nov/msg00267.html
  // This is an unsupported system call that kernel panics on PPC boxes
  io_registry_entry_t r = IORegistryEntryFromPath(kIOMasterPortDefault, "IOService:/IOResources/IODisplayWrangler");
  if(!r) return 1;
  int err = IORegistryEntrySetCFProperty(r, CFSTR("IORequestIdle"), kCFBooleanTrue);
  IOObjectRelease(r);
  return err;
#else
  return 0;
#endif
}

/* 10.5 only
void Cocoa_SetSystemSleep(bool enable)
{
  // kIOPMAssertionTypeNoIdleSleep prevents idle sleep
  IOPMAssertionID assertionID;
  IOReturn success;
  
  if (enable) {
    success= IOPMAssertionCreate(kIOPMAssertionTypeNoDisplaySleep, kIOPMAssertionLevelOn, &assertionID); 
  } else {
    success = IOPMAssertionRelease(assertionID);
  }
}

void Cocoa_SetDisplaySleep(bool enable)
{
  // kIOPMAssertionTypeNoIdleSleep prevents idle sleep
  IOPMAssertionID assertionID;
  IOReturn success;
  
  if (enable) {
    success= IOPMAssertionCreate(kIOPMAssertionTypeNoIdleSleep, kIOPMAssertionLevelOn, &assertionID); 
  } else {
    success = IOPMAssertionRelease(assertionID);
  }
}
*/

void Cocoa_UpdateSystemActivity()
{
  // Original Author: Elan Feingold
  UpdateSystemActivity(UsrActivity);   
}

void Cocoa_DisableOSXScreenSaver()
{
  // If we don't call this, the screen saver will just stop and then start up again.
  UpdateSystemActivity(UsrActivity);      

  NSDictionary* errorDict;
  NSAppleEventDescriptor* returnDescriptor = NULL;
  NSAppleScript* scriptObject = [[NSAppleScript alloc] initWithSource:
    @"tell application \"ScreenSaverEngine\" to quit"];
  returnDescriptor = [scriptObject executeAndReturnError: &errorDict];
  [scriptObject release];
}

void Cocoa_DoAppleScript(const char* scriptSource)
{
  NSDictionary* errorDict;
  NSAppleEventDescriptor* returnDescriptor = NULL;
  NSAppleScript* scriptObject = [[NSAppleScript alloc] initWithSource:
    [NSString stringWithUTF8String:scriptSource]];
  returnDescriptor = [scriptObject executeAndReturnError: &errorDict];
  [scriptObject release];
}
                   
void Cocoa_MountPoint2DeviceName(char* path)
{
  // if physical DVDs, libdvdnav wants "/dev/rdiskN" device name for OSX,
  // path will get realloc'ed and replaced IF this is a physical DVD.
  char* strDVDDevice;
  strDVDDevice = strdup(path);
  if (strncasecmp(strDVDDevice + strlen(strDVDDevice) - 8, "VIDEO_TS", 8) == 0)
  {
    struct statfs *mntbufp;
    int i, mounts;
    
    strDVDDevice[strlen(strDVDDevice) - 9] = '\0';

    // find a match for /Volumes/<disk name>
    mounts = getmntinfo(&mntbufp, MNT_WAIT);  // NOT THREAD SAFE!
    for (i = 0; i < mounts; i++)
    {
      if( !strcasecmp(mntbufp[i].f_mntonname, strDVDDevice) )
      {
        // Replace "/dev/" with "/dev/r"
        path = (char*)realloc(path, strlen(mntbufp[i].f_mntfromname) + 2 );
        strcpy( path, "/dev/r" );
        strcat( path, mntbufp[i].f_mntfromname + strlen( "/dev/" ) );
        break;
      }
    }
    free(strDVDDevice);
  }
}

/*
@interface MyView : NSOpenGLView
{
    CVDisplayLinkRef displayLink; //display link for managing rendering thread
}
@end

- (void)prepareOpenGL
{
    // Synchronize buffer swaps with vertical refresh rate
    GLint swapInt = 1;
    [[self openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval]; 

    // Create a display link capable of being used with all active displays
    CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);

    // Set the renderer output callback function
    CVDisplayLinkSetOutputCallback(displayLink, &MyViewDisplayLinkCallback, self);

    // Set the display link for the current renderer
    CGLContextObj cglContext = [[self openGLContext] CGLContextObj];
    CGLPixelFormatObj cglPixelFormat = [[self pixelFormat] CGLPixelFormatObj];
    CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, cglContext, cglPixelFormat);

    // Activate the display link
    CVDisplayLinkStart(displayLink);
}

// This is the renderer output callback function
static CVReturn MyDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
    CVReturn result = [(MyView*)displayLinkContext getFrameForTime:outputTime];
    return result;
}

- (CVReturn)getFrameForTime:(const CVTimeStamp*)outputTime
{
    // Add your drawing codes here

    return kCVReturnSuccess;
}

- (void)dealloc
{
    // Release the display link
    CVDisplayLinkRelease(displayLink);

    [super dealloc];
}

//888888888888888888

// Synchronize buffer swaps with vertical refresh rate (NSTimer)
- (void)prepareOpenGL
{
    GLint swapInt = 1;
    [[self openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];
}

// Put our timer in -awakeFromNib, so it can start up right from the beginning
-(void)awakeFromNib
{
    renderTimer = [[NSTimer timerWithTimeInterval:0.001   //a 1ms time interval
                                target:self
                                selector:@selector(timerFired:)
                                userInfo:nil
                                repeats:YES];

    [[NSRunLoop currentRunLoop] addTimer:renderTimer 
                                forMode:NSDefaultRunLoopMode];
    [[NSRunLoop currentRunLoop] addTimer:renderTimer 
                                forMode:NSEventTrackingRunLoopMode]; //Ensure timer fires during resize
}

// Timer callback method
- (void)timerFired:(id)sender
{
    // It is good practice in a Cocoa application to allow the system to send the -drawRect:
    // message when it needs to draw, and not to invoke it directly from the timer. 
    // All we do here is tell the display it needs a refresh
    [self setNeedsDisplay:YES];
}
*/
