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
 */

//hack around problem with xbmc's typedef int BOOL
// and obj-c's typedef unsigned char BOOL
#define BOOL XBMC_BOOL 
#include <sys/resource.h>
#include <signal.h>
#include <stdio.h>

#include "system.h"
#include "settings/AdvancedSettings.h"
#include "FileItem.h"
#include "Application.h"
#include "messaging/ApplicationMessenger.h"
#include "windowing/WindowingFactory.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "Util.h"
#include "platform/XbmcContext.h"
#undef BOOL

#import <QuartzCore/QuartzCore.h>

#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import "IOSEAGLView.h"
#import "XBMCController.h"
#import "IOSScreenManager.h"
#import "platform/darwin/AutoPool.h"
#import "platform/darwin/DarwinUtils.h"
#import "platform/darwin/ios-common/AnnounceReceiver.h"
#import "XBMCDebugHelpers.h"

using namespace KODI::MESSAGING;

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
@synthesize readyToRun;
@synthesize pause;
@synthesize currentScreen;
@synthesize framebufferResizeRequested;
@synthesize context;

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
  //needed for tvout on iPad3/4 and iphone4/5 and maybe AppleTV3
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
    [self deleteFramebuffer];
    [self createFramebuffer];
    [self setFramebuffer];
  }
}

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
    
    //if no retina display scale detected yet -
    //ensure retina resolution on supported devices mainScreen
    //even on older iOS SDKs
    double screenScale = 1.0;
    if (ret == 1.0 && screen == [UIScreen mainScreen] && CDarwinUtils::DeviceHasRetina(screenScale))
    {
      ret = screenScale;//set scale factor from our static list in case older SDKs report 1.0
    }

    // fix for ip6 plus which seems to report 2.0 when not compiled with ios8 sdk
    if (CDarwinUtils::DeviceHasRetina(screenScale) && screenScale == 3.0)
    {
      ret = screenScale;
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
  //PRINT_SIGNATURE();
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
      ELOG(@"Failed to create ES context");
    else if (![EAGLContext setCurrentContext:aContext])
      ELOG(@"Failed to set ES context current");
    
    self.context = aContext;
    [aContext release];

    animating = FALSE;
    xbmcAlive = FALSE;
    pause = FALSE;
    [self setContext:context];
    [self createFramebuffer];
    [self setFramebuffer];
  }

  return self;
}

//--------------------------------------------------------------
- (void) dealloc
{
  //PRINT_SIGNATURE();
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
  PRINT_SIGNATURE();
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
    //PRINT_SIGNATURE();
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
      ELOG(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
  }
}
//--------------------------------------------------------------
- (void) deleteFramebuffer
{
  if (context && !pause)
  {
    PRINT_SIGNATURE();
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
  PRINT_SIGNATURE();
  pause = TRUE;
  g_application.SetRenderGUI(false);
}
//--------------------------------------------------------------
- (void) resumeAnimation
{
  PRINT_SIGNATURE();
  pause = FALSE;
  g_application.SetRenderGUI(true);
}
//--------------------------------------------------------------
- (void) startAnimation
{
  PRINT_SIGNATURE();
	if (!animating && context)
	{
		animating = TRUE;

    // kick off an animation thread
    animationThreadLock = [[NSConditionLock alloc] initWithCondition: FALSE];
    animationThread = [[NSThread alloc] initWithTarget:self
      selector:@selector(runAnimation:)
      object:animationThreadLock];
    [animationThread start];
	}
}
//--------------------------------------------------------------
- (void) stopAnimation
{
  PRINT_SIGNATURE();
	if (animating && context)
	{
		animating = FALSE;
    xbmcAlive = FALSE;
    if (!g_application.m_bStop)
    {
      CApplicationMessenger::GetInstance().PostMsg(TMSG_QUIT);
    }
    
    CAnnounceReceiver::GetInstance()->DeInitialize();
      
    // wait for animation thread to die
    if ([animationThread isFinished] == NO)
      [animationThreadLock lockWhenCondition:TRUE];
	}
}
//--------------------------------------------------------------
- (void) runAnimation:(id) arg
{
  CCocoaAutoPool outerpool;
  
  [[NSThread currentThread] setName:@"XBMC_Run"]; 
  
  // set up some xbmc specific relationships
  XBMC::Context context;
  readyToRun = true;

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
    ELOG(@"%sUnable to create application", __PRETTY_FUNCTION__);
  }
  
  CAnnounceReceiver::GetInstance()->Initialize();

  if (!g_application.CreateGUI())
  {
    readyToRun = false;
    ELOG(@"%sUnable to create GUI", __PRETTY_FUNCTION__);
  }

  if (!g_application.Initialize())
  {
    readyToRun = false;
    ELOG(@"%sUnable to initialize application", __PRETTY_FUNCTION__);
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
      ELOG(@"%sException caught on main loop. Exiting", __PRETTY_FUNCTION__);
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
@end
