/*
 * iremoted.c
 * Display events received from the Apple Infrared Remote.
 *
 * gcc -Wall -o iremoted iremoted.c -framework IOKit -framework Carbon
 *
 * Copyright (c) 2006-2008 Amit Singh. All Rights Reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *     
 *  THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
 */

#define PROGNAME "iremoted"
#define PROGVERS "2.0"

#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/errno.h>
#include <sysexits.h>
#include <mach/mach.h>
#include <mach/mach_error.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/sysctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <mach-o/dyld.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/hid/IOHIDUsageTables.h>

#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>

#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <iterator>
#include <sstream>
#include <set>

#include "AppleRemote.h"
#include "XBox360.h"

using namespace std;

void ParseOptions(int argc, char** argv);
void ReadConfig();

static struct option long_options[] = {
  { "help",       no_argument,       0, 'h' },
  { "server",     required_argument, 0, 's' },
  { "universal",  no_argument,       0, 'u' },
  { "multiremote",no_argument,       0, 'm' },
  { "timeout",    required_argument, 0, 't' },
  { "verbose",    no_argument,       0, 'v' },
  { "externalConfig", no_argument,   0, 'x' },
  { "appPath",    required_argument, 0, 'a' },
  { "appHome",    required_argument, 0, 'z' },
  { 0, 0, 0, 0 },
};

static const char *options = "hsumtvxaz";

IOHIDElementCookie buttonNextID = 0;
IOHIDElementCookie buttonPreviousID = 0;

std::set<IOHIDElementCookie> g_registeredCookies;

AppleRemote g_appleRemote;

void            usage(void);
inline          void print_errmsg_if_io_err(int expr, char *msg);
inline          void print_errmsg_if_err(int expr, char *msg);
void            QueueCallbackFunction(void *target, IOReturn result,
                                      void *refcon, void *sender);
bool            addQueueCallbacks(IOHIDQueueInterface **hqi);
void            processQueue(IOHIDDeviceInterface **hidDeviceInterface);
void            doRun(IOHIDDeviceInterface **hidDeviceInterface);
void			getHIDCookies(IOHIDDeviceInterface122 **handle);
void            createHIDDeviceInterface(io_object_t hidDevice,
                                         IOHIDDeviceInterface ***hdi);
void            setupAndRun(void);

void
usage(void)
{
    printf("%s (version %s)\n", PROGNAME, PROGVERS);
    printf("   Sends Apple Remote events to XBMC.\n\n");
    printf("Usage: %s [OPTIONS...]\n\nOptions:\n", PROGNAME);
    printf("  -h, --help           print this help message and exit.\n");
    printf("  -s, --server <addr>  send events to the specified IP.\n");
    printf("  -u, --universal      runs in Universal Remote mode.\n");
    printf("  -t, --timeout <ms>   timeout length for sequences (default: 500ms).\n");
    printf("  -a, --appPath        path to XBMC.app (MenuPress launch support).\n");
    printf("  -z, --appHome        path to XBMC.app/Content/Resources/XBMX \n");
    printf("  -v, --verbose        prints lots of debugging information.\n");
}

inline void
print_errmsg_if_io_err(int expr, char *msg)
{
    IOReturn err = (expr);

    if (err != kIOReturnSuccess) {
        fprintf(stderr, "*** %s - %s(%x, %d).\n", msg, mach_error_string(err),
                err, err & 0xffffff);
        fflush(stderr);
        exit(EX_OSERR);
    }
}

inline void
print_errmsg_if_err(int expr, char *msg)
{
    if (expr) {
        fprintf(stderr, "*** %s.\n", msg);
        fflush(stderr);
        exit(EX_OSERR);
    }
}

void
QueueCallbackFunction(void *target, IOReturn result, void *refcon, void *sender)
{
  HRESULT               ret = kIOReturnSuccess;
  AbsoluteTime          zeroTime = {0,0};
  IOHIDQueueInterface **hqi;
  IOHIDEventStruct      event;

  std::set<int> events;	
  bool bKeyDown = false;
  while (ret == kIOReturnSuccess) {
    hqi = (IOHIDQueueInterface **)sender;
    ret = (*hqi)->getNextEvent(hqi, &event, zeroTime, 0);
		if (ret != kIOReturnSuccess)
			continue;

      //printf("%d %d %d\n", (int)event.elementCookie, (int)event.value, (int)event.longValue);		

      if (event.value > 0)
        bKeyDown = true;
            
      events.insert((int)event.elementCookie);
  }
	
	if (events.size() > 1)
	{
		std::set<int>::iterator iter = events.find( g_appleRemote.GetButtonEventTerminator() );
		if (iter != events.end())
			events.erase(iter);
	}
	
	std::string strEvents;
	std::set<int>::const_iterator iter = events.begin();
	while (iter != events.end())
	{
		//printf("*iter = %d\n", *iter);		
		char strEvent[10];
		snprintf(strEvent, 10, "%d_", *iter); 
		strEvents += strEvent;
		iter++;
	}
			
	if (bKeyDown)
		g_appleRemote.OnKeyDown(strEvents);
	else
		g_appleRemote.OnKeyUp(strEvents);
}

bool
addQueueCallbacks(IOHIDQueueInterface **hqi)
{
    IOReturn               ret;
    CFRunLoopSourceRef     eventSource;
    IOHIDQueueInterface ***privateData;

    privateData = (IOHIDQueueInterface ***)malloc(sizeof(*privateData));
    *privateData = hqi;

    ret = (*hqi)->createAsyncEventSource(hqi, &eventSource);
    if (ret != kIOReturnSuccess)
        return false;

    ret = (*hqi)->setEventCallout(hqi, QueueCallbackFunction,
                                  NULL, &privateData);
    if (ret != kIOReturnSuccess)
        return false;

    CFRunLoopAddSource(CFRunLoopGetCurrent(), eventSource,
                       kCFRunLoopDefaultMode);
    return true;
}

void
processQueue(IOHIDDeviceInterface **hidDeviceInterface)
{
    HRESULT               result;
    IOHIDQueueInterface **queue;

    queue = (*hidDeviceInterface)->allocQueue(hidDeviceInterface);
    if (!queue) {
        fprintf(stderr, "Failed to allocate event queue.\n");
        return;
    }

    (void)(*queue)->create(queue, 0, 99);

    std::set<IOHIDElementCookie>::const_iterator iter = g_registeredCookies.begin();
	while (iter != g_registeredCookies.end())
	{
		(void)(*queue)->addElement(queue, *iter, 0);
		iter++;
	}
							   
    addQueueCallbacks(queue);

    result = (*queue)->start(queue);
    
    CFRunLoopRun();

    result = (*queue)->stop(queue);

    result = (*queue)->dispose(queue);

    (*queue)->Release(queue);
}

void
doRun(IOHIDDeviceInterface **hidDeviceInterface)
{
    IOReturn ioReturnValue;

    ioReturnValue = (*hidDeviceInterface)->open(hidDeviceInterface, kIOHIDOptionsTypeSeizeDevice);
	if (ioReturnValue == kIOReturnExclusiveAccess) {
		printf("exclusive lock failed\n");
	}
	
    processQueue(hidDeviceInterface);

    if (ioReturnValue == KERN_SUCCESS)
        ioReturnValue = (*hidDeviceInterface)->close(hidDeviceInterface);
    (*hidDeviceInterface)->Release(hidDeviceInterface);
}

void
getHIDCookies(IOHIDDeviceInterface122 **handle)
{
    IOHIDElementCookie cookie;
    CFTypeRef          object;
    long               number;
    long               usage;
    long               usagePage;
    CFArrayRef         elements;
    CFDictionaryRef    element;
    IOReturn           result;

    if (!handle || !(*handle))
        return ;

    result = (*handle)->copyMatchingElements(handle, NULL, &elements);

    if (result != kIOReturnSuccess) {
        fprintf(stderr, "Failed to copy cookies.\n");
        exit(1);
    }

    CFIndex i;
    for (i = 0; i < CFArrayGetCount(elements); i++) {
        element = (CFDictionaryRef)CFArrayGetValueAtIndex(elements, i);
        object = (CFDictionaryGetValue(element, CFSTR(kIOHIDElementCookieKey)));
        if (object == 0 || CFGetTypeID(object) != CFNumberGetTypeID())
            continue;
        if(!CFNumberGetValue((CFNumberRef) object, kCFNumberLongType, &number))
            continue;
        cookie = (IOHIDElementCookie)number;
        object = CFDictionaryGetValue(element, CFSTR(kIOHIDElementUsageKey));
        if (object == 0 || CFGetTypeID(object) != CFNumberGetTypeID())
            continue;
        if (!CFNumberGetValue((CFNumberRef)object, kCFNumberLongType, &number))
            continue;
        usage = number;
        object = CFDictionaryGetValue(element,CFSTR(kIOHIDElementUsagePageKey));
        if (object == 0 || CFGetTypeID(object) != CFNumberGetTypeID())
            continue;
        if (!CFNumberGetValue((CFNumberRef)object, kCFNumberLongType, &number))
            continue;
        usagePage = number;
		if (usagePage == 0x06 && usage == 0x22 ) // event code "39". menu+select for 5 secs
			g_registeredCookies.insert(cookie);
		else if (usagePage == 0xff01 && usage == 0x23 ) // event code "35". long click - select
			g_registeredCookies.insert(cookie);
		else if (usagePage == kHIDPage_GenericDesktop && usage >= 0x80 && usage <= 0x8d ) // regular keys
			g_registeredCookies.insert(cookie);
		else if (usagePage == kHIDPage_Consumer && usage >= 0xB0 && usage <= 0xBF)
			g_registeredCookies.insert(cookie);		
		else if (usagePage == kHIDPage_Consumer && usage == kHIDUsage_Csmr_Menu ) // event code "18". long "menu"
			g_registeredCookies.insert(cookie);		
    }
}

void
createHIDDeviceInterface(io_object_t hidDevice, IOHIDDeviceInterface ***hdi)
{
    io_name_t             className;
    IOCFPlugInInterface **plugInInterface = NULL;
    HRESULT               plugInResult = S_OK;
    SInt32                score = 0;
    IOReturn              ioReturnValue = kIOReturnSuccess;

    ioReturnValue = IOObjectGetClass(hidDevice, className);
    print_errmsg_if_io_err(ioReturnValue, "Failed to get class name.");

    ioReturnValue = IOCreatePlugInInterfaceForService(
                        hidDevice,
                        kIOHIDDeviceUserClientTypeID,
                        kIOCFPlugInInterfaceID,
                        &plugInInterface,
                        &score);

    if (ioReturnValue != kIOReturnSuccess)
        return;

    plugInResult = (*plugInInterface)->QueryInterface(
                        plugInInterface,
                        CFUUIDGetUUIDBytes(kIOHIDDeviceInterfaceID),
                        (LPVOID*)hdi);
    print_errmsg_if_err(plugInResult != S_OK,
                        "Failed to create device interface.\n");

    (*plugInInterface)->Release(plugInInterface);
}

void
setupAndRun(void)
{
    CFMutableDictionaryRef hidMatchDictionary = NULL;
    io_service_t           hidService = (io_service_t)0;
    io_object_t            hidDevice = (io_object_t)0;
    IOHIDDeviceInterface **hidDeviceInterface = NULL;
    IOReturn               ioReturnValue = kIOReturnSuccess;
    
    //hidMatchDictionary = IOServiceNameMatching("AppleIRController");
    hidMatchDictionary = IOServiceMatching("AppleIRController");
    hidService = IOServiceGetMatchingService(kIOMasterPortDefault,
                                             hidMatchDictionary);

    if (!hidService) {
        fprintf(stderr, "Apple Infrared Remote not found.\n");
        exit(1);
    }

    hidDevice = (io_object_t)hidService;

    createHIDDeviceInterface(hidDevice, &hidDeviceInterface);
    getHIDCookies((IOHIDDeviceInterface122 **)hidDeviceInterface);
    ioReturnValue = IOObjectRelease(hidDevice);
    print_errmsg_if_io_err(ioReturnValue, "Failed to release HID.");

    if (hidDeviceInterface == NULL) {
        fprintf(stderr, "No HID.\n");
        exit(1);
    }

    g_appleRemote.Initialize();
    doRun(hidDeviceInterface);

    if (ioReturnValue == KERN_SUCCESS)
        ioReturnValue = (*hidDeviceInterface)->close(hidDeviceInterface);

    (*hidDeviceInterface)->Release(hidDeviceInterface);
}

void Reconfigure(int nSignal)
{
	if (nSignal == SIGHUP) {
    	//fprintf(stderr, "Reconfigure\n");
		ReadConfig();
	} else {
		exit(0);
  	}
}

void ReadConfig()
{
	// Compute filename.
	string strFile = getenv("HOME");
	strFile += "/Library/Application Support/XBMC/XBMCHelper.conf";

	// Open file.
	ifstream ifs(strFile.c_str());
	if (!ifs)
		return;

	// Read file.
	stringstream oss;
	oss << ifs.rdbuf();

	if (!ifs && !ifs.eof())
		return;

	// Tokenize.
	string strData(oss.str());
	istringstream is(strData);
	vector<string> args = vector<string>(istream_iterator<string>(is), istream_iterator<string>());

	// Convert to char**.
	int argc = args.size() + 1;
	char** argv = new char*[argc + 1];
	int i = 0;
	argv[i++] = "XBMCHelper";

	for (vector<string>::iterator it = args.begin(); it != args.end(); )
		argv[i++] = (char* )(*it++).c_str();
	
	argv[i] = 0;

	// Parse the arguments.
	ParseOptions(argc, argv);

	delete[] argv;
}

void ParseOptions(int argc, char** argv)
{
  int c, option_index = 0;
	bool readExternal = false;
	
  while ((c = getopt_long(argc, argv, options, long_options, &option_index)) != -1) 
	{
    switch (c) {
    case 'h':
      usage();
      exit(0);
      break;
    case 'v':
      g_appleRemote.SetVerbose(true);
      break;
    case 's':
      g_appleRemote.SetServerAddress(optarg);
      break;
    case 'u':
      g_appleRemote.SetRemoteMode(REMOTE_UNIVERSAL);
      break;
    case 'm':
      break;
    case 't':
      if (optarg)
        g_appleRemote.SetMaxClickTimeout( atof(optarg) * 0.001 );
      break;
    case 'a':
      g_appleRemote.SetAppPath(optarg);
      break;
    case 'z':
      g_appleRemote.SetAppHome(optarg);
      break;
    case 'x':
      readExternal = true;
      break;
    default:
      usage();
      exit(1);
      break;
    }
  }
  //reset getopts state
  optreset = 1;
  optind = 0;
  
	if (readExternal == true)
		ReadConfig();	
}

int
main (int argc, char **argv)
{
	ParseOptions(argc,argv);
	
	signal(SIGHUP, Reconfigure);
	signal(SIGINT, Reconfigure);
	signal(SIGTERM, Reconfigure);

	XBox360 xbox360;
	xbox360.start();    
	
	setupAndRun();

	xbox360.join();
	
    return 0;
}
