/*****************************************************************************
 * MultiClickRemoteBehavior.h
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
#import "RemoteControl.h"

/**
	A behavior that adds multiclick and hold events on top of a device.
	Events are generated and send to a delegate
 */
@interface MultiClickRemoteBehavior : NSObject {
	id delegate;
	
	// state for simulating plus/minus hold
	BOOL simulateHoldEvents;	
	BOOL lastEventSimulatedHold;
	RemoteControlEventIdentifier lastHoldEvent;
	NSTimeInterval lastHoldEventTime;	
	
	// state for multi click
	unsigned int clickCountEnabledButtons;
	NSTimeInterval maxClickTimeDifference;
	NSTimeInterval lastClickCountEventTime;	
	RemoteControlEventIdentifier lastClickCountEvent;
	unsigned int eventClickCount;	
}

- (id) init;

// Delegates are not retained
- (void) setDelegate: (id) delegate;
- (id) delegate;

// Simulating hold events does deactivate sending of individual requests for pressed down/released.
// Instead special hold events are being triggered when the user is pressing and holding a button for a small period.
// Simulation is activated only for those buttons and remote control that do not have a seperate event already
- (BOOL) simulateHoldEvent;
- (void) setSimulateHoldEvent: (BOOL) value;

// click counting makes it possible to recognize if the user has pressed a button repeatedly
// click counting does delay each event as it has to wait if there is another event (second click)
// therefore there is a slight time difference (maximumClickCountTimeDifference) between a single click
// of the user and the call of your delegate method
// click counting can be enabled individually for specific buttons. Use the property clickCountEnableButtons to
// set the buttons for which click counting shall be enabled
- (BOOL) clickCountingEnabled;
- (void) setClickCountingEnabled: (BOOL) value;

- (unsigned int) clickCountEnabledButtons;
- (void) setClickCountEnabledButtons: (unsigned int)value;

// the maximum time difference till which clicks are recognized as multi clicks
- (NSTimeInterval) maximumClickCountTimeDifference;
- (void) setMaximumClickCountTimeDifference: (NSTimeInterval) timeDiff;

@end

/* 
 * Method definitions for the delegate of the MultiClickRemoteBehavior class
 */
@interface NSObject(MultiClickRemoteBehaviorDelegate)

- (void) remoteButton: (RemoteControlEventIdentifier)buttonIdentifier pressedDown: (BOOL) pressedDown clickCount: (unsigned int) count;

@end
