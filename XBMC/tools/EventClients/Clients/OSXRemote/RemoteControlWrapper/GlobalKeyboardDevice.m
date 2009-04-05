/*****************************************************************************
 * GlobalKeyboardDevice.m
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


#import "GlobalKeyboardDevice.h"

#define F1 122
#define F2 120
#define F3 99
#define F4 118
#define F5 96
#define F6 97
#define F7 98

/*
 the following default keys are read and shall be used to change the keyboard mapping
 
 mac.remotecontrols.GlobalKeyboardDevice.plus_modifiers
 mac.remotecontrols.GlobalKeyboardDevice.plus_keycode
 mac.remotecontrols.GlobalKeyboardDevice.minus_modifiers
 mac.remotecontrols.GlobalKeyboardDevice.minus_keycode
 mac.remotecontrols.GlobalKeyboardDevice.play_modifiers
 mac.remotecontrols.GlobalKeyboardDevice.play_keycode
 mac.remotecontrols.GlobalKeyboardDevice.left_modifiers
 mac.remotecontrols.GlobalKeyboardDevice.left_keycode
 mac.remotecontrols.GlobalKeyboardDevice.right_modifiers
 mac.remotecontrols.GlobalKeyboardDevice.right_keycode
 mac.remotecontrols.GlobalKeyboardDevice.menu_modifiers
 mac.remotecontrols.GlobalKeyboardDevice.menu_keycode
 mac.remotecontrols.GlobalKeyboardDevice.playhold_modifiers
 mac.remotecontrols.GlobalKeyboardDevice.playhold_keycode
 */


@implementation GlobalKeyboardDevice

- (id) initWithDelegate: (id) _remoteControlDelegate {	
	if (self = [super initWithDelegate: _remoteControlDelegate]) {
		hotKeyRemoteEventMapping = [[NSMutableDictionary alloc] init];
		
		unsigned int modifiers = cmdKey + shiftKey /*+ optionKey*/ + controlKey;
				
		[self mapRemoteButton:kRemoteButtonPlus			defaultKeycode:F1 defaultModifiers:modifiers];
		[self mapRemoteButton:kRemoteButtonMinus		defaultKeycode:F2 defaultModifiers:modifiers];
		[self mapRemoteButton:kRemoteButtonPlay			defaultKeycode:F3 defaultModifiers:modifiers];
		[self mapRemoteButton:kRemoteButtonLeft			defaultKeycode:F4 defaultModifiers:modifiers];
		[self mapRemoteButton:kRemoteButtonRight		defaultKeycode:F5 defaultModifiers:modifiers];
		[self mapRemoteButton:kRemoteButtonMenu			defaultKeycode:F6 defaultModifiers:modifiers];
		[self mapRemoteButton:kRemoteButtonPlay_Hold	defaultKeycode:F7 defaultModifiers:modifiers];		
	}
	return self;
}

- (void) dealloc {
	[hotKeyRemoteEventMapping release];
	[super dealloc];
}

- (void) mapRemoteButton: (RemoteControlEventIdentifier) remoteButtonIdentifier defaultKeycode: (unsigned int) defaultKeycode defaultModifiers: (unsigned int) defaultModifiers {
	NSString* defaultsKey;	
	
	switch(remoteButtonIdentifier) {
		case kRemoteButtonPlus:
			defaultsKey = @"plus";
			break;
		case kRemoteButtonMinus:
			defaultsKey = @"minus";
			break;			
		case kRemoteButtonMenu:
			defaultsKey = @"menu";
			break;			
		case kRemoteButtonPlay:
			defaultsKey = @"play";
			break;
		case kRemoteButtonRight:
			defaultsKey = @"right";
			break;			
		case kRemoteButtonLeft:			
			defaultsKey = @"left";
			break;
		case kRemoteButtonPlay_Hold:
			defaultsKey = @"playhold";
			break;			
		default: 
			NSLog(@"Unknown global keyboard defaults key for remote button identifier %d", remoteButtonIdentifier);
	}
	
	NSNumber* modifiersCfg = [[NSUserDefaults standardUserDefaults] objectForKey: [NSString stringWithFormat: @"mac.remotecontrols.GlobalKeyboardDevice.%@_modifiers", defaultsKey]];
	NSNumber* keycodeCfg   = [[NSUserDefaults standardUserDefaults] objectForKey: [NSString stringWithFormat: @"mac.remotecontrols.GlobalKeyboardDevice.%@_keycode", defaultsKey]];
	
	unsigned int modifiers = defaultModifiers;
	if (modifiersCfg) modifiers = [modifiersCfg unsignedIntValue];
	
	unsigned int keycode = defaultKeycode;
	if (keycodeCfg) keycode = [keycodeCfg unsignedIntValue];
				  
    [self registerHotKeyCode: keycode  modifiers: modifiers remoteEventIdentifier: remoteButtonIdentifier];
}	

- (void) setListeningToRemote: (BOOL) value {
	if (value == [self isListeningToRemote]) return;
	if (value) {
		[self startListening: self];
	} else {
		[self stopListening: self];
	}
}
- (BOOL) isListeningToRemote {	
	return (eventHandlerRef!=NULL);
}

- (IBAction) startListening: (id) sender {
	
	if (eventHandlerRef) return;
		
	EventTypeSpec eventSpec[2] = {
		{ kEventClassKeyboard, kEventHotKeyPressed },
		{ kEventClassKeyboard, kEventHotKeyReleased }
	};    
	
	InstallEventHandler( GetEventDispatcherTarget(),
						 (EventHandlerProcPtr)hotKeyEventHandler, 
						 2, eventSpec, self, &eventHandlerRef);	
}
- (IBAction) stopListening: (id) sender {
	RemoveEventHandler(eventHandlerRef);
	eventHandlerRef = NULL;
}

- (BOOL) sendsEventForButtonIdentifier: (RemoteControlEventIdentifier) identifier {
	NSEnumerator* values = [hotKeyRemoteEventMapping objectEnumerator];
	NSNumber* remoteIdentifier;
	while( (remoteIdentifier = [values nextObject]) ) {
		if ([remoteIdentifier unsignedIntValue] == identifier) return YES;
	}
	return NO;
}

+ (const char*) remoteControlDeviceName {
	return "Keyboard";
}

- (BOOL)registerHotKeyCode: (unsigned int) keycode modifiers: (unsigned int) modifiers remoteEventIdentifier: (RemoteControlEventIdentifier) identifier {	
	OSStatus err;
	EventHotKeyID hotKeyID;
	EventHotKeyRef carbonHotKey;

	hotKeyID.signature = 'PTHk';
	hotKeyID.id = (long)keycode;
	
	err = RegisterEventHotKey(keycode, modifiers, hotKeyID, GetEventDispatcherTarget(), (int)nil, &carbonHotKey );
	
	if( err )
		return NO;
	
	[hotKeyRemoteEventMapping setObject: [NSNumber numberWithInt:identifier] forKey: [NSNumber numberWithUnsignedInt: hotKeyID.id]];
	
	return YES;
}
/*
- (void)unregisterHotKey: (PTHotKey*)hotKey
{
	OSStatus err;
	EventHotKeyRef carbonHotKey;
	NSValue* key;
	
	if( [[self allHotKeys] containsObject: hotKey] == NO )
		return;
	
	carbonHotKey = [self _carbonHotKeyForHotKey: hotKey];
	NSAssert( carbonHotKey != nil, @"" );
	
	err = UnregisterEventHotKey( carbonHotKey );
	//Watch as we ignore 'err':
	
	key = [NSValue valueWithPointer: carbonHotKey];
	[mHotKeys removeObjectForKey: key];
	
	[self _updateEventHandler];
	
	//See that? Completely ignored
}
*/

- (RemoteControlEventIdentifier) remoteControlEventIdentifierForID: (unsigned int) id {
	NSNumber* remoteEventIdentifier = [hotKeyRemoteEventMapping objectForKey:[NSNumber numberWithUnsignedInt: id]];
	return [remoteEventIdentifier unsignedIntValue];
}

- (void) sendRemoteButtonEvent: (RemoteControlEventIdentifier) event pressedDown: (BOOL) pressedDown {
	[delegate sendRemoteButtonEvent: event pressedDown: pressedDown remoteControl:self];
}

static RemoteControlEventIdentifier lastEvent;

static OSStatus hotKeyEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void* userData )
{
	GlobalKeyboardDevice* keyboardDevice = (GlobalKeyboardDevice*) userData;
	EventHotKeyID hkCom;
	GetEventParameter(inEvent,kEventParamDirectObject,typeEventHotKeyID,NULL,sizeof(hkCom),NULL,&hkCom);

	RemoteControlEventIdentifier identifier = [keyboardDevice remoteControlEventIdentifierForID:hkCom.id];
	if (identifier == 0) return noErr;
	
	BOOL pressedDown = YES;
	if (identifier != lastEvent) {
		lastEvent = identifier;
	} else {
		lastEvent = 0;
	    pressedDown = NO;
	}
	[keyboardDevice sendRemoteButtonEvent: identifier pressedDown: pressedDown];
	
	return noErr;
}

@end
