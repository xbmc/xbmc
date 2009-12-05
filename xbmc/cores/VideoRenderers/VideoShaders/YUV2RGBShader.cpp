/*
* XBMC Media Center
* Shader Classes
* Copyright (c) 2007 d4rk
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include "system.h"
#include "../RenderFlags.h"
#include "YUV2RGBShader.h"
#include "AdvancedSettings.h"
#include "TransformMatrix.h"
#include "utils/log.h"

#include <string>
#include <sstream>

#ifdef HAS_GL

using namespace Shaders;
using namespace std;

//
// Transformation matrixes for different colorspaces.
//
static float yuv_coef_bt601[4][4] = 
{
    { 1.0f,      1.0f,     1.0f,     0.0f },
    { 0.0f,     -0.344f,   1.773f,   0.0f },
    { 1.403f,   -0.714f,   0.0f,     0.0f },
    { 0.0f,      0.0f,     0.0f,     0.0f } 
};

static float yuv_coef_bt709[4][4] =
{
    { 1.0f,      1.0f,     1.0f,     0.0f },
    { 0.0f,     -0.1870f,  1.8556f,  0.0f },
    { 1.5701f,  -0.4664f,  0.0f,     0.0f },
    { 0.0f,      0.0f,     0.0f,     0.0f }
};

static float yuv_coef_ebu[4][4] = 
{
    { 1.0f,      1.0f,     1.0f,     0.0f },
    { 0.0f,     -0.3960f,  2.029f,   0.0f },
    { 1.140f,   -0.581f,   0.0f,     0.0f },
    { 0.0f,      0.0f,     0.0f,     0.0f }
};

static float yuv_coef_smtp240m[4][4] =
{
    { 1.0f,      1.0f,     1.0f,     0.0f },
    { 0.0f,     -0.2253f,  1.8270f,  0.0f },
    { 1.5756f,  -0.5000f,  0.0f,     0.0f },
    { 0.0f,      0.0f,     0.0f,     0.0f }
};

static float** PickYUVConversionMatrix(unsigned flags)
{
  // Pick the matrix.
   
   switch(CONF_FLAGS_YUVCOEF_MASK(flags))
   {
     case CONF_FLAGS_YUVCOEF_240M:
       return (float**)yuv_coef_smtp240m; break;
     case CONF_FLAGS_YUVCOEF_BT709:
       return (float**)yuv_coef_bt709; break;
     case CONF_FLAGS_YUVCOEF_BT601:    
       return (float**)yuv_coef_bt601; break;
     case CONF_FLAGS_YUVCOEF_EBU:
       return (float**)yuv_coef_ebu; break;
   }
   
   return (float**)yuv_coef_bt601;
}

static void CalculateYUVMatrix(GLfloat      res[4][4]
                             , unsigned int flags
                             , float        black
                             , float        contrast)
{
  TransformMatrix matrix, coef;

  matrix *= TransformMatrix::CreateScaler(contrast, contrast, contrast);
  matrix *= TransformMatrix::CreateTranslation(black, black, black);

  float (*conv)[4] = (float (*)[4])PickYUVConversionMatrix(flags);
  for(int row = 0; row < 3; row++)
    for(int col = 0; col < 4; col++)
      coef.m[row][col] = conv[col][row];

  matrix *= coef;
  matrix *= TransformMatrix::CreateTranslation(0.0, -0.5, -0.5);
  if (!(flags & CONF_FLAGS_YUV_FULLRANGE))
  {
    matrix *= TransformMatrix::CreateScaler(255.0f / (235 - 16)
                                          , 255.0f / (240 - 16)
                                          , 255.0f / (240 - 16));
    matrix *= TransformMatrix::CreateTranslation(- 16.0f / 255
                                               , - 16.0f / 255
                                               , - 16.0f / 255);
  }

  for(int row = 0; row < 3; row++)
    for(int col = 0; col < 4; col++)
      res[col][row] = matrix.m[row][col];

  res[0][3] = 0.0f;
  res[1][3] = 0.0f;
  res[2][3] = 0.0f;
  res[3][3] = 1.0f;
}

//////////////////////////////////////////////////////////////////////
// BaseYUV2RGBGLSLShader - base class for GLSL YUV2RGB shaders
//////////////////////////////////////////////////////////////////////

BaseYUV2RGBGLSLShader::BaseYUV2RGBGLSLShader(bool rect, unsigned flags)
{
  m_width      = 1;
  m_height     = 1;
  m_field      = 0;
  m_flags      = flags;

  m_black      = 0.0f;
  m_contrast   = 1.0f;

  // shader attribute handles
  m_hYTex  = -1;
  m_hUTex  = -1;
  m_hVTex  = -1;

  if(rect)
    m_defines += "#define XBMC_texture_rectangle 1\n";
  else
    m_defines += "#define XBMC_texture_rectangle 0\n";

  if(g_advancedSettings.m_GLRectangleHack)
    m_defines += "#define XBMC_texture_rectangle_hack 1\n";
  else
    m_defines += "#define XBMC_texture_rectangle_hack 0\n";

  VertexShader()->LoadSource("yuv2rgb_vertex.glsl", m_defines);
}

void BaseYUV2RGBGLSLShader::OnCompiledAndLinked()
{
  m_hYTex   = glGetUniformLocation(ProgramHandle(), "m_sampY");
  m_hUTex   = glGetUniformLocation(ProgramHandle(), "m_sampU");
  m_hVTex   = glGetUniformLocation(ProgramHandle(), "m_sampV");
  m_hMatrix = glGetUniformLocation(ProgramHandle(), "m_yuvmat");
  VerifyGLState();
}

bool BaseYUV2RGBGLSLShader::OnEnabled()
{
  // set shader attributes once enabled
  glUniform1i(m_hYTex, 0);
  glUniform1i(m_hUTex, 1);
  glUniform1i(m_hVTex, 2);

  GLfloat matrix[4][4];
  CalculateYUVMatrix(matrix, m_flags, m_black, m_contrast);

  glUniformMatrix4fv(m_hMatrix, 1, GL_FALSE, (GLfloat*)matrix);
  VerifyGLState();
  return true;
}

//////////////////////////////////////////////////////////////////////
// BaseYUV2RGBGLSLShader - base class for GLSL YUV2RGB shaders
//////////////////////////////////////////////////////////////////////

BaseYUV2RGBARBShader::BaseYUV2RGBARBShader(unsigned flags)
{
  m_width         = 1;
  m_height        = 1;
  m_field         = 0;
  m_flags         = flags;

  // shader attribute handles
  m_hYTex  = -1;
  m_hUTex  = -1;
  m_hVTex  = -1;
}


//////////////////////////////////////////////////////////////////////
// YUV2RGBProgressiveShader - YUV2RGB with no deinterlacing
// Use for weave deinterlacing / progressive
//////////////////////////////////////////////////////////////////////

YUV2RGBProgressiveShader::YUV2RGBProgressiveShader(bool rect, unsigned flags)
  : BaseYUV2RGBGLSLShader(rect, flags)
{
  PixelShader()->LoadSource("yuv2rgb_basic.glsl", m_defines);
}


//////////////////////////////////////////////////////////////////////
// YUV2RGBBobShader - YUV2RGB with Bob deinterlacing
//////////////////////////////////////////////////////////////////////

YUV2RGBBobShader::YUV2RGBBobShader(bool rect, unsigned flags)
  : BaseYUV2RGBGLSLShader(rect, flags)
{
  m_hStepX = -1;
  m_hStepY = -1;
  m_hField = -1;
  PixelShader()->LoadSource("yuv2rgb_bob.glsl", m_defines);
}

void YUV2RGBBobShader::OnCompiledAndLinked()
{
  BaseYUV2RGBGLSLShader::OnCompiledAndLinked();
  m_hStepX = glGetUniformLocation(ProgramHandle(), "m_stepX");
  m_hStepY = glGetUniformLocation(ProgramHandle(), "m_stepY");
  m_hField = glGetUniformLocation(ProgramHandle(), "m_field");
  VerifyGLState();
}

bool YUV2RGBBobShader::OnEnabled()
{
  if(!BaseYUV2RGBGLSLShader::OnEnabled())
    return false;

  glUniform1i(m_hField, m_field);
  glUniform1f(m_hStepX, 1.0f / (float)m_width);
  glUniform1f(m_hStepY, 1.0f / (float)m_height);
  VerifyGLState();
  return true;
}

//////////////////////////////////////////////////////////////////////
// YUV2RGBProgressiveShaderARB - YUV2RGB with no deinterlacing
//////////////////////////////////////////////////////////////////////

YUV2RGBProgressiveShaderARB::YUV2RGBProgressiveShaderARB(bool rect, unsigned flags)
  : BaseYUV2RGBARBShader(flags)
{
  m_black      = 0.0f;
  m_contrast   = 1.0f;
  if(rect)
    PixelShader()->LoadSource("yuv2rgb_basic_rect.arb");
  else
    PixelShader()->LoadSource("yuv2rgb_basic_2d.arb");

}

void YUV2RGBProgressiveShaderARB::OnCompiledAndLinked()
{
}

bool YUV2RGBProgressiveShaderARB::OnEnabled()
{
  GLfloat matrix[4][4];
  CalculateYUVMatrix(matrix, m_flags, m_black, m_contrast);

  for(int i=0;i<4;i++)
    glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, i
                               , matrix[0][i]
                               , matrix[1][i]
                               , matrix[2][i]
                               , matrix[3][i]);
  return true;
}

//////////////////////////////////////////////////////////////////////
// NV122RGBProgressiveShaderARB - NV122RGB with no deinterlacing
//////////////////////////////////////////////////////////////////////

NV12ToRGBProgressiveShaderARB::NV12ToRGBProgressiveShaderARB(bool rect, unsigned flags)
  : BaseYUV2RGBARBShader(flags)
{
  m_black      = 0.0f;
  m_contrast   = 1.0f;
/*
  if(rect)
    PixelShader()->LoadSource("nv12rgb_basic_rect.arb");
  else
    PixelShader()->LoadSource("nv12rgb_basic_2d.arb");
*/
  string source = "";
  string target = "2D";
  if (rect)
  {
    target = "RECT";
  }
  
  // N.B. If you're changing this code, bear in mind that the GMA X3100 
  // (at least with OS X drivers in 10.5.2), doesn't allow for negation 
  // of constants, like "-c[0].y".
  //
  // Thanks to Aras Pranckevicius for bringing this bug to light.
  // Quoting him, in case the link dies:
  // "In my experience, the X3100 drivers seem to ignore negate modifiers on
  // constant registers in fragment (and possibly vertex) programs. It just
  // seems someone forgot to implement that. (radar: 5632811)"
  // - http://lists.apple.com/archives/mac-opengl/2008/Feb/msg00191.html

  source = 
    "!!ARBfp1.0\n"
    "PARAM c[4] = \n"
    "{{1.0,   -0.0625, 1.1643835, 1.1383928},\n"
    "{-0.5,    0.0,   -0.187,     1.8556},\n"
    "{1.5701, -0.4664, 0.0,       0.0},\n"
    "{ -0.5,   0.0,    0.187,    -1.8556}\n"
    "};\n"
    "TEMP T0;\n"
    "TEMP T1;\n"
    "TEMP yuv;\n"
    "TEX T0.x, fragment.texcoord[0], texture[0], "+target+";\n"
    "ADD yuv.x, T0.x, c[0].y;\n" // Y (yuv.x = tex0.x -0.0625)
    "MUL yuv.x, yuv.x, c[0].z;\n" //  (yuv.x *= 1.1643835)
    "TEX T0.xw, fragment.texcoord[1], texture[1], "+target+";\n"
    "ADD yuv.z, T0.x, c[0].y;\n" // V (yuv.z = tex1.x - 0.0625)
    "MUL yuv.z, yuv.z, c[0].w;\n" //  (yuv.z *= 1.1383928)
    "ADD yuv.z, yuv.z, c[1].x;\n" //  (yuv.z -= 0.5)
    "ADD yuv.y, T0.w, c[0].y;\n" // U (yuv.y = tex1.w - 0.0625)
    "MUL yuv.y, yuv.y, c[0].w;\n" //  (yuv.y *= 1.1383928)
    "ADD yuv.y, yuv.y, c[3].x;\n" //  (yuv.y -= 0.5)
    "MAD T1.xyz, yuv.z, c[1].yzww, yuv.x;\n"
    "MAD result.color.xyz, yuv.y, c[2], T1;\n"
    "MOV result.color.w, fragment.color.w;\n"
    "END\n";

  PixelShader()->SetSource(source);
}


void NV12ToRGBProgressiveShaderARB::OnCompiledAndLinked()
{

}

bool NV12ToRGBProgressiveShaderARB::OnEnabled()
{
  return true;
}

#endif // HAS_GL
