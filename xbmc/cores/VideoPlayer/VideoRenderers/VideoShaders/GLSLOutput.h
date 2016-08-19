/*
 *      Copyright (C) 2007-2015 Team XBMC
 *      Copyright (C) 2015 Lauri Mylläri
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "system.h"
#include "utils/GLUtils.h"

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
