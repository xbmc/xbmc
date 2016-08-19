/*
 *      Copyright (c) 2007 d4rk
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

#include "system_gl.h"
#include "utils/log.h"

#include "GLSLOutput.h"
#include "dither.h"

using namespace Shaders;

GLSLOutput::GLSLOutput(int texunit, bool useDithering, unsigned int ditherDepth, bool fullrange, GLuint clutTex, int clutSize)
{
  // set member variable initial values
  m_1stTexUnit = texunit;
  m_uDither = m_1stTexUnit+0;
  m_uCLUT = m_1stTexUnit+1;

  //   textures
  m_tDitherTex  = 0;
  m_tCLUTTex  = clutTex;

  //   shader attribute handles
  m_hDither      = -1;
  m_hDitherQuant = -1;
  m_hDitherSize  = -1;
  m_hCLUT        = -1;
  m_hCLUTSize    = -1;

  m_dither = useDithering;
  m_ditherDepth = ditherDepth;
  m_fullRange = fullrange;
  // make sure CMS is enabled - this allows us to keep the texture
  // around to quickly switch between CMS on and off
  m_3DLUT = clutTex > 0;
  m_uCLUTSize = clutSize;
}

std::string GLSLOutput::GetDefines()
{
  std::string defines;
  if (m_dither)
    defines += "#define XBMC_DITHER\n";
  if (m_fullRange)
    defines += "#define XBMC_FULLRANGE\n";
#ifdef HAS_GL
  if (m_3DLUT)
    defines += "#define KODI_3DLUT\n";
#endif //HAS_GL
  return defines;
}

void GLSLOutput::OnCompiledAndLinked(GLuint programHandle)
{
  FreeTextures();

  // get uniform locations
  //   dithering
  if (m_dither)
  {
    m_hDither = glGetUniformLocation(programHandle, "m_dither");
    m_hDitherQuant = glGetUniformLocation(programHandle, "m_ditherquant");
    m_hDitherSize = glGetUniformLocation(programHandle, "m_dithersize");
  }
  //   3DLUT
  if (m_3DLUT)
  {
#ifdef HAS_GL
    m_hCLUT        = glGetUniformLocation(programHandle, "m_CLUT");
    m_hCLUTSize    = glGetUniformLocation(programHandle, "m_CLUTsize");
#endif //HAS_GL
  }

  if (m_dither)
  {
    //! @todo create a dither pattern

    // create a dither texture
    glGenTextures(1, &m_tDitherTex);
    if ( m_tDitherTex <= 0 )
    {
      CLog::Log(LOGERROR, "Error creating dither texture");
      return;
    }
    // bind and set texture parameters
    glActiveTexture(GL_TEXTURE0 + m_uDither);
    glBindTexture(GL_TEXTURE_2D, m_tDitherTex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
#if HAS_GLES != 2
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

#if HAS_GLES != 2
    // load dither texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, dither_size, dither_size, 0, GL_RED, GL_UNSIGNED_SHORT, dither_matrix);
#endif
  }

  glActiveTexture(GL_TEXTURE0);

  VerifyGLState();
}

bool GLSLOutput::OnEnabled()
{

  if (m_dither)
  {
    // set texture units
    glUniform1i(m_hDither, m_uDither);
    VerifyGLState();

    // bind textures
    glActiveTexture(GL_TEXTURE0 + m_uDither);
    glBindTexture(GL_TEXTURE_2D, m_tDitherTex);
    glActiveTexture(GL_TEXTURE0);
    VerifyGLState();

    // dither settings
    glUniform1f(m_hDitherQuant, (1<<m_ditherDepth)-1.0);
    VerifyGLState();
    glUniform2f(m_hDitherSize, dither_size, dither_size);
    VerifyGLState();
  }

  if (m_3DLUT)
  {
#ifdef HAS_GL
    // set texture units
    glUniform1i(m_hCLUT, m_uCLUT);
    glUniform1f(m_hCLUTSize, m_uCLUTSize);
    VerifyGLState();

    // bind textures
    glActiveTexture(GL_TEXTURE0 + m_uCLUT);
    glBindTexture(GL_TEXTURE_3D, m_tCLUTTex);
    glActiveTexture(GL_TEXTURE0);
    VerifyGLState();
#endif //HAS_GL
  }

  VerifyGLState();
  return true;
}

void GLSLOutput::OnDisabled()
{
  // disable textures
  if (m_dither)
  {
    glActiveTexture(GL_TEXTURE0 + m_uDither);
    glDisable(GL_TEXTURE_2D);
  }
  if (m_3DLUT)
  {
#ifdef HAS_GL
    glActiveTexture(GL_TEXTURE0 + m_uCLUT);
    glDisable(GL_TEXTURE_3D);
#endif //HAS_GL
  }
  glActiveTexture(GL_TEXTURE0);
  VerifyGLState();
}

void GLSLOutput::Free()
{
  FreeTextures();
}

void GLSLOutput::FreeTextures()
{
  if (m_tDitherTex)
  {
    glDeleteTextures(1, &m_tDitherTex);
    m_tDitherTex = 0;
  }
}

