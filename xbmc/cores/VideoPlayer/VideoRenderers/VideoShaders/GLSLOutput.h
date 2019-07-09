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
    GLint m_hDither;
    GLint m_hDitherQuant;
    GLint m_hDitherSize;
    GLint m_hCLUT;
    GLint m_hCLUTSize;

    // textures
    GLuint m_tDitherTex;
    GLuint m_tCLUTTex;
  };
}
