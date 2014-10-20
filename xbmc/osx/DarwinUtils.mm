/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#define BOOL XBMC_BOOL 
#include "system.h"
#include "Application.h"
#include "DllPaths.h"
#include "GUIUserMessages.h"
#include "utils/log.h"

#undef BOOL

#if defined(TARGET_DARWIN)
#if defined(TARGET_DARWIN_IOS)
  #import <Foundation/Foundation.h>
  #import <UIKit/UIKit.h>
  #import <mach/mach_host.h>
  #import <sys/sysctl.h>
#else
  #import <Cocoa/Cocoa.h>
  #import <CoreFoundation/CoreFoundation.h>
  #import <IOKit/ps/IOPowerSources.h>
  #import <IOKit/ps/IOPSKeys.h>
#endif

#import "AutoPool.h"
#import "DarwinUtils.h"

#ifndef NSAppKitVersionNumber10_5
#define NSAppKitVersionNumber10_5 949
#endif

#ifndef NSAppKitVersionNumber10_6
#define NSAppKitVersionNumber10_6 1038
#endif

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
  iPad3WIFI,
  iPad3GSMCDMA,
  iPad3,
  iPad4WIFI,
  iPad4,
  iPad4GSMCDMA,
  iPadAirWifi,
  iPadAirCellular,
  iPadMini2Wifi,
  iPadMini2Cellular,
  iPhone6,
  iPadAir2Wifi,
  iPadAir2Cellular,
  iPadMini3Wifi,
  iPadMini3Cellular,
  iPhone6Plus,        //from here on list devices with retina support which have scale == 3.0
};

// platform strings are based on http://theiphonewiki.com/wiki/Models
enum iosPlatform getIosPlatform()
{
#if defined(TARGET_DARWIN_IOS)
  // Gets a string with the device model
  size_t size;  
  sysctlbyname("hw.machine", NULL, &size, NULL, 0);  
  char *machine = new char[size];  
  sysctlbyname("hw.machine", machine, &size, NULL, 0);  
  NSString *platform = [NSString stringWithCString:machine encoding:NSUTF8StringEncoding];  
  delete [] machine; 
  
  if ([platform isEqualToString:@"iPhone1,1"])    return iPhone2G;
  if ([platform isEqualToString:@"iPhone1,2"])    return iPhone3G;
  if ([platform isEqualToString:@"iPhone2,1"])    return iPhone3GS;
  if ([platform isEqualToString:@"iPhone3,1"])    return iPhone4;
  if ([platform isEqualToString:@"iPhone3,2"])    return iPhone4;
  if ([platform isEqualToString:@"iPhone3,3"])    return iPhone4CDMA;    
  if ([platform isEqualToString:@"iPhone4,1"])    return iPhone4S;
  if ([platform isEqualToString:@"iPhone5,1"])    return iPhone5;
  if ([platform isEqualToString:@"iPhone5,2"])    return iPhone5GSMCDMA;
  if ([platform isEqualToString:@"iPhone5,3"])    return iPhone5CGSM;
  if ([platform isEqualToString:@"iPhone5,4"])    return iPhone5CGlobal;
  if ([platform isEqualToString:@"iPhone6,1"])    return iPhone5SGSM;
  if ([platform isEqualToString:@"iPhone6,2"])    return iPhone5SGlobal;
  if ([platform isEqualToString:@"iPhone7,1"])    return iPhone6Plus;
  if ([platform isEqualToString:@"iPhone7,2"])    return iPhone6;
  
  if ([platform isEqualToString:@"iPod1,1"])      return iPodTouch1G;
  if ([platform isEqualToString:@"iPod2,1"])      return iPodTouch2G;
  if ([platform isEqualToString:@"iPod3,1"])      return iPodTouch3G;
  if ([platform isEqualToString:@"iPod4,1"])      return iPodTouch4G;
  if ([platform isEqualToString:@"iPod5,1"])      return iPodTouch5G;
  
  if ([platform isEqualToString:@"iPad1,1"])      return iPad;
  if ([platform isEqualToString:@"iPad1,2"])      return iPad;
  if ([platform isEqualToString:@"iPad2,1"])      return iPad2WIFI;
  if ([platform isEqualToString:@"iPad2,2"])      return iPad2;
  if ([platform isEqualToString:@"iPad2,3"])      return iPad2CDMA;
  if ([platform isEqualToString:@"iPad2,4"])      return iPad2;
  if ([platform isEqualToString:@"iPad2,5"])      return iPadMiniWIFI;
  if ([platform isEqualToString:@"iPad2,6"])      return iPadMini;
  if ([platform isEqualToString:@"iPad2,7"])      return iPadMiniGSMCDMA;
  if ([platform isEqualToString:@"iPad3,1"])      return iPad3WIFI;
  if ([platform isEqualToString:@"iPad3,2"])      return iPad3GSMCDMA;
  if ([platform isEqualToString:@"iPad3,3"])      return iPad3;
  if ([platform isEqualToString:@"iPad3,4"])      return iPad4WIFI;
  if ([platform isEqualToString:@"iPad3,5"])      return iPad4;
  if ([platform isEqualToString:@"iPad3,6"])      return iPad4GSMCDMA;
  if ([platform isEqualToString:@"iPad4,1"])      return iPadAirWifi;
  if ([platform isEqualToString:@"iPad4,2"])      return iPadAirCellular;
  if ([platform isEqualToString:@"iPad4,4"])      return iPadMini2Wifi;
  if ([platform isEqualToString:@"iPad4,5"])      return iPadMini2Cellular;
  if ([platform isEqualToString:@"iPad4,7"])      return iPadMini3Wifi;
  if ([platform isEqualToString:@"iPad4,8"])      return iPadMini3Cellular;
  if ([platform isEqualToString:@"iPad4,9"])      return iPadMini3Cellular;
  if ([platform isEqualToString:@"iPad5,3"])      return iPadAir2Wifi;
  if ([platform isEqualToString:@"iPad5,4"])      return iPadAir2Cellular;
  
  if ([platform isEqualToString:@"AppleTV2,1"])   return AppleTV2;
#endif
  return iDeviceUnknown;
}

bool DarwinIsAppleTV2(void)
{
  static enum iosPlatform platform = iDeviceUnknown;
#if defined(TARGET_DARWIN_IOS)
  if( platform == iDeviceUnknown )
  {
    platform = getIosPlatform();
  }
#endif
  return (platform == AppleTV2);
}

bool DarwinIsMavericks(void)
{
  static int isMavericks = -1;
#if defined(TARGET_DARWIN_OSX)
  // there is no NSAppKitVersionNumber10_9 out there anywhere
  // so we detect mavericks by one of these newly added app nap
  // methods - and fix the ugly mouse rect problem which was hitting
  // us when mavericks came out
  if (isMavericks == -1)
  {
    CLog::Log(LOGDEBUG, "Detected Mavericks...");
    isMavericks = [NSProcessInfo instancesRespondToSelector:@selector(beginActivityWithOptions:reason:)] == TRUE ? 1 : 0;
  }
#endif
  return isMavericks == 1;
}

bool DarwinIsSnowLeopard(void)
{
  static int isSnowLeopard = -1;
#if defined(TARGET_DARWIN_OSX)
  if (isSnowLeopard == -1)
  {
    double appKitVersion = floor(NSAppKitVersionNumber);
    isSnowLeopard = (appKitVersion <= NSAppKitVersionNumber10_6 && appKitVersion > NSAppKitVersionNumber10_5) ? 1 : 0;
  }
#endif
  return isSnowLeopard == 1;
}

bool DarwinHasRetina(double &scale)
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

const char *GetDarwinOSReleaseString(void)
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

const char *GetDarwinVersionString(void)
{
  CCocoaAutoPool pool;
  return [[[NSProcessInfo processInfo] operatingSystemVersionString] UTF8String];
}

float GetIOSVersion(void)
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

int  GetDarwinFrameworkPath(bool forPython, char* path, uint32_t *pathsize)
{
  CCocoaAutoPool pool;
  // see if we can figure out who we are
  NSString *pathname;

  path[0] = 0;
  *pathsize = 0;

  // a) XBMC frappliance running under ATV2
  Class XBMCfrapp = NSClassFromString(@"XBMCATV2Detector");
  if (XBMCfrapp != NULL)
  {
    pathname = [[NSBundle bundleForClass:XBMCfrapp] pathForResource:@"Frameworks" ofType:@""];
    strcpy(path, [pathname UTF8String]);
    *pathsize = strlen(path);
    //CLog::Log(LOGDEBUG, "DarwinFrameworkPath(a) -> %s", path);
    return 0;
  }

  // b) XBMC application running under IOS
  pathname = [[NSBundle mainBundle] executablePath];
  if (pathname && strstr([pathname UTF8String], "XBMC.app/XBMC"))
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

  // d) XBMC application running under OSX
  pathname = [[NSBundle mainBundle] executablePath];
  if (pathname && strstr([pathname UTF8String], "Contents"))
  {
    strcpy(path, [pathname UTF8String]);
    // ExectuablePath is <product>.app/Contents/MacOS/<executable>
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

  // e) XBMC OSX binary running under xcode or command-line
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

int  GetDarwinExecutablePath(char* path, uint32_t *pathsize)
{
  CCocoaAutoPool pool;
  // see if we can figure out who we are
  NSString *pathname;

  // a) XBMC frappliance running under ATV2
  Class XBMCfrapp = NSClassFromString(@"XBMCATV2Detector");
  if (XBMCfrapp != NULL)
  {
    pathname = [[NSBundle bundleForClass:XBMCfrapp] pathForResource:@"XBMC" ofType:@""];
    strcpy(path, [pathname UTF8String]);
    *pathsize = strlen(path);
    //CLog::Log(LOGDEBUG, "DarwinExecutablePath(a) -> %s", path);
    return 0;
  }

  // b) XBMC application running under IOS
  // c) XBMC application running under OSX
  pathname = [[NSBundle mainBundle] executablePath];
  strcpy(path, [pathname UTF8String]);
  *pathsize = strlen(path);
  //CLog::Log(LOGDEBUG, "DarwinExecutablePath(b/c) -> %s", path);

  return 0;
}

const char* DarwinGetXbmcRootFolder(void)
{
  static std::string rootFolder = "";
  if ( rootFolder.length() == 0)
  {
    if (DarwinIsIosSandboxed())
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

bool DarwinIsIosSandboxed(void)
{
  static int ret = -1;
  if (ret == -1)
  {
    uint32_t path_size = 2*MAXPATHLEN;
    char     given_path[2*MAXPATHLEN];
    int      result = -1; 
    ret = 0;
    memset(given_path, 0x0, path_size);
    /* Get Application directory */  
    result = GetDarwinExecutablePath(given_path, &path_size);
    if (result == 0)
    {
      // we re sandboxed if we are installed in /var/mobile/Applications
      if (strlen("/var/mobile/Applications/") < path_size &&
        strncmp(given_path, "/var/mobile/Applications/", strlen("/var/mobile/Applications/")) == 0)
      {
        ret = 1;
      }
    }
  }
  return ret == 1;
}

bool DarwinHasVideoToolboxDecoder(void)
{
  static int DecoderAvailable = -1;

  if (DecoderAvailable == -1)
  {
    Class XBMCfrapp = NSClassFromString(@"XBMCATV2Detector");
    if (XBMCfrapp != NULL)
    {
      // atv2 has seatbelt profile key removed so nothing to do here
      DecoderAvailable = 1;
    }
    else
    {
      /* When XBMC is started from a sandbox directory we have to check the sysctl values */      
      if (DarwinIsIosSandboxed())
      {
        uint64_t proc_enforce = 0;
        uint64_t vnode_enforce = 0; 
        size_t size = sizeof(vnode_enforce);

        sysctlbyname("security.mac.proc_enforce",  &proc_enforce,  &size, NULL, 0);  
        sysctlbyname("security.mac.vnode_enforce", &vnode_enforce, &size, NULL, 0);

        if (vnode_enforce && proc_enforce)
        {
          DecoderAvailable = 1;
          CLog::Log(LOGINFO, "VideoToolBox decoder not available. Use : sysctl -w security.mac.proc_enforce=0; sysctl -w security.mac.vnode_enforce=0\n");
        }
        else
        {
          DecoderAvailable = 1;
          CLog::Log(LOGINFO, "VideoToolBox decoder available\n");
        }  
      }
      else
      {
        DecoderAvailable = 1;
      }
    }
  }

  return (DecoderAvailable == 1);
}

int DarwinBatteryLevel(void)
{
  float batteryLevel = 0;
#if defined(TARGET_DARWIN_IOS)
  if(!DarwinIsAppleTV2())
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
#endif
  return batteryLevel * 100;  
}

void DarwinSetScheduling(int message)
{
  int policy;
  struct sched_param param;
  pthread_t this_pthread_self = pthread_self();

  int32_t result = pthread_getschedparam(this_pthread_self, &policy, &param );

  policy = SCHED_OTHER;
  thread_extended_policy_data_t theFixedPolicy={true};

  if (message == GUI_MSG_PLAYBACK_STARTED && g_application.m_pPlayer->IsPlayingVideo())
  {
    policy = SCHED_RR;
    theFixedPolicy.timeshare = false;
  }

  result = thread_policy_set(pthread_mach_thread_np(this_pthread_self),
    THREAD_EXTENDED_POLICY, 
    (thread_policy_t)&theFixedPolicy,
    THREAD_EXTENDED_POLICY_COUNT);

  result = pthread_setschedparam(this_pthread_self, policy, &param );
}

bool DarwinCFStringRefToStringWithEncoding(CFStringRef source, std::string &destination, CFStringEncoding encoding)
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

bool DarwinCFStringRefToString(CFStringRef source, std::string &destination)
{
  return DarwinCFStringRefToStringWithEncoding(source, destination, CFStringGetSystemEncoding());
}

bool DarwinCFStringRefToUTF8String(CFStringRef source, std::string &destination)
{
  return DarwinCFStringRefToStringWithEncoding(source, destination, kCFStringEncodingUTF8);
}

#endif
