/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  Copyright (C) 2015 Lauri Mylläri
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/GLUtils.h"

#include <string>

namespace Shaders
{
  class GLSLOutput
  {
  public:
    // take the 1st available texture unit as a parameter
    GLSLOutput(
      int texunit,
      bool useDithering,
      unsigned int ditherDepth,
      bool fullrange,
      GLuint clutTex,
      int clutSize);
    std::string GetDefines();
    void OnCompiledAndLinked(GLuint programHandle);
    bool OnEnabled();
    void OnDisabled();
    void Free();

  private:
    void FreeTextures();

    bool m_dither;
    unsigned int m_ditherDepth;
    bool m_fullRange;
    bool m_3DLUT;
    // first texture unit available to us
    int m_1stTexUnit;
    int m_uDither;
    int m_uCLUT;
    int m_uCLUTSize;

    // defines

    // attribute locations
    GLint m_hDither = -1;
    GLint m_hDitherQuant = -1;
    GLint m_hDitherSize = -1;
    GLint m_hCLUT = -1;
    GLint m_hCLUTSize = -1;

    // textures
    GLuint m_tDitherTex = 0;
    GLuint m_tCLUTTex;
  };
}
