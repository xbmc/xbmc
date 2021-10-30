/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "platform/darwin/tvos/TVOSEAGLView.h"

#include "messaging/ApplicationMessenger.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"

#import "platform/darwin/DarwinUtils.h"
#import "platform/darwin/tvos/XBMCController.h"

#import <signal.h>
#import <stdio.h>

using namespace KODI::MESSAGING;

@implementation TVOSEAGLView
@synthesize m_context;
@synthesize m_currentScreen;

+ (Class)layerClass
{
  return [CAEAGLLayer class];
}

- (id)initWithFrame:(CGRect)frame withScreen:(UIScreen*)screen
{
  m_framebufferResizeRequested = FALSE;
  if ((self = [super initWithFrame:frame]))
  {
    auto scaleFactor = [self getScreenScale:UIScreen.mainScreen];

    // Get the layer
    auto eaglLayer = static_cast<CAEAGLLayer*>(self.layer);
    eaglLayer.contentsScale = scaleFactor;
    self.contentScaleFactor = scaleFactor;

    eaglLayer.opaque = TRUE;
    eaglLayer.drawableProperties = @{
      kEAGLDrawablePropertyRetainedBacking : @NO,
      kEAGLDrawablePropertyColorFormat : kEAGLColorFormatRGBA8
    };

    // Try OpenGL ES 3.0
    auto aContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];

    // Fallback to OpenGL ES 2.0
    if (aContext == nullptr)
      aContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];

    if (!aContext)
      CLog::Log(LOGERROR, "Failed to create ES context");
    else if (![EAGLContext setCurrentContext:aContext])
      CLog::Log(LOGERROR, "Failed to set ES context current");

    m_context = aContext;

    [self createFramebuffer];
    [self setFramebuffer];
  }

  return self;
}

- (void)dealloc
{
  [self deleteFramebuffer];
}

- (BOOL)canBecomeFocused
{
  // need this or we do not get GestureRecognizers under tvos.
  return YES;
}

- (void)setContext:(EAGLContext*)newContext
{
  if (m_context != newContext)
  {
    [self deleteFramebuffer];
    m_context = newContext;
    [EAGLContext setCurrentContext:nil];
  }
}

- (void)createFramebuffer
{
  if (m_context && !m_defaultFramebuffer)
  {
    [EAGLContext setCurrentContext:m_context];

    // Create default framebuffer object.
    gl::GenFramebuffers(1, &m_defaultFramebuffer);
    gl::BindFramebuffer(GL_FRAMEBUFFER, m_defaultFramebuffer);

    // Create color render buffer and allocate backing store.
    gl::GenRenderbuffers(1, &m_colorRenderbuffer);
    gl::BindRenderbuffer(GL_RENDERBUFFER, m_colorRenderbuffer);
    [m_context renderbufferStorage:GL_RENDERBUFFER
                      fromDrawable:static_cast<CAEAGLLayer*>(self.layer)];
    gl::GetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &m_framebufferWidth);
    gl::GetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &m_framebufferHeight);
    gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
                                m_colorRenderbuffer);

    gl::GenRenderbuffers(1, &m_depthRenderbuffer);
    gl::BindRenderbuffer(GL_RENDERBUFFER, m_depthRenderbuffer);
    gl::RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, m_framebufferWidth,
                            m_framebufferHeight);
    gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                m_depthRenderbuffer);

    if (gl::CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      CLog::Log(LOGERROR, "Failed to make complete framebuffer object {}",
                gl::CheckFramebufferStatus(GL_FRAMEBUFFER));
  }
}

- (void)deleteFramebuffer
{
  if (m_context)
  {
    [EAGLContext setCurrentContext:m_context];

    if (m_defaultFramebuffer)
    {
      gl::DeleteFramebuffers(1, &m_defaultFramebuffer);
      m_defaultFramebuffer = 0;
    }

    if (m_colorRenderbuffer)
    {
      gl::DeleteRenderbuffers(1, &m_colorRenderbuffer);
      m_colorRenderbuffer = 0;
    }

    if (m_depthRenderbuffer)
    {
      gl::DeleteRenderbuffers(1, &m_depthRenderbuffer);
      m_depthRenderbuffer = 0;
    }
  }
}

- (void)setFramebuffer
{
  if (m_context)
  {
    if ([EAGLContext currentContext] != m_context)
      [EAGLContext setCurrentContext:m_context];

    gl::BindFramebuffer(GL_FRAMEBUFFER, m_defaultFramebuffer);

    if (m_framebufferHeight > m_framebufferWidth)
    {
      gl::Viewport(0, 0, m_framebufferHeight, m_framebufferWidth);
      gl::Scissor(0, 0, m_framebufferHeight, m_framebufferWidth);
    }
    else
    {
      gl::Viewport(0, 0, m_framebufferWidth, m_framebufferHeight);
      gl::Scissor(0, 0, m_framebufferWidth, m_framebufferHeight);
    }
  }
}

- (bool)presentFramebuffer
{
  bool success = FALSE;
  if (m_context)
  {
    if ([EAGLContext currentContext] != m_context)
      [EAGLContext setCurrentContext:m_context];

    gl::BindRenderbuffer(GL_RENDERBUFFER, m_colorRenderbuffer);
    success = [m_context presentRenderbuffer:GL_RENDERBUFFER];
  }

  return success;
}

- (CGFloat)getScreenScale:(UIScreen*)screen
{
  return screen.scale;
}

@end
