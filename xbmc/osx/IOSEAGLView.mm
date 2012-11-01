/*
 *      Copyright (C) 2010-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

//hack around problem with xbmc's typedef int BOOL
// and obj-c's typedef unsigned char BOOL
#define BOOL XBMC_BOOL 
#include <sys/resource.h>
#include <signal.h>
#include <stdio.h>

#include "system.h"
#include "AdvancedSettings.h"
#include "FileItem.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "WindowingFactory.h"
#include "VideoReferenceClock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "Util.h"
#undef BOOL

#import <QuartzCore/QuartzCore.h>

#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import "IOSEAGLView.h"
#import "XBMCController.h"
#import "IOSScreenManager.h"
#import "AutoPool.h"
#import "DarwinUtils.h"

//--------------------------------------------------------------
@interface IOSEAGLView (PrivateMethods)
- (void) setContext:(EAGLContext *)newContext;
- (void) createFramebuffer;
- (void) deleteFramebuffer;
- (void) runDisplayLink;
@end

@implementation IOSEAGLView
@synthesize animating;
@synthesize xbmcAlive;
@synthesize pause;
@synthesize currentScreen;
@synthesize framebufferResizeRequested;

// You must implement this method
+ (Class) layerClass
{
  return [CAEAGLLayer class];
}
//--------------------------------------------------------------
- (void) resizeFrameBuffer
{
  CGRect frame = [IOSScreenManager getLandscapeResolution: currentScreen]; 
  CAEAGLLayer *eaglLayer = (CAEAGLLayer *)[self layer];  
  //allow a maximum framebuffer size of 1080p
  //needed for tvout on iPad3 and maybe AppleTV3
  if(frame.size.width * frame.size.height > 2073600)
    return;
  //resize the layer - ios will delay this
  //and call layoutSubviews when its done with resizing
  //so the real framebuffer resize is done there then ...
  if(framebufferWidth != frame.size.width ||
     framebufferHeight != frame.size.height )
  {
    framebufferResizeRequested = TRUE;
    [eaglLayer setFrame:frame];
  }
}

- (void)layoutSubviews
{
  if(framebufferResizeRequested)
  {
    framebufferResizeRequested = FALSE;
    [self deinitDisplayLink];
    [self deleteFramebuffer];
    [self createFramebuffer];
    [self setFramebuffer];  
    [self initDisplayLink];
  }
}

- (CGFloat) getScreenScale:(UIScreen *)screen
{
  CGFloat ret = 1.0;
  if ([screen respondsToSelector:@selector(scale)])
  {    
    // atv2 reports 0.0 for scale - thats why we check this here
    // normal other iDevices report 1.0 here
    // retina devices report 2.0 here
    // this info is true as of 19.3.2012.
    if([screen scale] > 1.0)
    {
      ret = [screen scale];
    }
    
    //if no retina display scale detected yet -
    //ensure retina resolution on ipad3's mainScreen
    //even on older iOS SDKs
    if (ret == 1.0 && screen == [UIScreen mainScreen] && DarwinIsIPad3())
    {
      ret = 2.0;//iPad3 has scale factor 2 (like iPod 4g, iPhone4 and iPhone4s)
    }
  }
  return ret;
}

- (void) setScreen:(UIScreen *)screen withFrameBufferResize:(BOOL)resize;
{
  CGFloat scaleFactor = 1.0;
  CAEAGLLayer *eaglLayer = (CAEAGLLayer *)[self layer];

  currentScreen = screen;
  scaleFactor = [self getScreenScale: currentScreen];

  //this will activate retina on supported devices
  [eaglLayer setContentsScale:scaleFactor];
  [self setContentScaleFactor:scaleFactor];
  if(resize)
  {
    [self resizeFrameBuffer];
  }
}

//--------------------------------------------------------------
- (id)initWithFrame:(CGRect)frame withScreen:(UIScreen *)screen
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);
  framebufferResizeRequested = FALSE;
  if ((self = [super initWithFrame:frame]))
  {
    // Get the layer
    CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
    //set screen, handlescreenscale
    //and set frame size
    [self setScreen:screen withFrameBufferResize:FALSE];

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
    
    if(framebufferHeight > framebufferWidth) {
      glViewport(0, 0, framebufferHeight, framebufferWidth);
      glScissor(0, 0, framebufferHeight, framebufferWidth);
    } 
    else
    {
      glViewport(0, 0, framebufferWidth, framebufferHeight);
      glScissor(0, 0, framebufferWidth, framebufferHeight);
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
    xbmcAlive = FALSE;
    if (!g_application.m_bStop)
    {
      ThreadMessage tMsg = {TMSG_QUIT};
      CApplicationMessenger::Get().SendMessage(tMsg);
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
  bool readyToRun = true;

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
 
  g_application.Preflight();
  if (!g_application.Create())
  {
    readyToRun = false;
    NSLog(@"%sUnable to create application", __PRETTY_FUNCTION__);
  }

  if (!g_application.CreateGUI())
  {
    readyToRun = false;
    NSLog(@"%sUnable to create GUI", __PRETTY_FUNCTION__);
  }

  if (!g_application.Initialize())
  {
    readyToRun = false;
    NSLog(@"%sUnable to initialize application", __PRETTY_FUNCTION__);
  }
  
  if (readyToRun)
  {
    g_advancedSettings.m_startFullScreen = true;
    g_advancedSettings.m_canWindowed = false;
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
    if (g_VideoReferenceClock.IsRunning())
      g_VideoReferenceClock.VblankHandler(CurrentHostCounter(), displayFPS);
  }
  [pool release];
}
//--------------------------------------------------------------
- (void) initDisplayLink
{
  //init with the appropriate display link for the
  //used screen
  bool external = currentScreen != [UIScreen mainScreen];
  
  if(external)
  {
    fprintf(stderr,"InitDisplayLink on external");
  }
  else
  {
    fprintf(stderr,"InitDisplayLink on internal");
  }
  
  
  displayLink = [currentScreen displayLinkWithTarget:self selector:@selector(runDisplayLink)];
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
