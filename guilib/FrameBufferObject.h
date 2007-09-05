#ifndef __FRAMEBUFFEROBJECT_H__
#define __FRAMEBUFFEROBJECT_H__

#include "include.h"

#ifdef HAS_SDL_OPENGL

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
  bool CreateAndBindToTexture(GLenum target, int width, int height, GLenum format);

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
