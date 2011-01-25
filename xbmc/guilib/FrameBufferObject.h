#ifndef __FRAMEBUFFEROBJECT_H__
#define __FRAMEBUFFEROBJECT_H__

/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "system.h" // for HAS_GL

#if defined(HAS_GL) || HAS_GLES == 2

//
// CFrameBufferObject
// A class that abstracts FBOs to facilitate Render To Texture
//
// Requires OpenGL 1.5+ or the GL_EXT_framebuffer_object extension.
//
// Usage:
//
//     CFrameBufferObject *fbo = new CFrameBufferObject();
//     fbo->Initialize();
//     fbo->CreateAndBindToTexture(GL_TEXTURE_2D, 256, 256, GL_RGBA);
//  OR fbo->BindToTexture(GL_TEXTURE_2D, <existing texture ID>);
//     fbo->BeginRender();
//     <normal GL rendering calls>
//     fbo->EndRender();
//     bind and use texture anywhere
//     glBindTexture(GL_TEXTURE_2D, fbo->Texture());
//

#if HAS_GLES == 2
// For OpenGL ES2.0, FBO are not extensions but part of the API.
#define glBindFramebufferEXT  glBindFramebuffer
#define GL_FRAMEBUFFER_EXT    GL_FRAMEBUFFER
#endif

class CFrameBufferObject
{
public:
  // Constructor
  CFrameBufferObject();

  // returns true if FBO support is detected
  bool IsSupported();

  // returns true if FBO has been initialized
  bool IsValid() { return m_valid; }

  // returns true if FBO has a texture bound to it
  bool IsBound() { return m_bound; }

  // initialize the FBO
  bool Initialize();

  // Cleanup
  void Cleanup();

  // Bind to an exiting texture
  bool BindToTexture(GLenum target, GLuint texid);

  // Set texture filtering
  void SetFiltering(GLenum target, GLenum mode);

  // Create a new texture and bind to it
  bool CreateAndBindToTexture(GLenum target, int width, int height, GLenum format,
                              GLenum filter=GL_LINEAR, GLenum clamp=GL_CLAMP_TO_EDGE);

  // Return the internally created texture ID
  GLuint Texture() { return m_texid; }

  // Begin rendering to FBO
  bool BeginRender()
  {
    if (IsValid() && IsBound())
    {
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fbo);
      return true;
    }
    return false;
  }

  // Finish rendering to FBO
  void EndRender()
  {
    if (IsValid())
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
  }

private:
  GLuint m_fbo;
  bool   m_valid;
  bool   m_bound;
  bool   m_supported;
  GLuint m_texid;
};

#endif

#endif
