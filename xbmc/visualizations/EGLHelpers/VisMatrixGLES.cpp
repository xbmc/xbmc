/*
*      Copyright (C) 2005-2012 Team XBMC
*      http://www.xbmc.org
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

#include "VisMatrixGLES.h"
#include <cmath>

#define MODE_WITHIN_RANGE(m)       ((m >= 0) && (m < (int)MM_MATRIXSIZE))

CVisMatrixGLES::CVisMatrixGLES()
{
  for (unsigned int i=0; i < MM_MATRIXSIZE; i++)
  {
    m_matrices[i].push_back(MatrixWrapper());
    MatrixMode((EMATRIXMODE)i);
    LoadIdentity();
  }
  m_matrixMode = (EMATRIXMODE)-1;
  m_pMatrix    = NULL;
}

CVisMatrixGLES::~CVisMatrixGLES()
{
}

GLfloat* CVisMatrixGLES::GetMatrix(EMATRIXMODE mode)
{
  if (MODE_WITHIN_RANGE(mode))
  {
    if (!m_matrices[mode].empty())
    {
      return m_matrices[mode].back();
    }
  }
  return NULL;
}

void CVisMatrixGLES::MatrixMode(EMATRIXMODE mode)
{
  if (MODE_WITHIN_RANGE(mode))
  {
    m_matrixMode = mode;
    m_pMatrix    = m_matrices[mode].back();
  }
  else
  {
    m_matrixMode = (EMATRIXMODE)-1;
    m_pMatrix    = NULL;
  }
}

void CVisMatrixGLES::PushMatrix()
{
  if (m_pMatrix && MODE_WITHIN_RANGE(m_matrixMode))
  {
    m_matrices[m_matrixMode].push_back(MatrixWrapper(m_pMatrix));
    m_pMatrix =  m_matrices[m_matrixMode].back();
  }
}

void CVisMatrixGLES::PopMatrix()
{
  if (MODE_WITHIN_RANGE(m_matrixMode))
  {
    if (m_matrices[m_matrixMode].size() > 1)
    { 
      m_matrices[m_matrixMode].pop_back();
    }
    m_pMatrix = m_matrices[m_matrixMode].back();
  }
}

void CVisMatrixGLES::LoadIdentity()
{
  if (m_pMatrix)
  {
    m_pMatrix[0] = 1.0f;  m_pMatrix[4] = 0.0f;  m_pMatrix[8]  = 0.0f;  m_pMatrix[12] = 0.0f;
    m_pMatrix[1] = 0.0f;  m_pMatrix[5] = 1.0f;  m_pMatrix[9]  = 0.0f;  m_pMatrix[13] = 0.0f;
    m_pMatrix[2] = 0.0f;  m_pMatrix[6] = 0.0f;  m_pMatrix[10] = 1.0f;  m_pMatrix[14] = 0.0f;
    m_pMatrix[3] = 0.0f;  m_pMatrix[7] = 0.0f;  m_pMatrix[11] = 0.0f;  m_pMatrix[15] = 1.0f;
  }
}

void CVisMatrixGLES::Ortho(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f)
{
  GLfloat u =  2.0f / (r - l);
  GLfloat v =  2.0f / (t - b);
  GLfloat w = -2.0f / (f - n);
  GLfloat x = - (r + l) / (r - l);
  GLfloat y = - (t + b) / (t - b);
  GLfloat z = - (f + n) / (f - n);
  GLfloat matrix[16] = {   u, 0.0f, 0.0f, 0.0f,
                        0.0f,    v, 0.0f, 0.0f,
                        0.0f, 0.0f,    w, 0.0f,
                           x,    y,    z, 1.0f};
  MultMatrixf(matrix);
}

void CVisMatrixGLES::Ortho2D(GLfloat l, GLfloat r, GLfloat b, GLfloat t)
{
  GLfloat u =  2.0f / (r - l);
  GLfloat v =  2.0f / (t - b);
  GLfloat x = - (r + l) / (r - l);
  GLfloat y = - (t + b) / (t - b);
  GLfloat matrix[16] = {   u, 0.0f, 0.0f, 0.0f,
                        0.0f,    v, 0.0f, 0.0f,
                        0.0f, 0.0f,-1.0f, 0.0f,
                           x,    y, 0.0f, 1.0f};
  MultMatrixf(matrix);
}

void CVisMatrixGLES::Frustum(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f)
{
  GLfloat u = (2.0f * n) / (r - l);
  GLfloat v = (2.0f * n) / (t - b);
  GLfloat w = (r + l) / (r - l);
  GLfloat x = (t + b) / (t - b);
  GLfloat y = - (f + n) / (f - n);
  GLfloat z = - (2.0f * f * n) / (f - n);
  GLfloat matrix[16] = {   u, 0.0f, 0.0f, 0.0f,
                        0.0f,    v, 0.0f, 0.0f,
                           w,    x,    y,-1.0f,
                        0.0f, 0.0f,    z, 0.0f};
  MultMatrixf(matrix);
}

void CVisMatrixGLES::Translatef(GLfloat x, GLfloat y, GLfloat z)
{
  GLfloat matrix[16] = {1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 1.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 1.0f, 0.0f,
                           x,    y,    z, 1.0f};
  MultMatrixf(matrix);
}

void CVisMatrixGLES::Scalef(GLfloat x, GLfloat y, GLfloat z)
{
  GLfloat matrix[16] = {   x, 0.0f, 0.0f, 0.0f,
                        0.0f,    y, 0.0f, 0.0f,
                        0.0f, 0.0f,    z, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f};
  MultMatrixf(matrix);
}

void CVisMatrixGLES::Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
  GLfloat modulous = sqrt((x*x)+(y*y)+(z*z));
  if (modulous != 0.0)
  {
    x /= modulous;
    y /= modulous;
    z /= modulous;
  }
  GLfloat cosine = cos(angle);
  GLfloat sine   = sin(angle);
  GLfloat cos1   = 1 - cosine;
  GLfloat a = (x*x*cos1) + cosine;
  GLfloat b = (x*y*cos1) - (z*sine);
  GLfloat c = (x*z*cos1) + (y*sine);
  GLfloat d = (y*x*cos1) + (z*sine);
  GLfloat e = (y*y*cos1) + cosine;
  GLfloat f = (y*z*cos1) - (x*sine);
  GLfloat g = (z*x*cos1) - (y*sine);
  GLfloat h = (z*y*cos1) + (x*sine);
  GLfloat i = (z*z*cos1) + cosine;
  GLfloat matrix[16] = {   a,    d,    g, 0.0f,
                           b,    e,    h, 0.0f,
                           c,    f,    i, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f};
  MultMatrixf(matrix);
}

#if defined(__ARM_NEON__)
  
inline void Matrix4Mul(const float* src_mat_1, const float* src_mat_2, float* dst_mat)
{
  asm volatile (
    // Store A & B leaving room at top of registers for result (q0-q3)
    "vldmia %1, { q4-q7 }  \n\t"
    "vldmia %2, { q8-q11 } \n\t"

    // result = first column of B x first row of A
    "vmul.f32 q0, q8, d8[0]\n\t"
    "vmul.f32 q1, q8, d10[0]\n\t"
    "vmul.f32 q2, q8, d12[0]\n\t"
    "vmul.f32 q3, q8, d14[0]\n\t"

    // result += second column of B x second row of A
    "vmla.f32 q0, q9, d8[1]\n\t"
    "vmla.f32 q1, q9, d10[1]\n\t"
    "vmla.f32 q2, q9, d12[1]\n\t"
    "vmla.f32 q3, q9, d14[1]\n\t"

    // result += third column of B x third row of A
    "vmla.f32 q0, q10, d9[0]\n\t"
    "vmla.f32 q1, q10, d11[0]\n\t"
    "vmla.f32 q2, q10, d13[0]\n\t"
    "vmla.f32 q3, q10, d15[0]\n\t"

    // result += last column of B x last row of A
    "vmla.f32 q0, q11, d9[1]\n\t"
    "vmla.f32 q1, q11, d11[1]\n\t"
    "vmla.f32 q2, q11, d13[1]\n\t"
    "vmla.f32 q3, q11, d15[1]\n\t"

    // output = result registers
    "vstmia %2, { q0-q3 }"
    : //no output 
    : "r" (dst_mat), "r" (src_mat_2), "r" (src_mat_1)       // input - note *value* of pointer doesn't change
    : "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11" //clobber
    );
}
void CVisMatrixGLES::MultMatrixf(const GLfloat *matrix)
{
  if (m_pMatrix)
  {
    GLfloat m[16];
    Matrix4Mul(m_pMatrix, matrix, m);
  }
}

#else
void CVisMatrixGLES::MultMatrixf(const GLfloat *matrix)
{
  if (m_pMatrix)
  {
    GLfloat a = (matrix[0]  * m_pMatrix[0]) + (matrix[1]  * m_pMatrix[4]) + (matrix[2]  * m_pMatrix[8])  + (matrix[3]  * m_pMatrix[12]);
    GLfloat b = (matrix[0]  * m_pMatrix[1]) + (matrix[1]  * m_pMatrix[5]) + (matrix[2]  * m_pMatrix[9])  + (matrix[3]  * m_pMatrix[13]);
    GLfloat c = (matrix[0]  * m_pMatrix[2]) + (matrix[1]  * m_pMatrix[6]) + (matrix[2]  * m_pMatrix[10]) + (matrix[3]  * m_pMatrix[14]);
    GLfloat d = (matrix[0]  * m_pMatrix[3]) + (matrix[1]  * m_pMatrix[7]) + (matrix[2]  * m_pMatrix[11]) + (matrix[3]  * m_pMatrix[15]);
    GLfloat e = (matrix[4]  * m_pMatrix[0]) + (matrix[5]  * m_pMatrix[4]) + (matrix[6]  * m_pMatrix[8])  + (matrix[7]  * m_pMatrix[12]);
    GLfloat f = (matrix[4]  * m_pMatrix[1]) + (matrix[5]  * m_pMatrix[5]) + (matrix[6]  * m_pMatrix[9])  + (matrix[7]  * m_pMatrix[13]);
    GLfloat g = (matrix[4]  * m_pMatrix[2]) + (matrix[5]  * m_pMatrix[6]) + (matrix[6]  * m_pMatrix[10]) + (matrix[7]  * m_pMatrix[14]);
    GLfloat h = (matrix[4]  * m_pMatrix[3]) + (matrix[5]  * m_pMatrix[7]) + (matrix[6]  * m_pMatrix[11]) + (matrix[7]  * m_pMatrix[15]);
    GLfloat i = (matrix[8]  * m_pMatrix[0]) + (matrix[9]  * m_pMatrix[4]) + (matrix[10] * m_pMatrix[8])  + (matrix[11] * m_pMatrix[12]);
    GLfloat j = (matrix[8]  * m_pMatrix[1]) + (matrix[9]  * m_pMatrix[5]) + (matrix[10] * m_pMatrix[9])  + (matrix[11] * m_pMatrix[13]);
    GLfloat k = (matrix[8]  * m_pMatrix[2]) + (matrix[9]  * m_pMatrix[6]) + (matrix[10] * m_pMatrix[10]) + (matrix[11] * m_pMatrix[14]);
    GLfloat l = (matrix[8]  * m_pMatrix[3]) + (matrix[9]  * m_pMatrix[7]) + (matrix[10] * m_pMatrix[11]) + (matrix[11] * m_pMatrix[15]);
    GLfloat m = (matrix[12] * m_pMatrix[0]) + (matrix[13] * m_pMatrix[4]) + (matrix[14] * m_pMatrix[8])  + (matrix[15] * m_pMatrix[12]);
    GLfloat n = (matrix[12] * m_pMatrix[1]) + (matrix[13] * m_pMatrix[5]) + (matrix[14] * m_pMatrix[9])  + (matrix[15] * m_pMatrix[13]);
    GLfloat o = (matrix[12] * m_pMatrix[2]) + (matrix[13] * m_pMatrix[6]) + (matrix[14] * m_pMatrix[10]) + (matrix[15] * m_pMatrix[14]);
    GLfloat p = (matrix[12] * m_pMatrix[3]) + (matrix[13] * m_pMatrix[7]) + (matrix[14] * m_pMatrix[11]) + (matrix[15] * m_pMatrix[15]);
    m_pMatrix[0] = a;  m_pMatrix[4] = e;  m_pMatrix[8]  = i;  m_pMatrix[12] = m;
    m_pMatrix[1] = b;  m_pMatrix[5] = f;  m_pMatrix[9]  = j;  m_pMatrix[13] = n;
    m_pMatrix[2] = c;  m_pMatrix[6] = g;  m_pMatrix[10] = k;  m_pMatrix[14] = o;
    m_pMatrix[3] = d;  m_pMatrix[7] = h;  m_pMatrix[11] = l;  m_pMatrix[15] = p;
  }
}
#endif

// gluLookAt implementation taken from Mesa3D
void CVisMatrixGLES::LookAt(GLfloat eyex, GLfloat eyey, GLfloat eyez, GLfloat centerx, GLfloat centery, GLfloat centerz, GLfloat upx, GLfloat upy, GLfloat upz)
{
  GLfloat forward[3], side[3], up[3];
  GLfloat m[4][4];

  forward[0] = centerx - eyex;
  forward[1] = centery - eyey;
  forward[2] = centerz - eyez;

  up[0] = upx;
  up[1] = upy;
  up[2] = upz;

  GLfloat tmp = sqrt(forward[0]*forward[0] + forward[1]*forward[1] + forward[2]*forward[2]);
  if (tmp != 0.0)
  {
    forward[0] /= tmp;
    forward[1] /= tmp;
    forward[2] /= tmp;
  }

  side[0] = forward[1]*up[2] - forward[2]*up[1];
  side[1] = forward[2]*up[0] - forward[0]*up[2];
  side[2] = forward[0]*up[1] - forward[1]*up[0];

  tmp = sqrt(side[0]*side[0] + side[1]*side[1] + side[2]*side[2]);
  if (tmp != 0.0)
  {
    side[0] /= tmp;
    side[1] /= tmp;
    side[2] /= tmp;
  }

  up[0] = side[1]*forward[2] - side[2]*forward[1];
  up[1] = side[2]*forward[0] - side[0]*forward[2];
  up[2] = side[0]*forward[1] - side[1]*forward[0];

  m[0][0] = 1.0f; m[0][1] = 0.0f; m[0][2] = 0.0f; m[0][3] = 0.0f;
  m[1][0] = 0.0f; m[1][1] = 1.0f; m[1][2] = 0.0f; m[1][3] = 0.0f;
  m[2][0] = 0.0f; m[2][1] = 0.0f; m[2][2] = 1.0f; m[2][3] = 0.0f;
  m[3][0] = 0.0f; m[3][1] = 0.0f; m[3][2] = 0.0f; m[3][3] = 1.0f;

  m[0][0] = side[0];
  m[1][0] = side[1];
  m[2][0] = side[2];

  m[0][1] = up[0];
  m[1][1] = up[1];
  m[2][1] = up[2];

  m[0][2] = -forward[0];
  m[1][2] = -forward[1];
  m[2][2] = -forward[2];

  MultMatrixf(&m[0][0]);
  Translatef(-eyex, -eyey, -eyez);
}

static void __gluMultMatrixVecf(const GLfloat matrix[16], const GLfloat in[4], GLfloat out[4])
{
  int i;

  for (i=0; i<4; i++)
  {
    out[i] = in[0] * matrix[0*4+i] +
             in[1] * matrix[1*4+i] +
             in[2] * matrix[2*4+i] +
             in[3] * matrix[3*4+i];
  }
}

// gluProject implementation taken from Mesa3D
bool CVisMatrixGLES::Project(GLfloat objx, GLfloat objy, GLfloat objz, const GLfloat modelMatrix[16], const GLfloat projMatrix[16], const GLint viewport[4], GLfloat* winx, GLfloat* winy, GLfloat* winz)
{
  GLfloat in[4];
  GLfloat out[4];

  in[0]=objx;
  in[1]=objy;
  in[2]=objz;
  in[3]=1.0;
  __gluMultMatrixVecf(modelMatrix, in, out);
  __gluMultMatrixVecf(projMatrix, out, in);
  if (in[3] == 0.0)
    return false;
  in[0] /= in[3];
  in[1] /= in[3];
  in[2] /= in[3];
  /* Map x, y and z to range 0-1 */
  in[0] = in[0] * 0.5 + 0.5;
  in[1] = in[1] * 0.5 + 0.5;
  in[2] = in[2] * 0.5 + 0.5;

  /* Map x,y to viewport */
  in[0] = in[0] * viewport[2] + viewport[0];
  in[1] = in[1] * viewport[3] + viewport[1];

  *winx=in[0];
  *winy=in[1];
  *winz=in[2];
  return true;
}
