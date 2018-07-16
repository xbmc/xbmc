/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "system_gl.h"

#include <stack>

class TransformMatrix;

class CMatrixGL
{
public:
  CMatrixGL() = default;

  constexpr CMatrixGL(GLfloat x0, GLfloat x1, GLfloat x2, GLfloat x3,
                      GLfloat x4, GLfloat x5, GLfloat x6, GLfloat x7,
                      GLfloat x8, GLfloat x9, GLfloat x10, GLfloat x11,
                      GLfloat x12, GLfloat x13, GLfloat x14, GLfloat x15)
    :m_pMatrix{x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15} {}

  CMatrixGL(const TransformMatrix &src) noexcept;

  operator const float*() const                { return m_pMatrix; }

  void LoadIdentity();
  void Ortho(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
  void Ortho2D(GLfloat l, GLfloat r, GLfloat b, GLfloat t);
  void Frustum(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
  void Translatef(GLfloat x, GLfloat y, GLfloat z);
  void Scalef(GLfloat x, GLfloat y, GLfloat z);
  void Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
  void MultMatrixf(const CMatrixGL &matrix) noexcept;
  void LookAt(GLfloat eyex, GLfloat eyey, GLfloat eyez, GLfloat centerx, GLfloat centery, GLfloat centerz, GLfloat upx, GLfloat upy, GLfloat upz);

  static bool Project(GLfloat objx, GLfloat objy, GLfloat objz, const GLfloat modelMatrix[16], const GLfloat projMatrix[16], const GLint viewport[4], GLfloat* winx, GLfloat* winy, GLfloat* winz);

private:
  /* alignas(16) allows better SIMD optimizations (e.g. SSE2 benefits
     a lot from this) */
  alignas(16) GLfloat m_pMatrix[16];
};

class CMatrixGLStack
{
public:
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
  std::stack<CMatrixGL> m_stack;
  CMatrixGL             m_current;
};

extern CMatrixGLStack glMatrixModview;
extern CMatrixGLStack glMatrixProject;
extern CMatrixGLStack glMatrixTexture;
