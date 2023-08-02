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
- (CGLContextObj)getGLContextObj;

/**
 * @brief Application renders out of the NSOpenGLView drawRect (on a different thread). Hence the current
 *  NSOpenGLContext needs to be make current so that the view on the context is valid for rendering.
 *  This should be done whenever gl calls are about to be done.
 */
- (void)NotifyContext;
/**
 * @brief Update the current OpenGL context (view is set before updating)
 */
- (void)Update;
/**
 * @brief Copies the back buffer to the front buffer of the OpenGL context.
 */
- (void)FlushBuffer;

/**
 * @brief Specifies if the glContext is currently owned by the view
 */
@property(atomic, assign) BOOL glContextOwned;

@end
