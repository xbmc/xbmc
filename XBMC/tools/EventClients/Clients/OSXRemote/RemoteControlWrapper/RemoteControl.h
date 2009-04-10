/*****************************************************************************
 * RemoteControl.h
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
 
#import <Cocoa/Cocoa.h>

// notifaction names that are being used to signal that an application wants to 
// have access to the remote control device or if the application has finished
// using the remote control device
extern NSString* REQUEST_FOR_REMOTE_CONTROL_NOTIFCATION;
extern NSString* FINISHED_USING_REMOTE_CONTROL_NOTIFICATION;

// keys used in user objects for distributed notifications
extern NSString* kRemoteControlDeviceName;
extern NSString* kApplicationIdentifier;
extern NSString* kTargetApplicationIdentifier;

// we have a 6 bit offset to make a hold event out of a normal event
#define EVENT_TO_HOLD_EVENT_OFFSET 6 

@class RemoteControl;

typedef enum _RemoteControlEventIdentifier {
	// normal events
	kRemoteButtonPlus				=1<<1,
	kRemoteButtonMinus				=1<<2,
	kRemoteButtonMenu				=1<<3,
	kRemoteButtonPlay				=1<<4,
	kRemoteButtonRight				=1<<5,
	kRemoteButtonLeft				=1<<6,
	
	// hold events
	kRemoteButtonPlus_Hold			=1<<7,
	kRemoteButtonMinus_Hold			=1<<8,	
	kRemoteButtonMenu_Hold			=1<<9,	
	kRemoteButtonPlay_Hold			=1<<10,	
	kRemoteButtonRight_Hold			=1<<11,
	kRemoteButtonLeft_Hold			=1<<12,
	
	// special events (not supported by all devices)	
	kRemoteControl_Switched			=1<<13,
} RemoteControlEventIdentifier;

@interface NSObject(RemoteControlDelegate)

- (void) sendRemoteButtonEvent: (RemoteControlEventIdentifier) event pressedDown: (BOOL) pressedDown remoteControl: (RemoteControl*) remoteControl;

@end

/*
	Base Interface for Remote Control devices
*/
@interface RemoteControl : NSObject {
	id delegate;
}

// returns nil if the remote control device is not available
- (id) initWithDelegate: (id) remoteControlDelegate;

- (void) setListeningToRemote: (BOOL) value;
- (BOOL) isListeningToRemote;

- (BOOL) isOpenInExclusiveMode;
- (void) setOpenInExclusiveMode: (BOOL) value;

- (IBAction) startListening: (id) sender;
- (IBAction) stopListening: (id) sender;

// is this remote control sending the given event?
- (BOOL) sendsEventForButtonIdentifier: (RemoteControlEventIdentifier) identifier;

// sending of notifications between applications
+ (void) sendFinishedNotifcationForAppIdentifier: (NSString*) identifier;
+ (void) sendRequestForRemoteControlNotification;

// name of the device
+ (const char*) remoteControlDeviceName;

@end
