/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GuiCompositeGLES.h"

#include "GuiCompositeShaderGLES.h"
#include "utils/log.h"

#include <string>

#include "system_gl.h"

// Defined here, where CGuiCompositeShaderGLES is complete, for the unique_ptr member.
CGuiCompositeGLES::CGuiCompositeGLES() = default;
CGuiCompositeGLES::~CGuiCompositeGLES() = default;

bool CGuiCompositeGLES::Enable(int colorTransfer, bool limitedRange)
{
  m_guiCompositing = (colorTransfer != 0);

  if (m_guiCompositing)
  {
    if (!m_compositeShader)
    {
      std::string defines;
      if (limitedRange)
        defines += "#define KODI_LIMITED_RANGE 1\n";
      m_compositeShader = std::make_unique<CGuiCompositeShaderGLES>(defines);
      if (!m_compositeShader->CompileAndLink())
      {
        CLog::Log(LOGERROR, "CGuiCompositeGLES: failed to compile GUI composite shader");
        m_compositeShader.reset();
        m_guiCompositing = false;
        return false;
      }
    }

    if (!m_compositeShader->CreateLUTs(colorTransfer))
    {
      CLog::Log(LOGERROR, "CGuiCompositeGLES: failed to create LUTs");
      m_compositeShader.reset();
      m_guiCompositing = false;
      return false;
    }
  }
  else
  {
    m_guiFbo.Cleanup();
    m_guiFboWidth = 0;
    m_guiFboHeight = 0;
    m_compositeShader.reset();
  }

  return m_guiCompositing;
}

bool CGuiCompositeGLES::Begin(bool guiWillRender, int width, int height, bool attachDepth)
{
  if (!m_guiCompositing)
    return false;

  m_guiWillRender = guiWillRender;

  // create or recreate FBO if size changed
  if (!m_guiFbo.IsValid() || m_guiFboWidth != width || m_guiFboHeight != height)
  {
    m_guiFbo.Cleanup();

    if (!m_guiFbo.Initialize())
    {
      CLog::Log(LOGERROR, "CGuiCompositeGLES: failed to initialize GUI FBO");
      return false;
    }

    if (!m_guiFbo.CreateAndBindToTexture(GL_TEXTURE_2D, width, height, GL_RGBA))
    {
      CLog::Log(LOGERROR, "CGuiCompositeGLES: failed to create GUI FBO texture {}x{}", width,
                height);
      m_guiFbo.Cleanup();
      return false;
    }

    if (attachDepth && !m_guiFbo.AttachDepthBuffer(width, height))
    {
      CLog::Log(LOGERROR, "CGuiCompositeGLES: failed to attach depth buffer to GUI FBO {}x{}",
                width, height);
      m_guiFbo.Cleanup();
      return false;
    }

    m_guiFboWidth = width;
    m_guiFboHeight = height;
    m_guiFboClean = false; // fresh FBO is undefined, force a clear
    CLog::Log(LOGDEBUG, "CGuiCompositeGLES: created GUI FBO {}x{}", width, height);
  }

  // When GUI render is being skipped, leave the FBO bind/clear out: nothing
  // will draw into it this frame. The FBO's prior sRGB GUI content is
  // implicitly preserved across the skipped frame as a side effect.
  //! @todo The preserved sRGB FBO is currently not leveraged: a separate video
  //! plane reuses the post-PQ GUI back buffer directly via the display, and
  //! single-plane never reaches !guiWillRender (the dirty-driven skip is gated
  //! on IsRenderingVideoLayer). Future single-plane "gate, don't move" work
  //! lets the GUI walk skip while Composite still runs each video frame,
  //! re-using this cached sRGB FBO as the composite source.
  if (!guiWillRender)
    return true;

  if (!m_guiFbo.BeginRender())
    return false;

  // Clear only when the FBO holds stale content; idle frames are already clean.
  if (!m_guiFboClean)
  {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    m_guiFboClean = true;
  }

  return true;
}

void CGuiCompositeGLES::End(bool videoOnSeparatePlane)
{
  if (m_guiWillRender)
    m_guiFbo.EndRender();

  // When the GUI render is skipped this frame, Flip(hasRendered=false, ...)
  // will skip the buffer swap and the back-buffer contents never reach the
  // screen. Clearing it is pure waste. Gate on a separate video plane because
  // single-plane never reaches !m_guiWillRender (the dirty-driven skip is
  // gated on IsRenderingVideoLayer()), so this is the only path that triggers.
  if (videoOnSeparatePlane && !m_guiWillRender)
    return;

  // Clear the backbuffer before video renders. In the FBO compositing path,
  // video renders in the RenderEx pass with clear=false, so DrawBlackBars is
  // never called. Without this clear, letterbox areas retain stale content
  // from the swap chain when the display resolution doesn't change between
  // GUI and video playback.
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
}

// Composite is the last GL operation in the frame (called just before EndRender).
// GL state (blend mode, active texture, vertex arrays) is not restored afterward;
// the next frame's rendering sets its own state.
void CGuiCompositeGLES::Composite(bool videoOnSeparatePlane, unsigned int guiElementCount)
{
  if (!m_guiFbo.IsValid() || !m_guiFbo.IsBound() || !m_compositeShader)
    return;

  // Only update m_guiFboClean when GUI render fired this frame; otherwise the
  // FBO is in the same state as the previous frame and the flag stays as-is.
  // m_guiFboClean meaning depends on context:
  //   single-plane:   "FBO is empty/clean" (no composite work needed)
  //   separate plane: "FBO is empty/clean AND back buffer cache is invalid"
  if (m_guiWillRender)
  {
    const bool guiEmpty = (guiElementCount == 0);
    m_guiFboClean = guiEmpty;
    if (guiEmpty)
      return;
  }
  else if (m_guiFboClean)
  {
    return;
  }

  // Separate video plane with no new render: the cached PQ frame is already in
  // the GUI back buffer from the prior composite. Skip the shader pass
  // entirely; Flip will skip the buffer swap too (hasRendered==false because
  // Render was not called), and the display keeps showing the cached frame
  // while the video plane updates independently.
  if (!m_guiWillRender && videoOnSeparatePlane)
    return;

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_guiFbo.Texture());

  glEnable(GL_BLEND);

  // On a separate video plane the GUI buffer is composited against the video
  // by the display hardware or the system compositor. The default glBlendFunc
  // also blends the alpha channel, leaving the GUI buffer with src.a^2; that
  // composite then reads the squared alpha and translucent GUI pixels render
  // at the wrong opacity. Replace the stored alpha so it sees src.a.
  if (videoOnSeparatePlane)
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
  else
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // set up orthographic projection (screen coords, Y-down)
  float w = static_cast<float>(m_guiFboWidth);
  float h = static_cast<float>(m_guiFboHeight);

  GLfloat proj[16] = {2.0f / w, 0, 0, 0, 0, -2.0f / h, 0, 0, 0, 0, -1, 0, -1.0f, 1.0f, 0, 1};

  m_compositeShader->SetProjection(proj);
  m_compositeShader->Enable();

  GLint posLoc = m_compositeShader->GetPosLoc();
  GLint texLoc = m_compositeShader->GetTexLoc();

  GLfloat vert[4][2] = {{0, 0}, {w, 0}, {w, h}, {0, h}};
  GLfloat tex[4][2] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};
  GLubyte idx[4] = {0, 1, 3, 2};

  glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, vert);
  glVertexAttribPointer(texLoc, 2, GL_FLOAT, GL_FALSE, 0, tex);
  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(texLoc);

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(texLoc);

  m_compositeShader->Disable();
}
