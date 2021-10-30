/*
 *  Copyright (c) 2007 d4rk
 *  Copyright (C) 2007-2018 Team Kodi
 *  Copyright (C) 2015 Lauri Mylläri
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GLSLOutput.h"

#include "RenderingGL.hpp"
#include "dither.h"
#include "utils/log.h"

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
  if (m_3DLUT)
    defines += "#define KODI_3DLUT\n";
  return defines;
}

void GLSLOutput::OnCompiledAndLinked(GLuint programHandle)
{
  FreeTextures();

  // get uniform locations
  //   dithering
  if (m_dither)
  {
    m_hDither = gl::GetUniformLocation(programHandle, "m_dither");
    m_hDitherQuant = gl::GetUniformLocation(programHandle, "m_ditherquant");
    m_hDitherSize = gl::GetUniformLocation(programHandle, "m_dithersize");
  }
  //   3DLUT
  if (m_3DLUT)
  {
    m_hCLUT = gl::GetUniformLocation(programHandle, "m_CLUT");
    m_hCLUTSize = gl::GetUniformLocation(programHandle, "m_CLUTsize");
  }

  if (m_dither)
  {
    //! @todo create a dither pattern

    // create a dither texture
    gl::GenTextures(1, &m_tDitherTex);
    if ( m_tDitherTex <= 0 )
    {
      CLog::Log(LOGERROR, "Error creating dither texture");
      return;
    }
    // bind and set texture parameters
    gl::ActiveTexture(GL_TEXTURE0 + m_uDither);
    gl::BindTexture(GL_TEXTURE_2D, m_tDitherTex);
    gl::PixelStorei(GL_UNPACK_ALIGNMENT, 1);
    gl::PixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // load dither texture data
    gl::TexImage2D(GL_TEXTURE_2D, 0, GL_R16, dither_size, dither_size, 0, GL_RED, GL_UNSIGNED_SHORT,
                   dither_matrix);
  }

  gl::ActiveTexture(GL_TEXTURE0);

  VerifyGLState();
}

bool GLSLOutput::OnEnabled()
{

  if (m_dither)
  {
    // set texture units
    gl::Uniform1i(m_hDither, m_uDither);
    VerifyGLState();

    // bind textures
    gl::ActiveTexture(GL_TEXTURE0 + m_uDither);
    gl::BindTexture(GL_TEXTURE_2D, m_tDitherTex);
    gl::ActiveTexture(GL_TEXTURE0);
    VerifyGLState();

    // dither settings
    gl::Uniform1f(m_hDitherQuant, (1 << m_ditherDepth) - 1.0);
    VerifyGLState();
    gl::Uniform2f(m_hDitherSize, dither_size, dither_size);
    VerifyGLState();
  }

  if (m_3DLUT)
  {
    // set texture units
    gl::Uniform1i(m_hCLUT, m_uCLUT);
    gl::Uniform1f(m_hCLUTSize, m_uCLUTSize);
    VerifyGLState();

    // bind textures
    gl::ActiveTexture(GL_TEXTURE0 + m_uCLUT);
    gl::BindTexture(GL_TEXTURE_3D, m_tCLUTTex);
    gl::ActiveTexture(GL_TEXTURE0);
    VerifyGLState();
  }

  VerifyGLState();
  return true;
}

void GLSLOutput::OnDisabled()
{
  // disable textures
  if (m_dither)
  {
    gl::ActiveTexture(GL_TEXTURE0 + m_uDither);
  }
  if (m_3DLUT)
  {
    gl::ActiveTexture(GL_TEXTURE0 + m_uCLUT);
    gl::Disable(GL_TEXTURE_3D);
  }
  gl::ActiveTexture(GL_TEXTURE0);
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
    gl::DeleteTextures(1, &m_tDitherTex);
    m_tDitherTex = 0;
  }
}

