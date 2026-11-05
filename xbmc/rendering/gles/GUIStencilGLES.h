/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/log.h"
#include "windowing/GraphicContext.h"

#include "system_gl.h"

class CGUIStencilGLES
{
public:
  CGUIStencilGLES() = default;
  CGUIStencilGLES(const CGUIStencilGLES&) = delete;
  CGUIStencilGLES& operator=(const CGUIStencilGLES&) = delete;

  ~CGUIStencilGLES()
  {
    if (!m_saved)
      return;

    RestoreFace(GL_FRONT, m_front);
    RestoreFace(GL_BACK, m_back);
    if (!m_enabled)
      glDisable(GL_STENCIL_TEST);
  }

  bool Clip(STENCIL_LAYER layer)
  {
    if (layer == STENCIL_LAYER::NONE)
      return true;
    if (layer == STENCIL_LAYER::INVALID || !HasStencilBuffer())
      return false;

    Save();
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 0xFF, 1U << static_cast<unsigned int>(layer));
    glStencilMask(0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    return true;
  }

  bool Mask(STENCIL_LAYER layer, STENCIL_LAYER parent)
  {
    if (layer == STENCIL_LAYER::NONE || layer == STENCIL_LAYER::INVALID ||
        parent == STENCIL_LAYER::INVALID || !HasStencilBuffer())
      return false;

    Save();
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(parent == STENCIL_LAYER::NONE ? GL_ALWAYS : GL_EQUAL, 0xFF,
                  parent == STENCIL_LAYER::NONE ? 0xFF : 1U << static_cast<unsigned int>(parent));
    glStencilMask(1U << static_cast<unsigned int>(layer));
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    return true;
  }

private:
  struct Face
  {
    GLint function{GL_ALWAYS};
    GLint reference{0};
    GLint valueMask{0};
    GLint writeMask{0};
    GLint fail{GL_KEEP};
    GLint depthFail{GL_KEEP};
    GLint depthPass{GL_KEEP};
  };

  static bool HasStencilBuffer()
  {
    GLint bits{0};
    glGetIntegerv(GL_STENCIL_BITS, &bits);
    static bool unavailable{false};
    if (bits < 8 && !unavailable)
      CLog::Log(LOGERROR,
                "GUI masking requires 8 stencil bits; target has {}. Masked controls are "
                "hidden.",
                bits);
    unavailable = bits < 8;
    return !unavailable;
  }

  static void SaveFace(Face& state, bool back)
  {
    glGetIntegerv(back ? GL_STENCIL_BACK_FUNC : GL_STENCIL_FUNC, &state.function);
    glGetIntegerv(back ? GL_STENCIL_BACK_REF : GL_STENCIL_REF, &state.reference);
    glGetIntegerv(back ? GL_STENCIL_BACK_VALUE_MASK : GL_STENCIL_VALUE_MASK, &state.valueMask);
    glGetIntegerv(back ? GL_STENCIL_BACK_WRITEMASK : GL_STENCIL_WRITEMASK, &state.writeMask);
    glGetIntegerv(back ? GL_STENCIL_BACK_FAIL : GL_STENCIL_FAIL, &state.fail);
    glGetIntegerv(back ? GL_STENCIL_BACK_PASS_DEPTH_FAIL : GL_STENCIL_PASS_DEPTH_FAIL,
                  &state.depthFail);
    glGetIntegerv(back ? GL_STENCIL_BACK_PASS_DEPTH_PASS : GL_STENCIL_PASS_DEPTH_PASS,
                  &state.depthPass);
  }

  static void RestoreFace(GLenum face, const Face& state)
  {
    glStencilFuncSeparate(face, state.function, state.reference, state.valueMask);
    glStencilMaskSeparate(face, state.writeMask);
    glStencilOpSeparate(face, state.fail, state.depthFail, state.depthPass);
  }

  void Save()
  {
    m_enabled = glIsEnabled(GL_STENCIL_TEST);
    SaveFace(m_front, false);
    SaveFace(m_back, true);
    m_saved = true;
  }

  Face m_front;
  Face m_back;
  GLboolean m_enabled{GL_FALSE};
  bool m_saved{false};
};
