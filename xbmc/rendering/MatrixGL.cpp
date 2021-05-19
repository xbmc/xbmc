/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MatrixGL.h"

#include "ServiceBroker.h"
#include "utils/TransformMatrix.h"

#if defined(HAS_NEON) && !defined(__LP64__)
#include "utils/CPUInfo.h"
void Matrix4Mul(float* src_mat_1, const float* src_mat_2);
#endif

#include <cmath>

CMatrixGLStack glMatrixModview = CMatrixGLStack();
CMatrixGLStack glMatrixProject = CMatrixGLStack();
CMatrixGLStack glMatrixTexture = CMatrixGLStack();

CMatrixGL::CMatrixGL(const TransformMatrix &src) noexcept
{
  for(int i = 0; i < 3; i++)
    for(int j = 0; j < 4; j++)
      m_pMatrix[j * 4 + i] = src.m[i][j];

  m_pMatrix[3] = 0.0f;
  m_pMatrix[7] = 0.0f;
  m_pMatrix[11] = 0.0f;
  m_pMatrix[15] = 1.0f;
}

void CMatrixGL::LoadIdentity()
{
  m_pMatrix[0] = 1.0f;  m_pMatrix[4] = 0.0f;  m_pMatrix[8]  = 0.0f;  m_pMatrix[12] = 0.0f;
  m_pMatrix[1] = 0.0f;  m_pMatrix[5] = 1.0f;  m_pMatrix[9]  = 0.0f;  m_pMatrix[13] = 0.0f;
  m_pMatrix[2] = 0.0f;  m_pMatrix[6] = 0.0f;  m_pMatrix[10] = 1.0f;  m_pMatrix[14] = 0.0f;
  m_pMatrix[3] = 0.0f;  m_pMatrix[7] = 0.0f;  m_pMatrix[11] = 0.0f;  m_pMatrix[15] = 1.0f;
}

void CMatrixGL::Ortho(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f)
{
  GLfloat u =  2.0f / (r - l);
  GLfloat v =  2.0f / (t - b);
  GLfloat w = -2.0f / (f - n);
  GLfloat x = - (r + l) / (r - l);
  GLfloat y = - (t + b) / (t - b);
  GLfloat z = - (f + n) / (f - n);
  const CMatrixGL matrix{   u, 0.0f, 0.0f, 0.0f,
                         0.0f,    v, 0.0f, 0.0f,
                         0.0f, 0.0f,    w, 0.0f,
                            x,    y,    z, 1.0f};
  MultMatrixf(matrix);
}

void CMatrixGL::Ortho2D(GLfloat l, GLfloat r, GLfloat b, GLfloat t)
{
  GLfloat u =  2.0f / (r - l);
  GLfloat v =  2.0f / (t - b);
  GLfloat x = - (r + l) / (r - l);
  GLfloat y = - (t + b) / (t - b);
  const CMatrixGL matrix{   u, 0.0f, 0.0f, 0.0f,
                         0.0f,    v, 0.0f, 0.0f,
                         0.0f, 0.0f,-1.0f, 0.0f,
                            x,    y, 0.0f, 1.0f};
  MultMatrixf(matrix);
}

void CMatrixGL::Frustum(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f)
{
  GLfloat u = (2.0f * n) / (r - l);
  GLfloat v = (2.0f * n) / (t - b);
  GLfloat w = (r + l) / (r - l);
  GLfloat x = (t + b) / (t - b);
  GLfloat y = - (f + n) / (f - n);
  GLfloat z = - (2.0f * f * n) / (f - n);
  const CMatrixGL matrix{   u, 0.0f, 0.0f, 0.0f,
                         0.0f,    v, 0.0f, 0.0f,
                            w,    x,    y,-1.0f,
                         0.0f, 0.0f,    z, 0.0f};
  MultMatrixf(matrix);
}

void CMatrixGL::Translatef(GLfloat x, GLfloat y, GLfloat z)
{
  const CMatrixGL matrix{1.0f, 0.0f, 0.0f, 0.0f,
                         0.0f, 1.0f, 0.0f, 0.0f,
                         0.0f, 0.0f, 1.0f, 0.0f,
                            x,    y,    z, 1.0f};
  MultMatrixf(matrix);
}

void CMatrixGL::Scalef(GLfloat x, GLfloat y, GLfloat z)
{
  const CMatrixGL matrix{   x, 0.0f, 0.0f, 0.0f,
                         0.0f,    y, 0.0f, 0.0f,
                         0.0f, 0.0f,    z, 0.0f,
                         0.0f, 0.0f, 0.0f, 1.0f};
  MultMatrixf(matrix);
}

void CMatrixGL::Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
  GLfloat modulus = std::sqrt((x*x)+(y*y)+(z*z));
  if (modulus != 0.0f)
  {
    x /= modulus;
    y /= modulus;
    z /= modulus;
  }
  GLfloat cosine = std::cos(angle);
  GLfloat sine   = std::sin(angle);
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
  const CMatrixGL matrix{   a,    d,    g, 0.0f,
                            b,    e,    h, 0.0f,
                            c,    f,    i, 0.0f,
                         0.0f, 0.0f, 0.0f, 1.0f};
  MultMatrixf(matrix);
}

void CMatrixGL::MultMatrixf(const CMatrixGL &matrix) noexcept
{
#if defined(HAS_NEON) && !defined(__LP64__)
  if ((CServiceBroker::GetCPUInfo()->GetCPUFeatures() & CPU_FEATURE_NEON) == CPU_FEATURE_NEON)
  {
    Matrix4Mul(m_pMatrix, matrix.m_pMatrix);
    return;
  }
#endif
    GLfloat a = (matrix.m_pMatrix[0]  * m_pMatrix[0]) + (matrix.m_pMatrix[1]  * m_pMatrix[4]) + (matrix.m_pMatrix[2]  * m_pMatrix[8])  + (matrix.m_pMatrix[3]  * m_pMatrix[12]);
    GLfloat b = (matrix.m_pMatrix[0]  * m_pMatrix[1]) + (matrix.m_pMatrix[1]  * m_pMatrix[5]) + (matrix.m_pMatrix[2]  * m_pMatrix[9])  + (matrix.m_pMatrix[3]  * m_pMatrix[13]);
    GLfloat c = (matrix.m_pMatrix[0]  * m_pMatrix[2]) + (matrix.m_pMatrix[1]  * m_pMatrix[6]) + (matrix.m_pMatrix[2]  * m_pMatrix[10]) + (matrix.m_pMatrix[3]  * m_pMatrix[14]);
    GLfloat d = (matrix.m_pMatrix[0]  * m_pMatrix[3]) + (matrix.m_pMatrix[1]  * m_pMatrix[7]) + (matrix.m_pMatrix[2]  * m_pMatrix[11]) + (matrix.m_pMatrix[3]  * m_pMatrix[15]);
    GLfloat e = (matrix.m_pMatrix[4]  * m_pMatrix[0]) + (matrix.m_pMatrix[5]  * m_pMatrix[4]) + (matrix.m_pMatrix[6]  * m_pMatrix[8])  + (matrix.m_pMatrix[7]  * m_pMatrix[12]);
    GLfloat f = (matrix.m_pMatrix[4]  * m_pMatrix[1]) + (matrix.m_pMatrix[5]  * m_pMatrix[5]) + (matrix.m_pMatrix[6]  * m_pMatrix[9])  + (matrix.m_pMatrix[7]  * m_pMatrix[13]);
    GLfloat g = (matrix.m_pMatrix[4]  * m_pMatrix[2]) + (matrix.m_pMatrix[5]  * m_pMatrix[6]) + (matrix.m_pMatrix[6]  * m_pMatrix[10]) + (matrix.m_pMatrix[7]  * m_pMatrix[14]);
    GLfloat h = (matrix.m_pMatrix[4]  * m_pMatrix[3]) + (matrix.m_pMatrix[5]  * m_pMatrix[7]) + (matrix.m_pMatrix[6]  * m_pMatrix[11]) + (matrix.m_pMatrix[7]  * m_pMatrix[15]);
    GLfloat i = (matrix.m_pMatrix[8]  * m_pMatrix[0]) + (matrix.m_pMatrix[9]  * m_pMatrix[4]) + (matrix.m_pMatrix[10] * m_pMatrix[8])  + (matrix.m_pMatrix[11] * m_pMatrix[12]);
    GLfloat j = (matrix.m_pMatrix[8]  * m_pMatrix[1]) + (matrix.m_pMatrix[9]  * m_pMatrix[5]) + (matrix.m_pMatrix[10] * m_pMatrix[9])  + (matrix.m_pMatrix[11] * m_pMatrix[13]);
    GLfloat k = (matrix.m_pMatrix[8]  * m_pMatrix[2]) + (matrix.m_pMatrix[9]  * m_pMatrix[6]) + (matrix.m_pMatrix[10] * m_pMatrix[10]) + (matrix.m_pMatrix[11] * m_pMatrix[14]);
    GLfloat l = (matrix.m_pMatrix[8]  * m_pMatrix[3]) + (matrix.m_pMatrix[9]  * m_pMatrix[7]) + (matrix.m_pMatrix[10] * m_pMatrix[11]) + (matrix.m_pMatrix[11] * m_pMatrix[15]);
    GLfloat m = (matrix.m_pMatrix[12] * m_pMatrix[0]) + (matrix.m_pMatrix[13] * m_pMatrix[4]) + (matrix.m_pMatrix[14] * m_pMatrix[8])  + (matrix.m_pMatrix[15] * m_pMatrix[12]);
    GLfloat n = (matrix.m_pMatrix[12] * m_pMatrix[1]) + (matrix.m_pMatrix[13] * m_pMatrix[5]) + (matrix.m_pMatrix[14] * m_pMatrix[9])  + (matrix.m_pMatrix[15] * m_pMatrix[13]);
    GLfloat o = (matrix.m_pMatrix[12] * m_pMatrix[2]) + (matrix.m_pMatrix[13] * m_pMatrix[6]) + (matrix.m_pMatrix[14] * m_pMatrix[10]) + (matrix.m_pMatrix[15] * m_pMatrix[14]);
    GLfloat p = (matrix.m_pMatrix[12] * m_pMatrix[3]) + (matrix.m_pMatrix[13] * m_pMatrix[7]) + (matrix.m_pMatrix[14] * m_pMatrix[11]) + (matrix.m_pMatrix[15] * m_pMatrix[15]);
    m_pMatrix[0] = a;  m_pMatrix[4] = e;  m_pMatrix[8]  = i;  m_pMatrix[12] = m;
    m_pMatrix[1] = b;  m_pMatrix[5] = f;  m_pMatrix[9]  = j;  m_pMatrix[13] = n;
    m_pMatrix[2] = c;  m_pMatrix[6] = g;  m_pMatrix[10] = k;  m_pMatrix[14] = o;
    m_pMatrix[3] = d;  m_pMatrix[7] = h;  m_pMatrix[11] = l;  m_pMatrix[15] = p;
}

// gluLookAt implementation taken from Mesa3D
void CMatrixGL::LookAt(GLfloat eyex, GLfloat eyey, GLfloat eyez, GLfloat centerx, GLfloat centery, GLfloat centerz, GLfloat upx, GLfloat upy, GLfloat upz)
{
  GLfloat forward[3], side[3], up[3];

  forward[0] = centerx - eyex;
  forward[1] = centery - eyey;
  forward[2] = centerz - eyez;

  up[0] = upx;
  up[1] = upy;
  up[2] = upz;

  GLfloat tmp = std::sqrt(forward[0]*forward[0] + forward[1]*forward[1] + forward[2]*forward[2]);
  if (tmp != 0.0f)
  {
    forward[0] /= tmp;
    forward[1] /= tmp;
    forward[2] /= tmp;
  }

  side[0] = forward[1]*up[2] - forward[2]*up[1];
  side[1] = forward[2]*up[0] - forward[0]*up[2];
  side[2] = forward[0]*up[1] - forward[1]*up[0];

  tmp = std::sqrt(side[0]*side[0] + side[1]*side[1] + side[2]*side[2]);
  if (tmp != 0.0f)
  {
    side[0] /= tmp;
    side[1] /= tmp;
    side[2] /= tmp;
  }

  up[0] = side[1]*forward[2] - side[2]*forward[1];
  up[1] = side[2]*forward[0] - side[0]*forward[2];
  up[2] = side[0]*forward[1] - side[1]*forward[0];

  const CMatrixGL matrix{
    side[0], up[0], -forward[0], 0.0f,
    side[1], up[1], -forward[1], 0.0f,
    side[2], up[2], -forward[2], 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
  };

  MultMatrixf(matrix);
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
bool CMatrixGL::Project(GLfloat objx, GLfloat objy, GLfloat objz, const GLfloat modelMatrix[16], const GLfloat projMatrix[16], const GLint viewport[4], GLfloat* winx, GLfloat* winy, GLfloat* winz)
{
  GLfloat in[4];
  GLfloat out[4];

  in[0]=objx;
  in[1]=objy;
  in[2]=objz;
  in[3]=1.0;
  __gluMultMatrixVecf(modelMatrix, in, out);
  __gluMultMatrixVecf(projMatrix, out, in);
  if (in[3] == 0.0f)
    return false;
  in[0] /= in[3];
  in[1] /= in[3];
  in[2] /= in[3];
  /* Map x, y and z to range 0-1 */
  in[0] = in[0] * 0.5f + 0.5f;
  in[1] = in[1] * 0.5f + 0.5f;
  in[2] = in[2] * 0.5f + 0.5f;

  /* Map x,y to viewport */
  in[0] = in[0] * viewport[2] + viewport[0];
  in[1] = in[1] * viewport[3] + viewport[1];

  *winx=in[0];
  *winy=in[1];
  *winz=in[2];
  return true;
}

void CMatrixGLStack::Load()
{

}
