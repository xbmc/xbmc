#pragma once

/*
 *  Copyright (C) 2021- Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import <Cocoa/Cocoa.h>

@interface OSXGLWindow : NSWindow <NSWindowDelegate>

@property(atomic) bool resizeState;

- (id)initWithContentRect:(NSRect)box styleMask:(uint)style;
- (void)dealloc;

- (BOOL)windowShouldClose:(id)sender;
- (void)windowWillEnterFullScreen:(NSNotification*)pNotification;
- (void)windowDidExitFullScreen:(NSNotification*)pNotification;

- (void)windowDidChangeScreen:(NSNotification*)notification;
- (void)windowDidExpose:(NSNotification*)aNotification;
- (void)windowDidMove:(NSNotification*)aNotification;
- (void)windowDidMiniaturize:(NSNotification*)aNotification;
- (void)windowDidDeminiaturize:(NSNotification*)aNotification;

- (void)windowDidBecomeKey:(NSNotification*)aNotification;
- (void)windowDidResignKey:(NSNotification*)aNotification;

- (void)windowDidResize:(NSNotification*)aNotification;
- (NSSize)windowWillResize:(NSWindow*)sender toSize:(NSSize)frameSize;
- (void)windowWillStartLiveResize:(NSNotification*)notification;
- (void)windowDidEndLiveResize:(NSNotification*)notification;

@end
