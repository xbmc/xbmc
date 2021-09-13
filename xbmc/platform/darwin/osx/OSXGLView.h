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
{
  NSOpenGLContext* m_glcontext;
  NSOpenGLPixelFormat* m_pixFmt;
  NSTrackingArea* m_trackingArea;
  BOOL pause;
}

@property(readonly, getter=getCurrentNSContext) NSOpenGLContext* context;

- (id)initWithFrame:(NSRect)frameRect;
- (void)dealloc;
- (NSOpenGLContext*)getGLContext;

@end
