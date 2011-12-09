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
#if !defined(__arm__)
#import <unistd.h>
#import <sys/mount.h>

#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#import <Carbon/Carbon.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>

#import "CocoaInterface.h"
#import "DllPaths_generated.h"

#import "AutoPool.h"

// hack for Cocoa_GL_ResizeWindow
//extern "C" void SDL_SetWidthHeight(int w, int h);

//#define MAX_DISPLAYS 32
//static NSWindow* blankingWindows[MAX_DISPLAYS];

//display link for display managment
static CVDisplayLinkRef displayLink = NULL; 

CGDirectDisplayID Cocoa_GetDisplayIDFromScreen(NSScreen *screen);

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
  CCocoaAutoPool pool;

  NSDictionary* errorDict;
  NSAppleEventDescriptor* returnDescriptor = NULL;
  NSAppleScript* scriptObject = [[NSAppleScript alloc] initWithSource:
    [NSString stringWithUTF8String:scriptSource]];
  returnDescriptor = [scriptObject executeAndReturnError: &errorDict];
  [scriptObject release];
}
  
void Cocoa_DoAppleScriptFile(const char* filePath)
{
  NSString* scriptFile = [NSString stringWithUTF8String:filePath];
  NSString* userScriptsPath = [@"~/Library/Application Support/XBMC/scripts" stringByExpandingTildeInPath];
  NSString* bundleScriptsPath = [[[NSBundle mainBundle] bundlePath] stringByAppendingPathComponent:@"Contents/Resources/XBMC/scripts"];
  NSString* bundleSysScriptsPath = [[[NSBundle mainBundle] bundlePath] stringByAppendingPathComponent:@"Contents/Resources/XBMC/system/AppleScripts"];

  // Check whether a script exists in the app bundle's AppleScripts folder
  if ([[NSFileManager defaultManager] fileExistsAtPath:[bundleSysScriptsPath stringByAppendingPathComponent:scriptFile]])
    scriptFile = [bundleSysScriptsPath stringByAppendingPathComponent:scriptFile];

  // Check whether a script exists in app support
  else if ([[NSFileManager defaultManager] fileExistsAtPath:[userScriptsPath stringByAppendingPathComponent:scriptFile]]) // Check whether a script exists in the app bundle
    scriptFile = [userScriptsPath stringByAppendingPathComponent:scriptFile];

  // Check whether a script exists in the app bundle's Scripts folder
  else if ([[NSFileManager defaultManager] fileExistsAtPath:[bundleScriptsPath stringByAppendingPathComponent:scriptFile]])
    scriptFile = [bundleScriptsPath stringByAppendingPathComponent:scriptFile];

  // If no script could be found, check if we were given a full path
  else if (![[NSFileManager defaultManager] fileExistsAtPath:scriptFile])
    return;

  NSAppleScript* appleScript = [[NSAppleScript alloc] initWithContentsOfURL:[NSURL fileURLWithPath:scriptFile] error:nil];
  [appleScript executeAndReturnError:nil];
  [appleScript release];
}

const char* Cocoa_GetIconFromBundle(const char *_bundlePath, const char* _iconName)
{
  NSString* bundlePath = [NSString stringWithUTF8String:_bundlePath];
  NSString* iconName = [NSString stringWithUTF8String:_iconName];
  NSBundle* bundle = [NSBundle bundleWithPath:bundlePath];
  NSString* iconPath = [bundle pathForResource:iconName ofType:@"icns"];
  NSString* bundleIdentifier = [bundle bundleIdentifier];

  if (![[NSFileManager defaultManager] fileExistsAtPath:iconPath]) return NULL;

  // Get the path to the target PNG icon
  NSString* pngFile = [[NSString stringWithFormat:@"~/Library/Application Support/XBMC/userdata/Thumbnails/%@-%@.png",
    bundleIdentifier, iconName] stringByExpandingTildeInPath];

  // If no PNG has been created, open the ICNS file & convert
  if (![[NSFileManager defaultManager] fileExistsAtPath:pngFile])
  {
    NSImage* icon = [[NSImage alloc] initWithContentsOfFile:iconPath];
    if (!icon) return NULL;
    NSBitmapImageRep* rep = [[NSBitmapImageRep alloc] initWithData:[icon TIFFRepresentation]];
    NSData* png = [rep representationUsingType:NSPNGFileType properties:nil];
    [png writeToFile:pngFile atomically:YES];
    [png release];
    [rep release];
    [icon release];
  }
  return [pngFile UTF8String];
}

void Cocoa_MountPoint2DeviceName(char* path)
{
  CCocoaAutoPool pool;
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

bool Cocoa_GetVolumeNameFromMountPoint(const char *mountPoint, CStdString &volumeName)
{
  CCocoaAutoPool pool;
  unsigned i, count = 0;
  struct statfs *buf = NULL;
  CStdString mountpoint, devicepath;

  count = getmntinfo(&buf, 0);
  for (i=0; i<count; i++)
  {
    mountpoint = buf[i].f_mntonname;
    if (mountpoint == mountPoint)
    {
      devicepath = buf[i].f_mntfromname;
      break;
    }
  }
  if (devicepath.empty())
  {
    return false;
  }

  DASessionRef session = DASessionCreate(kCFAllocatorDefault);
  if (!session)
  {
      return false;
  }

  DADiskRef disk = DADiskCreateFromBSDName(kCFAllocatorDefault, session, devicepath.c_str());
  if (!disk)
  {
      CFRelease(session);
      return false;
  }

  NSDictionary *dd = (NSDictionary*) DADiskCopyDescription(disk);
  if (!dd)
  {
      CFRelease(session);
      CFRelease(disk);
      return false;
  }

  NSString *volumename = [dd objectForKey:(NSString*)kDADiskDescriptionVolumeNameKey];
  volumeName = [volumename UTF8String];

  CFRelease(session);		        
  CFRelease(disk);		        
  [dd release];

  return true ;
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

void Cocoa_ShowMouse()
{
  [NSCursor unhide];
}

void Cocoa_HideDock()
{
  // Find which display we are on
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

bool Cocoa_HasVDADecoder()
{
  static int result = -1;

  if (result == -1)
  {
    if (Cocoa_GetOSVersion() >= 0x1063)
      result = (access(DLL_PATH_LIBVDADECODER, 0) == 0) ? 1:0;
    else
      result = 0;
  }

  return (result == 1);
}

bool Cocoa_GPUForDisplayIsNvidiaPureVideo3()
{
  bool result = false;
  std::string str;
  const char *cstr;
  CGDirectDisplayID display_id;

  // try for display we are running on
  display_id = (CGDirectDisplayID)Cocoa_GL_GetCurrentDisplayID();
 
  io_registry_entry_t dspPort = CGDisplayIOServicePort(display_id);
  // if fails, go for main display
  if (dspPort == MACH_PORT_NULL)
    dspPort = CGDisplayIOServicePort(kCGDirectMainDisplay);

  CFDataRef model;
  model = (CFDataRef)IORegistryEntrySearchCFProperty(dspPort, kIOServicePlane, CFSTR("model"),
    kCFAllocatorDefault,kIORegistryIterateRecursively | kIORegistryIterateParents);

  if (model)
  {
    cstr = (const char*)CFDataGetBytePtr(model);
    if (std::string(cstr).find("NVIDIA GeForce 9400") != std::string::npos)
      result = true;

    CFRelease(model);
  }

  return(result);
}

int Cocoa_GetOSVersion()
{
  static SInt32 version = -1;

  if (version == -1)
    Gestalt(gestaltSystemVersion, &version);
  
  return(version);
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

OSStatus SendAppleEventToSystemProcess(AEEventID EventToSend)
{
  AEAddressDesc targetDesc;
  static const ProcessSerialNumber kPSNOfSystemProcess = { 0, kSystemProcess };
  AppleEvent eventReply = {typeNull, NULL};
  AppleEvent appleEventToSend = {typeNull, NULL};

  OSStatus error = noErr;

  error = AECreateDesc(typeProcessSerialNumber, &kPSNOfSystemProcess, 
                       sizeof(kPSNOfSystemProcess), &targetDesc);

  if (error != noErr)
  {
    return(error);
  }

  error = AECreateAppleEvent(kCoreEventClass, EventToSend, &targetDesc, 
                             kAutoGenerateReturnID, kAnyTransactionID, &appleEventToSend);

  AEDisposeDesc(&targetDesc);
  if (error != noErr)
  {
    return(error);
  }

  error = AESend(&appleEventToSend, &eventReply, kAENoReply, 
                 kAENormalPriority, kAEDefaultTimeout, NULL, NULL);

  AEDisposeDesc(&appleEventToSend);
  if (error != noErr)
  {
    return(error);
  }

  AEDisposeDesc(&eventReply);

  return(error); 
}
#endif
