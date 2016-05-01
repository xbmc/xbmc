#pragma once

/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h" // for HAS_GL

#if defined(HAS_GL) || HAS_GLES == 2
#include "system_gl.h"

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

class CFrameBufferObject
{
public:
  // Constructor
  CFrameBufferObject();

  // returns true if FBO support is detected
  bool IsSupported();

  // returns true if FBO has been initialized
  bool IsValid() const { return m_valid; }

  // returns true if FBO has a texture bound to it
  bool IsBound() const { return m_bound; }

  // initialize the FBO
  bool Initialize();

  // Cleanup
  void Cleanup();

  // Bind to an exiting texture
  bool BindToTexture(GLenum target, GLuint texid);

  // Set texture filtering
  void SetFiltering(GLenum target, GLenum mode);

  // Create a new texture and bind to it
  bool CreateAndBindToTexture(GLenum target, int width, int height, GLenum format, GLenum type=GL_UNSIGNED_BYTE,
                              GLenum filter=GL_LINEAR, GLenum clampmode=GL_CLAMP_TO_EDGE);

  // Return the internally created texture ID
  GLuint Texture() const { return m_texid; }

  // Begin rendering to FBO
  bool BeginRender();
  // Finish rendering to FBO
  void EndRender() const;

private:
  GLuint m_fbo;
  bool   m_valid;
  bool   m_bound;
  bool   m_supported;
  GLuint m_texid;
};

#endif

