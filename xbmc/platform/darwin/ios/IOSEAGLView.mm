/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "IOSEAGLView.h"

#include "FileItem.h"
#import "IOSScreenManager.h"
#include "ServiceBroker.h"
#include "Util.h"
#import "XBMCController.h"
#include "application/AppEnvironment.h"
#include "application/AppInboundProtocol.h"
#include "application/AppParams.h"
#include "application/Application.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"

#import "platform/darwin/DarwinUtils.h"
#import "platform/darwin/NSLogDebugHelpers.h"
#import "platform/darwin/ios-common/AnnounceReceiver.h"

#include <signal.h>
#include <stdio.h>

#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import <QuartzCore/QuartzCore.h>
#include <sys/resource.h>

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
  auto frame = currentScreen.bounds;
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

- (CGFloat)getScreenScale:(UIScreen *)screen
{
  LOG(@"nativeScale %lf, scale %lf, traitScale %lf", screen.nativeScale, screen.scale,
            screen.traitCollection.displayScale);
  return std::max({screen.nativeScale, screen.scale, screen.traitCollection.displayScale});
}

- (void) setScreen:(UIScreen *)screen withFrameBufferResize:(BOOL)resize
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

    context = newContext;

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
  std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
  if (appPort)
    appPort->SetRenderGUI(false);
}
//--------------------------------------------------------------
- (void) resumeAnimation
{
  PRINT_SIGNATURE();
  pause = FALSE;
  std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
  if (appPort)
    appPort->SetRenderGUI(true);
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
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_QUIT);
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
  @autoreleasepool
  {
    [[NSThread currentThread] setName:@"XBMC_Run"];

    // set up some xbmc specific relationships
    readyToRun = true;

    // signal we are alive
    NSConditionLock* myLock = arg;
    [myLock lock];

    // Prevent child processes from becoming zombies on exit if not waited upon. See also Util::Command
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_NOCLDWAIT;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGCHLD, &sa, NULL);

    setlocale(LC_NUMERIC, "C");

    const auto params = std::make_shared<CAppParams>();
#ifdef _DEBUG
    params->SetLogLevel(LOG_LEVEL_DEBUG);
#else
    params->SetLogLevel(LOG_LEVEL_NORMAL);
#endif
    CAppEnvironment::SetUp(params);

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
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_startFullScreen = true;
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_canWindowed = false;
      xbmcAlive = TRUE;
      [g_xbmcController onXbmcAlive];
      try
      {
        @autoreleasepool
        {
          g_application.Run();
        }
      }
      catch (...)
      {
        ELOG(@"%sException caught on main loop. Exiting", __PRETTY_FUNCTION__);
      }
    }

    CAppEnvironment::TearDown();

    // signal we are dead
    [myLock unlockWithCondition:TRUE];

    // grrr, xbmc does not shutdown properly and leaves
    // several classes in an indeterminate state, we must exit and
    // reload Lowtide/AppleTV, boo.
    [g_xbmcController enableScreenSaver];
    [g_xbmcController enableSystemSleep];
    exit(0);
  }
}
//--------------------------------------------------------------
@end
