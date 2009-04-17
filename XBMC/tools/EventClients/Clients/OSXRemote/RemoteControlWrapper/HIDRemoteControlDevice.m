/*****************************************************************************
 * HIDRemoteControlDevice.m
 * RemoteControlWrapper
 *
 * Created by Martin Kahr on 11.03.06 under a MIT-style license. 
 * Copyright (c) 2006 martinkahr.com. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 *****************************************************************************/

#import "HIDRemoteControlDevice.h"

#import <mach/mach.h>
#import <mach/mach_error.h>
#import <IOKit/IOKitLib.h>
#import <IOKit/IOCFPlugIn.h>
#import <IOKit/hid/IOHIDKeys.h>
#import <Carbon/Carbon.h>

@interface HIDRemoteControlDevice (PrivateMethods) 
- (NSDictionary*) cookieToButtonMapping;
- (IOHIDQueueInterface**) queue;
- (IOHIDDeviceInterface**) hidDeviceInterface;
- (void) removeNotifcationObserver;
- (void) remoteControlAvailable:(NSNotification *)notification;

@end

@interface HIDRemoteControlDevice (IOKitMethods) 
+ (io_object_t) findRemoteDevice;
- (IOHIDDeviceInterface**) createInterfaceForDevice: (io_object_t) hidDevice;
- (BOOL) initializeCookies;
- (BOOL) openDevice;
@end

@implementation HIDRemoteControlDevice

+ (const char*) remoteControlDeviceName {
	return "";
}

+ (BOOL) isRemoteAvailable {	
	io_object_t hidDevice = [self findRemoteDevice];
	if (hidDevice != 0) {
		IOObjectRelease(hidDevice);
		return YES;
	} else {
		return NO;		
	}
}

- (id) initWithDelegate: (id) _remoteControlDelegate {	
	if ([[self class] isRemoteAvailable] == NO) return nil;
	
	if ( self = [super initWithDelegate: _remoteControlDelegate] ) {
		openInExclusiveMode = YES;
		queue = NULL;
		hidDeviceInterface = NULL;
		cookieToButtonMapping = [[NSMutableDictionary alloc] init];
		
		[self setCookieMappingInDictionary: cookieToButtonMapping];
    
		NSEnumerator* enumerator = [cookieToButtonMapping objectEnumerator];
		NSNumber* identifier;
		supportedButtonEvents = 0;
		while(identifier = [enumerator nextObject]) {
			supportedButtonEvents |= [identifier intValue];
		}
		
		fixSecureEventInputBug = [[NSUserDefaults standardUserDefaults] boolForKey: @"remoteControlWrapperFixSecureEventInputBug"];
	}
	
	return self;
}

- (void) dealloc {
	[self removeNotifcationObserver];
	[self stopListening:self];
	[cookieToButtonMapping release];
	[super dealloc];
}

- (void) sendRemoteButtonEvent: (RemoteControlEventIdentifier) event pressedDown: (BOOL) pressedDown {
	[delegate sendRemoteButtonEvent: event pressedDown: pressedDown remoteControl:self];
}

- (void) setCookieMappingInDictionary: (NSMutableDictionary*) cookieToButtonMapping {
}
- (int) remoteIdSwitchCookie {
	return 0;
}

- (BOOL) sendsEventForButtonIdentifier: (RemoteControlEventIdentifier) identifier {
	return (supportedButtonEvents & identifier) == identifier;
}

- (BOOL) isListeningToRemote {
	return (hidDeviceInterface != NULL && allCookies != NULL && queue != NULL);	
}

- (void) setListeningToRemote: (BOOL) value {
	if (value == NO) {
		[self stopListening:self];
	} else {
		[self startListening:self];
	}
}

- (BOOL) isOpenInExclusiveMode {
	return openInExclusiveMode;
}
- (void) setOpenInExclusiveMode: (BOOL) value {
	openInExclusiveMode = value;
}

- (BOOL) processesBacklog {
	return processesBacklog;
}
- (void) setProcessesBacklog: (BOOL) value {
	processesBacklog = value;
}

- (IBAction) startListening: (id) sender {	
	if ([self isListeningToRemote]) return;
	
	// 4th July 2007
	// 
	// A security update in february of 2007 introduced an odd behavior.
	// Whenever SecureEventInput is activated or deactivated the exclusive access
	// to the remote control device is lost. This leads to very strange behavior where
	// a press on the Menu button activates FrontRow while your app still gets the event.
	// A great number of people have complained about this.	
	// 
	// Enabling the SecureEventInput and keeping it enabled does the trick.
	//
	// I'm pretty sure this is a kind of bug at Apple and I'm in contact with the responsible
	// Apple Engineer. This solution is not a perfect one - I know. 	
	// One of the side effects is that applications that listen for special global keyboard shortcuts (like Quicksilver)
	// may get into problems as they no longer get the events.
	// As there is no official Apple Remote API from Apple I also failed to open a technical incident on this.
	// 
	// Note that there is a corresponding DisableSecureEventInput in the stopListening method below.
	// 
	if ([self isOpenInExclusiveMode] && fixSecureEventInputBug) EnableSecureEventInput();	
	
	[self removeNotifcationObserver];
	
	io_object_t hidDevice = [[self class] findRemoteDevice];
	if (hidDevice == 0) return;
	
	if ([self createInterfaceForDevice:hidDevice] == NULL) {
		goto error;
	}
	
	if ([self initializeCookies]==NO) {
		goto error;
	}
  
	if ([self openDevice]==NO) {
		goto error;
	}
	// be KVO friendly
	[self willChangeValueForKey:@"listeningToRemote"];
	[self didChangeValueForKey:@"listeningToRemote"];
	goto cleanup;
	
error:
	[self stopListening:self];
	DisableSecureEventInput();
	
cleanup:	
	IOObjectRelease(hidDevice);	
}

- (IBAction) stopListening: (id) sender {
	if ([self isListeningToRemote]==NO) return;
	
	BOOL sendNotification = NO;
	
	if (eventSource != NULL) {
		CFRunLoopRemoveSource(CFRunLoopGetCurrent(), eventSource, kCFRunLoopDefaultMode);
		CFRelease(eventSource);
		eventSource = NULL;
	}
	if (queue != NULL) {
		(*queue)->stop(queue);		
		
		//dispose of queue
		(*queue)->dispose(queue);		
		
		//release the queue we allocated
		(*queue)->Release(queue);	
		
		queue = NULL;
		
		sendNotification = YES;
	}
	
	if (allCookies != nil) {
		[allCookies autorelease];
		allCookies = nil;
	}
	
	if (hidDeviceInterface != NULL) {
		//close the device
		(*hidDeviceInterface)->close(hidDeviceInterface);
		
		//release the interface	
		(*hidDeviceInterface)->Release(hidDeviceInterface);
		
		hidDeviceInterface = NULL;
	}
	
	if ([self isOpenInExclusiveMode] && fixSecureEventInputBug) DisableSecureEventInput();
	
	if ([self isOpenInExclusiveMode] && sendNotification) {
		[[self class] sendFinishedNotifcationForAppIdentifier: nil];		
	}
	// be KVO friendly
	[self willChangeValueForKey:@"listeningToRemote"];
	[self didChangeValueForKey:@"listeningToRemote"];	
}

- (eCookieModifier) handleCookie: (long)f_cookie value:(int) f_value {
  if(f_cookie == 5)
    return DISCARD_COOKIE;
  else
    return PASS_COOKIE;
}

@end

@implementation HIDRemoteControlDevice (PrivateMethods) 

- (IOHIDQueueInterface**) queue {
	return queue;
}

- (IOHIDDeviceInterface**) hidDeviceInterface {
	return hidDeviceInterface;
}


- (NSDictionary*) cookieToButtonMapping {
	return cookieToButtonMapping;
}

- (void) removeNotifcationObserver {
	[[NSDistributedNotificationCenter defaultCenter] removeObserver:self name:FINISHED_USING_REMOTE_CONTROL_NOTIFICATION object:nil];
}

- (void) remoteControlAvailable:(NSNotification *)notification {
	[self removeNotifcationObserver];
	[self startListening: self];
}

@end

/*	Callback method for the device queue
 Will be called for any event of any type (cookie) to which we subscribe
 */
static void QueueCallbackFunction(void* target,  IOReturn result, void* refcon, void* sender) {	
	if (target < 0) {
		NSLog(@"QueueCallbackFunction called with invalid target!");
		return;
	}
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	
	HIDRemoteControlDevice* remote = (HIDRemoteControlDevice*)target;	
	IOHIDEventStruct event;	
	AbsoluteTime 	 zeroTime = {0,0};
	NSMutableString* cookieString = [NSMutableString stringWithCapacity: 10];
	SInt32			 sumOfValues = 0;
  NSNumber* lastSubButtonId = nil;
	while (result == kIOReturnSuccess)
	{
		result = (*[remote queue])->getNextEvent([remote queue], &event, zeroTime, 0);		
		if ( result != kIOReturnSuccess )
			continue;
    
		//printf("%d %d %d\n", event.elementCookie, event.value, event.longValue);
    if([remote handleCookie:(long)event.elementCookie value:event.value] != DISCARD_COOKIE){
			sumOfValues+=event.value;
			[cookieString appendString:[NSString stringWithFormat:@"%d_", event.elementCookie]];
		}
    //check if this is a valid button
    NSNumber* buttonId = [[remote cookieToButtonMapping] objectForKey: cookieString];
    if (buttonId != nil){
      if([remote processesBacklog]) {
        //send the button
        [remote sendRemoteButtonEvent: [buttonId intValue] pressedDown: (sumOfValues>0)];
        //reset
      } else {
        //store button for later use
        lastSubButtonId = buttonId;
      }
      sumOfValues = 0;
      cookieString = [NSMutableString stringWithCapacity: 10];
    }
	}
  if ([remote processesBacklog] == NO && lastSubButtonId != nil) {
    // process the last event of the backlog and assume that the button is not pressed down any longer.
    // The events in the backlog do not seem to be in order and therefore (in rare cases) the last event might be 
    // a button pressed down event while in reality the user has released it. 
    // NSLog(@"processing last event of backlog");
    [remote sendRemoteButtonEvent: [lastSubButtonId intValue] pressedDown: (sumOfValues>0)];
  }
  if ([cookieString length] > 0) {
    NSLog(@"Unknown button for cookiestring %@", cookieString);
  }  
	[pool release];
}

@implementation HIDRemoteControlDevice (IOKitMethods)

- (IOHIDDeviceInterface**) createInterfaceForDevice: (io_object_t) hidDevice {
	io_name_t				className;
	IOCFPlugInInterface**   plugInInterface = NULL;
	HRESULT					plugInResult = S_OK;
	SInt32					score = 0;
	IOReturn				ioReturnValue = kIOReturnSuccess;
	
	hidDeviceInterface = NULL;
	
	ioReturnValue = IOObjectGetClass(hidDevice, className);
	
	if (ioReturnValue != kIOReturnSuccess) {
		NSLog(@"Error: Failed to get class name.");
		return NULL;
	}
	
	ioReturnValue = IOCreatePlugInInterfaceForService(hidDevice,
                                                    kIOHIDDeviceUserClientTypeID,
                                                    kIOCFPlugInInterfaceID,
                                                    &plugInInterface,
                                                    &score);
	if (ioReturnValue == kIOReturnSuccess)
	{
		//Call a method of the intermediate plug-in to create the device interface
		plugInResult = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOHIDDeviceInterfaceID), (LPVOID) &hidDeviceInterface);
		
		if (plugInResult != S_OK) {
			NSLog(@"Error: Couldn't create HID class device interface");
		}
		// Release
		if (plugInInterface) (*plugInInterface)->Release(plugInInterface);
	}
	return hidDeviceInterface;
}

- (BOOL) initializeCookies {
	IOHIDDeviceInterface122** handle = (IOHIDDeviceInterface122**)hidDeviceInterface;
	IOHIDElementCookie		cookie;
	long					usage;
	long					usagePage;
	id						object;
	NSArray*				elements = nil;
	NSDictionary*			element;
	IOReturn success;
	
	if (!handle || !(*handle)) return NO;
	
	// Copy all elements, since we're grabbing most of the elements
	// for this device anyway, and thus, it's faster to iterate them
	// ourselves. When grabbing only one or two elements, a matching
	// dictionary should be passed in here instead of NULL.
	success = (*handle)->copyMatchingElements(handle, NULL, (CFArrayRef*)&elements);
	
	if (success == kIOReturnSuccess) {
		
		[elements autorelease];		
		/*
     cookies = calloc(NUMBER_OF_APPLE_REMOTE_ACTIONS, sizeof(IOHIDElementCookie)); 
     memset(cookies, 0, sizeof(IOHIDElementCookie) * NUMBER_OF_APPLE_REMOTE_ACTIONS);
     */
		allCookies = [[NSMutableArray alloc] init];
		
		NSEnumerator *elementsEnumerator = [elements objectEnumerator];
		
		while (element = [elementsEnumerator nextObject]) {						
			//Get cookie
			object = [element valueForKey: (NSString*)CFSTR(kIOHIDElementCookieKey) ];
			if (object == nil || ![object isKindOfClass:[NSNumber class]]) continue;
			if (object == 0 || CFGetTypeID(object) != CFNumberGetTypeID()) continue;
			cookie = (IOHIDElementCookie) [object longValue];
			
			//Get usage
			object = [element valueForKey: (NSString*)CFSTR(kIOHIDElementUsageKey) ];
			if (object == nil || ![object isKindOfClass:[NSNumber class]]) continue;			
			usage = [object longValue];
			
			//Get usage page
			object = [element valueForKey: (NSString*)CFSTR(kIOHIDElementUsagePageKey) ];
			if (object == nil || ![object isKindOfClass:[NSNumber class]]) continue;			
			usagePage = [object longValue];
      
			[allCookies addObject: [NSNumber numberWithInt:(int)cookie]];
		}
	} else {
		return NO;
	}
	
	return YES;
}

- (BOOL) openDevice {
	HRESULT  result;
	
	IOHIDOptionsType openMode = kIOHIDOptionsTypeNone;
	if ([self isOpenInExclusiveMode]) openMode = kIOHIDOptionsTypeSeizeDevice;	
	IOReturn ioReturnValue = (*hidDeviceInterface)->open(hidDeviceInterface, openMode);	
	
	if (ioReturnValue == KERN_SUCCESS) {		
		queue = (*hidDeviceInterface)->allocQueue(hidDeviceInterface);
		if (queue) {
			result = (*queue)->create(queue, 0, 12);	//depth: maximum number of elements in queue before oldest elements in queue begin to be lost.
      
			IOHIDElementCookie cookie;
			NSEnumerator *allCookiesEnumerator = [allCookies objectEnumerator];
			
			while (cookie = (IOHIDElementCookie)[[allCookiesEnumerator nextObject] intValue]) {
				(*queue)->addElement(queue, cookie, 0);
			}
      
			// add callback for async events			
			ioReturnValue = (*queue)->createAsyncEventSource(queue, &eventSource);			
			if (ioReturnValue == KERN_SUCCESS) {
				ioReturnValue = (*queue)->setEventCallout(queue,QueueCallbackFunction, self, NULL);
				if (ioReturnValue == KERN_SUCCESS) {
					CFRunLoopAddSource(CFRunLoopGetCurrent(), eventSource, kCFRunLoopDefaultMode);
					
					//start data delivery to queue
					(*queue)->start(queue);	
					return YES;
				} else {
					NSLog(@"Error when setting event callback");
				}
			} else {
				NSLog(@"Error when creating async event source");
			}
		} else {
			NSLog(@"Error when opening device");
		}
	} else if (ioReturnValue == kIOReturnExclusiveAccess) {
		// the device is used exclusive by another application
		
		// 1. we register for the FINISHED_USING_REMOTE_CONTROL_NOTIFICATION notification
		[[NSDistributedNotificationCenter defaultCenter] addObserver:self selector:@selector(remoteControlAvailable:) name:FINISHED_USING_REMOTE_CONTROL_NOTIFICATION object:nil];
		
		// 2. send a distributed notification that we wanted to use the remote control				
		[[self class] sendRequestForRemoteControlNotification];
	}
	return NO;				
}

+ (io_object_t) findRemoteDevice {
	CFMutableDictionaryRef hidMatchDictionary = NULL;
	IOReturn ioReturnValue = kIOReturnSuccess;	
	io_iterator_t hidObjectIterator = 0;
	io_object_t	hidDevice = 0;
	
	// Set up a matching dictionary to search the I/O Registry by class
	// name for all HID class devices
	hidMatchDictionary = IOServiceMatching([self remoteControlDeviceName]);
	
	// Now search I/O Registry for matching devices.
	ioReturnValue = IOServiceGetMatchingServices(kIOMasterPortDefault, hidMatchDictionary, &hidObjectIterator);
	
	if ((ioReturnValue == kIOReturnSuccess) && (hidObjectIterator != 0)) {
		hidDevice = IOIteratorNext(hidObjectIterator);
	}
	
	// release the iterator
	IOObjectRelease(hidObjectIterator);
	
	return hidDevice;
}

@end

