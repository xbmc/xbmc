/*****************************************************************************
 * RemoteControl.m
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
 
#import "RemoteControl.h"

// notifaction names that are being used to signal that an application wants to 
// have access to the remote control device or if the application has finished
// using the remote control device
NSString* REQUEST_FOR_REMOTE_CONTROL_NOTIFCATION     = @"mac.remotecontrols.RequestForRemoteControl";
NSString* FINISHED_USING_REMOTE_CONTROL_NOTIFICATION = @"mac.remotecontrols.FinishedUsingRemoteControl";

// keys used in user objects for distributed notifications
NSString* kRemoteControlDeviceName = @"RemoteControlDeviceName";
NSString* kApplicationIdentifier   = @"CFBundleIdentifier";
// bundle identifier of the application that should get access to the remote control
// this key is being used in the FINISHED notification only
NSString* kTargetApplicationIdentifier = @"TargetBundleIdentifier";


@implementation RemoteControl

// returns nil if the remote control device is not available
- (id) initWithDelegate: (id) _remoteControlDelegate {	
	if (self = [super init]) {
		delegate = [_remoteControlDelegate retain];
	}
	return self;
}

- (void) dealloc {
	[delegate release];
	[super dealloc];
}

- (void) setListeningToRemote: (BOOL) value {
}
- (BOOL) isListeningToRemote {
	return NO;
}

- (IBAction) startListening: (id) sender {
}
- (IBAction) stopListening: (id) sender {
	
}

- (BOOL) isOpenInExclusiveMode {
	return YES;
}
- (void) setOpenInExclusiveMode: (BOOL) value {
}

- (BOOL) sendsEventForButtonIdentifier: (RemoteControlEventIdentifier) identifier {
	return YES;
}

+ (void) sendDistributedNotification: (NSString*) notificationName targetBundleIdentifier: (NSString*) targetIdentifier {
	NSDictionary* userInfo = [NSDictionary dictionaryWithObjectsAndKeys: [NSString stringWithCString:[self remoteControlDeviceName] encoding:NSASCIIStringEncoding],
		kRemoteControlDeviceName, [[NSBundle mainBundle] bundleIdentifier], kApplicationIdentifier, 
		targetIdentifier, kTargetApplicationIdentifier, nil];
	
	[[NSDistributedNotificationCenter defaultCenter] postNotificationName:notificationName
																   object:nil
																 userInfo:userInfo
													   deliverImmediately:YES];	
}

+ (void) sendFinishedNotifcationForAppIdentifier: (NSString*) identifier {
	[self sendDistributedNotification:FINISHED_USING_REMOTE_CONTROL_NOTIFICATION targetBundleIdentifier:identifier];
}
+ (void) sendRequestForRemoteControlNotification {
	[self sendDistributedNotification:REQUEST_FOR_REMOTE_CONTROL_NOTIFCATION targetBundleIdentifier:nil];
}

+ (const char*) remoteControlDeviceName {
	return NULL;
}

@end
