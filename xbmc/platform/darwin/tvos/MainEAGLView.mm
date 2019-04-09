/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *
 *  Refactored. Copyright (C) 2015 Team MrMC
 *  https://github.com/MrMC
 *
 */

#import <signal.h>
#import <stdio.h>
//hack around problem with xbmc's typedef int BOOL
// and obj-c's typedef unsigned char BOOL
#define BOOL XBMC_BOOL

#import "system.h"

#include "settings/AdvancedSettings.h"
#import "messaging/ApplicationMessenger.h"
#import "utils/log.h"
#undef BOOL

#import "platform/darwin/AutoPool.h"
#import "platform/darwin/DarwinUtils.h"
#import "platform/darwin/NSLogDebugHelpers.h"
#import "platform/darwin/tvos/MainEAGLView.h"
#import "platform/darwin/tvos/MainController.h"

using namespace KODI::MESSAGING;

//--------------------------------------------------------------
@implementation   MainEAGLView
@synthesize       m_context;
@synthesize       m_currentScreen;

// You must implement this method
+ (Class) layerClass
{
  return [CAEAGLLayer class];
}

//--------------------------------------------------------------
- (id)initWithFrame:(CGRect)frame withScreen:(UIScreen *)screen
{
  //PRINT_SIGNATURE();
  m_framebufferResizeRequested = FALSE;
  if ((self = [super initWithFrame:frame]))
  {
    // Get the layer
    CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;

    CGFloat scaleFactor = 1.0;
    UIScreen *currentScreen = [UIScreen mainScreen];;
    scaleFactor = [self getScreenScale: currentScreen];
    
    //this will activate retina on supported devices
    if (scaleFactor > 1.0)
    {
      CLog::Log(LOGINFO, "Enabling 4K for AppleTV4 4k %f", scaleFactor);
    }

    [eaglLayer setContentsScale:scaleFactor];
    [self setContentScaleFactor:scaleFactor];
    
    eaglLayer.opaque = NO;
    eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
      [NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking,
      kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];

    // Try OpenGL ES 3.0
    EAGLContext *aContext = [[EAGLContext alloc]
      initWithAPI:kEAGLRenderingAPIOpenGLES3];

    // Fallback to OpenGL ES 2.0
    if (aContext == nullptr)
      aContext = [[EAGLContext alloc]
        initWithAPI:kEAGLRenderingAPIOpenGLES2];

    if (!aContext)
      ELOG(@"Failed to create ES context");
    else if (![EAGLContext setCurrentContext:aContext])
      ELOG(@"Failed to set ES context current");
    
    m_context = aContext;
    [aContext release];

    [self createFramebuffer];
    [self setFramebuffer];
  }

  return self;
}

//--------------------------------------------------------------
- (void) dealloc
{
  [self deleteFramebuffer];    
  [m_context release];
  
  [super dealloc];
}

//--------------------------------------------------------------
- (BOOL)canBecomeFocused
{
  // need this or we do not get GestureRecognizers under tvos.
  return YES;
}

//--------------------------------------------------------------
- (void)setContext:(EAGLContext *)newContext
{
  if (m_context != newContext)
  {
    [self deleteFramebuffer];
    
    [m_context release];
    m_context = [newContext retain];
    
    [EAGLContext setCurrentContext:nil];
  }
}

//--------------------------------------------------------------
- (void)createFramebuffer
{
  if (m_context && !m_defaultFramebuffer)
  {
    //PRINT_SIGNATURE();
    [EAGLContext setCurrentContext:m_context];
    
    // Create default framebuffer object.
    glGenFramebuffers(1, &m_defaultFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFramebuffer);
    
    // Create color render buffer and allocate backing store.
    glGenRenderbuffers(1, &m_colorRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_colorRenderbuffer);
    [m_context renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer *)self.layer];
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &m_framebufferWidth);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &m_framebufferHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_colorRenderbuffer);

    glGenRenderbuffers(1, &m_depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, m_framebufferWidth, m_framebufferHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRenderbuffer);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      ELOG(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
  }
}

//--------------------------------------------------------------
- (void) deleteFramebuffer
{
  if (m_context)
  {
    [EAGLContext setCurrentContext:m_context];
    
    if (m_defaultFramebuffer)
    {
      glDeleteFramebuffers(1, &m_defaultFramebuffer);
      m_defaultFramebuffer = 0;
    }
    
    if (m_colorRenderbuffer)
    {
      glDeleteRenderbuffers(1, &m_colorRenderbuffer);
      m_colorRenderbuffer = 0;
    }

    if (m_depthRenderbuffer)
    {
      glDeleteRenderbuffers(1, &m_depthRenderbuffer);
      m_depthRenderbuffer = 0;
    }
  }
}

//--------------------------------------------------------------
- (void) setFramebuffer
{
  if (m_context)
  {
    if ([EAGLContext currentContext] != m_context)
      [EAGLContext setCurrentContext:m_context];
    
    glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFramebuffer);
    
    if (m_framebufferHeight > m_framebufferWidth)
    {
      glViewport(0, 0, m_framebufferHeight, m_framebufferWidth);
      glScissor( 0, 0, m_framebufferHeight, m_framebufferWidth);
    } 
    else
    {
      glViewport(0, 0, m_framebufferWidth, m_framebufferHeight);
      glScissor( 0, 0, m_framebufferWidth, m_framebufferHeight);
    }
  }
}

//--------------------------------------------------------------
- (bool) presentFramebuffer
{
  bool success = FALSE;
  if (m_context)
  {
    if ([EAGLContext currentContext] != m_context)
      [EAGLContext setCurrentContext:m_context];
    
    glBindRenderbuffer(GL_RENDERBUFFER, m_colorRenderbuffer);
    success = [m_context presentRenderbuffer:GL_RENDERBUFFER];
  }
  
  return success;
}

//--------------------------------------------------------------
- (CGFloat) getScreenScale:(UIScreen *)screen
{
  CGFloat ret = 1.0;
  if ([screen respondsToSelector:@selector(scale)])
  {
    // normal other iDevices report 1.0 here
    // retina devices report 2.0 here
    // this info is true as of 19.3.2012.
    if([screen scale] > 1.0)
    {
      ret = [screen scale];
    }
  }
  return ret;
}

@end
