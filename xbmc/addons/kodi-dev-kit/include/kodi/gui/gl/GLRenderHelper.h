/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#ifdef __cplusplus

#include "GL.h"

#if defined(HAS_GL)

namespace kodi
{
namespace gui
{
namespace gl
{
class ATTR_DLL_LOCAL CGLRenderHelper : public kodi::gui::IRenderHelper
{
public:
  CGLRenderHelper() = default;

  ~CGLRenderHelper() { glDeleteVertexArrays(1, &m_vao); }

  bool Init() override
  {
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (!version)
      return false;

    int major = 0;
    int minor = 0;

    int match = sscanf(version, "%d.%d", &major, &minor);
    if (match != 2)
      return false;

    if (major < 3 || (major == 3 && minor < 2))
      return false;

    glGenVertexArrays(1, &m_vao);

    return true;
  }

  void Begin() override { glBindVertexArray(m_vao); }

  void End() override { glBindVertexArray(0); }

private:
  GLuint m_vao = 0;
};
} // namespace gl

using CRenderHelper = gl::CGLRenderHelper;

} // namespace gui
} // namespace kodi

#endif // HAS_GL

#endif // __cplusplus
