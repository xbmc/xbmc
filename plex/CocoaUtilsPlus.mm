//
//  CocoaUtilsPlus.MM
//  Plex
//
//  Created by Enrique Osuna on 10/26/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <IOKit/IOMessage.h>

#include "CocoaUtilsPlus.h"
#include "Settings.h"
#include "AdvancedSettings.h"
#include "Application.h"
#include "MediaSource.h"
#define BOOL COCOA_BOOL
#include <Cocoa/Cocoa.h>
#undef BOOL
#import <AddressBook/AddressBook.h>
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#import <SystemConfiguration/SystemConfiguration.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/network/IOEthernetInterface.h>
#include <IOKit/network/IONetworkInterface.h>
#include <IOKit/network/IOEthernetController.h>

//#include "PlexMediaServerHelper.h"
#include "CriticalSection.h"
#include "SingleLock.h"

#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <map>

#include "Log.h"
#ifdef WORKING
using namespace boost;

// Save the root port.
io_connect_t root_port = 0;

#define COCOA_KEY_PLAYPAUSE  1051136
#define COCOA_KEY_PREV_TRACK 1313280
#define COCOA_KEY_NEXT_TRACK 1248000

static map<string, string> g_isoLangMap;

struct CachedTime
{
  CachedTime(int time, const string& strTime)
    : time(time)
    , strTime(strTime)
    {}

  int time;
  string strTime;
};
typedef boost::shared_ptr<CachedTime> CachedTimePtr;

class CCocoaData
{
 public:
  CCocoaData()
    : lastDateFormat(-1)
    , lastTimeFormat(-1)
    , lastTime(-1)
    {}

  CCocoaData& Get()
  {
    if (g_instance == 0)
      g_instance = new CCocoaData();

    return *g_instance;
  }

  int lastDateFormat;
  int lastTimeFormat;
  string lastFormatString;

  int lastTime;
  string lastFormatFormat;
  string lastTimeString;

  map<string, CachedTimePtr> cachedTimeMap;

  CCriticalSection formatCriticalSection;

 private:
  static CCocoaData* g_instance;
};

CCocoaData* CCocoaData::g_instance = 0;


///////////////////////////////////////////////////////////////////////////////
CGEventRef tapEventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon)
{
  // 1051136
  NSEvent* nsEvent = [NSEvent eventWithCGEvent:event];
  NSInteger data = [nsEvent data1];

  if(data!=1051136)
    return event;
  else
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
void Cocoa_PowerStateNotification(void* x, io_service_t y, natural_t messageType, void* messageArgument)
{
  switch (messageType)
  {
  case kIOMessageSystemWillSleep:
    // Handle demand sleep (such as sleep caused by running out of batteries,
    // closing the lid of a laptop, or selecting sleep from the Apple menu).
    //
    g_application.getApplicationMessenger().SystemWillSleep();
    IOAllowPowerChange(root_port, (long)messageArgument);
    break;

  case kIOMessageCanSystemSleep:
    // In this case, the computer has been idle for several minutes
    // and will sleep soon so you must either allow or cancel
    // this notification. Important: if you don't respond, there will
    // be a 30-second timeout before the computer sleeps.
    //
    IOAllowPowerChange(root_port, (long)messageArgument);
    break;

  case kIOMessageSystemHasPoweredOn:
    // Handle wake-up.
    g_application.getApplicationMessenger().SystemWokeUp();
    break;
  }
}

///////////////////////////////////////////////////////////////////////////////
void CocoaPlus_Initialize()
{
  g_isoLangMap["aar"] = "aa";
  g_isoLangMap["abk"] = "ab";
  g_isoLangMap["ave"] = "ae";
  g_isoLangMap["afr"] = "af";
  g_isoLangMap["aka"] = "ak";
  g_isoLangMap["amh"] = "am";
  g_isoLangMap["arg"] = "an";
  g_isoLangMap["ara"] = "ar";
  g_isoLangMap["asm"] = "as";
  g_isoLangMap["ava"] = "av";
  g_isoLangMap["aym"] = "ay";
  g_isoLangMap["aze"] = "az";
  g_isoLangMap["bak"] = "ba";
  g_isoLangMap["bel"] = "be";
  g_isoLangMap["bul"] = "bg";
  g_isoLangMap["bih"] = "bh";
  g_isoLangMap["bis"] = "bi";
  g_isoLangMap["bam"] = "bm";
  g_isoLangMap["ben"] = "bn";
  g_isoLangMap["bod"] = "bo";
  g_isoLangMap["tib"] = "bo"; // Alternate.
  g_isoLangMap["bre"] = "br";
  g_isoLangMap["bos"] = "bs";
  g_isoLangMap["cat"] = "ca";
  g_isoLangMap["che"] = "ce";
  g_isoLangMap["cha"] = "ch";
  g_isoLangMap["cos"] = "co";
  g_isoLangMap["cre"] = "cr";
  g_isoLangMap["ces"] = "cs";
  g_isoLangMap["cze"] = "cs";
  g_isoLangMap["chu"] = "cu";
  g_isoLangMap["chv"] = "cv";
  g_isoLangMap["cym"] = "cy";
  g_isoLangMap["wel"] = "cy"; // Alternate.
  g_isoLangMap["dan"] = "da";
  g_isoLangMap["deu"] = "de";
  g_isoLangMap["ger"] = "de"; // Alternate.
  g_isoLangMap["div"] = "dv";
  g_isoLangMap["dzo"] = "dz";
  g_isoLangMap["ewe"] = "ee";
  g_isoLangMap["ell"] = "el";
  g_isoLangMap["gre"] = "el"; // Alternate.
  g_isoLangMap["eng"] = "en";
  g_isoLangMap["epo"] = "eo";
  g_isoLangMap["spa"] = "es";
  g_isoLangMap["est"] = "et";
  g_isoLangMap["eus"] = "eu";
  g_isoLangMap["baq"] = "eu"; // Alternate.
  g_isoLangMap["fas"] = "fa";
  g_isoLangMap["per"] = "fa"; // Alternate.
  g_isoLangMap["ful"] = "ff";
  g_isoLangMap["fin"] = "fi";
  g_isoLangMap["fij"] = "fj";
  g_isoLangMap["fao"] = "fo";
  g_isoLangMap["fra"] = "fr";
  g_isoLangMap["fre"] = "fr"; // Alternate.
  g_isoLangMap["fry"] = "fy";
  g_isoLangMap["gle"] = "ga";
  g_isoLangMap["gla"] = "gd";
  g_isoLangMap["glg"] = "gl";
  g_isoLangMap["grn"] = "gn";
  g_isoLangMap["guj"] = "gu";
  g_isoLangMap["glv"] = "gv";
  g_isoLangMap["hau"] = "ha";
  g_isoLangMap["heb"] = "he";
  g_isoLangMap["hin"] = "hi";
  g_isoLangMap["hmo"] = "ho";
  g_isoLangMap["hrv"] = "hr";
  g_isoLangMap["hat"] = "ht";
  g_isoLangMap["hun"] = "hu";
  g_isoLangMap["hye"] = "hy";
  g_isoLangMap["arm"] = "hy"; // Alternate.
  g_isoLangMap["her"] = "hz";
  g_isoLangMap["ina"] = "ia";
  g_isoLangMap["ind"] = "id";
  g_isoLangMap["ile"] = "ie";
  g_isoLangMap["ibo"] = "ig";
  g_isoLangMap["iii"] = "ii";
  g_isoLangMap["ipk"] = "ik";
  g_isoLangMap["ido"] = "io";
  g_isoLangMap["isl"] = "is";
  g_isoLangMap["ice"] = "is";
  g_isoLangMap["ita"] = "it";
  g_isoLangMap["iku"] = "iu";
  g_isoLangMap["jpn"] = "ja";
  g_isoLangMap["jav"] = "jv";
  g_isoLangMap["kat"] = "ka";
  g_isoLangMap["geo"] = "ka";
  g_isoLangMap["kon"] = "kg";
  g_isoLangMap["kik"] = "ki";
  g_isoLangMap["kua"] = "kj";
  g_isoLangMap["kaz"] = "kk";
  g_isoLangMap["kal"] = "kl";
  g_isoLangMap["khm"] = "km";
  g_isoLangMap["kan"] = "kn";
  g_isoLangMap["kor"] = "ko";
  g_isoLangMap["kau"] = "kr";
  g_isoLangMap["kas"] = "ks";
  g_isoLangMap["kur"] = "ku";
  g_isoLangMap["kom"] = "kv";
  g_isoLangMap["cor"] = "kw";
  g_isoLangMap["kir"] = "ky";
  g_isoLangMap["lat"] = "la";
  g_isoLangMap["ltz"] = "lb";
  g_isoLangMap["lug"] = "lg";
  g_isoLangMap["lim"] = "li";
  g_isoLangMap["lin"] = "ln";
  g_isoLangMap["lao"] = "lo";
  g_isoLangMap["lit"] = "lt";
  g_isoLangMap["lub"] = "lu";
  g_isoLangMap["lav"] = "lv";
  g_isoLangMap["mlg"] = "mg";
  g_isoLangMap["mah"] = "mh";
  g_isoLangMap["mri"] = "mi";
  g_isoLangMap["mao"] = "mi"; // Alternate.
  g_isoLangMap["mkd"] = "mk";
  g_isoLangMap["mac"] = "mk"; // Alternate.
  g_isoLangMap["mal"] = "ml";
  g_isoLangMap["mon"] = "mn";
  g_isoLangMap["mar"] = "mr";
  g_isoLangMap["msa"] = "ms";
  g_isoLangMap["may"] = "ms"; // Alternate.
  g_isoLangMap["mlt"] = "mt";
  g_isoLangMap["mya"] = "my";
  g_isoLangMap["bur"] = "my"; // Alternate.
  g_isoLangMap["nau"] = "na";
  g_isoLangMap["nob"] = "nb";
  g_isoLangMap["nde"] = "nd";
  g_isoLangMap["nep"] = "ne";
  g_isoLangMap["ndo"] = "ng";
  g_isoLangMap["nld"] = "nl";
  g_isoLangMap["dut"] = "nl"; // Alternate.
  g_isoLangMap["nno"] = "nn";
  g_isoLangMap["nor"] = "no";
  g_isoLangMap["nbl"] = "nr";
  g_isoLangMap["nav"] = "nv";
  g_isoLangMap["nya"] = "ny";
  g_isoLangMap["oci"] = "oc";
  g_isoLangMap["oji"] = "oj";
  g_isoLangMap["orm"] = "om";
  g_isoLangMap["ori"] = "or";
  g_isoLangMap["oss"] = "os";
  g_isoLangMap["pan"] = "pa";
  g_isoLangMap["pli"] = "pi";
  g_isoLangMap["pol"] = "pl";
  g_isoLangMap["pus"] = "ps";
  g_isoLangMap["por"] = "pt";
  g_isoLangMap["que"] = "qu";
  g_isoLangMap["roh"] = "rm";
  g_isoLangMap["run"] = "rn";
  g_isoLangMap["ron"] = "ro";
  g_isoLangMap["rum"] = "ro"; // Alternate.
  g_isoLangMap["rus"] = "ru";
  g_isoLangMap["kin"] = "rw";
  g_isoLangMap["san"] = "sa";
  g_isoLangMap["srd"] = "sc";
  g_isoLangMap["snd"] = "sd";
  g_isoLangMap["sme"] = "se";
  g_isoLangMap["sag"] = "sg";
  g_isoLangMap["sin"] = "si";
  g_isoLangMap["slk"] = "sk";
  g_isoLangMap["slo"] = "sk"; // Alternate.
  g_isoLangMap["slv"] = "sl";
  g_isoLangMap["smo"] = "sm";
  g_isoLangMap["sna"] = "sn";
  g_isoLangMap["som"] = "so";
  g_isoLangMap["sqi"] = "sq";
  g_isoLangMap["srp"] = "sr";
  g_isoLangMap["ssw"] = "ss";
  g_isoLangMap["sot"] = "st";
  g_isoLangMap["sun"] = "su";
  g_isoLangMap["swe"] = "sv";
  g_isoLangMap["swa"] = "sw";
  g_isoLangMap["tam"] = "ta";
  g_isoLangMap["tel"] = "te";
  g_isoLangMap["tgk"] = "tg";
  g_isoLangMap["tha"] = "th";
  g_isoLangMap["tir"] = "ti";
  g_isoLangMap["tuk"] = "tk";
  g_isoLangMap["tgl"] = "tl";
  g_isoLangMap["tsn"] = "tn";
  g_isoLangMap["ton"] = "to";
  g_isoLangMap["tur"] = "tr";
  g_isoLangMap["tso"] = "ts";
  g_isoLangMap["tat"] = "tt";
  g_isoLangMap["twi"] = "tw";
  g_isoLangMap["tah"] = "ty";
  g_isoLangMap["uig"] = "ug";
  g_isoLangMap["ukr"] = "uk";
  g_isoLangMap["urd"] = "ur";
  g_isoLangMap["uzb"] = "uz";
  g_isoLangMap["ven"] = "ve";
  g_isoLangMap["vie"] = "vi";
  g_isoLangMap["vol"] = "vo";
  g_isoLangMap["wln"] = "wa";
  g_isoLangMap["wol"] = "wo";
  g_isoLangMap["xho"] = "xh";
  g_isoLangMap["yid"] = "yi";
  g_isoLangMap["yor"] = "yo";
  g_isoLangMap["zha"] = "za";
  g_isoLangMap["zho"] = "zh";
  g_isoLangMap["chi"] = "zh"; // Alternate.
  g_isoLangMap["zul"] = "zu";

#if 0
  // kCGHeadInsertEventTap - can't use this because stopping in debugger/hang means nobody gets it!
  CFMachPortRef eventPort = CGEventTapCreate(kCGSessionEventTap, kCGTailAppendEventTap, kCGEventTapOptionDefault, CGEventMaskBit(NX_SYSDEFINED), tapEventCallback, NULL);
  if (eventPort == 0)
    NSLog(@"No event port available.");

  CFRunLoopSourceRef eventSrc = CFMachPortCreateRunLoopSource(NULL, eventPort, 0);
  if (eventSrc == 0)
    NSLog(@"No event run loop ");

  CFRunLoopRef runLoop = CFRunLoopGetCurrent();
  if ( runLoop == NULL)
    NSLog(@"No run loop for tap callback.");

  CFRunLoopAddSource(runLoop,  eventSrc, kCFRunLoopDefaultMode);
#endif

  // Add notification for power events.
  IONotificationPortRef notify; io_object_t anIterator;
  root_port = IORegisterForSystemPower (0, &notify, Cocoa_PowerStateNotification, &anIterator);
  if (root_port == 0)
    printf("IORegisterForSystemPower failed\n");

  CFRunLoopAddSource(CFRunLoopGetCurrent(), IONotificationPortGetRunLoopSource(notify), kCFRunLoopDefaultMode);
}
#endif // WORKING

///////////////////////////////////////////////////////////////////////////////
vector<string> Cocoa_GetSystemFonts()
{
  vector<string> result;

  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  @try
  {
    NSArray *fonts = [[[NSFontManager sharedFontManager] availableFontFamilies] sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)];
    for (unsigned i = 0; i < [fonts count]; i++)
    {
      result.push_back([[fonts objectAtIndex:i] cStringUsingEncoding:NSUTF8StringEncoding]);
    }
  }
  @finally
  {
    [pool drain];
  }
  return result;
}


///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetSystemFontPathFromDisplayName(const string displayName)
{
  CFURLRef fontFileURL = NULL;
  CFStringRef fontName = CFStringCreateWithCString(kCFAllocatorDefault, displayName.c_str(), kCFStringEncodingUTF8);
  if (!fontName)
    return displayName;

  @try
  {
    ATSFontRef fontRef = ATSFontFindFromName(fontName, kATSOptionFlagsDefault);
    if (!fontRef)
      return displayName;

    FSRef fontFileRef;
    if (ATSFontGetFileReference(fontRef, &fontFileRef) != noErr)
      return displayName;

    fontFileURL = CFURLCreateFromFSRef(kCFAllocatorDefault, &fontFileRef);
    return [[(NSURL *)fontFileURL path] cStringUsingEncoding:NSUTF8StringEncoding];
  }
  @finally
  {
    if (fontName)
      CFRelease(fontName);
    if (fontFileURL)
      CFRelease(fontFileURL);
  }
  return displayName;
}

#ifdef WORKING

///////////////////////////////////////////////////////////////////////////////
vector<in_addr_t> Cocoa_AddressesForHost(const string& hostname)
{
  NSHost *host = [NSHost hostWithName:[NSString stringWithCString:hostname.c_str() encoding:NSUTF8StringEncoding]];
  vector<in_addr_t> ret;
  for (NSString *address in [host addresses])
    ret.push_back(inet_addr([address UTF8String]));
  return ret;

}

#endif // WORKING

#ifdef WORKING

///////////////////////////////////////////////////////////////////////////////
bool Cocoa_AreHostsEqual(const string& host1, const string& host2)
{
  return (host1 == host2);
#if 0
  NSHost *h1 = [NSHost hostWithName:[NSString stringWithCString:host1.c_str() encoding:NSUTF8StringEncoding]];
  NSHost *h2 = [NSHost hostWithName:[NSString stringWithCString:host2.c_str() encoding:NSUTF8StringEncoding]];
  return ([h1 isEqualToHost:h2] || ([h1 isEqualToHost:[NSHost currentHost]] && [h2 isEqualToHost:[NSHost currentHost]]));
#endif
}

///////////////////////////////////////////////////////////////////////////////
bool Cocoa_IsLocalPlexMediaServerRunning()
{
  bool isRunning = PlexMediaServerHelper::Get().IsRunning();
  return isRunning;
}

///////////////////////////////////////////////////////////////////////////////
vector<CStdString> Cocoa_Proxy_ExceptionList()
{
  vector<CStdString> ret;
  NSDictionary* proxyDict = (NSDictionary*)SCDynamicStoreCopyProxies(NULL);
  @try
  {
    for (id exceptionItem in [proxyDict objectForKey:(id)kSCPropNetProxiesExceptionsList])
      ret.push_back(CStdString([exceptionItem UTF8String]));
  }
  @finally
  {
    [proxyDict release];
  }
  return ret;
}
#endif // WORKING

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetLanguage()
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];  
    
  // See if we're overriden.
  if (g_advancedSettings.m_language.size() > 0)
    return g_advancedSettings.m_language;

  // Otherwise, use the OS X default.
  NSArray* languages = [NSLocale preferredLanguages];
  std::string language = [[languages objectAtIndex:0] UTF8String];
  
  [pool release];
  return language;
}

#ifdef _WORKING
///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetSimpleLanguage()
{
  string lang = Cocoa_GetLanguage();
  int    dash = lang.find("-");

  if (dash != -1)
    lang = lang.substr(0, dash);

  return lang;
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_ConvertIso6392ToIso6391(const string& lang)
{
  if (g_isoLangMap.find(lang) != g_isoLangMap.end())
    return g_isoLangMap[lang];

  return "";
}

///////////////////////////////////////////////////////////////////////////////
bool Cocoa_IsMetricSystem()
{
  // See if we're overriden.
  if (g_advancedSettings.m_units.size() > 0)
  {
    if (g_advancedSettings.m_units == "metric")
      return true;
    else
      return false;
  }

  // Otherwise, use the OS X default.
  NSLocale* locale = [NSLocale currentLocale];
  NSNumber* isMetric = [locale objectForKey:NSLocaleUsesMetricSystem];

  return [isMetric boolValue] == YES;
}

///////////////////////////////////////////////////////////////////////////////
static string Cocoa_GetFormatString(int dateFormat, int timeFormat)
{
  CSingleLock lock(CCocoaData().Get().formatCriticalSection);

  // Quick exit for repeat question.
  if (CCocoaData().Get().lastDateFormat == dateFormat && CCocoaData().Get().lastTimeFormat == timeFormat)
    return CCocoaData().Get().lastFormatString;

  id pool = [[NSAutoreleasePool alloc] init];

  NSDateFormatter *dateFormatter = [[[NSDateFormatter alloc] init] autorelease];
  [dateFormatter setLocale:[NSLocale currentLocale]];
  [dateFormatter setDateStyle:dateFormat];
  [dateFormatter setTimeStyle:timeFormat];

  string ret = [[dateFormatter dateFormat] UTF8String];
  [pool release];

  // Cache for next time.
  CCocoaData().Get().lastDateFormat = dateFormat;
  CCocoaData().Get().lastTimeFormat = timeFormat;
  CCocoaData().Get().lastFormatString = ret;

  return ret;
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetLongDateFormat()
{
  return Cocoa_GetFormatString(kCFDateFormatterLongStyle, kCFDateFormatterNoStyle);
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetShortDateFormat()
{
  return Cocoa_GetFormatString(kCFDateFormatterShortStyle, kCFDateFormatterNoStyle);
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetTimeFormat(bool withMeridian)
{
  string ret = Cocoa_GetFormatString(kCFDateFormatterNoStyle, kCFDateFormatterShortStyle);

  // Optionally remove meridian.
  if (withMeridian == false)
    boost::replace_all(ret, "a", "");

  // Remove timezone.
  boost::replace_all(ret, "z", "");
  boost::replace_all(ret, "Z", "");

  // Remove extra spaces.
  boost::replace_all(ret, "  ", "");

  boost::trim(ret);
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetMeridianSymbol(int i)
{
  string ret;
  id pool = [[NSAutoreleasePool alloc] init];
  NSDateFormatter *dateFormatter = [[[NSDateFormatter alloc] init] autorelease];
  [dateFormatter setLocale:[NSLocale currentLocale]];

  if (i == 0)
    ret = [[dateFormatter PMSymbol] UTF8String];
  else if (i == 1)
    ret = [[dateFormatter AMSymbol] UTF8String];

  [pool release];

  return ret;
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetCountryCode()
{
  id pool = [[NSAutoreleasePool alloc] init];
  NSLocale* myLocale = [NSLocale currentLocale];
  NSString* countryCode = [myLocale objectForKey:NSLocaleCountryCode];

  string ret = [countryCode UTF8String];
  [pool release];

  return ret;
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetDateString(time_t time, bool longDate)
{
  id pool = [[NSAutoreleasePool alloc] init];
  NSDateFormatter *dateFormatter = [[[NSDateFormatter alloc] init] autorelease];
  [dateFormatter setLocale:[NSLocale currentLocale]];

  if (longDate)
    [dateFormatter setDateStyle:kCFDateFormatterLongStyle];
  else
    [dateFormatter setDateStyle:kCFDateFormatterShortStyle];

  [dateFormatter setTimeStyle:kCFDateFormatterNoStyle];

  NSDate* date = [NSDate dateWithTimeIntervalSince1970:time];
  NSString* formattedDateString = [dateFormatter stringFromDate:date];

  string ret = [formattedDateString UTF8String];

  [pool release];
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetTimeString(time_t time)
{
  id pool = [[NSAutoreleasePool alloc] init];
  NSDateFormatter *dateFormatter = [[[NSDateFormatter alloc] init] autorelease];
  [dateFormatter setLocale:[NSLocale currentLocale]];
  [dateFormatter setDateStyle:kCFDateFormatterNoStyle];
  [dateFormatter setTimeStyle:kCFDateFormatterShortStyle];

  NSDate* date = [NSDate dateWithTimeIntervalSince1970:time];
  NSString* formattedDateString = [dateFormatter stringFromDate:date];

  string ret = [formattedDateString UTF8String];

  [pool release];
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetTimeString(const string& format, time_t time)
{
  CSingleLock lock(CCocoaData().Get().formatCriticalSection);

  // If we're requesting the same time, return it.
  if (CCocoaData().Get().cachedTimeMap.find(format) != CCocoaData().Get().cachedTimeMap.end())
  {
    CachedTimePtr cachedTime = CCocoaData().Get().cachedTimeMap[format];
    if (cachedTime->time == time)
      return cachedTime->strTime;
  }

  id pool = [[NSAutoreleasePool alloc] init];
  NSDateFormatter *dateFormatter = [[[NSDateFormatter alloc] init] autorelease];
  [dateFormatter setLocale:[NSLocale currentLocale]];
  [dateFormatter setDateFormat:[NSString stringWithCString:format.c_str() encoding:NSUTF8StringEncoding]];

  NSDate* date = [NSDate dateWithTimeIntervalSince1970:time];
  NSString* formattedDateString = [dateFormatter stringFromDate:date];

  string ret = [formattedDateString UTF8String];

  [pool release];

  CCocoaData().Get().cachedTimeMap[format] = CachedTimePtr(new CachedTime(time, ret));
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetMyAddressField(NSString* field)
{
  string ret;

  id pool = [[NSAutoreleasePool alloc] init];
  ABPerson* aPerson = [[ABAddressBook sharedAddressBook] me];
  ABMutableMultiValue* anAddressList = [aPerson valueForProperty:kABAddressProperty];

  if (anAddressList)
  {
    int primaryIndex = [anAddressList indexForIdentifier:[anAddressList primaryIdentifier]];
    if (primaryIndex >= 0)
    {
      NSMutableDictionary* anAddress = [anAddressList valueAtIndex:primaryIndex];
      NSString* value = (NSString* )[anAddress objectForKey:field];

      if (value)
        ret = [value UTF8String];
    }
  }

  [pool release];

  return ret;
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetMyZip()
{
  return Cocoa_GetMyAddressField(kABAddressZIPKey);
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetMyCity()
{
  return Cocoa_GetMyAddressField(kABAddressCityKey);
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetMyCountry()
{
  return Cocoa_GetMyAddressField(kABAddressCountryKey);
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetMyState()
{
  return Cocoa_GetMyAddressField(kABAddressStateKey);
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetMachineSerialNumber()
{
  NSString* result = nil;
  CFStringRef serialNumber = NULL;

  io_service_t platformExpert = IOServiceGetMatchingService(
     kIOMasterPortDefault,
     IOServiceMatching("IOPlatformExpertDevice")
  );

  if (platformExpert)
  {
     CFTypeRef serialNumberAsCFString = IORegistryEntryCreateCFProperty(
        platformExpert,
        CFSTR(kIOPlatformSerialNumberKey),
        kCFAllocatorDefault,
        0
     );
     serialNumber = (CFStringRef)serialNumberAsCFString;
     IOObjectRelease(platformExpert);
   }

  if (serialNumber)
    result = [(NSString*)serialNumber autorelease];
  else
    result = @"";

  return [result UTF8String];
}

// Returns an iterator containing the primary (built-in) Ethernet interface. The caller is responsible for
// releasing the iterator after the caller is done with it.
static kern_return_t FindEthernetInterfaces(io_iterator_t *matchingServices)
{
    kern_return_t    kernResult;
    mach_port_t      masterPort;
    CFMutableDictionaryRef  matchingDict;
    CFMutableDictionaryRef  propertyMatchDict;

    // Retrieve the Mach port used to initiate communication with I/O Kit
    kernResult = IOMasterPort(MACH_PORT_NULL, &masterPort);
    if (KERN_SUCCESS != kernResult)
    {
        printf("IOMasterPort returned %d\n", kernResult);
        return kernResult;
    }

    // Ethernet interfaces are instances of class kIOEthernetInterfaceClass.
    // IOServiceMatching is a convenience function to create a dictionary with the key kIOProviderClassKey and
    // the specified value.
    matchingDict = IOServiceMatching(kIOEthernetInterfaceClass);

    // Note that another option here would be:
    // matchingDict = IOBSDMatching("en0");

    if (NULL == matchingDict)
    {
        printf("IOServiceMatching returned a NULL dictionary.\n");
    }
    else {
        // Each IONetworkInterface object has a Boolean property with the key kIOPrimaryInterface. Only the
        // primary (built-in) interface has this property set to TRUE.

        // IOServiceGetMatchingServices uses the default matching criteria defined by IOService. This considers
        // only the following properties plus any family-specific matching in this order of precedence
        // (see IOService::passiveMatch):
        //
        // kIOProviderClassKey (IOServiceMatching)
        // kIONameMatchKey (IOServiceNameMatching)
        // kIOPropertyMatchKey
        // kIOPathMatchKey
        // kIOMatchedServiceCountKey
        // family-specific matching
        // kIOBSDNameKey (IOBSDNameMatching)
        // kIOLocationMatchKey

        // The IONetworkingFamily does not define any family-specific matching. This means that in
        // order to have IOServiceGetMatchingServices consider the kIOPrimaryInterface property, we must
        // add that property to a separate dictionary and then add that to our matching dictionary
        // specifying kIOPropertyMatchKey.

        propertyMatchDict = CFDictionaryCreateMutable( kCFAllocatorDefault, 0,
                                                       &kCFTypeDictionaryKeyCallBacks,
                                                       &kCFTypeDictionaryValueCallBacks);

        if (NULL == propertyMatchDict)
        {
            printf("CFDictionaryCreateMutable returned a NULL dictionary.\n");
        }
        else {
            // Set the value in the dictionary of the property with the given key, or add the key
            // to the dictionary if it doesn't exist. This call retains the value object passed in.
            CFDictionarySetValue(propertyMatchDict, CFSTR(kIOPrimaryInterface), kCFBooleanTrue);

            // Now add the dictionary containing the matching value for kIOPrimaryInterface to our main
            // matching dictionary. This call will retain propertyMatchDict, so we can release our reference
            // on propertyMatchDict after adding it to matchingDict.
            CFDictionarySetValue(matchingDict, CFSTR(kIOPropertyMatchKey), propertyMatchDict);
            CFRelease(propertyMatchDict);
        }
    }

    // IOServiceGetMatchingServices retains the returned iterator, so release the iterator when we're done with it.
    // IOServiceGetMatchingServices also consumes a reference on the matching dictionary so we don't need to release
    // the dictionary explicitly.
    kernResult = IOServiceGetMatchingServices(masterPort, matchingDict, matchingServices);
    if (KERN_SUCCESS != kernResult)
    {
        printf("IOServiceGetMatchingServices returned %d\n", kernResult);
    }

    return kernResult;
}

// Given an iterator across a set of Ethernet interfaces, return the MAC address of the last one.
// If no interfaces are found the MAC address is set to an empty string.
// In this sample the iterator should contain just the primary interface.
static kern_return_t GetMACAddress(io_iterator_t intfIterator, UInt8 *MACAddress)
{
    io_object_t    intfService;
    io_object_t    controllerService;
    kern_return_t  kernResult = KERN_FAILURE;

    // Initialize the returned address
    bzero(MACAddress, kIOEthernetAddressSize);

    // IOIteratorNext retains the returned object, so release it when we're done with it.
    while (intfService = IOIteratorNext(intfIterator))
    {
        CFTypeRef  MACAddressAsCFData;

        // IONetworkControllers can't be found directly by the IOServiceGetMatchingServices call,
        // since they are hardware nubs and do not participate in driver matching. In other words,
        // registerService() is never called on them. So we've found the IONetworkInterface and will
        // get its parent controller by asking for it specifically.

        // IORegistryEntryGetParentEntry retains the returned object, so release it when we're done with it.
        kernResult = IORegistryEntryGetParentEntry( intfService,
                                                    kIOServicePlane,
                                                    &controllerService );

        if (KERN_SUCCESS != kernResult)
        {
            printf("IORegistryEntryGetParentEntry returned 0x%08x\n", kernResult);
        }
        else
        {
            // Retrieve the MAC address property from the I/O Registry in the form of a CFData
            MACAddressAsCFData = IORegistryEntryCreateCFProperty( controllerService,
                                                                  CFSTR(kIOMACAddress),
                                                                  kCFAllocatorDefault,
                                                                  0);
            if (MACAddressAsCFData)
            {
                // Get the raw bytes of the MAC address from the CFData
                CFDataGetBytes((CFDataRef)MACAddressAsCFData, CFRangeMake(0, kIOEthernetAddressSize), MACAddress);
                CFRelease(MACAddressAsCFData);
            }
            else
            {
              printf("Error obtaining MAC Address.\n");
            }

            // Done with the parent Ethernet controller object so we release it.
            IOObjectRelease(controllerService);
        }

        // Done with the Ethernet interface object so we release it.
        IOObjectRelease(intfService);
    }

    return kernResult;
}


///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetPrimaryMacAddress()
{
  io_iterator_t  intfIterator;
  UInt8          MACAddress[kIOEthernetAddressSize];
  kern_return_t  kernResult = KERN_SUCCESS;
  string         ret;

  memset(MACAddress, 0, sizeof(MACAddress));
  kernResult = FindEthernetInterfaces(&intfIterator);

  if (KERN_SUCCESS != kernResult)
  {
    printf("FindEthernetInterfaces returned 0x%08x\n", kernResult);
  }
  else
  {
    kernResult = GetMACAddress(intfIterator, MACAddress);
    if (KERN_SUCCESS == kernResult)
    {
      for (int i=0; i<kIOEthernetAddressSize; i++)
      {
        char digit[3];
        sprintf(digit, "%02x", MACAddress[i]);
        ret += digit;
      }
    }
  }

  IOObjectRelease(intfIterator);
  return ret;
}
#endif
