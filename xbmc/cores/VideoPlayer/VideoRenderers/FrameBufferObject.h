/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
  GLuint m_fbo = 0;
  bool   m_valid;
  bool   m_bound;
  bool   m_supported;
  GLuint m_texid = 0;
};


