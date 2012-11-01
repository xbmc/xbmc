#pragma once
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

#if defined(__APPLE__)                                                                                                                                                                                           
  #include <OpenGLES/ES2/gl.h>                                                                                                                                                                                     
  #include <OpenGLES/ES2/glext.h>                                                                                                                                                                                  
#else                                                                                                                                                                                                            
  #include <GLES2/gl2.h>
  #include <GLES2/gl2ext.h>
#endif//__APPLE__

#include <string.h>
#include <vector>

enum EMATRIXMODE
{
  MM_PROJECTION = 0,
  MM_MODELVIEW,
  MM_TEXTURE,
  MM_MATRIXSIZE  // Must be last! used for size of matrices
};

class CVisMatrixGLES
{
public:
  CVisMatrixGLES();
  ~CVisMatrixGLES();
  
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
  bool Project(GLfloat objx, GLfloat objy, GLfloat objz, const GLfloat modelMatrix[16], const GLfloat projMatrix[16], const GLint viewport[4], GLfloat* winx, GLfloat* winy, GLfloat* winz);

protected:

  struct MatrixWrapper 
  {
    MatrixWrapper(){};
    MatrixWrapper( const float values[16]) { memcpy(m_values,values,sizeof(m_values)); }
    MatrixWrapper( const MatrixWrapper &rhs ) { memcpy(m_values, rhs.m_values, sizeof(m_values)); }
    MatrixWrapper &operator=( const MatrixWrapper &rhs ) { memcpy(m_values, rhs.m_values, sizeof(m_values)); return *this;}
    operator float*() { return m_values; }
    operator const float*() const { return m_values; }

    float m_values[16];
  };

  std::vector<struct MatrixWrapper> m_matrices[(int)MM_MATRIXSIZE];
  GLfloat *m_pMatrix;
  EMATRIXMODE m_matrixMode;
};
