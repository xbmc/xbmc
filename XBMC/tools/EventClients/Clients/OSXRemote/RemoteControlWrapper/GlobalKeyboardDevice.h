/*****************************************************************************
 * GlobalKeyboardDevice.h
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
#import <Carbon/Carbon.h>

#import "RemoteControl.h"

/*
 This class registers for a number of global keyboard shortcuts to simulate a remote control
 */
@interface GlobalKeyboardDevice : RemoteControl {
	
	NSMutableDictionary* hotKeyRemoteEventMapping;
	EventHandlerRef eventHandlerRef;
	
}

- (void) mapRemoteButton: (RemoteControlEventIdentifier) remoteButtonIdentifier defaultKeycode: (unsigned int) defaultKeycode defaultModifiers: (unsigned int) defaultModifiers;

- (BOOL)registerHotKeyCode: (unsigned int) keycode modifiers: (unsigned int) modifiers remoteEventIdentifier: (RemoteControlEventIdentifier) identifier;

static OSStatus hotKeyEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void* refCon );

@end
