/*****************************************************************************
 * KeyspanFrontRowControl.m
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
 
#import "KeyspanFrontRowControl.h"
#import <mach/mach.h>
#import <mach/mach_error.h>
#import <IOKit/IOKitLib.h>
#import <IOKit/IOCFPlugIn.h>
#import <IOKit/hid/IOHIDKeys.h>

@implementation KeyspanFrontRowControl

- (void) setCookieMappingInDictionary: (NSMutableDictionary*) _cookieToButtonMapping	{	
	[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonPlus]		 forKey:@"11_18_99_10_"];
	[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonMinus]		 forKey:@"11_18_98_10_"];		
	[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonMenu]		 forKey:@"11_18_58_10_"];		
	[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonPlay]		 forKey:@"11_18_61_10_"];
	[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonRight]		 forKey:@"11_18_96_10_"];
	[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonLeft]		 forKey:@"11_18_97_10_"];
	/* hold events are not being send by this device
	[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonRight_Hold]	forKey:@"14_6_4_2_"];
	[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonLeft_Hold]	forKey:@"14_6_3_2_"];
	[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonMenu_Hold]	forKey:@"14_6_14_6_"];
	[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonPlay_Sleep]	forKey:@"18_14_6_18_14_6_"];
	[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteControl_Switched]	forKey:@"19_"];			
	*/
}

+ (io_object_t) findRemoteDevice {
	CFMutableDictionaryRef hidMatchDictionary = NULL;
	IOReturn ioReturnValue = kIOReturnSuccess;	
	io_iterator_t hidObjectIterator = 0;
	io_object_t	hidDevice = 0;
	SInt32                  idVendor = 1741;
	SInt32                  idProduct = 0x420;	
	
	// Set up a matching dictionary to search the I/O Registry by class
	// name for all HID class devices
	hidMatchDictionary = IOServiceMatching(kIOHIDDeviceKey);
	
	CFNumberRef numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &idVendor);
    CFDictionaryAddValue(hidMatchDictionary, CFSTR(kIOHIDVendorIDKey), numberRef);
    CFRelease(numberRef);
	
    numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &idProduct);
    CFDictionaryAddValue(hidMatchDictionary, CFSTR(kIOHIDProductIDKey), numberRef);
    CFRelease(numberRef);	
	
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
