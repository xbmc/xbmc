/*****************************************************************************
 * MultiClickRemoteBehavior.m
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

#import "MultiClickRemoteBehavior.h"

const NSTimeInterval DEFAULT_MAXIMUM_CLICK_TIME_DIFFERENCE=0.35;
const NSTimeInterval HOLD_RECOGNITION_TIME_INTERVAL=0.4;

@implementation MultiClickRemoteBehavior

- (id) init {
	if (self = [super init]) {
		maxClickTimeDifference = DEFAULT_MAXIMUM_CLICK_TIME_DIFFERENCE;
	}
	return self;
}

// Delegates are not retained!
// http://developer.apple.com/documentation/Cocoa/Conceptual/CocoaFundamentals/CommunicatingWithObjects/chapter_6_section_4.html
// Delegating objects do not (and should not) retain their delegates. 
// However, clients of delegating objects (applications, usually) are responsible for ensuring that their delegates are around
// to receive delegation messages. To do this, they may have to retain the delegate.
- (void) setDelegate: (id) _delegate {
	if (_delegate && [_delegate respondsToSelector:@selector(remoteButton:pressedDown:clickCount:)]==NO) return;
	
	delegate = _delegate;
}
- (id) delegate {
	return delegate;
}

- (BOOL) simulateHoldEvent {
	return simulateHoldEvents;
}
- (void) setSimulateHoldEvent: (BOOL) value {
	simulateHoldEvents = value;
}

- (BOOL) simulatesHoldForButtonIdentifier: (RemoteControlEventIdentifier) identifier remoteControl: (RemoteControl*) remoteControl {
	// we do that check only for the normal button identifiers as we would check for hold support for hold events instead
	if (identifier > (1 << EVENT_TO_HOLD_EVENT_OFFSET)) return NO; 
	
	return [self simulateHoldEvent] && [remoteControl sendsEventForButtonIdentifier: (identifier << EVENT_TO_HOLD_EVENT_OFFSET)]==NO;
}

- (BOOL) clickCountingEnabled {
	return clickCountEnabledButtons != 0;
}
- (void) setClickCountingEnabled: (BOOL) value {
	if (value) {
		[self setClickCountEnabledButtons: kRemoteButtonPlus | kRemoteButtonMinus | kRemoteButtonPlay | kRemoteButtonLeft | kRemoteButtonRight | kRemoteButtonMenu];
	} else {
		[self setClickCountEnabledButtons: 0];
	}
}

- (unsigned int) clickCountEnabledButtons {
	return clickCountEnabledButtons;
}
- (void) setClickCountEnabledButtons: (unsigned int)value {
	clickCountEnabledButtons = value;
}

- (NSTimeInterval) maximumClickCountTimeDifference {
	return maxClickTimeDifference;
}
- (void) setMaximumClickCountTimeDifference: (NSTimeInterval) timeDiff {
	maxClickTimeDifference = timeDiff;
}

- (void) sendSimulatedHoldEvent: (id) time {
	BOOL startSimulateHold = NO;
	RemoteControlEventIdentifier event = lastHoldEvent;
	@synchronized(self) {
		startSimulateHold = (lastHoldEvent>0 && lastHoldEventTime == [time doubleValue]);
	}
	if (startSimulateHold) {
		lastEventSimulatedHold = YES;
		event = (event << EVENT_TO_HOLD_EVENT_OFFSET);
		[delegate remoteButton:event pressedDown: YES clickCount: 1];
	}
}

- (void) executeClickCountEvent: (NSArray*) values {
	RemoteControlEventIdentifier event = [[values objectAtIndex: 0] unsignedIntValue]; 
	NSTimeInterval eventTimePoint = [[values objectAtIndex: 1] doubleValue];
	
	BOOL finishedClicking = NO;
	int finalClickCount = eventClickCount;	
	
	@synchronized(self) {
		finishedClicking = (event != lastClickCountEvent || eventTimePoint == lastClickCountEventTime);
		if (finishedClicking) {
			eventClickCount = 0;		
			lastClickCountEvent = 0;
			lastClickCountEventTime = 0;
		}
	}
	
	if (finishedClicking) {	
		[delegate remoteButton:event pressedDown: YES clickCount:finalClickCount];		
		// trigger a button release event, too
		[NSThread sleepUntilDate: [NSDate dateWithTimeIntervalSinceNow:0.1]];
		[delegate remoteButton:event pressedDown: NO clickCount:finalClickCount];
	}
}

- (void) sendRemoteButtonEvent: (RemoteControlEventIdentifier) event pressedDown: (BOOL) pressedDown remoteControl: (RemoteControl*) remoteControl {	
	if (!delegate)  return;
	
	BOOL clickCountingForEvent = ([self clickCountEnabledButtons] & event) == event;

	if ([self simulatesHoldForButtonIdentifier: event remoteControl: remoteControl] && lastClickCountEvent==0) {
		if (pressedDown) {
			// wait to see if it is a hold
			lastHoldEvent = event;
			lastHoldEventTime = [NSDate timeIntervalSinceReferenceDate];
			[self performSelector:@selector(sendSimulatedHoldEvent:) 
					   withObject:[NSNumber numberWithDouble:lastHoldEventTime]
					   afterDelay:HOLD_RECOGNITION_TIME_INTERVAL];
			return;
		} else {
			if (lastEventSimulatedHold) {
				// it was a hold
				// send an event for "hold release"
				event = (event << EVENT_TO_HOLD_EVENT_OFFSET);
				lastHoldEvent = 0;
				lastEventSimulatedHold = NO;

				[delegate remoteButton:event pressedDown: pressedDown clickCount:1];
				return;
			} else {
				RemoteControlEventIdentifier previousEvent = lastHoldEvent;
				@synchronized(self) {
					lastHoldEvent = 0;
				}						
				
				// in case click counting is enabled we have to setup the state for that, too
				if (clickCountingForEvent) {
					lastClickCountEvent = previousEvent;
					lastClickCountEventTime = lastHoldEventTime;
					NSNumber* eventNumber;
					NSNumber* timeNumber;		
					eventClickCount = 1;
					timeNumber = [NSNumber numberWithDouble:lastClickCountEventTime];
					eventNumber= [NSNumber numberWithUnsignedInt:previousEvent];
					NSTimeInterval diffTime = maxClickTimeDifference-([NSDate timeIntervalSinceReferenceDate]-lastHoldEventTime);
					[self performSelector: @selector(executeClickCountEvent:) 
							   withObject: [NSArray arrayWithObjects:eventNumber, timeNumber, nil]
							   afterDelay: diffTime];							
					// we do not return here because we are still in the press-release event
					// that will be consumed below
				} else {
					// trigger the pressed down event that we consumed first
					[delegate remoteButton:event pressedDown: YES clickCount:1];							
				}
			}										
		}
	}
	
	if (clickCountingForEvent) {
		if (pressedDown == NO) return;

		NSNumber* eventNumber;
		NSNumber* timeNumber;
		@synchronized(self) {
			lastClickCountEventTime = [NSDate timeIntervalSinceReferenceDate];
			if (lastClickCountEvent == event) {
				eventClickCount = eventClickCount + 1;
			} else {
				eventClickCount = 1;
			}
			lastClickCountEvent = event;
			timeNumber = [NSNumber numberWithDouble:lastClickCountEventTime];
			eventNumber= [NSNumber numberWithUnsignedInt:event];
		}
		[self performSelector: @selector(executeClickCountEvent:) 
				   withObject: [NSArray arrayWithObjects:eventNumber, timeNumber, nil]
				   afterDelay: maxClickTimeDifference];
	} else {
		[delegate remoteButton:event pressedDown: pressedDown clickCount:1];
	}		

}

@end
