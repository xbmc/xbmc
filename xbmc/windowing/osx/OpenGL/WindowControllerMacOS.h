#pragma once

/*
 *  Copyright (C) 2023- Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import <Cocoa/Cocoa.h>

@interface XBMCWindowControllerMacOS : NSWindowController <NSWindowDelegate>

- (nullable instancetype)initWithTitle:(nonnull NSString*)title
                           defaultSize:(NSSize)size NS_DESIGNATED_INITIALIZER;

- (nonnull instancetype)initWithWindow:(nullable NSWindow*)window NS_UNAVAILABLE;
- (nullable instancetype)initWithCoder:(nonnull NSCoder*)coder NS_UNAVAILABLE;
@end
