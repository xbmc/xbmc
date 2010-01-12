/*
* XBMC Media Center
* Video Filter Classes
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
#include "VideoFilterShader.h"
#include "utils/log.h"
#include "ConvolutionKernels.h"

#include <string>
#include <math.h>

#ifdef HAS_GL

using namespace Shaders;
using namespace std;

//////////////////////////////////////////////////////////////////////
// BaseVideoFilterShader - base class for video filter shaders
//////////////////////////////////////////////////////////////////////

BaseVideoFilterShader::BaseVideoFilterShader()
{
  m_width = 1;
  m_height = 1;
  m_hStepX = 0;
  m_hStepY = 0;
  m_stepX = 0;
  m_stepY = 0;
  m_sourceTexUnit = 0;
  m_hSourceTex = 0;

  string shaderv = 
    "uniform float stepx;"
    "uniform float stepy;"
    "void main()"
    "{"
    "gl_TexCoord[0].xy = gl_MultiTexCoord0.xy - vec2(stepx * 0.5, stepy * 0.5);"
    "gl_Position = ftransform();"
    "gl_FrontColor = gl_Color;"
    "}";
  VertexShader()->SetSource(shaderv);
}

//////////////////////////////////////////////////////////////////////
// BicubicFilterShader
//////////////////////////////////////////////////////////////////////

BicubicFilterShader::BicubicFilterShader(float B, float C)
{
  PixelShader()->LoadSource("bicubic.glsl");
  m_kernelTex1 = 0;
  m_B = B;
  m_C = C;
  if (B<0)
    m_B=1.0f/3.0f;
  if (C<0)
    m_C=1.0f/3.0f;
}

void BicubicFilterShader::OnCompiledAndLinked()
{
  // TODO: check for floating point texture support GL_ARB_texture_float

  // obtain shader attribute handles on successfull compilation
  m_hSourceTex = glGetUniformLocation(ProgramHandle(), "img");
  m_hStepX = glGetUniformLocation(ProgramHandle(), "stepx");
  m_hStepY = glGetUniformLocation(ProgramHandle(), "stepy");
  m_hKernTex = glGetUniformLocation(ProgramHandle(), "kernelTex");
  //CreateKernels(512, 1.0/2.0, 1.0/2.0);
  //CreateKernels(512, 0.0, 0.6); // mplayer's coeffs
  //CreateKernels(512, 1.0/3.0, 1.0/3.0);
  CreateKernels(256, m_B, m_C);
  VerifyGLState();
}

bool BicubicFilterShader::OnEnabled()
{
  // set shader attributes once enabled
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, m_kernelTex1);

  glActiveTexture(GL_TEXTURE0);
  glUniform1i(m_hSourceTex, m_sourceTexUnit);
  glUniform1i(m_hKernTex, 2);
  glUniform1f(m_hStepX, m_stepX);
  glUniform1f(m_hStepY, m_stepY);
  VerifyGLState();
  return true;
}

// Implementation idea taken from GPU Gems Vol.1

bool BicubicFilterShader::CreateKernels(int size, float B, float C)
{
  float *img = new float[size*4];

  if (!img)
  {
    CLog::Log(LOGERROR, "GL: Error creating bicubic kernels, out of memory");
    return false;
  }

  memset(img, 0, sizeof(float)*4);

  float *ptr = img;
  float x, val;

  for (int i = 0; i<size ; i++)
  {
    x = (float)i / (float)(size - 1);

    val = MitchellNetravali(x+1, B, C);
    *ptr = val;
    ptr++;

    val = MitchellNetravali(x, B, C);
    *ptr = val;
    ptr++;

    val = MitchellNetravali(1-x, B, C);
    *ptr = val;
    ptr++;

    val = MitchellNetravali(2-x, B, C);
    *ptr = val;;
    ptr++;
  }

  if (m_kernelTex1)
  {
    glDeleteTextures(1, &m_kernelTex1);
    m_kernelTex1 = 0;
  }

  glGenTextures(1, &m_kernelTex1);

  if ((m_kernelTex1<=0))
  {
    CLog::Log(LOGERROR, "GL: Error creating bicubic kernels, could not create textures");
    delete[] img;
    return false;
  }

  // bind positive portion of cubic curve to texture unit 2
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, m_kernelTex1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, size, 1, 0, GL_RGBA, GL_FLOAT, img);

  glActiveTexture(GL_TEXTURE0);
  delete[] img;
  return true;
}

void BicubicFilterShader::Free()
{
  if (m_kernelTex1)
    glDeleteTextures(1, &m_kernelTex1);
  m_kernelTex1 = 0;
  BaseVideoFilterShader::Free();
}

// Mitchell Netravali Reconstruction Filter
// B = 1,   C = 0     - cubic B-spline 
// B = 1/3, C = 1/3   - recommended
// B = 0,   C = 1/2   - Catmull-Rom spline
float BicubicFilterShader::MitchellNetravali(float x, float B, float C)
{
  float val = 0;
  float ax = fabs(x);
  if (ax<1.0f)
  {
    val =  ((12 - 9*B - 6*C) * ax * ax * ax +
            (-18 + 12*B + 6*C) * ax * ax +
            (6 - 2*B))/6;
  }
  else if (ax<2.0f)
  {
    val =  ((-B - 6*C) * ax * ax * ax + 
            (6*B + 30*C) * ax * ax + (-12*B - 48*C) * 
            ax + (8*B + 24*C)) / 6;
  }
  else
  {
    val = 0;
  }
//  val = ((val + 0.5) / 2);
  return val;
}

ConvolutionFilterShader::ConvolutionFilterShader(ESCALINGMETHOD method)
{
  m_method = method;
  m_kernelTex1 = 0;

  if (m_method == VS_SCALINGMETHOD_CUBIC || m_method == VS_SCALINGMETHOD_LANCZOS2)
    PixelShader()->LoadSource("convolution-4x4.glsl");
  else if (m_method == VS_SCALINGMETHOD_LANCZOS3)
    PixelShader()->LoadSource("convolution-6x6.glsl");
}

void ConvolutionFilterShader::OnCompiledAndLinked()
{
  // obtain shader attribute handles on successfull compilation
  m_hSourceTex = glGetUniformLocation(ProgramHandle(), "img");
  m_hStepX     = glGetUniformLocation(ProgramHandle(), "stepx");
  m_hStepY     = glGetUniformLocation(ProgramHandle(), "stepy");
  m_hKernTex   = glGetUniformLocation(ProgramHandle(), "kernelTex");

  int kernelsize = 256;
  CConvolutionKernel kernel(m_method, kernelsize);

  if (m_kernelTex1)
  {
    glDeleteTextures(1, &m_kernelTex1);
    m_kernelTex1 = 0;
  }

  glGenTextures(1, &m_kernelTex1);

  if ((m_kernelTex1<=0))
  {
    CLog::Log(LOGERROR, "GL: Error creating bicubic kernels, could not create textures");
    return;
  }

  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, m_kernelTex1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, kernelsize, 1, 0, GL_RGBA, GL_FLOAT, kernel.GetPixels());

  glActiveTexture(GL_TEXTURE0);

  VerifyGLState();
}

bool ConvolutionFilterShader::OnEnabled()
{
  // set shader attributes once enabled
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, m_kernelTex1);

  glActiveTexture(GL_TEXTURE0);
  glUniform1i(m_hSourceTex, m_sourceTexUnit);
  glUniform1i(m_hKernTex, 2);
  glUniform1f(m_hStepX, m_stepX);
  glUniform1f(m_hStepY, m_stepY);
  VerifyGLState();
  return true;
}

void ConvolutionFilterShader::Free()
{
  if (m_kernelTex1)
    glDeleteTextures(1, &m_kernelTex1);
  m_kernelTex1 = 0;
  BaseVideoFilterShader::Free();
}

#endif
