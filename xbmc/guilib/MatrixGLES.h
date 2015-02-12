#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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


#include <cmath>
#include <cstring>
#include <stack>

class CMatrixGL
{
public:

  CMatrixGL()                                  { memset(&m_pMatrix, 0, sizeof(m_pMatrix)); };
  CMatrixGL(const float matrix[16])            { memcpy(m_pMatrix, matrix, sizeof(m_pMatrix)); }
  CMatrixGL(const CMatrixGL &rhs )             { memcpy(m_pMatrix, rhs.m_pMatrix, sizeof(m_pMatrix)); }
  CMatrixGL &operator=( const CMatrixGL &rhs ) { memcpy(m_pMatrix, rhs.m_pMatrix, sizeof(m_pMatrix)); return *this;}
  operator float*()                            { return m_pMatrix; }
  operator const float*() const                { return m_pMatrix; }

  void LoadIdentity();
  void Ortho(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
  void Ortho2D(GLfloat l, GLfloat r, GLfloat b, GLfloat t);
  void Frustum(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
  void Translatef(GLfloat x, GLfloat y, GLfloat z);
  void Scalef(GLfloat x, GLfloat y, GLfloat z);
  void Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
  void MultMatrixf(const GLfloat *matrix);
  void LookAt(GLfloat eyex, GLfloat eyey, GLfloat eyez, GLfloat centerx, GLfloat centery, GLfloat centerz, GLfloat upx, GLfloat upy, GLfloat upz);

  static bool Project(GLfloat objx, GLfloat objy, GLfloat objz, const GLfloat modelMatrix[16], const GLfloat projMatrix[16], const GLint viewport[4], GLfloat* winx, GLfloat* winy, GLfloat* winz);

  void PrintMatrix(void);

  GLfloat m_pMatrix[16];
};

class CMatrixGLStack
{
public:
  CMatrixGLStack(GLenum type)
  : m_type(type)
  {}

  void Push()
  {
    m_stack.push(m_current);
  }

  void Clear()
  {
    m_stack = std::stack<CMatrixGL>();
  }

  void Pop()
  {
    if(!m_stack.empty())
    {
      m_current = m_stack.top();
      m_stack.pop();
    }
  }

  void Load();
  void PopLoad() { Pop(); Load(); }

  CMatrixGL& Get()        { return m_current; }
  CMatrixGL* operator->() { return &m_current; }

private:
  GLint                 m_type;
  std::stack<CMatrixGL> m_stack;
  CMatrixGL             m_current;
};

extern CMatrixGLStack glMatrixModview;
extern CMatrixGLStack glMatrixProject;
extern CMatrixGLStack glMatrixTexture;
