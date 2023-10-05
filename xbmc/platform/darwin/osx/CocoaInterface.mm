/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#import "CocoaInterface.h"

#include "CompileInfo.h"
#import "DllPaths_generated.h"
#include "ServiceBroker.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "windowing/osx/WinSystemOSX.h"

#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>
#import <QuartzCore/QuartzCore.h>
#import <sys/mount.h>
#import <unistd.h>


//display link for display management
static CVDisplayLinkRef displayLink = NULL;

CGDirectDisplayID Cocoa_GetDisplayIDFromScreen(NSScreen *screen);

uint32_t Cocoa_GL_GetCurrentDisplayID(void)
{
  // Find which display we are on from the current context (default to main display)
  CGDirectDisplayID display_id = kCGDirectMainDisplay;

  NSNumber* __block screenID;
  auto getScreenNumber = ^{
    screenID =
        NSApplication.sharedApplication.keyWindow.screen.deviceDescription[@"NSScreenNumber"];
  };
  if (NSThread.isMainThread)
    getScreenNumber();
  else
    dispatch_sync(dispatch_get_main_queue(), getScreenNumber);
  if (screenID)
    display_id = static_cast<CGDirectDisplayID>(screenID.unsignedIntValue);

  return static_cast<uint32_t>(display_id);
}

bool Cocoa_CVDisplayLinkCreate(void *displayLinkcallback, void *displayLinkContext)
{
  CVReturn status = kCVReturnError;
  CGDirectDisplayID display_id;

  // OpenGL Flush synchronised with vertical retrace
  GLint swapInterval = 1;
  [[NSOpenGLContext currentContext] setValues:&swapInterval
                                 forParameter:NSOpenGLContextParameterSwapInterval];

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

void Cocoa_DoAppleScript(const char* scriptSource)
{
  @autoreleasepool
  {
    auto scriptObject =
        [[NSAppleScript alloc] initWithSource:[NSString stringWithUTF8String:scriptSource]];
    [scriptObject executeAndReturnError:nil];
  }
}

void Cocoa_DoAppleScriptFile(const char* filePath)
{
  NSString* scriptFile = [NSString stringWithUTF8String:filePath];
  NSString* appName = [NSString stringWithUTF8String:CCompileInfo::GetAppName()];
  NSMutableString *tmpStr = [NSMutableString stringWithString:@"~/Library/Application Support/"];
  [tmpStr appendString:appName];
  [tmpStr appendString:@"/scripts"];
  NSString* userScriptsPath = [tmpStr stringByExpandingTildeInPath];
  [tmpStr setString:@"Contents/Resources/"];
  [tmpStr appendString:appName];
  [tmpStr appendString:@"/scripts"];
  NSString* bundleScriptsPath = [[[NSBundle mainBundle] bundlePath] stringByAppendingPathComponent:tmpStr];
  [tmpStr setString:@"Contents/Resources/"];
  [tmpStr appendString:appName];
  [tmpStr appendString:@"/system/AppleScripts"];
  NSString* bundleSysScriptsPath = [[[NSBundle mainBundle] bundlePath] stringByAppendingPathComponent:tmpStr];

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
}

char* Cocoa_MountPoint2DeviceName(char *path)
{
  // if physical DVDs, libdvdnav wants "/dev/rdiskN" device name for OSX,
  // path will get realloc'ed and replaced IF this is a physical DVD.
  char* strDVDDevice;
  strDVDDevice = strdup(path);
  if (StringUtils::CompareNoCase(strDVDDevice, "/Volumes/", 9) == 0)
  {
    struct statfs *mntbufp;
    int i, mounts;

    // find a match for /Volumes/<disk name>
    mounts = getmntinfo(&mntbufp, MNT_WAIT);  // NOT THREAD SAFE!
    for (i = 0; i < mounts; i++)
    {
      if (!StringUtils::CompareNoCase(mntbufp[i].f_mntonname, strDVDDevice))
      {
        // Replace "/dev/" with "/dev/r"
        path = (char*)realloc(path, strlen(mntbufp[i].f_mntfromname) + 2 );
        strcpy( path, "/dev/r" );
        strcat( path, mntbufp[i].f_mntfromname + strlen( "/dev/" ) );
        break;
      }
    }
  }
  free(strDVDDevice);
  return path;
}

bool Cocoa_GetVolumeNameFromMountPoint(const std::string &mountPoint, std::string &volumeName)
{
  @autoreleasepool
  {
    NSFileManager* fm = [NSFileManager defaultManager];
    NSArray* mountedVolumeUrls =
        [fm mountedVolumeURLsIncludingResourceValuesForKeys:@[ NSURLVolumeNameKey, NSURLPathKey ]
                                                    options:0];
    bool resolved = false;

    for (NSURL* volumeURL in mountedVolumeUrls)
    {
      NSString* path;
      BOOL success = [volumeURL getResourceValue:&path forKey:NSURLPathKey error:nil];

      if (success && path != nil)
      {
        std::string mountpoint = [path UTF8String];
        if (mountpoint == mountPoint)
        {
          NSString* name;
          success = [volumeURL getResourceValue:&name forKey:NSURLVolumeNameKey error:nil];
          if (success && name != nil)
          {
            volumeName = [name UTF8String];
            resolved = true;
            break;
          }
        }
      }
    }
    return resolved;
  }
}

//---------------------------------------------------------------------------------
const char *Cocoa_Paste()
{
  NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
  NSString* type =
      [pasteboard availableTypeFromArray:[NSArray arrayWithObject:NSPasteboardTypeString]];
  if (type != nil) {
    NSString *contents = [pasteboard stringForType:type];
    if (contents != nil) {
      return [contents UTF8String];
    }
  }

  return NULL;
}
