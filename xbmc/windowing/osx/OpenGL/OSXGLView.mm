/*
 *  Copyright (C) 2021- Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "OSXGLView.h"

#include "ServiceBroker.h"
#include "utils/log.h"
#import "windowing/osx/WinSystemOSX.h"

#include <vector>

#include "system_gl.h"

@implementation OSXGLView
{
  NSOpenGLContext* m_glcontext;
  NSTrackingArea* m_trackingArea;
}

- (void)SendInputEvent:(NSEvent*)nsEvent
{
  CWinSystemOSX* winSystem = dynamic_cast<CWinSystemOSX*>(CServiceBroker::GetWinSystem());
  if (winSystem)
  {
    winSystem->SendInputEvent(nsEvent);
  }
}

- (id)initWithFrame:(NSRect)frameRect
{
  auto createGLContext = [](NSOpenGLPixelFormatAttribute profile, bool accelerated)
  {
    // clang-format off
    std::vector<NSOpenGLPixelFormatAttribute> wattrs = {
      NSOpenGLPFAOpenGLProfile, profile,
    };
    if (accelerated)
      wattrs.push_back(NSOpenGLPFAAccelerated);
    wattrs.insert(wattrs.end(), {
      NSOpenGLPFAAlphaSize, 8,
      NSOpenGLPFAColorSize, 32,
      NSOpenGLPFADepthSize, 24,
      NSOpenGLPFADoubleBuffer,
      NSOpenGLPFANoRecovery,
      0
    });
    // clang-format on

    auto pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:wattrs.data()];
    return [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
  };

  self = [super initWithFrame:frameRect];
  if (self)
  {
    m_glcontext = createGLContext(NSOpenGLProfileVersion3_2Core, true);
    if (!m_glcontext)
    {
      CLog::Log(LOGERROR,
                "failed to create NSOpenGLContext, falling back to legacy OpenGL profile");
      m_glcontext = createGLContext(NSOpenGLProfileVersionLegacy, true);
    }
    if (!m_glcontext)
    {
      CLog::Log(LOGWARNING, "failed to create accelerated NSOpenGLContext, falling back to a "
                            "software-rendered context");
      m_glcontext = createGLContext(NSOpenGLProfileVersionLegacy, false);
    }
    if (!m_glcontext)
    {
      CLog::Log(LOGERROR, "failed to create any NSOpenGLContext");
      return nil;
    }
  }
  self.wantsBestResolutionOpenGLSurface = YES;
  [self updateTrackingAreas];

  GLint swapInterval = 1;
  [m_glcontext setValues:&swapInterval forParameter:NSOpenGLContextParameterSwapInterval];
  [m_glcontext makeCurrentContext];

  return self;
}

- (void)dealloc
{
  [NSOpenGLContext clearCurrentContext];
  [m_glcontext clearDrawable];
}

- (BOOL)acceptsFirstResponder
{
  return YES;
}

- (void)drawRect:(NSRect)rect
{
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    [self setOpenGLContext:m_glcontext];

    // clear screen on first render
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0, 0, 0, 0);

    [m_glcontext update];
  });
}

- (void)updateTrackingAreas
{
  if (m_trackingArea != nil)
  {
    [self removeTrackingArea:m_trackingArea];
  }

  const int opts =
      (NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingActiveAlways);
  m_trackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds
                                                options:opts
                                                  owner:self
                                               userInfo:nil];
  [self addTrackingArea:m_trackingArea];
}

- (void)mouseEntered:(NSEvent*)nsEvent
{
  CWinSystemOSX* winSystem = dynamic_cast<CWinSystemOSX*>(CServiceBroker::GetWinSystem());
  if (winSystem)
    winSystem->signalMouseEntered();
}

#pragma mark - Input Events

- (void)mouseMoved:(NSEvent*)nsEvent
{
  [self SendInputEvent:nsEvent];
}

- (void)mouseDown:(NSEvent*)nsEvent
{
  [self SendInputEvent:nsEvent];
}

- (void)mouseDragged:(NSEvent*)nsEvent
{
  [self SendInputEvent:nsEvent];
}

- (void)mouseUp:(NSEvent*)nsEvent
{
  [self SendInputEvent:nsEvent];
}

- (void)rightMouseDown:(NSEvent*)nsEvent
{
  [self SendInputEvent:nsEvent];
}

- (void)rightMouseDragged:(NSEvent*)nsEvent
{
  [self SendInputEvent:nsEvent];
}

- (void)rightMouseUp:(NSEvent*)nsEvent
{
  [self SendInputEvent:nsEvent];
}

- (void)otherMouseUp:(NSEvent*)nsEvent
{
  [self SendInputEvent:nsEvent];
}

- (void)otherMouseDown:(NSEvent*)nsEvent
{
  [self SendInputEvent:nsEvent];
}

- (void)scrollWheel:(NSEvent*)nsEvent
{
  [self SendInputEvent:nsEvent];
}

- (void)otherMouseDragged:(NSEvent*)nsEvent
{
  [self SendInputEvent:nsEvent];
}

- (void)keyDown:(NSEvent*)nsEvent
{
  [self SendInputEvent:nsEvent];
}

- (void)keyUp:(NSEvent*)nsEvent
{
  [self SendInputEvent:nsEvent];
}

- (void)mouseExited:(NSEvent*)nsEvent
{
  CWinSystemOSX* winSystem = dynamic_cast<CWinSystemOSX*>(CServiceBroker::GetWinSystem());
  if (winSystem)
    winSystem->signalMouseExited();
}

- (CGLContextObj)getGLContextObj
{
  assert(m_glcontext);
  return [m_glcontext CGLContextObj];
}

- (void)Update
{
  assert(m_glcontext);
  [m_glcontext makeCurrentContext];
  [m_glcontext update];
}

- (void)FlushBuffer
{
  assert(m_glcontext);
  [m_glcontext makeCurrentContext];
  [m_glcontext flushBuffer];
}
@end
