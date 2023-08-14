/*
 *  Copyright (C) 2021- Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "OSXGLView.h"

#include "ServiceBroker.h"
#import "windowing/osx/WinSystemOSX.h"

#include "system_gl.h"

@implementation OSXGLView
{
  NSOpenGLContext* m_glcontext;
  NSOpenGLPixelFormat* m_pixFmt;
  NSTrackingArea* m_trackingArea;
}

@synthesize glContextOwned;

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
  NSOpenGLPixelFormatAttribute wattrs[] = {
      NSOpenGLPFANoRecovery,    NSOpenGLPFAAccelerated,
      NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
      NSOpenGLPFAColorSize,     (NSOpenGLPixelFormatAttribute)32,
      NSOpenGLPFAAlphaSize,     (NSOpenGLPixelFormatAttribute)8,
      NSOpenGLPFADepthSize,     (NSOpenGLPixelFormatAttribute)24,
      NSOpenGLPFADoubleBuffer,  (NSOpenGLPixelFormatAttribute)0};

  self = [super initWithFrame:frameRect];
  if (self)
  {
    m_pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:wattrs];
    m_glcontext = [[NSOpenGLContext alloc] initWithFormat:m_pixFmt shareContext:nil];
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
  // whenever the view/window is resized the glContext is made current to the main (rendering) thread.
  // Since kodi does its rendering on the application main thread (not the macOS rendering thread), we
  // need to store this so that on a subsquent frame render we get the ownership of the gl context again.
  // doing this blindly without any sort of control may stall the main thread and lead to low GUI fps
  // since the glContext ownership needs to be obtained from the rendering thread (diverged from the actual
  // thread doing the rendering calls).
  [self setGlContextOwned:TRUE];

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
  [self NotifyContext];
  [m_glcontext update];
}

- (void)NotifyContext
{
  assert(m_glcontext);
  // signals/notifies the context that this view is current (required if we render out of DrawRect)
  // ownership of the context is transferred to the callee thread
  [m_glcontext makeCurrentContext];
  [self setGlContextOwned:FALSE];
}

- (void)FlushBuffer
{
  assert(m_glcontext);
  [m_glcontext makeCurrentContext];
  [m_glcontext flushBuffer];
}
@end
