/*
*      Copyright (C) 2005-2008 Team XBMC
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
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*/


#include "system.h"

#if HAS_GLES == 2

#include <cmath>
#include "MatrixGLES.h"
#include "utils/log.h"

CMatrixGLES g_matrices;

#define MODE_WITHIN_RANGE(m)       ((m >= 0) && (m < (int)MM_MATRIXSIZE))

CMatrixGLES::CMatrixGLES()
{
  for (int i=0; i<(int)MM_MATRIXSIZE; i++)
  {
    m_matrices[i].push_back(new GLfloat[16]);
    MatrixMode((EMATRIXMODE)i);
    LoadIdentity();
  }
  m_matrixMode = (EMATRIXMODE)-1;
  m_pMatrix    = NULL;
}

CMatrixGLES::~CMatrixGLES()
{
  for (int i=0; i<(int)MM_MATRIXSIZE; i++)
  {
    while (!m_matrices[i].empty())
    {
      m_matrices[i].pop_back();
    }
  }
  m_matrixMode = (EMATRIXMODE)-1;
  m_pMatrix    = NULL;
}

GLfloat* CMatrixGLES::GetMatrix(EMATRIXMODE mode)
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

void CMatrixGLES::MatrixMode(EMATRIXMODE mode)
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

void CMatrixGLES::PushMatrix()
{
  if (m_pMatrix && MODE_WITHIN_RANGE(m_matrixMode))
  {
    GLfloat *matrix = new GLfloat[16];
    memcpy(matrix, m_pMatrix, sizeof(GLfloat)*16);
    m_matrices[m_matrixMode].push_back(matrix);
  }
}

void CMatrixGLES::PopMatrix()
{
  if (MODE_WITHIN_RANGE(m_matrixMode) && (m_matrices[m_matrixMode].size() > 1))
  {
    m_matrices[m_matrixMode].pop_back();
    m_pMatrix = m_matrices[m_matrixMode].back();
  }
}

void CMatrixGLES::LoadIdentity()
{
  if (m_pMatrix)
  {
    m_pMatrix[0] = 1.0f;  m_pMatrix[4] = 0.0f;  m_pMatrix[8]  = 0.0f;  m_pMatrix[12] = 0.0f;
    m_pMatrix[1] = 0.0f;  m_pMatrix[5] = 1.0f;  m_pMatrix[9]  = 0.0f;  m_pMatrix[13] = 0.0f;
    m_pMatrix[2] = 0.0f;  m_pMatrix[6] = 0.0f;  m_pMatrix[10] = 1.0f;  m_pMatrix[14] = 0.0f;
    m_pMatrix[3] = 0.0f;  m_pMatrix[7] = 0.0f;  m_pMatrix[11] = 0.0f;  m_pMatrix[15] = 1.0f;
  }
}

void CMatrixGLES::Ortho(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f)
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

void CMatrixGLES::Ortho2D(GLfloat l, GLfloat r, GLfloat b, GLfloat t)
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

void CMatrixGLES::Frustum(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f)
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

void CMatrixGLES::Translatef(GLfloat x, GLfloat y, GLfloat z)
{
  GLfloat matrix[16] = {1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 1.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 1.0f, 0.0f,
                           x,    y,    z, 1.0f};
  MultMatrixf(matrix);
}

void CMatrixGLES::Scalef(GLfloat x, GLfloat y, GLfloat z)
{
  GLfloat matrix[16] = {   x, 0.0f, 0.0f, 0.0f,
                        0.0f,    y, 0.0f, 0.0f,
                        0.0f, 0.0f,    z, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f};
  MultMatrixf(matrix);
}

void CMatrixGLES::Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
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

void CMatrixGLES::MultMatrixf(const GLfloat *matrix)
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

// gluLookAt implementation taken from Mesa3D
void CMatrixGLES::LookAt(GLfloat eyex, GLfloat eyey, GLfloat eyez, GLfloat centerx, GLfloat centery, GLfloat centerz, GLfloat upx, GLfloat upy, GLfloat upz)
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

void CMatrixGLES::PrintMatrix(void)
{
  for (int i=0; i<(int)MM_MATRIXSIZE; i++)
  {
    GLfloat *m = GetMatrix((EMATRIXMODE)i);
    CLog::Log(LOGDEBUG, "MatrixGLES - Matrix:%d", i);
    CLog::Log(LOGDEBUG, "%f %f %f %f", m[0], m[4], m[8],  m[12]);
    CLog::Log(LOGDEBUG, "%f %f %f %f", m[1], m[5], m[9],  m[13]);
    CLog::Log(LOGDEBUG, "%f %f %f %f", m[2], m[6], m[10], m[14]);
    CLog::Log(LOGDEBUG, "%f %f %f %f", m[3], m[7], m[11], m[15]);
  }
}

#endif
