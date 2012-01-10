/*
 *      Copyright (C) 2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

//hack around problem with xbmc's typedef int BOOL
// and obj-c's typedef unsigned char BOOL
#define BOOL XBMC_BOOL 
#include <sys/resource.h>
#include <signal.h>

#include "system.h"
#include "AdvancedSettings.h"
#include "FileItem.h"
#include "Application.h"
#include "WindowingFactory.h"
#include "VideoReferenceClock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "Util.h"
#undef BOOL

#import <QuartzCore/QuartzCore.h>

#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import "XBMCEAGLView.h"
#import "XBMCController.h"
#import "AutoPool.h"

//--------------------------------------------------------------
@interface XBMCEAGLView (PrivateMethods)
- (void) setContext:(EAGLContext *)newContext;
- (void) createFramebuffer;
- (void) deleteFramebuffer;
- (void) runDisplayLink;
@end

@implementation XBMCEAGLView
@synthesize animating;
@synthesize xbmcAlive;
@synthesize pause;

// You must implement this method
+ (Class) layerClass
{
  return [CAEAGLLayer class];
}

//--------------------------------------------------------------
- (id)initWithFrame:(CGRect)frame
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);
  if ((self = [super initWithFrame:frame]))
  {
    // Get the layer
    CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
    
    eaglLayer.opaque = TRUE;
    eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
      [NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking,
      kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat,
      nil];
		
    EAGLContext *aContext = [[EAGLContext alloc] 
      initWithAPI:kEAGLRenderingAPIOpenGLES2];
    
    if (!aContext)
      NSLog(@"Failed to create ES context");
    else if (![EAGLContext setCurrentContext:aContext])
      NSLog(@"Failed to set ES context current");
    
    self.context = aContext;
    [aContext release];

    animating = FALSE;
    xbmcAlive = FALSE;
    pause = FALSE;
    [self setContext:context];
    [self createFramebuffer];
    [self setFramebuffer];

		displayLink = nil;
  }

  return self;
}

//--------------------------------------------------------------
- (void) dealloc
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);
  [self deleteFramebuffer];    
  [context release];
  
  [super dealloc];
}

//--------------------------------------------------------------
- (EAGLContext *)context
{
  return context;
}
//--------------------------------------------------------------
- (void)setContext:(EAGLContext *)newContext
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);
  if (context != newContext)
  {
    [self deleteFramebuffer];
    
    [context release];
    context = [newContext retain];
    
    [EAGLContext setCurrentContext:nil];
  }
}

//--------------------------------------------------------------
- (void)createFramebuffer
{
  if (context && !defaultFramebuffer)
  {
    //NSLog(@"%s", __PRETTY_FUNCTION__);
    [EAGLContext setCurrentContext:context];
    
    // Create default framebuffer object.
    glGenFramebuffers(1, &defaultFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);
    
    // Create color render buffer and allocate backing store.
    glGenRenderbuffers(1, &colorRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
    [context renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer *)self.layer];
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &framebufferWidth);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &framebufferHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderbuffer);

    glGenRenderbuffers(1, &depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, framebufferWidth, framebufferHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      NSLog(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
  }
}
//--------------------------------------------------------------
- (void) deleteFramebuffer
{
  if (context && !pause)
  {
    //NSLog(@"%s", __PRETTY_FUNCTION__);
    [EAGLContext setCurrentContext:context];
    
    if (defaultFramebuffer)
    {
      glDeleteFramebuffers(1, &defaultFramebuffer);
      defaultFramebuffer = 0;
    }
    
    if (colorRenderbuffer)
    {
      glDeleteRenderbuffers(1, &colorRenderbuffer);
      colorRenderbuffer = 0;
    }

    if (depthRenderbuffer)
    {
      glDeleteRenderbuffers(1, &depthRenderbuffer);
      depthRenderbuffer = 0;
    }
  }
}
//--------------------------------------------------------------
- (void) setFramebuffer
{
  if (context && !pause)
  {
    if ([EAGLContext currentContext] != context)
      [EAGLContext setCurrentContext:context];
    
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);
    
    CGRect rect = self.bounds; 
    
    if(rect.size.height > rect.size.width) {
      glViewport(0, 0, rect.size.height, rect.size.width);
      glScissor(0, 0, rect.size.height, rect.size.width);
    } 
    else
    {
      glViewport(0, 0, rect.size.width, rect.size.height);
      glScissor(0, 0, rect.size.width, rect.size.height);
    }
  }
}
//--------------------------------------------------------------
- (bool) presentFramebuffer
{
  bool success = FALSE;
  
  if (context && !pause)
  {
    if ([EAGLContext currentContext] != context)
      [EAGLContext setCurrentContext:context];
    
    glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
    success = [context presentRenderbuffer:GL_RENDERBUFFER];
  }
  
  return success;
}
//--------------------------------------------------------------
- (void) pauseAnimation
{
  pause = TRUE;
}
//--------------------------------------------------------------
- (void) resumeAnimation
{
  pause = FALSE;
}
//--------------------------------------------------------------
- (void) startAnimation
{
	if (!animating && context)
	{
		animating = TRUE;
    CWinEventsIOS::Init();

    // kick off an animation thread
    animationThreadLock = [[NSConditionLock alloc] initWithCondition: FALSE];
    animationThread = [[NSThread alloc] initWithTarget:self
      selector:@selector(runAnimation:)
      object:animationThreadLock];
    [animationThread start];
    [self initDisplayLink];
	}
}
//--------------------------------------------------------------
- (void) stopAnimation
{
	if (animating && context)
	{
    [self deinitDisplayLink];
		animating = FALSE;
    if (!g_application.m_bStop)
    {
      ThreadMessage tMsg = {TMSG_QUIT};
      g_application.getApplicationMessenger().SendMessage(tMsg);
    }
    // wait for animation thread to die
    if ([animationThread isFinished] == NO)
      [animationThreadLock lockWhenCondition:TRUE];
    CWinEventsIOS::DeInit();
	}
}
//--------------------------------------------------------------
- (void) runAnimation:(id) arg
{
  CCocoaAutoPool outerpool;

  // signal we are alive
  NSConditionLock* myLock = arg;
  [myLock lock];
  
  #ifdef _DEBUG
    g_advancedSettings.m_logLevel     = LOG_LEVEL_DEBUG;
    g_advancedSettings.m_logLevelHint = LOG_LEVEL_DEBUG;
  #else
    g_advancedSettings.m_logLevel     = LOG_LEVEL_NORMAL;
    g_advancedSettings.m_logLevelHint = LOG_LEVEL_NORMAL;
  #endif

  // Prevent child processes from becoming zombies on exit if not waited upon. See also Util::Command
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_flags = SA_NOCLDWAIT;
  sa.sa_handler = SIG_IGN;
  sigaction(SIGCHLD, &sa, NULL);

  setlocale(LC_NUMERIC, "C");
  g_advancedSettings.Initialize();

  g_advancedSettings.m_startFullScreen = true;
  //g_application.SetStandAlone(true);
 
  g_application.Preflight();
  if (g_application.Create())
  {
    xbmcAlive = TRUE;
    try
    {
      CCocoaAutoPool innerpool;
      g_application.Run();
    }
    catch(...)
    {
      NSLog(@"%sException caught on main loop. Exiting", __PRETTY_FUNCTION__);
    }
  }
  else
  {
    NSLog(@"%sUnable to create application", __PRETTY_FUNCTION__);
  }

  // signal we are dead
  [myLock unlockWithCondition:TRUE];

  // grrr, xbmc does not shutdown properly and leaves
  // several classes in an indeterminant state, we must exit and
  // reload Lowtide/AppleTV, boo.
  [g_xbmcController enableScreenSaver];
  [g_xbmcController enableSystemSleep];
  //[g_xbmcController applicationDidExit];
  exit(0);
}

//--------------------------------------------------------------
- (void) runDisplayLink;
{
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

  displayFPS = 1.0 / ([displayLink duration] * [displayLink frameInterval]);
  if (animationThread && [animationThread isExecuting] == YES)
  {
    if (g_VideoReferenceClock)
      g_VideoReferenceClock.VblankHandler(CurrentHostCounter(), displayFPS);
  }
  [pool release];
}
//--------------------------------------------------------------
- (void) initDisplayLink
{
  displayLink = [NSClassFromString(@"CADisplayLink") 
    displayLinkWithTarget:self
    selector:@selector(runDisplayLink)];
  [displayLink setFrameInterval:1];
  [displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
  displayFPS = 1.0 / ([displayLink duration] * [displayLink frameInterval]);
}
//--------------------------------------------------------------
- (void) deinitDisplayLink
{
  [displayLink invalidate];
  displayLink = nil;
}
//--------------------------------------------------------------
- (double) getDisplayLinkFPS;
{
  return displayFPS;
}
//--------------------------------------------------------------
@end
