#ifndef __GLSLOUTPUT_H__
#define __GLSLOUTPUT_H__

/*
 *      Copyright (C) 2007-2013 Team XBMC
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

#include "system.h"
#include "utils/GLUtils.h"

namespace Shaders {

  class GLSLOutput
  {
  public:
    // take the 1st available texture unit as a parameter
    GLSLOutput(int texunit);
    std::string GetDefines();
    void OnCompiledAndLinked(GLuint programHandle);
    bool OnEnabled();
    void OnDisabled();
    void Free();

  private:
    void FreeTextures();

    bool m_dither;
    bool m_fullRange;
    // first texture unit available to us
    int m_1stTexUnit;
    int m_uDither;

    // defines

    // attribute locations
    GLint m_hDither;
    GLint m_hDitherQuant;
    GLint m_hDitherSize;

    // textures
    GLuint m_tDitherTex;


  };
}
#endif // __GLSLOUTPUT_H__
