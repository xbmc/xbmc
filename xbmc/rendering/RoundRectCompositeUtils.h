/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Geometry.h" // CRect

#include <algorithm>
#include <array>
#include <utility>

namespace ROUNDRECT
{
inline CRect ScreenTLToFramebufferBL(const CRect& rectScreenTL, const GLint vp[4])
{
  const float vpX = static_cast<float>(vp[0]);
  const float vpY = static_cast<float>(vp[1]);
  const float vpH = static_cast<float>(vp[3]);

  CRect rectFbBL(rectScreenTL.x1 + vpX, vpY + (vpH - rectScreenTL.y2), rectScreenTL.x2 + vpX,
                 vpY + (vpH - rectScreenTL.y1));

  if (rectFbBL.x2 < rectFbBL.x1)
    std::swap(rectFbBL.x1, rectFbBL.x2);
  if (rectFbBL.y2 < rectFbBL.y1)
    std::swap(rectFbBL.y1, rectFbBL.y2);

  return rectFbBL;
}

inline float ClampRadiusToRect(float radiusPx, const CRect& rectFbBL)
{
  const float maxR = std::min(rectFbBL.Width(), rectFbBL.Height()) * 0.5f;
  return std::max(0.0f, std::min(radiusPx, maxR));
}

struct GLCompositeStateGuardBase
{
  GLint program{0};
  GLint arrayBuffer{0};
  GLint activeTex{0};
  GLint tex2D{0};

  GLboolean blend{GL_FALSE};
  GLint blendSrcRGB{0};
  GLint blendDstRGB{0};
  GLint blendSrcA{0};
  GLint blendDstA{0};

  GLboolean scissor{GL_FALSE};
  std::array<GLint, 4> scissorBox{{0, 0, 0, 0}};

  GLCompositeStateGuardBase()
  {
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBuffer);

    glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTex);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex2D);
    glActiveTexture(activeTex);

    blend = glIsEnabled(GL_BLEND);
    glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrcRGB);
    glGetIntegerv(GL_BLEND_DST_RGB, &blendDstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrcA);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDstA);

    scissor = glIsEnabled(GL_SCISSOR_TEST);
    glGetIntegerv(GL_SCISSOR_BOX, scissorBox.data());
  }

  void RestoreCommon() const
  {
    if (scissor)
      glEnable(GL_SCISSOR_TEST);
    else
      glDisable(GL_SCISSOR_TEST);
    glScissor(scissorBox[0], scissorBox[1], scissorBox[2], scissorBox[3]);

    if (blend)
      glEnable(GL_BLEND);
    else
      glDisable(GL_BLEND);
    glBlendFuncSeparate(static_cast<GLenum>(blendSrcRGB), static_cast<GLenum>(blendDstRGB),
                        static_cast<GLenum>(blendSrcA), static_cast<GLenum>(blendDstA));

    glUseProgram(static_cast<GLuint>(program));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(arrayBuffer));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(tex2D));
    glActiveTexture(static_cast<GLenum>(activeTex));
  }
};
} // namespace ROUNDRECT
