/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Application.h"
#include "DllPaths.h"
#include "GUIUserMessages.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "CompileInfo.h"

#if defined(TARGET_DARWIN_EMBEDDED)
  #import <Foundation/Foundation.h>
  #import <UIKit/UIKit.h>
  #import <mach/mach_host.h>
  #import <sys/sysctl.h>
#else
  #import <Cocoa/Cocoa.h>
  #import <CoreFoundation/CoreFoundation.h>
  #import <IOKit/IOKitLib.h>
#endif

#import "DarwinUtils.h"

#include <mutex>


// platform strings are based on http://theiphonewiki.com/wiki/Models
const char* CDarwinUtils::getIosPlatformString(void)
{
  static std::string iOSPlatformString;
  static std::once_flag flag;
  std::call_once(flag, []
  {
#if defined(TARGET_DARWIN_EMBEDDED)
    // Gets a string with the device model
    size_t size;
    sysctlbyname("hw.machine", nullptr, &size, nullptr, 0);
    char machine[size];
    if (sysctlbyname("hw.machine", machine, &size, nullptr, 0) == 0 && machine[0])
      iOSPlatformString.assign(machine, size-1);
    else
#endif
      iOSPlatformString = "unknown0,0";
  });
  return iOSPlatformString.c_str();
}

const char *CDarwinUtils::GetOSReleaseString(void)
{
  static std::string osreleaseStr;
  static std::once_flag flag;
  std::call_once(flag, []
  {
    size_t size;
    sysctlbyname("kern.osrelease", nullptr, &size, nullptr, 0);
    char osrelease[size];
    sysctlbyname("kern.osrelease", osrelease, &size, nullptr, 0);
    osreleaseStr.assign(osrelease);
  });
  return osreleaseStr.c_str();
}

const char *CDarwinUtils::GetOSVersionString(void)
{
  @autoreleasepool
  {
    return NSProcessInfo.processInfo.operatingSystemVersionString.UTF8String;
  }
}

const char* CDarwinUtils::GetVersionString()
{
  static std::string versionString;
  static std::once_flag flag;
#if defined(TARGET_DARWIN_EMBEDDED)
  std::call_once(flag, []
  {
    versionString.assign(UIDevice.currentDevice.systemVersion.UTF8String);
  });
#else
  std::call_once(flag, []
  {
    versionString.assign([[[NSDictionary dictionaryWithContentsOfFile:
                            @"/System/Library/CoreServices/SystemVersion.plist"] objectForKey:@"ProductVersion"] UTF8String]);
  });
#endif
  return versionString.c_str();
}

std::string CDarwinUtils::GetFrameworkPath(bool forPython)
{
  @autoreleasepool
  {
    auto mainBundle = NSBundle.mainBundle;
#if defined(TARGET_DARWIN_EMBEDDED)
    return std::string{mainBundle.privateFrameworksPath.UTF8String};
#else
    if ([mainBundle.executablePath containsString:@"Contents"])
    {
      // ExecutablePath is <product>.app/Contents/MacOS/<executable>
      // we should have <product>.app/Contents/Libraries
      return std::string{[[mainBundle.bundlePath stringByAppendingPathComponent:@"Contents"]
                             stringByAppendingPathComponent:@"Libraries"]
                             .UTF8String};
    }
#endif
  }

  // Kodi OSX binary running under xcode or command-line
  // but only if it's not for python. In this case, let python
  // use it's internal compiled paths.
#if defined(TARGET_DARWIN_OSX)
  if (!forPython)
    return std::string{PREFIX_USR_PATH"/lib"};
  return std::string{};
#endif
}

namespace
{
NSString* getExecutablePath()
{
  return NSBundle.mainBundle.executablePath;
}
}

int  CDarwinUtils::GetExecutablePath(char* path, size_t *pathsize)
{
  @autoreleasepool
  {
    strcpy(path, getExecutablePath().UTF8String);
  }
  *pathsize = strlen(path);

  return 0;
}

void CDarwinUtils::SetScheduling(bool realtime)
{
  int policy;
  struct sched_param param;
  pthread_t this_pthread_self = pthread_self();

  pthread_getschedparam(this_pthread_self, &policy, &param );

  policy = SCHED_OTHER;
  thread_extended_policy_data_t theFixedPolicy={true};

  if (realtime)
  {
    policy = SCHED_RR;
    theFixedPolicy.timeshare = false;
  }

  thread_policy_set(pthread_mach_thread_np(this_pthread_self),
    THREAD_EXTENDED_POLICY,
    (thread_policy_t)&theFixedPolicy,
    THREAD_EXTENDED_POLICY_COUNT);

  pthread_setschedparam(this_pthread_self, policy, &param );
}

bool CFStringRefToStringWithEncoding(CFStringRef source, std::string &destination, CFStringEncoding encoding)
{
  const char *cstr = CFStringGetCStringPtr(source, encoding);
  if (!cstr)
  {
    CFIndex strLen = CFStringGetMaximumSizeForEncoding(CFStringGetLength(source) + 1,
                                                       encoding);
    char* allocStr = static_cast<char*>(malloc(strLen));

    if(!allocStr)
      return false;

    if(!CFStringGetCString(source, allocStr, strLen, encoding))
    {
      free(static_cast<void*>(allocStr));
      return false;
    }

    destination = allocStr;
    free(static_cast<void*>(allocStr));

    return true;
  }

  destination = cstr;
  return true;
}


bool CDarwinUtils::CFStringRefToString(CFStringRef source, std::string &destination)
{
  return CFStringRefToStringWithEncoding(source, destination, CFStringGetSystemEncoding());
}

bool CDarwinUtils::CFStringRefToUTF8String(CFStringRef source, std::string &destination)
{
  return CFStringRefToStringWithEncoding(source, destination, kCFStringEncodingUTF8);
}

const std::string& CDarwinUtils::GetManufacturer(void)
{
  static std::string manufName;
  if (manufName.empty())
  {
#ifdef TARGET_DARWIN_EMBEDDED
    // to avoid dlloading of IOIKit, hardcode return value
	// until other than Apple devices with iOS will be released
    manufName = "Apple Inc.";
#elif defined(TARGET_DARWIN_OSX)
    const CFMutableDictionaryRef matchExpDev = IOServiceMatching("IOPlatformExpertDevice");
    if (matchExpDev)
    {
      const io_service_t servExpDev = IOServiceGetMatchingService(kIOMasterPortDefault, matchExpDev);
      if (servExpDev)
      {
        CFTypeRef manufacturer = IORegistryEntryCreateCFProperty(servExpDev, CFSTR("manufacturer"), kCFAllocatorDefault, 0);
        if (manufacturer)
        {
          auto typeId = CFGetTypeID(manufacturer);
          if (typeId == CFStringGetTypeID())
            manufName = static_cast<const char*>([NSString stringWithString:(__bridge NSString*)manufacturer].UTF8String);
          else if (typeId == CFDataGetTypeID())
          {
            manufName.assign(reinterpret_cast<const char*>(CFDataGetBytePtr((CFDataRef)manufacturer)), CFDataGetLength((CFDataRef)manufacturer));
            if (!manufName.empty() && manufName[manufName.length() - 1] == 0)
              manufName.erase(manufName.length() - 1); // remove extra null at the end if any
          }
          CFRelease(manufacturer);
        }
      }
      IOObjectRelease(servExpDev);
    }
#endif // TARGET_DARWIN_OSX
  }
  return manufName;
}

bool CDarwinUtils::IsAliasShortcut(const std::string& path, bool isdirectory)
{
  bool ret = false;

#if defined(TARGET_DARWIN_OSX)
  @autoreleasepool
  {
    NSURL* nsUrl;
    if (isdirectory)
    {
      std::string cleanpath = path;
      URIUtils::RemoveSlashAtEnd(cleanpath);
      NSString* nsPath = [NSString stringWithUTF8String:cleanpath.c_str()];
      nsUrl = [NSURL fileURLWithPath:nsPath isDirectory:YES];
    }
    else
    {
      NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
      nsUrl = [NSURL fileURLWithPath:nsPath isDirectory:NO];
    }

    if (nsUrl != nil)
    {
      NSNumber* wasAliased;
      if ([nsUrl getResourceValue:&wasAliased forKey:NSURLIsAliasFileKey error:nil])
        ret = [wasAliased boolValue];
    }
  }
#endif
  return ret;
}

void CDarwinUtils::TranslateAliasShortcut(std::string& path)
{
#if defined(TARGET_DARWIN_OSX)
  NSString *nsPath = [NSString stringWithUTF8String:path.c_str()];
  NSURL *nsUrl = [NSURL fileURLWithPath:nsPath];

  if (nsUrl != nil)
  {
    NSError *error = nil;
    NSData * bookmarkData = [NSURL bookmarkDataWithContentsOfURL:nsUrl error:&error];
    if (bookmarkData)
    {
      BOOL isStale = NO;
      NSURLBookmarkResolutionOptions options = NSURLBookmarkResolutionWithoutUI |
                                               NSURLBookmarkResolutionWithoutMounting;

      NSURL* resolvedURL = [NSURL URLByResolvingBookmarkData:bookmarkData
                                                     options:options
                                               relativeToURL:nil
                                         bookmarkDataIsStale:&isStale
                                                       error:&error];
      if (resolvedURL)
      {
        // [resolvedURL path] returns a path as /dir/dir/file ...
        path = (const char*)[[resolvedURL path] UTF8String];
      }
    }
  }
#endif
}

bool CDarwinUtils::CreateAliasShortcut(const std::string& fromPath, const std::string& toPath)
{
  bool ret = false;
#if defined(TARGET_DARWIN_OSX)
  NSString *nsToPath = [NSString stringWithUTF8String:toPath.c_str()];
  NSURL *toUrl = [NSURL fileURLWithPath:nsToPath];
  NSString *nsFromPath = [NSString stringWithUTF8String:fromPath.c_str()];
  NSURL *fromUrl = [NSURL fileURLWithPath:nsFromPath];
  NSError *error = nil;
  NSData *bookmarkData = [toUrl bookmarkDataWithOptions: NSURLBookmarkCreationSuitableForBookmarkFile includingResourceValuesForKeys:nil relativeToURL:nil error:&error];

  if(bookmarkData != nil && fromUrl != nil && toUrl != nil)
  {
    if([NSURL writeBookmarkData:bookmarkData toURL:fromUrl options:NSURLBookmarkCreationSuitableForBookmarkFile error:&error])
    {
      ret = true;
    }
  }
#endif
  return ret;
}
