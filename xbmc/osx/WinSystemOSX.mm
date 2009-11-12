/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#ifdef __APPLE__

//hack around problem with xbmc's typedef int BOOL
// and obj-c's typedef unsigned char BOOL
#define BOOL XBMC_BOOL 
#include "WinSystemOSX.h"
#include "Settings.h"
#include "GUISettings.h"
#include "KeyboardStat.h"
#include "utils/log.h"
#undef BOOL

#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#import <Carbon/Carbon.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>


#define MAX_DISPLAYS 32
static NSWindow* blankingWindows[MAX_DISPLAYS];

void* CWinSystemOSX::m_lastOwnedContext = 0;

//------------------------------------------------------------------------------------------
Boolean GetDictionaryBoolean(CFDictionaryRef theDict, const void* key)
{
        // get a boolean from the dictionary
        Boolean value = false;
        CFBooleanRef boolRef;
        boolRef = (CFBooleanRef)CFDictionaryGetValue(theDict, key);
        if (boolRef != NULL)
                value = CFBooleanGetValue(boolRef);
        return value;
}
//------------------------------------------------------------------------------------------
long GetDictionaryLong(CFDictionaryRef theDict, const void* key)
{
        // get a long from the dictionary
        long value = 0;
        CFNumberRef numRef;
        numRef = (CFNumberRef)CFDictionaryGetValue(theDict, key);
        if (numRef != NULL)
                CFNumberGetValue(numRef, kCFNumberLongType, &value);
        return value;
}
//------------------------------------------------------------------------------------------
int GetDictionaryInt(CFDictionaryRef theDict, const void* key)
{
        // get a long from the dictionary
        int value = 0;
        CFNumberRef numRef;
        numRef = (CFNumberRef)CFDictionaryGetValue(theDict, key);
        if (numRef != NULL)
                CFNumberGetValue(numRef, kCFNumberIntType, &value);
        return value;
}
//------------------------------------------------------------------------------------------
float GetDictionaryFloat(CFDictionaryRef theDict, const void* key)
{
        // get a long from the dictionary
        int value = 0;
        CFNumberRef numRef;
        numRef = (CFNumberRef)CFDictionaryGetValue(theDict, key);
        if (numRef != NULL)
                CFNumberGetValue(numRef, kCFNumberFloatType, &value);
        return value;
}
//------------------------------------------------------------------------------------------
double GetDictionaryDouble(CFDictionaryRef theDict, const void* key)
{
        // get a long from the dictionary
        double value = 0.0;
        CFNumberRef numRef;
        numRef = (CFNumberRef)CFDictionaryGetValue(theDict, key);
        if (numRef != NULL)
                CFNumberGetValue(numRef, kCFNumberDoubleType, &value);
        return value;
}

//---------------------------------------------------------------------------------
CGDirectDisplayID GetDisplayID(int screen_index)
{
  CGDirectDisplayID displayArray[MAX_DISPLAYS];
  CGDisplayCount    numDisplays;

  // Get the list of displays.
  CGGetActiveDisplayList(MAX_DISPLAYS, displayArray, &numDisplays);
  return(displayArray[screen_index]);
}

CGDirectDisplayID GetDisplayIDFromScreen(NSScreen *screen)
{
  NSDictionary* screenInfo = [screen deviceDescription];
  NSNumber* screenID = [screenInfo objectForKey:@"NSScreenNumber"];
  
  return (CGDirectDisplayID)[screenID longValue];
}

int GetDisplayIndex(CGDirectDisplayID display)
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

void BlankOtherDisplays(int screen_index)
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

void UnblankDisplays(void)
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

CGDisplayFadeReservationToken DisplayFadeToBlack(void)
{
  // Fade to black to hide resolution-switching flicker and garbage.
  CGDisplayFadeReservationToken fade_token = kCGDisplayFadeReservationInvalidToken;
  if (CGAcquireDisplayFadeReservation (5, &fade_token) == kCGErrorSuccess )
    CGDisplayFade(fade_token, 0.3, kCGDisplayBlendNormal, kCGDisplayBlendSolidColor, 0.0, 0.0, 0.0, TRUE);

  return(fade_token);
}

void DisplayFadeFromBlack(CGDisplayFadeReservationToken fade_token)
{
  if (fade_token != kCGDisplayFadeReservationInvalidToken) 
  {
    CGDisplayFade(fade_token, 0.5, kCGDisplayBlendSolidColor, kCGDisplayBlendNormal, 0.0, 0.0, 0.0, FALSE);
    CGReleaseDisplayFadeReservation(fade_token);
  }
}


//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
CWinSystemOSX::CWinSystemOSX() : CWinSystemBase()
{
  m_eWindowSystem = WINDOW_SYSTEM_OSX;
  m_glContext = 0;
  m_SDLSurface = NULL;
}

CWinSystemOSX::~CWinSystemOSX()
{
  DestroyWindowSystem();
};

bool CWinSystemOSX::InitWindowSystem()
{
  SDL_EnableUNICODE(1);

  // set repeat to 10ms to ensure repeat time < frame time
  // so that hold times can be reliably detected
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, 10);
  
  if (!CWinSystemBase::InitWindowSystem())
    return false;
  
  return true;
}

bool CWinSystemOSX::DestroyWindowSystem()
{  
  return true;
}

bool CWinSystemOSX::CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction)
{
  m_nWidth  = res.iWidth;
  m_nHeight = res.iHeight;
  m_bFullScreen = fullScreen;

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  // Enable vertical sync to avoid any tearing.
  SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);  

  m_SDLSurface = SDL_SetVideoMode(m_nWidth, m_nHeight, 0, SDL_OPENGL | (m_bFullScreen ? 0 : SDL_RESIZABLE));
  if (!m_SDLSurface)
    return false;

  // the context SDL creates isn't full screen compatible, so we create new one
  // first, find the current contect and make sure a view is attached
  NSOpenGLContext* cur_context = [NSOpenGLContext currentContext];
  NSView* view = [cur_context view];
  if (!view)
    return false;
  
  // disassociate view from context
  [cur_context clearDrawable];
  
  // release the context
  if (m_lastOwnedContext == cur_context)
  {
    [ NSOpenGLContext clearCurrentContext ];
    [ cur_context clearDrawable ];
    [ cur_context release ];
  }
  
  // create a new context
  NSOpenGLContext* new_context = (NSOpenGLContext*)CreateWindowedContext(nil);
  if (!new_context)
    return false;
  
  // associate with current view
  [new_context setView:view];
  [new_context makeCurrentContext];

  // set the window title
  NSString *string;
  string = [ [ NSString alloc ] initWithUTF8String:"XBMC Media Center" ];
  [ [ [new_context view] window] setTitle:string ];
  [ string release ];

  m_glContext = new_context;
  m_lastOwnedContext = new_context;
  m_bWindowCreated = true;

  return true;
}

bool CWinSystemOSX::DestroyWindow()
{
  return true;
}
    
extern "C" void SDL_SetWidthHeight(int w, int h);
bool CWinSystemOSX::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  if (!m_glContext)
    return false;
  
  NSOpenGLContext* context = [NSOpenGLContext currentContext];
  NSView* view;
  NSWindow* window;
  
  view = [context view];
  if (view && (newWidth > 0) && (newHeight > 0))
  {
    window = [view window];
    if (window)
    {
      [window setContentSize:NSMakeSize(newWidth, newHeight)];
      [window update];
      [view setFrameSize:NSMakeSize(newWidth, newHeight)];
      [context update];
    }
  }

  // HACK: resize SDL's view manually so that mouse bounds are correctly updated.
  // there are two parts to this, the internal SDL (current_video->screen) and
  // the cocoa view ( handled in SetFullScreen).
  SDL_SetWidthHeight(newWidth, newHeight);

  [context makeCurrentContext];
  
  m_nWidth = newWidth;
  m_nHeight = newHeight;
  m_glContext = context;
  
  return true;
}

bool CWinSystemOSX::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{  
  static NSView* lastView = NULL;
  static CGDirectDisplayID fullScreenDisplayID = 0;
  static NSScreen* lastScreen = NULL;
  static NSWindow* mainWindow = NULL;
  static NSPoint last_origin;
  static NSSize view_size;
  static NSPoint view_origin;
  int screen_index;
  NSOpenGLContext* context;
  
  m_nWidth      = res.iWidth;
  m_nHeight     = res.iHeight;
  m_bFullScreen = fullScreen;

  // If we're already fullscreen then we must be moving to a different display.
  // Recurse to reset fullscreen mode and then continue.
  if (m_bFullScreen == true && lastScreen != NULL)
    SetFullScreen(false, res, blankOtherDisplays);
  
  context = [NSOpenGLContext currentContext];
  if (!context)
    return false;
  
  if (m_bFullScreen)
  {
    // FullScreen Mode
    NSOpenGLContext* newContext = NULL;
  
    // Fade to black to hide resolution-switching flicker and garbage.
    CGDisplayFadeReservationToken fade_token = DisplayFadeToBlack();
    
    // Save and make sure the view is on the screen that we're activating (to hide it).
    lastView = [context view];
    lastScreen = [[lastView window] screen];
    screen_index = res.iScreen;
    
    if (!g_guiSettings.GetBool("videoscreen.fakefullscreen"))
    {
      // hide the window
      view_size = [lastView frame].size;
      view_origin = [lastView frame].origin;
      last_origin = [[lastView window] frame].origin;
      [[lastView window] setFrameOrigin:[lastScreen frame].origin];
      // expand the mouse bounds in SDL view to fullscreen
      [ lastView setFrameOrigin:NSMakePoint(0.0, 0.0)];
      [ lastView setFrameSize:NSMakeSize(m_nWidth, m_nHeight) ];

      // This is OpenGL FullScreen Mode
      // create our new context (sharing with the current one)
      newContext = (NSOpenGLContext*)CreateFullScreenContext(screen_index, (void*)context);
      if (!newContext)
        return false;
      
      // clear the current context
      [NSOpenGLContext clearCurrentContext];
              
      // set fullscreen
      [newContext setFullScreen];
      
      // Capture the display before going fullscreen.
      fullScreenDisplayID = GetDisplayID(screen_index);
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
      
      // make our window the same level as the rest to enable cmd+tab switching
      [mainWindow setLevel:NSNormalWindowLevel]; 
      // this will make our window topmost and hide all system messages
      //[mainWindow setLevel:CGShieldingWindowLevel()];

      // ...and the original one beneath it and on the same screen.
      view_size = [lastView frame].size;
      view_origin = [lastView frame].origin;
      last_origin = [[lastView window] frame].origin;
      [[lastView window] setLevel:NSNormalWindowLevel-1];
      [[lastView window] setFrameOrigin:[pScreen frame].origin];
      // expand the mouse bounds in SDL view to fullscreen
      [ lastView setFrameOrigin:NSMakePoint(0.0, 0.0)];
      [ lastView setFrameSize:NSMakeSize(m_nWidth, m_nHeight) ];
          
      NSView* blankView = [[NSView alloc] init];
      [mainWindow setContentView:blankView];
      [mainWindow setContentSize:NSMakeSize(m_nWidth, m_nHeight)];
      [mainWindow update];
      [blankView setFrameSize:NSMakeSize(m_nWidth, m_nHeight)];
      
      // Obtain windowed pixel format and create a new context.
      newContext = (NSOpenGLContext*)CreateWindowedContext((void* )context);
      [newContext setView:blankView];
      
      // Hide the menu bar.
      fullScreenDisplayID = GetDisplayID(screen_index);
      if (fullScreenDisplayID == kCGDirectMainDisplay)
        HideMenuBar();
          
      // Blank other displays if requested.
      if (blankOtherDisplays)
        BlankOtherDisplays(screen_index);
    }

    // Hide the mouse.
    [NSCursor hide];
    
    // Release old context if we created it.
    if (m_lastOwnedContext == context)
    {
      [ NSOpenGLContext clearCurrentContext ];
      [ context clearDrawable ];
      [ context release ];
    }
    
    // activate context
    [newContext makeCurrentContext];
    m_lastOwnedContext = newContext;
    
    DisplayFadeFromBlack(fade_token);
  }
  else
  {
    // Windowed Mode
  	// Fade to black to hide resolution-switching flicker and garbage.
    CGDisplayFadeReservationToken fade_token = DisplayFadeToBlack();
    
    // exit fullscreen
    [context clearDrawable];
    
    [NSCursor unhide];
    
    // Show menubar.
    if (fullScreenDisplayID == kCGDirectMainDisplay)
      ShowMenuBar();

    if (!g_guiSettings.GetBool("videoscreen.fakefullscreen"))
    {
      // release displays
      CGReleaseAllDisplays();
    }
    else
    {
      [[lastView window] setLevel:NSNormalWindowLevel];
      
      // Get rid of the new window we created.
      [mainWindow close];
      [mainWindow release];
      
      // Unblank.
      if (blankOtherDisplays)
      {
        lastScreen = [[lastView window] screen];
        screen_index = GetDisplayIndex( GetDisplayIDFromScreen(lastScreen) );

        UnblankDisplays();
      }
    }
    
    // create our new context (sharing with the current one)
    NSOpenGLContext* newContext = (NSOpenGLContext*)CreateWindowedContext((void* )context);
    if (!newContext)
      return false;
    
    // Assign view from old context, move back to original screen.
    [newContext setView:lastView];
    [[lastView window] setFrameOrigin:last_origin];
    // return the mouse bounds in SDL view to prevous size
    [ lastView setFrameSize:view_size ];
    [ lastView setFrameOrigin:view_origin ];
    
    // Release the fullscreen context.
    if (m_lastOwnedContext == context)
    {
      [ NSOpenGLContext clearCurrentContext ];
      [ context clearDrawable ];
      [ context release ];
    }
    
    // Activate context.
    [newContext makeCurrentContext];
    m_lastOwnedContext = newContext;
    
    DisplayFadeFromBlack(fade_token);
    
    // Reset.
    lastView = NULL;
    lastScreen = NULL;
    mainWindow = NULL;
    fullScreenDisplayID = 0;
  }

  context = [NSOpenGLContext currentContext];
  [context makeCurrentContext];
  
  m_glContext = context;

  // need to make sure SDL tracks any window size changes
  ResizeWindow(m_nWidth, m_nHeight, -1, -1);

  return true;
}

void CWinSystemOSX::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();

  // Add desktop resolution
  int w, h;
  double fps;
  GetScreenResolution(&w, &h, &fps);
  UpdateDesktopResolution(g_settings.m_ResInfo[RES_DESKTOP], 0, w, h, fps);

  // Add full screen settings for additional monitors
  int numDisplays = [[NSScreen screens] count];
  for (int i = 1; i < numDisplays; i++)
  {
    CFDictionaryRef mode = CGDisplayCurrentMode( GetDisplayID(i) );
    w = GetDictionaryInt(mode, kCGDisplayWidth);
    h = GetDictionaryInt(mode, kCGDisplayHeight);
    fps = GetDictionaryDouble(mode, kCGDisplayRefreshRate);
    if ((int)fps == 0)
    {
      // NOTE: The refresh rate will be REPORTED AS 0 for many DVI and notebook displays.
      fps = 60.0;
    }
    CLog::Log(LOGINFO, "Extra display %d is %dx%d\n", i, w, h);

    RESOLUTION_INFO res;

    UpdateDesktopResolution(res, i, w, h, fps);
    g_graphicsContext.ResetOverscan(res);
    g_settings.m_ResInfo.push_back(res);
  }
  
  //GetVideoModes();
}

void* CWinSystemOSX::CreateWindowedContext(void* shareCtx)
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
  if (!pixFmt)
    return nil;
    
  NSOpenGLContext* newContext = [[NSOpenGLContext alloc] initWithFormat:(NSOpenGLPixelFormat*)pixFmt
    shareContext:(NSOpenGLContext*)shareCtx];
  [pixFmt release];

  return newContext;
}

void* CWinSystemOSX::CreateFullScreenContext(int screen_index, void* shareCtx)
{
  CGDirectDisplayID displayArray[MAX_DISPLAYS];
  CGDisplayCount    numDisplays;
  CGDirectDisplayID displayID;

  // Get the list of displays.
  CGGetActiveDisplayList(MAX_DISPLAYS, displayArray, &numDisplays);
  displayID = displayArray[screen_index];

  NSOpenGLPixelFormatAttribute fsattrs[] =
  {
    NSOpenGLPFADoubleBuffer,
    NSOpenGLPFAFullScreen,
    NSOpenGLPFANoRecovery,
    NSOpenGLPFAAccelerated,
    NSOpenGLPFADepthSize,  (NSOpenGLPixelFormatAttribute)8,
    NSOpenGLPFAScreenMask, (NSOpenGLPixelFormatAttribute)CGDisplayIDToOpenGLDisplayMask(displayID),
    (NSOpenGLPixelFormatAttribute)0
  };
  
  NSOpenGLPixelFormat* pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:fsattrs];
  if (!pixFmt)
    return nil;
    
  NSOpenGLContext* newContext = [[NSOpenGLContext alloc] initWithFormat:(NSOpenGLPixelFormat*)pixFmt
    shareContext:(NSOpenGLContext*)shareCtx];
  [pixFmt release];

  return newContext;
}

void CWinSystemOSX::GetScreenResolution(int* w, int* h, double* fps)
{
  // Figure out the screen size. (default to main screen)
  CGDirectDisplayID display_id = kCGDirectMainDisplay;
  CFDictionaryRef mode  = CGDisplayCurrentMode(display_id);
  
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
        display_id = GetDisplayIDFromScreen( [window screen] );      
        mode  = CGDisplayCurrentMode(display_id);
      }
    }
  }
  
  *w = GetDictionaryInt(mode, kCGDisplayWidth);
  *h = GetDictionaryInt(mode, kCGDisplayHeight);
  *fps = GetDictionaryDouble(mode, kCGDisplayRefreshRate);
  if ((int)*fps == 0)
  {
    // NOTE: The refresh rate will be REPORTED AS 0 for many DVI and notebook displays.
    *fps = 60.0;
  }
}

void CWinSystemOSX::EnableVSync(bool enable)
{
  // OpenGL Flush synchronised with vertical retrace                       
  GLint swapInterval;
  
  swapInterval = enable ? 1 : 0;
  [[NSOpenGLContext currentContext] setValues:(const long*)&swapInterval forParameter:NSOpenGLCPSwapInterval];
}

bool CWinSystemOSX::SwitchToVideoMode(int width, int height, double refreshrate)
{
  CGDirectDisplayID displayID = kCGDirectMainDisplay;
  CFDictionaryRef dispMode = NULL;
  int match = 0;

  // find mode that matches the desired size
  dispMode = CGDisplayBestModeForParametersAndRefreshRate(
    displayID, 32, width, height, (CGRefreshRate)(refreshrate), &match);

  if (!match)
    dispMode = CGDisplayBestModeForParameters(displayID, 32, width, height, &match);

  if (!match)
    dispMode = CGDisplayBestModeForParameters(displayID, 16, width, height, &match);

  if (!match)
    return false;

  // switch mode and return success
  CGDisplayCapture(displayID);
  CGDisplayConfigRef cfg;
  CGBeginDisplayConfiguration(&cfg);
  CGConfigureDisplayFadeEffect(cfg, 0.3f, 0.5f, 0, 0, 0);
  CGConfigureDisplayMode(cfg, displayID, dispMode);
  CGError err = CGCompleteDisplayConfiguration(cfg, kCGConfigureForAppOnly);
  CGDisplayRelease(displayID);
  
  return (err == kCGErrorSuccess);
}

void CWinSystemOSX::GetVideoModes(void)
{
  CGDirectDisplayID displayID = kCGDirectMainDisplay;
  CFArrayRef displayModes = CGDisplayAvailableModes(displayID);
  if (NULL == displayModes)
    return;

  Boolean stretched;
  Boolean interlaced;
  Boolean safeForHardware;
  Boolean televisionoutput;
  int width, height, bitsperpixel;
  double refreshrate;

  for (int i=0; i<CFArrayGetCount(displayModes); ++i)
  {
    CFDictionaryRef displayMode = (CFDictionaryRef)CFArrayGetValueAtIndex(displayModes, i);

    stretched = GetDictionaryBoolean(displayMode, kCGDisplayModeIsStretched);
    interlaced = GetDictionaryBoolean(displayMode, kCGDisplayModeIsInterlaced);
    bitsperpixel = GetDictionaryInt(displayMode, kCGDisplayBitsPerPixel);
    safeForHardware = GetDictionaryBoolean(displayMode, kCGDisplayModeIsSafeForHardware);
    televisionoutput = GetDictionaryBoolean(displayMode, kCGDisplayModeIsTelevisionOutput);

    if((bitsperpixel == 32) && (safeForHardware == YES) && (stretched == NO) && (interlaced == NO))
    {
      width = GetDictionaryInt(displayMode, kCGDisplayWidth);
      height = GetDictionaryInt(displayMode, kCGDisplayHeight);
      refreshrate = GetDictionaryDouble(displayMode, kCGDisplayRefreshRate);
      if ((int)refreshrate == 0)  // LCD display?
        refreshrate = 150.0;      // Divisible by 25Hz and 30Hz to minimise AV sync waiting
    }
  }
}

#endif
