//
//  IRKeyboardEmu.m
//  XBMCHelper
//
//  Created by Stephan Diederich on 14.04.09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "IRKeyboardEmu.h"


@implementation IRKeyboardEmu

+ (const char*) remoteControlDeviceName {
	return "IRKeyboardEmu";
}

- (void) sendRemoteButtonEvent: (RemoteControlEventIdentifier) event pressedDown: (BOOL) pressedDown {
	if (pressedDown == NO && event == kRemoteButtonMenu_Hold) {
		// There is no seperate event for pressed down on menu hold. We are simulating that event here
		[super sendRemoteButtonEvent:event pressedDown:YES];
	}	
	
	[super sendRemoteButtonEvent:event pressedDown:pressedDown];
}
@end
