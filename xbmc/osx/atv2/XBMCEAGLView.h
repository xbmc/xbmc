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

#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/gl.h>

// This class wraps the CAEAGLLayer from CoreAnimation into a convenient UIView subclass.
// The view content is basically an EAGL surface you render your OpenGL scene into.
// Note that setting the view non-opaque will only work if the EAGL surface has an alpha channel.
@interface XBMCEAGLView : UIView
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
  NSConditionLock* animationThreadLock;
  NSThread* animationThread;

	// Use of the CADisplayLink class is the preferred method for controlling the animation timing.
	// CADisplayLink will link to the main display and fire every vsync when added to a given run-loop.
	CADisplayLink *displayLink;
  CFTimeInterval displayFPS;
	BOOL displayLinkSupported;
}
@property (readonly, nonatomic, getter=isAnimating) BOOL animating;

- (void) initDisplayLink;
- (void) deinitDisplayLink;
- (double) getDisplayLinkFPS;
- (void) startAnimation;
- (void) stopAnimation;
- (void) setFramebuffer;
- (bool) presentFramebuffer;

@end
