#pragma once

/*
 *  Copyright (C) 2021- Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import <Cocoa/Cocoa.h>

@interface OSXGLView : NSOpenGLView

- (id)initWithFrame:(NSRect)frameRect;
- (NSOpenGLContext*)getGLContext;

@end
