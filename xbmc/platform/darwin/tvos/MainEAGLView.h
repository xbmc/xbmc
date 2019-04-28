/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/gl.h>

// This class wraps the CAEAGLLayer from CoreAnimation into a convenient UIView subclass.
// The view content is basically an EAGL surface you render your OpenGL scene into.
// Note that setting the view non-opaque will only work if the EAGL surface has an alpha channel.
@interface MainEAGLView : UIView
{
@private
  EAGLContext* m_context;
  // The pixel dimensions of the CAEAGLLayer.
  GLint m_framebufferWidth;
  GLint m_framebufferHeight;
  // The OpenGL ES names for the framebuffer and renderbuffer used to render to this view.
  GLuint m_defaultFramebuffer, m_colorRenderbuffer, m_depthRenderbuffer;
  UIScreen* m_currentScreen;
  BOOL m_framebufferResizeRequested;
}
@property (readonly, getter=getCurrentEAGLContext) EAGLContext* m_context;
@property (readonly, getter=getCurrentScreen) UIScreen* m_currentScreen;


- (id) initWithFrame:(CGRect)frame withScreen:(UIScreen*)screen;
- (void) setFramebuffer;
- (bool) presentFramebuffer;
- (CGFloat) getScreenScale:(UIScreen*)screen;

@end
