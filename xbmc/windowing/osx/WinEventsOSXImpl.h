/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "windowing/osx/WinEventsOSX.h"

#import <Foundation/Foundation.h>

@class NSEvent;

@interface CWinEventsOSXImpl : NSObject

- (void)MessagePush:(XBMC_Event*)newEvent;
- (size_t)GetQueueSize;
- (bool)MessagePump;
- (void)enableInputEvents;
- (void)disableInputEvents;
- (XBMC_Event)keyPressEvent:(NSEvent*)nsEvent;

- (void)signalMouseEntered;
- (void)signalMouseExited;
- (void)ProcessInputEvent:(NSEvent*)nsEvent;

@end
