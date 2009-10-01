/*****************************************************************************
 * RemoteControlWrapper.m
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

#import "AppleRemote.h"

#import <mach/mach.h>
#import <mach/mach_error.h>
#import <IOKit/IOKitLib.h>
#import <IOKit/IOCFPlugIn.h>
#import <IOKit/hid/IOHIDKeys.h>

const char* AppleRemoteDeviceName = "AppleIRController";

// the WWDC 07 Leopard Build is missing the constant
#ifndef NSAppKitVersionNumber10_4
#define NSAppKitVersionNumber10_4 824
#endif

@implementation AppleRemote

+ (const char*) remoteControlDeviceName {
	return AppleRemoteDeviceName;
}

- (void) setCookieMappingInDictionary: (NSMutableDictionary*) _cookieToButtonMapping	{	
	if (floor(NSAppKitVersionNumber) <= NSAppKitVersionNumber10_4) {
		// 10.4.x Tiger
		[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonPlus]		forKey:@"14_12_11_6_"];
		[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonMinus]		forKey:@"14_13_11_6_"];		
		[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonMenu]		forKey:@"14_7_6_14_7_6_"];			
		[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonPlay]		forKey:@"14_8_6_14_8_6_"];
		[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonRight]		forKey:@"14_9_6_14_9_6_"];
		[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonLeft]		forKey:@"14_10_6_14_10_6_"];
		[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonRight_Hold]	forKey:@"14_6_4_2_"];
		[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonLeft_Hold]	forKey:@"14_6_3_2_"];
		[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonMenu_Hold]	forKey:@"14_6_14_6_"];
		[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonPlay_Hold]	forKey:@"18_14_6_18_14_6_"];
		[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteControl_Switched]	forKey:@"19_"];			
	} else {
		// 10.5.x Leopard
		[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonPlus]		forKey:@"31_29_28_19_18_"];
		[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonMinus]		forKey:@"31_30_28_19_18_"];	
		[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonMenu]		forKey:@"31_20_19_18_31_20_19_18_"];
		[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonPlay]		forKey:@"31_21_19_18_31_21_19_18_"];
		[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonRight]		forKey:@"31_22_19_18_31_22_19_18_"];
		[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonLeft]		forKey:@"31_23_19_18_31_23_19_18_"];
		[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonRight_Hold]	forKey:@"31_19_18_4_2_"];
		[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonLeft_Hold]	forKey:@"31_19_18_3_2_"];
		[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonMenu_Hold]	forKey:@"31_19_18_31_19_18_"];
		[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteButtonPlay_Hold]	forKey:@"35_31_19_18_35_31_19_18_"];
		[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteControl_Switched]	forKey:@"19_"];	
		[_cookieToButtonMapping setObject:[NSNumber numberWithInt:kRemoteControl_Switched]	forKey:@"39_"];			
	}
}

- (void) sendRemoteButtonEvent: (RemoteControlEventIdentifier) event pressedDown: (BOOL) pressedDown {
	if (pressedDown == NO && event == kRemoteButtonMenu_Hold) {
		// There is no seperate event for pressed down on menu hold. We are simulating that event here
		[super sendRemoteButtonEvent:event pressedDown:YES];
	}	
	
	[super sendRemoteButtonEvent:event pressedDown:pressedDown];
	
	if (pressedDown && (event == kRemoteButtonRight || event == kRemoteButtonLeft || event == kRemoteButtonPlay || event == kRemoteButtonMenu || event == kRemoteButtonPlay_Hold)) {
		// There is no seperate event when the button is being released. We are simulating that event here
		[super sendRemoteButtonEvent:event pressedDown:NO];
	}
}

- (eCookieModifier) handleCookie: (long)f_cookie value:(int) f_value {
  switch(f_cookie)
  {
    case 39:
      deviceID = f_value;
      return PASS_COOKIE;
    default:
      return [super handleCookie:f_cookie value:f_value];
  }
}

- (int) deviceID {
  return deviceID;
}
@end
