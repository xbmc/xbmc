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

#if defined(TARGET_DARWIN_IOS)
  #import <Foundation/Foundation.h>
  #import <UIKit/UIKit.h>
  #import <mach/mach_host.h>
  #import <sys/sysctl.h>
#else
  #import <Cocoa/Cocoa.h>
  #import <CoreFoundation/CoreFoundation.h>
  #import <IOKit/IOKitLib.h>
  #import <IOKit/ps/IOPowerSources.h>
  #import <IOKit/ps/IOPSKeys.h>
#endif

#import "AutoPool.h"
#import "DarwinUtils.h"


enum iosPlatform
{
  iDeviceUnknown = -1,
  iPhone2G,
  iPhone3G,
  iPhone3GS,
  iPodTouch1G,
  iPodTouch2G,
  iPodTouch3G,
  iPad,
  iPad3G,
  iPad2WIFI,
  iPad2CDMA,
  iPad2,
  iPadMini,
  iPadMiniGSMCDMA,
  iPadMiniWIFI,
  AppleTV2,
  AppleTV4,
  AppleTV4K,
  iPhone4,            //from here on list devices with retina support (e.x. mainscreen scale == 2.0)
  iPhone4CDMA,
  iPhone4S,
  iPhone5,
  iPhone5GSMCDMA,
  iPhone5CGSM,
  iPhone5CGlobal,
  iPhone5SGSM,
  iPhone5SGlobal,
  iPodTouch4G,
  iPodTouch5G,
  iPodTouch6G,
  iPad3WIFI,
  iPad3GSMCDMA,
  iPad3,
  iPad4WIFI,
  iPad4,
  iPad4GSMCDMA,
  iPad5Wifi,
  iPad5Cellular,
  iPadAirWifi,
  iPadAirCellular,
  iPadAirTDLTE,
  iPadMini2Wifi,
  iPadMini2Cellular,
  iPhone6,
  iPhone6s,
  iPhoneSE,
  iPhone7,
  iPhone8,
  iPhoneXR,
  iPhone11,
  iPadAir2Wifi,
  iPadAir2Cellular,
  iPadPro9_7InchWifi,
  iPadPro9_7InchCellular,
  iPad6thGeneration9_7InchWifi,
  iPad6thGeneration9_7InchCellular,
  iPad7thGeneration10_2InchWifi,
  iPad7thGeneration10_2InchCellular,
  iPadPro12_9InchWifi,
  iPadPro12_9InchCellular,
  iPadPro2_12_9InchWifi,
  iPadPro2_12_9InchCellular,
  iPadPro3_12_9InchWifi,
  iPadPro3_12_9InchCellular,
  iPadPro_10_5InchWifi,
  iPadPro_10_5InchCellular,
  iPadPro11InchWifi,
  iPadPro11InchCellular,
  iPadMini3Wifi,
  iPadMini3Cellular,
  iPadMini4Wifi,
  iPadMini4Cellular,
  iPhone6Plus,        //from here on list devices with retina support which have scale == 3.0
  iPhone6sPlus,
  iPhone7Plus,
  iPhone8Plus,
  iPhoneX,
  iPhoneXS,
  iPhoneXSMax,
  iPhone11Pro,
  iPhone11ProMax,
};

// platform strings are based on http://theiphonewiki.com/wiki/Models
const char* CDarwinUtils::getIosPlatformString(void)
{
  static std::string iOSPlatformString;
  if (iOSPlatformString.empty())
  {
#if defined(TARGET_DARWIN_IOS)
    // Gets a string with the device model
    size_t size;
    sysctlbyname("hw.machine", NULL, &size, NULL, 0);
    char *machine = new char[size];
    if (sysctlbyname("hw.machine", machine, &size, NULL, 0) == 0 && machine[0])
      iOSPlatformString.assign(machine, size -1);
   else
#endif
      iOSPlatformString = "unknown0,0";

#if defined(TARGET_DARWIN_IOS)
    delete [] machine;
#endif
  }

  return iOSPlatformString.c_str();
}

enum iosPlatform getIosPlatform()
{
  static enum iosPlatform eDev = iDeviceUnknown;
#if defined(TARGET_DARWIN_IOS)
  if (eDev == iDeviceUnknown)
  {
    std::string devStr(CDarwinUtils::getIosPlatformString());

    if (devStr == "iPhone1,1") eDev = iPhone2G;
    else if (devStr == "iPhone1,2") eDev = iPhone3G;
    else if (devStr == "iPhone2,1") eDev = iPhone3GS;
    else if (devStr == "iPhone3,1") eDev = iPhone4;
    else if (devStr == "iPhone3,2") eDev = iPhone4;
    else if (devStr == "iPhone3,3") eDev = iPhone4CDMA;
    else if (devStr == "iPhone4,1") eDev = iPhone4S;
    else if (devStr == "iPhone5,1") eDev = iPhone5;
    else if (devStr == "iPhone5,2") eDev = iPhone5GSMCDMA;
    else if (devStr == "iPhone5,3") eDev = iPhone5CGSM;
    else if (devStr == "iPhone5,4") eDev = iPhone5CGlobal;
    else if (devStr == "iPhone6,1") eDev = iPhone5SGSM;
    else if (devStr == "iPhone6,2") eDev = iPhone5SGlobal;
    else if (devStr == "iPhone7,1") eDev = iPhone6Plus;
    else if (devStr == "iPhone7,2") eDev = iPhone6;
    else if (devStr == "iPhone8,1") eDev = iPhone6s;
    else if (devStr == "iPhone8,2") eDev = iPhone6sPlus;
    else if (devStr == "iPhone8,4") eDev = iPhoneSE;
    else if (devStr == "iPhone9,1") eDev = iPhone7;
    else if (devStr == "iPhone9,2") eDev = iPhone7Plus;
    else if (devStr == "iPhone9,3") eDev = iPhone7;
    else if (devStr == "iPhone9,4") eDev = iPhone7Plus;
    else if (devStr == "iPhone10,1") eDev = iPhone8;
    else if (devStr == "iPhone10,2") eDev = iPhone8Plus;
    else if (devStr == "iPhone10,3") eDev = iPhoneX;
    else if (devStr == "iPhone10,4") eDev = iPhone8;
    else if (devStr == "iPhone10,5") eDev = iPhone8Plus;
    else if (devStr == "iPhone10,6") eDev = iPhoneX;
    else if (devStr == "iPhone11,2") eDev = iPhoneXS;
    else if (devStr == "iPhone11,6") eDev = iPhoneXSMax;
    else if (devStr == "iPhone11,8") eDev = iPhoneXR;
    else if (devStr == "iPhone12,1") eDev = iPhone11;
    else if (devStr == "iPhone12,3") eDev = iPhone11Pro;
    else if (devStr == "iPhone12,5") eDev = iPhone11ProMax;
    else if (devStr == "iPod1,1") eDev = iPodTouch1G;
    else if (devStr == "iPod2,1") eDev = iPodTouch2G;
    else if (devStr == "iPod3,1") eDev = iPodTouch3G;
    else if (devStr == "iPod4,1") eDev = iPodTouch4G;
    else if (devStr == "iPod5,1") eDev = iPodTouch5G;
    else if (devStr == "iPod7,1") eDev = iPodTouch6G;
    else if (devStr == "iPad1,1") eDev = iPad;
    else if (devStr == "iPad1,2") eDev = iPad;
    else if (devStr == "iPad2,1") eDev = iPad2WIFI;
    else if (devStr == "iPad2,2") eDev = iPad2;
    else if (devStr == "iPad2,3") eDev = iPad2CDMA;
    else if (devStr == "iPad2,4") eDev = iPad2;
    else if (devStr == "iPad2,5") eDev = iPadMiniWIFI;
    else if (devStr == "iPad2,6") eDev = iPadMini;
    else if (devStr == "iPad2,7") eDev = iPadMiniGSMCDMA;
    else if (devStr == "iPad3,1") eDev = iPad3WIFI;
    else if (devStr == "iPad3,2") eDev = iPad3GSMCDMA;
    else if (devStr == "iPad3,3") eDev = iPad3;
    else if (devStr == "iPad3,4") eDev = iPad4WIFI;
    else if (devStr == "iPad3,5") eDev = iPad4;
    else if (devStr == "iPad3,6") eDev = iPad4GSMCDMA;
    else if (devStr == "iPad4,1") eDev = iPadAirWifi;
    else if (devStr == "iPad4,2") eDev = iPadAirCellular;
    else if (devStr == "iPad4,3") eDev = iPadAirTDLTE;
    else if (devStr == "iPad4,4") eDev = iPadMini2Wifi;
    else if (devStr == "iPad4,5") eDev = iPadMini2Cellular;
    else if (devStr == "iPad4,6") eDev = iPadMini2Cellular;
    else if (devStr == "iPad4,7") eDev = iPadMini3Wifi;
    else if (devStr == "iPad4,8") eDev = iPadMini3Cellular;
    else if (devStr == "iPad4,9") eDev = iPadMini3Cellular;
    else if (devStr == "iPad5,1") eDev = iPadMini4Wifi;
    else if (devStr == "iPad5,2") eDev = iPadMini4Cellular;
    else if (devStr == "iPad5,3") eDev = iPadAir2Wifi;
    else if (devStr == "iPad5,4") eDev = iPadAir2Cellular;
    else if (devStr == "iPad6,3") eDev = iPadPro9_7InchWifi;
    else if (devStr == "iPad6,4") eDev = iPadPro9_7InchCellular;
    else if (devStr == "iPad6,7") eDev = iPadPro12_9InchWifi;
    else if (devStr == "iPad6,8") eDev = iPadPro12_9InchCellular;
    else if (devStr == "iPad6,11") eDev = iPad5Wifi;
    else if (devStr == "iPad6,12") eDev = iPad5Cellular;
    else if (devStr == "iPad7,1") eDev = iPadPro2_12_9InchWifi;
    else if (devStr == "iPad7,2") eDev = iPadPro2_12_9InchCellular;
    else if (devStr == "iPad7,3") eDev = iPadPro_10_5InchWifi;
    else if (devStr == "iPad7,4") eDev = iPadPro_10_5InchCellular;
    else if (devStr == "iPad7,5") eDev = iPad6thGeneration9_7InchWifi;
    else if (devStr == "iPad7,6") eDev = iPad6thGeneration9_7InchCellular;
    else if (devStr == "iPad7,11") eDev = iPad7thGeneration10_2InchWifi;
    else if (devStr == "iPad7,12") eDev = iPad7thGeneration10_2InchCellular;
    else if (devStr == "iPad8,1") eDev = iPadPro11InchWifi;
    else if (devStr == "iPad8,2") eDev = iPadPro11InchWifi;
    else if (devStr == "iPad8,3") eDev = iPadPro11InchCellular;
    else if (devStr == "iPad8,4") eDev = iPadPro11InchCellular;
    else if (devStr == "iPad8,5") eDev = iPadPro3_12_9InchWifi;
    else if (devStr == "iPad8,6") eDev = iPadPro3_12_9InchWifi;
    else if (devStr == "iPad8,7") eDev = iPadPro3_12_9InchCellular;
    else if (devStr == "iPad8,8") eDev = iPadPro3_12_9InchCellular;
    else if (devStr == "AppleTV2,1") eDev = AppleTV2;
    else if (devStr == "AppleTV5,3") eDev = AppleTV4;
    else if (devStr == "AppleTV6,2") eDev = AppleTV4K;
  }
#endif
  return eDev;
}

bool CDarwinUtils::DeviceHasRetina(double &scale)
{
  static enum iosPlatform platform = iDeviceUnknown;

#if defined(TARGET_DARWIN_IOS)
  if( platform == iDeviceUnknown )
  {
    platform = getIosPlatform();
  }
#endif
  scale = 1.0; //no retina

  // see http://www.paintcodeapp.com/news/iphone-6-screens-demystified
  if (platform >= iPhone4 && platform < iPhone6Plus)
  {
    scale = 2.0; // 2x render retina
  }

  if (platform >= iPhone6Plus)
  {
    scale = 3.0; //3x render retina + downscale
  }

  return (platform >= iPhone4);
}

const char *CDarwinUtils::GetOSReleaseString(void)
{
  static std::string osreleaseStr;
  if (osreleaseStr.empty())
  {
    size_t size;
    sysctlbyname("kern.osrelease", NULL, &size, NULL, 0);
    char *osrelease = new char[size];
    sysctlbyname("kern.osrelease", osrelease, &size, NULL, 0);
    osreleaseStr = osrelease;
    delete [] osrelease;
  }
  return osreleaseStr.c_str();
}

const char *CDarwinUtils::GetOSVersionString(void)
{
  CCocoaAutoPool pool;
  return [[[NSProcessInfo processInfo] operatingSystemVersionString] UTF8String];
}

float CDarwinUtils::GetIOSVersion(void)
{
  CCocoaAutoPool pool;
  float version;
#if defined(TARGET_DARWIN_IOS)
  version = [[[UIDevice currentDevice] systemVersion] floatValue];
#else
  version = 0.0f;
#endif

  return(version);
}

const char *CDarwinUtils::GetIOSVersionString(void)
{
#if defined(TARGET_DARWIN_IOS)
  static std::string iOSVersionString;
  if (iOSVersionString.empty())
  {
    CCocoaAutoPool pool;
    iOSVersionString.assign((const char*)[[[UIDevice currentDevice] systemVersion] UTF8String]);
  }
  return iOSVersionString.c_str();
#else
  return "0.0";
#endif
}

const char *CDarwinUtils::GetOSXVersionString(void)
{
#if defined(TARGET_DARWIN_OSX)
  static std::string OSXVersionString;
  if (OSXVersionString.empty())
  {
    CCocoaAutoPool pool;
    OSXVersionString.assign((const char*)[[[NSDictionary dictionaryWithContentsOfFile:
                         @"/System/Library/CoreServices/SystemVersion.plist"] objectForKey:@"ProductVersion"] UTF8String]);
  }

  return OSXVersionString.c_str();
#else
  return "0.0";
#endif
}

int  CDarwinUtils::GetFrameworkPath(bool forPython, char* path, size_t *pathsize)
{
  CCocoaAutoPool pool;
  // see if we can figure out who we are
  NSString *pathname;

  path[0] = 0;
  *pathsize = 0;

  // 1) Kodi application running under IOS
  pathname = [[NSBundle mainBundle] executablePath];
  std::string appName = std::string(CCompileInfo::GetAppName()) + ".app/" + std::string(CCompileInfo::GetAppName());
  if (pathname && strstr([pathname UTF8String], appName.c_str()))
  {
    strcpy(path, [pathname UTF8String]);
    // Move backwards to last "/"
    for (int n=strlen(path)-1; path[n] != '/'; n--)
      path[n] = '\0';
    strcat(path, "Frameworks");
    *pathsize = strlen(path);
    //CLog::Log(LOGDEBUG, "DarwinFrameworkPath(c) -> %s", path);
    return 0;
  }

  // 2) Kodi application running under OSX
  pathname = [[NSBundle mainBundle] executablePath];
  if (pathname && strstr([pathname UTF8String], "Contents"))
  {
    strcpy(path, [pathname UTF8String]);
    // ExecutablePath is <product>.app/Contents/MacOS/<executable>
    char *lastSlash = strrchr(path, '/');
    if (lastSlash)
    {
      *lastSlash = '\0';//remove /<executable>
      lastSlash = strrchr(path, '/');
      if (lastSlash)
        *lastSlash = '\0';//remove /MacOS
    }
    strcat(path, "/Libraries");//add /Libraries
    //we should have <product>.app/Contents/Libraries now
    *pathsize = strlen(path);
    //CLog::Log(LOGDEBUG, "DarwinFrameworkPath(d) -> %s", path);
    return 0;
  }

  // e) Kodi OSX binary running under xcode or command-line
  // but only if it's not for python. In this case, let python
  // use it's internal compiled paths.
  if (!forPython)
  {
    strcpy(path, PREFIX_USR_PATH);
    strcat(path, "/lib");
    *pathsize = strlen(path);
    //CLog::Log(LOGDEBUG, "DarwinFrameworkPath(e) -> %s", path);
    return 0;
  }

  return -1;
}

int  CDarwinUtils::GetExecutablePath(char* path, size_t *pathsize)
{
  CCocoaAutoPool pool;
  // see if we can figure out who we are
  NSString *pathname;

  // 1) Kodi application running under IOS
  // 2) Kodi application running under OSX
  pathname = [[NSBundle mainBundle] executablePath];
  strcpy(path, [pathname UTF8String]);
  *pathsize = strlen(path);
  //CLog::Log(LOGDEBUG, "DarwinExecutablePath(b/c) -> %s", path);

  return 0;
}

const char* CDarwinUtils::GetAppRootFolder(void)
{
  static std::string rootFolder = "";
  if ( rootFolder.length() == 0)
  {
    if (IsIosSandboxed())
    {
      // when we are sandbox make documents our root
      // so that user can access everything he needs
      // via itunes sharing
      rootFolder = "Documents";
    }
    else
    {
      rootFolder = "Library/Preferences";
    }
  }
  return rootFolder.c_str();
}

bool CDarwinUtils::IsIosSandboxed(void)
{
  static int ret = -1;
  if (ret == -1)
  {
    size_t path_size = 2*MAXPATHLEN;
    char     given_path[2*MAXPATHLEN];
    int      result = -1;
    ret = 0;
    memset(given_path, 0x0, path_size);
    /* Get Application directory */
    result = GetExecutablePath(given_path, &path_size);
    if (result == 0)
    {
      // we re sandboxed if we are installed in /var/mobile/Applications
      if (strlen("/var/mobile/Applications/") < path_size &&
        strncmp(given_path, "/var/mobile/Applications/", strlen("/var/mobile/Applications/")) == 0)
      {
        ret = 1;
      }

      // since ios8 the sandbox filesystem has moved to container approach
      // we are also sandboxed if this is our bundle path
      if (strlen("/var/mobile/Containers/Bundle/") < path_size &&
        strncmp(given_path, "/var/mobile/Containers/Bundle/", strlen("/var/mobile/Containers/Bundle/")) == 0)
      {
        ret = 1;
      }

      // Some time after ios8, Apple decided to change this yet again
      if (strlen("/var/containers/Bundle/") < path_size &&
        strncmp(given_path, "/var/containers/Bundle/", strlen("/var/containers/Bundle/")) == 0)
      {
        ret = 1;
      }

      // since iOS 13
      if (strlen("/private/var/containers/Bundle/") < path_size &&
        strncmp(given_path, "/private/var/containers/Bundle/", strlen("/private/var/containers/Bundle/")) == 0)
      {
        ret = 1;
      }
    }
  }
  return ret == 1;
}

int CDarwinUtils::BatteryLevel(void)
{
  float batteryLevel = 0;
#if defined(TARGET_DARWIN_IOS)
  batteryLevel = [[UIDevice currentDevice] batteryLevel];
#else
  CFTypeRef powerSourceInfo = IOPSCopyPowerSourcesInfo();
  CFArrayRef powerSources = IOPSCopyPowerSourcesList(powerSourceInfo);

  CFDictionaryRef powerSource = NULL;
  const void *powerSourceVal;

  for (int i = 0 ; i < CFArrayGetCount(powerSources) ; i++)
  {
    powerSource = IOPSGetPowerSourceDescription(powerSourceInfo, CFArrayGetValueAtIndex(powerSources, i));
    if (!powerSource) break;

    powerSourceVal = (CFStringRef)CFDictionaryGetValue(powerSource, CFSTR(kIOPSNameKey));

    int curLevel = 0;
    int maxLevel = 0;

    powerSourceVal = CFDictionaryGetValue(powerSource, CFSTR(kIOPSCurrentCapacityKey));
    CFNumberGetValue((CFNumberRef)powerSourceVal, kCFNumberSInt32Type, &curLevel);

    powerSourceVal = CFDictionaryGetValue(powerSource, CFSTR(kIOPSMaxCapacityKey));
    CFNumberGetValue((CFNumberRef)powerSourceVal, kCFNumberSInt32Type, &maxLevel);

    batteryLevel = (double)curLevel/(double)maxLevel;
  }
  CFRelease(powerSources);
  CFRelease(powerSourceInfo);
#endif
  return batteryLevel * 100;
}

void CDarwinUtils::EnableOSScreenSaver(bool enable)
{

}

void CDarwinUtils::ResetSystemIdleTimer()
{

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
    char *allocStr = (char*)malloc(strLen);

    if(!allocStr)
      return false;

    if(!CFStringGetCString(source, allocStr, strLen, encoding))
    {
      free((void*)allocStr);
      return false;
    }

    destination = allocStr;
    free((void*)allocStr);

    return true;
  }

  destination = cstr;
  return true;
}

void CDarwinUtils::PrintDebugString(std::string debugString)
{
  NSLog(@"Debug Print: %s", debugString.c_str());
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
#ifdef TARGET_DARWIN_IOS
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
          if (CFGetTypeID(manufacturer) == CFStringGetTypeID())
            manufName = (const char*)[[NSString stringWithString:(NSString *)manufacturer] UTF8String];
          else if (CFGetTypeID(manufacturer) == CFDataGetTypeID())
          {
            manufName.assign((const char*)CFDataGetBytePtr((CFDataRef)manufacturer), CFDataGetLength((CFDataRef)manufacturer));
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
  CCocoaAutoPool pool;

  NSURL *nsUrl;
  if (isdirectory)
  {
    std::string cleanpath = path;
    URIUtils::RemoveSlashAtEnd(cleanpath);
    NSString *nsPath = [NSString stringWithUTF8String:cleanpath.c_str()];
    nsUrl = [NSURL fileURLWithPath:nsPath isDirectory:TRUE];
  }
  else
  {
    NSString *nsPath = [NSString stringWithUTF8String:path.c_str()];
    nsUrl = [NSURL fileURLWithPath:nsPath isDirectory:FALSE];
  }

  NSNumber* wasAliased = nil;

  if (nsUrl != nil)
  {
    NSError *error = nil;

    if ([nsUrl getResourceValue:&wasAliased forKey:NSURLIsAliasFileKey error:&error])
    {
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
