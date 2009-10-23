//
//  HIDRemote.h
//  HIDRemote V1.0
//
//  Created by Felix Schwarz on 06.04.07.
//  Copyright 2007-2009 IOSPIRIT GmbH. All rights reserved.
//
//  ** LICENSE *************************************************************************
//
//  Copyright (c) 2007-2009 IOSPIRIT GmbH
//  All rights reserved.
//  
//  Redistribution and use in source and binary forms, with or without modification,
//  are permitted provided that the following conditions are met:
//  
//  * Redistributions of source code must retain the above copyright notice, this list
//    of conditions and the following disclaimer.
//  
//  * Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//  
//  * Neither the name of IOSPIRIT GmbH nor the names of its contributors may be used to
//    endorse or promote products derived from this software without specific prior
//    written permission.
//  
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
//  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
//  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
//  SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
//  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
//  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
//  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
//  DAMAGE.
//
//  ************************************************************************************

#import <Cocoa/Cocoa.h>

#include <Carbon/Carbon.h>

#include <unistd.h>
#include <mach/mach.h>
#include <sys/types.h>

#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/IOMessage.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDUsageTables.h>
#include <IOKit/hidsystem/IOHIDLib.h>
#include <IOKit/hidsystem/IOHIDParameter.h>
#include <IOKit/hidsystem/IOHIDShared.h>

#pragma mark -- Enums / Codes  --

typedef enum
{
	kHIDRemoteModeNone = 0L,
	kHIDRemoteModeShared,		// Share the remote with others - let's you listen to the remote control events as long as noone has an exclusive lock on it
					// (RECOMMENDED ONLY FOR SPECIAL PURPOSES)

	kHIDRemoteModeExclusive,	// Try to acquire an exclusive lock on the remote (NOT RECOMMENDED)

	kHIDRemoteModeExclusiveAuto	// Try to acquire an exclusive lock on the remote whenever the application has focus. Temporarily release control over the
					// remote when another application has focus (RECOMMENDED)
} HIDRemoteMode;

typedef enum
{
	/* A code reserved for "no button" (needed for tracking) */
	kHIDRemoteButtonCodeNone	= 0L,

	/* HID Remote standard codes - you'll be able to receive all of these in your HIDRemote delegate */
	kHIDRemoteButtonCodePlus,
	kHIDRemoteButtonCodeMinus,
	kHIDRemoteButtonCodeLeft,
	kHIDRemoteButtonCodeRight,
	kHIDRemoteButtonCodePlayPause,
	kHIDRemoteButtonCodeMenu,

	/* Masks */
	kHIDRemoteButtonCodeCodeMask    = 0xFFL,
	kHIDRemoteButtonCodeHoldMask    = (1L << 16L),
	kHIDRemoteButtonCodeSpecialMask = (1L << 17L),

	/* Hold button codes */
	kHIDRemoteButtonCodePlusHold      = (kHIDRemoteButtonCodeHoldMask|kHIDRemoteButtonCodePlus),
	kHIDRemoteButtonCodeMinusHold     = (kHIDRemoteButtonCodeHoldMask|kHIDRemoteButtonCodeMinus),
	kHIDRemoteButtonCodeLeftHold      = (kHIDRemoteButtonCodeHoldMask|kHIDRemoteButtonCodeLeft),
	kHIDRemoteButtonCodeRightHold     = (kHIDRemoteButtonCodeHoldMask|kHIDRemoteButtonCodeRight),
	kHIDRemoteButtonCodePlayPauseHold = (kHIDRemoteButtonCodeHoldMask|kHIDRemoteButtonCodePlayPause),
	kHIDRemoteButtonCodeMenuHold	  = (kHIDRemoteButtonCodeHoldMask|kHIDRemoteButtonCodeMenu),

	/* Special purpose codes */
	kHIDRemoteButtonCodeIDChanged  = (kHIDRemoteButtonCodeSpecialMask|(1L << 18L)),	// (the ID of the connected remote has changed, you can safely ignore this)
	#ifdef _HIDREMOTE_EXTENSIONS
		#define _HIDREMOTE_EXTENSIONS_SECTION 1
		#include "HIDRemoteAdditions.h"
		#undef _HIDREMOTE_EXTENSIONS_SECTION
	#endif /* _HIDREMOTE_EXTENSIONS */
} HIDRemoteButtonCode;

@class HIDRemote;

#pragma mark -- Delegate protocol (mandatory) --
@protocol HIDRemoteDelegate

// Notification of button events
- (void)hidRemote:(HIDRemote *)hidRemote				// The instance of HIDRemote sending this
        eventWithButton:(HIDRemoteButtonCode)buttonCode			// Event for the button specified by code
	isPressed:(BOOL)isPressed;					// The button was pressed (YES) / released (NO)

@optional

// Notification of ID changes
- (void)hidRemote:(HIDRemote *)hidRemote
	remoteIDChangedOldID:(SInt32)old			// Invoked when the user switched to a remote control with a different ID
	newID:(SInt32)newID;

// Notification about hardware additions/removals 
- (void)hidRemote:(HIDRemote *)hidRemote				// Invoked when new hardware was found / added to HIDRemote's pool
	foundNewHardwareWithAttributes:(NSMutableDictionary *)attributes;

- (void)hidRemote:(HIDRemote *)hidRemote				// Invoked when initialization of new hardware as requested failed
	failedNewHardwareWithError:(NSError *)error;

- (void)hidRemote:(HIDRemote *)hidRemote				// Invoked when hardware was removed from HIDRemote's pool
	releasedHardwareWithAttributes:(NSMutableDictionary *)attributes;

// WARNING: Unless you know VERY PRECISELY what you are doing, do not implement any of the methods below.
// Matching of newly found receiver hardware
- (BOOL)hidRemote:(HIDRemote *)hidRemote				// Invoked when new hardware is inspected
	inspectNewHardwareWithService:(io_service_t)service		// 
	prematchResult:(BOOL)prematchResult;				// Return YES if HIDRemote should go on with this hardware and try
									// to use it, or NO if it should not be persued further.

// Exlusive lock lending
- (BOOL)hidRemote:(HIDRemote *)hidRemote
	lendExclusiveLockToApplicationWithInfo:(NSDictionary *)applicationInfo;

- (void)hidRemote:(HIDRemote *)hidRemote
	exclusiveLockReleasedByApplicationWithInfo:(NSDictionary *)applicationInfo;

- (BOOL)hidRemote:(HIDRemote *)hidRemote
	shouldRetryExclusiveLockWithInfo:(NSDictionary *)applicationInfo;

@end 


#pragma mark -- Actual header file for class  --

@interface HIDRemote : NSObject
{
	// IOMasterPort
	mach_port_t _masterPort;

	// Notification ports
	IONotificationPortRef _notifyPort;
	CFRunLoopSourceRef _notifyRLSource;
	
	// Matching iterator
	io_iterator_t _matchingServicesIterator;
	
	// Service attributes
	NSMutableDictionary *_serviceAttribMap;
	
	// Mode
	HIDRemoteMode _mode;
	BOOL _autoRecover;
	NSTimer *_autoRecoveryTimer;
	
	// Delegate
	NSObject <HIDRemoteDelegate> *_delegate;
	
	// Last seen ID
	SInt32 _lastSeenRemoteID;
	
	// Unused button codes
	NSArray *_unusedButtonCodes;
	
	// Simulate Plus/Minus Hold
	BOOL _simulateHoldEvents;
	
	// SecureEventInput workaround
	BOOL _secureEventInputWorkAround;
	BOOL _statusSecureEventInputWorkAroundEnabled;
	
	// Exclusive lock lending
	BOOL _exclusiveLockLending;
	NSNumber *_waitForReturnByPID;
	NSNumber *_returnToPID;
}

#pragma mark -- PUBLIC: Shared HID Remote --
/*
	DESCRIPTION
	It is possible to alloc & init multiple instances of this class. However, this is not recommended unless
	you subclass it and build something different. Instead of allocating & initializing the instance yourself,
	you can make use of the +sharedHIDRemote singleton.

	RESULT
	The HIDRemote instance globally shared in your application. You should not -release the returned object.
*/
+ (HIDRemote *)sharedHIDRemote;

#pragma mark -- PUBLIC: System Information --
/*
	DESCRIPTION
	Determine whether the Candelair driver version 1.7.0 or later is installed.
	
	RESULT
	YES, if it is installed. NO, if it isn't.
*/
+ (BOOL)isCandelairInstalled;

/*
	DESCRIPTION
	Determine whether the user needs to install the Candelair driver in order for your application to get
	access to the IR Receiver in a specific mode.
	
	RESULT
	YES, if the user runs your application on an operating system version that makes the installation of
	the Candelair driver necessary for your application to get access to the IR Receiver in the specified
	mode.
	NO, if the operating system version in use either doesn't make the installation of the Candelair driver
	a necessity - or - if it is already installed.
	
	SAMPLE CODE
	Please see DemoController.m from the HIDRemoteSample project for a reusable example on how to make best
	use of this method in your code.
*/
+ (BOOL)isCandelairInstallationRequiredForRemoteMode:(HIDRemoteMode)remoteMode;

#pragma mark -- PUBLIC: Interface / API --
/*
	DESCRIPTION
	Starts the HIDRemote in the respective mode kHIDRemoteModeShared, kHIDRemoteModeExclusive or
	kHIDRemoteModeExclusiveAuto.
	
	RESULT
	YES, if setup was successful. NO, if an error occured during setup. Note that a successful setup
	does not mean that you gained the respective level of access or that remote control hardware was
	actually found. This is only the case if -activeRemoteControlCount returns a value
	greater zero. I.e. your setup code could look like this:
	
	if ((hidRemoteControl = [HIDRemoteControl sharedHIDRemote]) != nil)
	{
		[hidRemoteControl setDelegate:myDelegate];
		
		if ([HIDRemote isCandelairInstallationRequiredForRemoteMode:kHIDRemoteModeExclusiveAuto])
		{
			NSLog(@"Installation of Candelair required."); // See DemoController.m for a reusable code snippet presenting an
								       // alert and offering to open related URLs
		}
		else
		{
			if ([hidRemoteControl startRemoteControl:kHIDRemoteModeExclusiveAuto])
			{
				NSLog(@"Driver has started successfully.");

				if ([hidRemoteControl activeRemoteControlCount])
				{
					NSLog(@"Driver has found %d remotes.", [hidRemoteControl activeRemoteControlCount]);
				}
				else
				{
					NSLog(@"Driver has not found any remotes it could use. Will use remotes as they become available.");
				}
			}
			else
			{
				// .. Setup failed ..
			}
		}
	}
*/
- (BOOL)startRemoteControl:(HIDRemoteMode)hidRemoteMode;	

/*
	DESCRIPTION
	Stops the HIDRemote. You will no longer get remote control events after this. Other applications can
	then access the remote again. To get a lock on the HIDRemote again, make use of -startRemoteControl:.
*/
- (void)stopRemoteControl;

/*
	DESCRIPTION
	Determine, whether the HIDRemote has been started with -startRemoteControl:.
		
	RESULT
	YES, if it was started. NO, if it was not.
*/
- (BOOL)isStarted;

/*
	DESCRIPTION
	Determine the number of remote controls HIDRemote has currently opened. This is usually 1 on current systems.
*/
- (unsigned)activeRemoteControlCount;

/*
	DESCRIPTION
	Returns the ID of the remote from which the button press was received from last. You can sign up your delegate for
	ID change notifications by implementing the (optional) -hidRemote:remoteIDChangedOldID:newID: selector.
	
	RESULT
	Returns the ID of the last seen remote. Returns -1, if the ID is unknown.
*/
- (SInt32)lastSeenRemoteControlID;

/*
	DESCRIPTION
	Set a new delegate object. This object has to implement the HIDRemoteDelegate protocol. If it is also implementing
	the optional HIDRemoteDelegate protocol methods, it will be able to receive additional notifications and events.
	
	IMPORTANT
	The delegate is not retained. Make sure you execute a -[hidRemoteInstance setDelegate:nil] in the dealloc method of
	your delegate.
*/
- (void)setDelegate:(NSObject <HIDRemoteDelegate> *)newDelegate;

/*
	DESCRIPTION
	Get the currently set delegate object.
	
	RESULT
	The currently set delegate object.
*/
- (NSObject <HIDRemoteDelegate> *)delegate;

/*
	DESCRIPTION
	Set whether hold events should be simulated for the + and - buttons. The simulation is active by default. This value
	should only be changed when no button is currently pressed (f.ex. before calling -startRemoteControl:). The behaviour
	is undefined if a button press is currently in progress.
*/
- (void)setSimulateHoldEvents:(BOOL)newSimulateHoldEvents;

/*
	DESCRIPTION
	Determine whether the simulation of hold events for the + and - buttons is currently active.
	
	RESULT
	YES or NO depending on whether the simulation is currently active.
*/
- (BOOL)simulateHoldEvents;

/*
	DESCRIPTION
	Set an array of NSNumbers with HIDRemoteButtonCodes that are not used by your application. This is empty by default.
	By providing this information, you improve interoperation with popular remote control solutions such as Remote Buddy.
	If, for example, you don't use the MenuHold button code, you'd express it like this in your sourcecode:
	
	if (hidRemote = [HIDRemote sharedHIDRemote])
	{
		// ..

		[hidRemote setUnusedButtonCodes:[NSArray arrayWithObjects:[NSNumber numberWithInt:(int)kHIDRemoteButtonCodeMenuHold], nil]];
		
		// ..
	}
	
	Advanced remote control solutions such as Remote Buddy do then know that you're not using the MenuHold button code and
	can automatically create a mapping table for your application, with all buttons presses except MenuHold being forwarded
	to your application. For MenuHold, Remote Buddy might map an action to open its own menu.
*/
- (void)setUnusedButtonCodes:(NSArray *)newArrayWithUnusedButtonCodesAsNSNumbers;

/*
	DESCRIPTION
	Return an array of NSNumbers with HIDRemoteButtonCodes your application does not use. For more information, see the
	description for -setUnusedButtonCodes:
	
	RESULT
	An array of NSNumbers with HIDRemoteButtonCodes your application does not use.
*/
- (NSArray *)unusedButtonCodes;

#pragma mark -- PUBLIC: Expert APIs --

/*
	DESCRIPTION
	Enables/disables a workaround to a locking issue introduced with Security Update 2008-004 / 10.4.9 and beyond. Essentially,
	without this workaround enabled, using an application that uses a password textfield would degrade exclusive locks to shared
	locks with the result being that both normal OS X as well as your application would react to the same HID event when really
	only your application should. Credit for finding this workaround goes to Martin Kahr.

	Enabled by default.
*/
- (void)setEnableSecureEventInputWorkaround:(BOOL)newEnableSecureEventInputWorkaround;

/*
	DESCRIPTION
	Determine whether aforementioned workaround is active.

	RESULT
	YES or NO.
*/
- (BOOL)enableSecureEventInputWorkaround;

/*
	DESCRIPTION
	Enables/disables lending of the exclusive lock to other applications when in kHIDRemoteModeExclusive mode. 
	
	Enable this option only when you are writing a background application that keeps a permanent, exclusive lock on the IR receiver.
	
	When this option is enabled and another application using the HIDRemote class indicates that it'd like to get exclusive access
	to the IR receiver itself while your application is having it, your application's instance of HIDRemote automatically stops and
	signals the other application that it can now get exclusive access. When the other application's HIDRemote instance no longer uses
	the IR receiver exclusively, it lets your application know so that it can recover its exclusive lock.
	
	This option is disabled by default. Unless you have special needs, you really should use the kHIDRemoteModeExclusiveAuto mode for
	best compatibility with other applications.
*/
- (void)setExclusiveLockLendingEnabled:(BOOL)newExclusiveLockLendingEnabled;
- (BOOL)exclusiveLockLendingEnabled;

#pragma mark -- PRIVATE: HID Event handling --
- (void)_handleButtonCode:(HIDRemoteButtonCode)buttonCode isPressed:(BOOL)isPressed hidAttribsDict:(NSMutableDictionary *)hidAttribsDict;
- (void)_sendButtonCode:(HIDRemoteButtonCode)buttonCode isPressed:(BOOL)isPressed;
- (void)_hidEventFor:(io_service_t)hidDevice from:(IOHIDQueueInterface **)interface withResult:(IOReturn)result;

#pragma mark -- PRIVATE: Service setup and destruction --
- (BOOL)_prematchService:(io_object_t)service;
- (BOOL)_setupService:(io_object_t)service;
- (void)_destructService:(io_object_t)service;

#pragma mark -- PRIVATE: Distributed notifiations handling --
- (void)_postStatusWithAction:(NSString *)action;
- (void)_handleNotifications:(NSNotification *)notification;

#pragma mark -- PRIVATE: Application becomes active / inactive handling for kHIDRemoteModeExclusiveAuto --
- (void)_appStatusChanged:(NSNotification *)notification;
- (void)_delayedAutoRecovery:(NSTimer *)aTimer;

#pragma mark -- PRIVATE: Notification handling --
- (void)_serviceMatching:(io_iterator_t)iterator;
- (void)_serviceNotificationFor:(io_service_t)service messageType:(natural_t)messageType messageArgument:(void *)messageArgument;

@end

#pragma mark -- Information attribute keys --
extern NSString *kHIDRemoteManufacturer;
extern NSString *kHIDRemoteProduct;
extern NSString *kHIDRemoteTransport;

#pragma mark -- Internal/Expert attribute keys (AKA: don't touch these unless you really, really, REALLY know what you do) --
extern NSString *kHIDRemoteCFPluginInterface;
extern NSString *kHIDRemoteHIDDeviceInterface;
extern NSString *kHIDRemoteCookieButtonCodeLUT;
extern NSString *kHIDRemoteHIDQueueInterface;
extern NSString *kHIDRemoteServiceNotification;
extern NSString *kHIDRemoteCFRunLoopSource;
extern NSString *kHIDRemoteLastButtonPressed;
extern NSString *kHIDRemoteService;
extern NSString *kHIDRemoteSimulateHoldEventsTimer;
extern NSString *kHIDRemoteSimulateHoldEventsOriginButtonCode;

#pragma mark -- Distributed notifications --
extern NSString *kHIDRemoteDNHIDRemotePing;
extern NSString *kHIDRemoteDNHIDRemoteRetry;
extern NSString *kHIDRemoteDNHIDRemoteStatus;

#pragma mark -- Distributed notifications userInfo keys and values --
extern NSString *kHIDRemoteDNStatusHIDRemoteVersionKey;
extern NSString *kHIDRemoteDNStatusPIDKey;
extern NSString *kHIDRemoteDNStatusModeKey;
extern NSString *kHIDRemoteDNStatusUnusedButtonCodesKey;
extern NSString *kHIDRemoteDNStatusRemoteControlCountKey;
extern NSString *kHIDRemoteDNStatusReturnToPIDKey;
extern NSString *kHIDRemoteDNStatusActionKey;
extern NSString *kHIDRemoteDNStatusActionStart;
extern NSString *kHIDRemoteDNStatusActionStop;
extern NSString *kHIDRemoteDNStatusActionUpdate;

#pragma mark -- Driver compatibility flags --
typedef enum
{
	kHIDRemoteCompatibilityFlagsStandardHIDRemoteDevice = 1L,
} HIDRemoteCompatibilityFlags;
