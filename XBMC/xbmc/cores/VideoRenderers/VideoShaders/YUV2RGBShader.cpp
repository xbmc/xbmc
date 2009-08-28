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


#include "stdafx.h"
#include "include.h"
#include "../RenderFlags.h"
#include "YUV2RGBShader.h"
#include "Settings.h"
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
     default:
       return (float**)yuv_coef_bt601; break;
   }
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

string BaseYUV2RGBARBShader::BuildYUVMatrix()
{
  // Pick the matrix.
  float (*matrix)[4] = (float (*)[4])PickYUVConversionMatrix(m_flags);
  
  // Convert to ARB matrix. The forth vector is needed because the generated code
  // uses negation on a vector, and also negation on an element of the vector, so
  // I needed to add another "pre-negated" vector in.
  //
  stringstream strStream;
  strStream << "{ ";
  strStream << "  {     1.0,                   -0.0625,             1.1643835,               1.1383928        },\n";
  strStream << "  {    -0.5,                    0.0, "          <<  matrix[1][1] << ", " <<  matrix[1][2] << "},\n";
  strStream << "  {" << matrix[2][0] << ", " << matrix[2][1] << ",  0.0,                     0.0              },\n"; 
  strStream << "  {    -0.5,                    0.0, "          << -matrix[1][1] << ", " << -matrix[1][2] << "}\n";
  strStream << "};\n";

  return strStream.str();
}


//////////////////////////////////////////////////////////////////////
// YUV2RGBProgressiveShaderARB - YUV2RGB with no deinterlacing
//////////////////////////////////////////////////////////////////////

YUV2RGBProgressiveShaderARB::YUV2RGBProgressiveShaderARB(bool rect, unsigned flags)
  : BaseYUV2RGBARBShader(flags)
{
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

  if (flags & CONF_FLAGS_YUV_FULLRANGE)
  {
    source ="!!ARBfp1.0\n"
      "PARAM c[2] = { { 0,           -0.1720674,  0.88599992, -1 },\n"
      "		             { 0.70099545,  -0.35706902, 0,           2 } };\n"
      "TEMP R0;\n"
      "TEMP R1;\n"
      "TEX R1.x, fragment.texcoord[2], texture[2], "+target+";\n"
      "TEX R0.x, fragment.texcoord[1], texture[1], "+target+";\n"
      "MUL R0.z, R0.x, c[1].w;\n"
      "MUL R0.y, R1.x, c[1].w;\n"
      "TEX R0.x, fragment.texcoord[0], texture[0], "+target+";\n"
      "ADD R0.z, R0, c[0].w;\n"
      "MAD R1.xyz, R0.z, c[0], R0.x;\n"
      "ADD R0.x, R0.y, c[0].w;\n"
      "MAD result.color.xyz, R0.x, c[1], R1;\n"
      "MOV result.color.w, fragment.color.w;\n"
      "END\n";
  }
  else
  {
    source = 
      "!!ARBfp1.0\n"
      "PARAM c[4] = \n" + BuildYUVMatrix() +
      "TEMP R0;\n"
      "TEMP R1;\n"
      "TEX R1.x, fragment.texcoord[1], texture[1], "+target+"\n;"
      "ADD R0.z, R1.x, c[0].y;\n"
      "TEX R0.x, fragment.texcoord[2], texture[2], "+target+"\n;"
      "ADD R0.x, R0, c[0].y;\n"
      "MUL R0.y, R0.x, c[0].w;\n"
      "TEX R0.x, fragment.texcoord[0], texture[0], "+target+"\n;"
      "ADD R0.x, R0, c[0].y;\n"
      "MUL R0.z, R0, c[0].w;\n"
      "MUL R0.x, R0, c[0].z;\n"
      "ADD R0.z, R0, c[1].x;\n"
      "MAD R1.xyz, R0.z, c[1].yzww, R0.x;\n"
      "ADD R0.x, R0.y, c[3];\n"
      "MAD result.color.xyz, R0.x, c[2], R1;\n"
      "MOV result.color.w, fragment.color.w;\n"
      "END\n";
  }
  PixelShader()->SetSource(source);
}

void YUV2RGBProgressiveShaderARB::OnCompiledAndLinked()
{

}

bool YUV2RGBProgressiveShaderARB::OnEnabled()
{
  return true;
}


#endif // HAS_GL
