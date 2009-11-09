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
#import <QuartzCore/QuartzCore.h>
#import <Carbon/Carbon.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>

#import "CocoaInterface.h"

// hack for Cocoa_GL_ResizeWindow
//extern "C" void SDL_SetWidthHeight(int w, int h);

//#define MAX_DISPLAYS 32
//static NSWindow* blankingWindows[MAX_DISPLAYS];

//display link for display managment
static CVDisplayLinkRef displayLink = NULL; 

CGDirectDisplayID Cocoa_GetDisplayIDFromScreen(NSScreen *screen);

void* Cocoa_Create_AutoReleasePool(void)
{
  // Original Author: Elan Feingold
	// Create an autorelease pool (necessary to call Obj-C code from non-Obj-C code)
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
  return pool;
}

void Cocoa_Destroy_AutoReleasePool(void* aPool)
{
  // Original Author: Elan Feingold
  NSAutoreleasePool* pool = (NSAutoreleasePool* )aPool;
  [pool release];
}

int Cocoa_GL_GetCurrentDisplayID(void)
{
  // Find which display we are on from the current context (default to main display)
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
        display_id = (CGDirectDisplayID)[screenID longValue];
      }
    }
  }
  
  return((int)display_id);
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

void Cocoa_UpdateSystemActivity(void)
{
  // Original Author: Elan Feingold
  UpdateSystemActivity(UsrActivity);   
}

void Cocoa_DisableOSXScreenSaver(void)
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

bool Cocoa_CVDisplayLinkCreate(void *displayLinkcallback, void *displayLinkContext)
{
  CVReturn status = kCVReturnError;
  CGDirectDisplayID display_id;
    
  // OpenGL Flush synchronised with vertical retrace                       
  GLint swapInterval = 1;
  [[NSOpenGLContext currentContext] setValues:&swapInterval forParameter:NSOpenGLCPSwapInterval];

  display_id = (CGDirectDisplayID)Cocoa_GL_GetCurrentDisplayID();
  if (!displayLink)
  {
    // Create a display link capable of being used with all active displays
    status = CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);

    // Set the renderer output callback function
    status = CVDisplayLinkSetOutputCallback(displayLink, (CVDisplayLinkOutputCallback)displayLinkcallback, displayLinkContext);
  }

  if (status == kCVReturnSuccess)
  {
    // Set the display link for the current display
    status = CVDisplayLinkSetCurrentCGDisplay(displayLink, display_id);

    // Activate the display link
    status = CVDisplayLinkStart(displayLink);
  }
  
  return(status == kCVReturnSuccess);
}

void Cocoa_CVDisplayLinkRelease(void)
{
  if (displayLink)
  {
    if (CVDisplayLinkIsRunning(displayLink))
      CVDisplayLinkStop(displayLink);
    // Release the display link
    CVDisplayLinkRelease(displayLink);
    displayLink = NULL;
  }
}

void Cocoa_CVDisplayLinkUpdate(void)
{
  if (displayLink)
  {
    CGDirectDisplayID display_id;
    
    display_id = (CGDirectDisplayID)Cocoa_GL_GetCurrentDisplayID();
    // Set the display link to the current display
    CVDisplayLinkSetCurrentCGDisplay(displayLink, display_id);
  }
}

double Cocoa_GetCVDisplayLinkRefreshPeriod(void)
{
  double fps = 60.0;

  if (displayLink && CVDisplayLinkIsRunning(displayLink) )
  {
    CVTime cvtime;
    cvtime = CVDisplayLinkGetNominalOutputVideoRefreshPeriod(displayLink);
    if (cvtime.timeValue > 0)
      fps = (double)cvtime.timeScale / (double)cvtime.timeValue;
    
    fps = CVDisplayLinkGetActualOutputVideoRefreshPeriod(displayLink);
    if (fps > 0.0)
      fps = 1.0 / fps;
    else
      fps = 60.0;
  }
  else
  {
    // NOTE: The refresh rate will be REPORTED AS 0 for many DVI and notebook displays.
    CGDirectDisplayID display_id;
    CFDictionaryRef mode;
  
    display_id = (CGDirectDisplayID)Cocoa_GL_GetCurrentDisplayID();
    mode = CGDisplayCurrentMode(display_id);
    if (mode)
    {
      CFNumberGetValue( (CFNumberRef)CFDictionaryGetValue(mode, kCGDisplayRefreshRate), kCFNumberDoubleType, &fps);
      if (fps <= 0.0)
        fps = 60.0;
    }
  }
  
  return(fps);
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
void SetPIDFrontProcess(pid_t pid) {
    ProcessSerialNumber psn;

    GetProcessForPID(pid, &psn );
    SetFrontProcess(&psn);
}
*/

/*
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

[newWindow setFrameAutosaveName:@"some name"] 

and the window's frame is automatically saved for you in the application 
defaults each time its location changes. 
*/


void Cocoa_HideMouse()
{
  [NSCursor hide];
}

void Cocoa_GetSmartFolderResults(const char* strFile, void (*CallbackFunc)(void* userData, void* userData2, const char* path), void* userData, void* userData2)
{
  NSString*     filePath = [[NSString alloc] initWithUTF8String:strFile];
  NSDictionary* doc = [[NSDictionary alloc] initWithContentsOfFile:filePath];
  NSString*     raw = [doc objectForKey:@"RawQuery"];
  NSArray*      searchPaths = [[doc objectForKey:@"SearchCriteria"] objectForKey:@"FXScopeArrayOfPaths"];

  if (raw == 0)
    return;

  // Ugh, Carbon from now on...
  MDQueryRef query = MDQueryCreate(kCFAllocatorDefault, (CFStringRef)raw, NULL, NULL);
  if (query)
  {
  	if (searchPaths)
  	  MDQuerySetSearchScope(query, (CFArrayRef)searchPaths, 0);
  	  
    MDQueryExecute(query, 0);

	// Keep track of when we started.
	CFAbsoluteTime startTime = CFAbsoluteTimeGetCurrent(); 
    for (;;)
    {
      CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, YES);
    
      // If we're done or we timed out.
      if (MDQueryIsGatheringComplete(query) == true ||
      	  CFAbsoluteTimeGetCurrent() - startTime >= 5)
      {
        // Stop the query.
        MDQueryStop(query);
      
    	CFIndex count = MDQueryGetResultCount(query);
    	char title[BUFSIZ];
    	int i;
  
    	for (i = 0; i < count; ++i) 
   		{
      	  MDItemRef resultItem = (MDItemRef)MDQueryGetResultAtIndex(query, i);
      	  CFStringRef titleRef = (CFStringRef)MDItemCopyAttribute(resultItem, kMDItemPath);
      
      	  CFStringGetCString(titleRef, title, BUFSIZ, kCFStringEncodingUTF8);
      	  CallbackFunc(userData, userData2, title);
      	  CFRelease(titleRef);
    	}  
    
        CFRelease(query);
    	break;
      }
    }
  }
  
  // Freeing these causes a crash when scanning for new content.
  CFRelease(filePath);
  CFRelease(doc);
}

static char strVersion[32];

const char* Cocoa_GetAppVersion()
{
  // Get the main bundle for the app and return the version.
  CFBundleRef mainBundle = CFBundleGetMainBundle();
  CFStringRef versStr = (CFStringRef)CFBundleGetValueForInfoDictionaryKey(mainBundle, kCFBundleVersionKey);
  
  memset(strVersion,0,32);
  
  if (versStr != NULL && CFGetTypeID(versStr) == CFStringGetTypeID())
  {
    bool res = CFStringGetCString(versStr, strVersion, 32,kCFStringEncodingUTF8);
    if (!res)
    {
      printf("Error converting version string\n");      
      strcpy(strVersion, "SVN");
    }
  }
  else
    strcpy(strVersion, "SVN");
  
  return strVersion;
}

NSWindow* childWindow = nil;
NSWindow* mainWindow = nil;


void Cocoa_MakeChildWindow()
{
  NSOpenGLContext* context = [NSOpenGLContext currentContext];
  NSView* view = [context view];
  NSWindow* window = [view window];

  // Create a child window.
  childWindow = [[NSWindow alloc] initWithContentRect:[window frame]
                                            styleMask:NSBorderlessWindowMask
                                              backing:NSBackingStoreBuffered
                                                defer:NO];
                                          
  [childWindow setContentSize:[view frame].size];
  [childWindow setBackgroundColor:[NSColor blackColor]];
  [window addChildWindow:childWindow ordered:NSWindowAbove];
  mainWindow = window;
  //childWindow.alphaValue = 0.5; 
}

void Cocoa_DestroyChildWindow()
{
  if (childWindow != nil)
  {
    [mainWindow removeChildWindow:childWindow];
    [childWindow close];
    childWindow = nil;
  }
}
const char *Cocoa_Paste() 
{
  NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
  NSString *type = [pasteboard availableTypeFromArray:[NSArray arrayWithObject:NSStringPboardType]];
  if (type != nil) {
    NSString *contents = [pasteboard stringForType:type];
    if (contents != nil) {
      return [contents UTF8String];
    }
  }
  
  return NULL;
}
