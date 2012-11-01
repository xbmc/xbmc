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

#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/gl.h>

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
  BOOL pause;
  NSConditionLock* animationThreadLock;
  NSThread* animationThread;
  UIScreen *currentScreen;

	// Use of the CADisplayLink class is the preferred method for controlling the animation timing.
	// CADisplayLink will link to the main display and fire every vsync when added to a given run-loop.
	CADisplayLink *displayLink;
  CFTimeInterval displayFPS;
	BOOL displayLinkSupported;
  BOOL framebufferResizeRequested;
}
@property (readonly, nonatomic, getter=isAnimating) BOOL animating;
@property (readonly, nonatomic, getter=isXBMCAlive) BOOL xbmcAlive;
@property (readonly, nonatomic, getter=isPause) BOOL pause;
@property (readonly, getter=getCurrentScreen) UIScreen *currentScreen;
@property BOOL framebufferResizeRequested;

- (id)initWithFrame:(CGRect)frame withScreen:(UIScreen *)screen;
- (void) initDisplayLink;
- (void) deinitDisplayLink;
- (double) getDisplayLinkFPS;
- (void) pauseAnimation;
- (void) resumeAnimation;
- (void) startAnimation;
- (void) stopAnimation;
- (void) setFramebuffer;
- (bool) presentFramebuffer;
- (void) setScreen:(UIScreen *)screen withFrameBufferResize:(BOOL)resize;
- (CGFloat) getScreenScale:(UIScreen *)screen;
@end
