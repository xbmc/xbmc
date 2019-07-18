/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/gl.h>
#import <UIKit/UIKit.h>

// This class wraps the CAEAGLLayer from CoreAnimation into a convenient UIView subclass.
// The view content is basically an EAGL surface you render your OpenGL scene into.
// Note that setting the view non-opaque will only work if the EAGL surface has an alpha channel.
@interface IOSEAGLView : UIView
{
@private
  EAGLContext *context;
  // The pixel dimensions of the CAEAGLLayer.
  GLint framebufferWidth;
  GLint framebufferHeight;
  // The OpenGL ES names for the framebuffer and renderbuffer used to render to this view.
  GLuint defaultFramebuffer, colorRenderbuffer, depthRenderbuffer;
	// the shader program object
	GLuint program;
	//
	GLfloat rotz;

	BOOL animating;
  BOOL xbmcAlive;
  BOOL readyToRun;
  BOOL pause;
  NSConditionLock* animationThreadLock;
  NSThread* animationThread;
  UIScreen* __weak currentScreen;

  BOOL framebufferResizeRequested;
}
@property (readonly, nonatomic, getter=isAnimating) BOOL animating;
@property (readonly, nonatomic, getter=isXBMCAlive) BOOL xbmcAlive;
@property (readonly, nonatomic, getter=isReadyToRun) BOOL readyToRun;
@property (readonly, nonatomic, getter=isPause) BOOL pause;
@property(weak, readonly, getter=getCurrentScreen) UIScreen* currentScreen;
@property (readonly, getter=getCurrentEAGLContext) EAGLContext *context;
@property BOOL framebufferResizeRequested;

- (id)initWithFrame:(CGRect)frame withScreen:(UIScreen *)screen;
- (void) pauseAnimation;
- (void) resumeAnimation;
- (void) startAnimation;
- (void) stopAnimation;
- (void) setFramebuffer;
- (bool) presentFramebuffer;
- (void) setScreen:(UIScreen *)screen withFrameBufferResize:(BOOL)resize;
- (CGFloat) getScreenScale:(UIScreen *)screen;
@end
