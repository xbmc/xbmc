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

#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/gl.h>

// This class wraps the CAEAGLLayer from CoreAnimation into a convenient UIView subclass.
// The view content is basically an EAGL surface you render your OpenGL scene into.
// Note that setting the view non-opaque will only work if the EAGL surface has an alpha channel.
@interface MainEAGLView : UIView
{    
@private
  EAGLContext  *m_context;
  // The pixel dimensions of the CAEAGLLayer.
  GLint         m_framebufferWidth;
  GLint         m_framebufferHeight;
  // The OpenGL ES names for the framebuffer and renderbuffer used to render to this view.
  GLuint        m_defaultFramebuffer, m_colorRenderbuffer, m_depthRenderbuffer;
  UIScreen     *m_currentScreen;
  BOOL          m_framebufferResizeRequested;
}
@property (readonly, getter=getCurrentEAGLContext) EAGLContext *m_context;
@property (readonly, getter=getCurrentScreen) UIScreen *m_currentScreen;


- (id)          initWithFrame:(CGRect)frame withScreen:(UIScreen *)screen;
- (void)        setFramebuffer;
- (bool)        presentFramebuffer;
- (CGFloat)     getScreenScale:(UIScreen *)screen;

@end
