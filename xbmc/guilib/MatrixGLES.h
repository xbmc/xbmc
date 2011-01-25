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

#ifndef MATRIX_GLES_H
#define MATRIX_GLES_H

#pragma once

#include <vector>

using namespace std;

enum EMATRIXMODE
{
  MM_PROJECTION = 0,
  MM_MODELVIEW,
  MM_TEXTURE,
  MM_MATRIXSIZE  // Must be last! used for size of matrices
};

class CMatrixGLES
{
public:
  CMatrixGLES();
  ~CMatrixGLES();
  
  GLfloat* GetMatrix(EMATRIXMODE mode);

  void MatrixMode(EMATRIXMODE mode);
  void PushMatrix();
  void PopMatrix();
  void LoadIdentity();
  void Ortho(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
  void Ortho2D(GLfloat l, GLfloat r, GLfloat b, GLfloat t);
  void Frustum(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
  void Translatef(GLfloat x, GLfloat y, GLfloat z);
  void Scalef(GLfloat x, GLfloat y, GLfloat z);
  void Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
  void MultMatrixf(const GLfloat *matrix);
  void LookAt(GLfloat eyex, GLfloat eyey, GLfloat eyez, GLfloat centerx, GLfloat centery, GLfloat centerz, GLfloat upx, GLfloat upy, GLfloat upz);
  void PrintMatrix(void);

protected:
  vector<GLfloat*> m_matrices[(int)MM_MATRIXSIZE];
  GLfloat *m_pMatrix;
  EMATRIXMODE m_matrixMode;
};

extern CMatrixGLES g_matrices;

#endif // MATRIX_GLES_H
