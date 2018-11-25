/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../definitions.h"
#include "../../AddonBase.h"

#ifdef HAS_GL
  // always define GL_GLEXT_PROTOTYPES before include gl headers
  #if !defined(GL_GLEXT_PROTOTYPES)
    #define GL_GLEXT_PROTOTYPES
  #endif
  #if defined(__APPLE__)
    #include <OpenGL/gl3.h>
    #include <OpenGL/glu.h>
    #include <OpenGL/gl3ext.h>
  #else
    #include <GL/gl.h>
    #include <GL/glu.h>
    #include <GL/glext.h>
  #endif
#elif HAS_GLES >= 2
  #if defined(__APPLE__)
    #include <OpenGLES/ES2/gl.h>
    #include <OpenGLES/ES2/glext.h>
  #else
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>
  #endif
  #if HAS_GLES == 3
    #include <GLES3/gl3.h>
  #endif
#endif

#include <stack>

namespace kodi
{
namespace gui
{
namespace gl
{

  class ATTRIBUTE_HIDDEN CMatrix
  {
  public:
    CMatrix()
      : m_interface(::kodi::addon::CAddonBase::m_interface->toKodi)
    {
      m_matrix = m_interface->kodi_gui->gl_shader_matrix->matrix_create(m_interface->kodiBase);
      if (!m_matrix)
        kodi::Log(ADDON_LOG_FATAL, "kodi::gui::CMatrix can't create shader class from Kodi !!!");
    }

    CMatrix(float x0, float x1, float x2, float x3,
            float x4, float x5, float x6, float x7,
            float x8, float x9, float x10, float x11,
            float x12, float x13, float x14, float x15)
      : m_interface(::kodi::addon::CAddonBase::m_interface->toKodi)
    {
      m_matrix = m_interface->kodi_gui->gl_shader_matrix->matrix_create_a(m_interface->kodiBase,
                                                                          x0, x1, x2, x3, x4, x5, x6, x7, x8,
                                                                          x9, x10, x11, x12, x13, x14, x15);
      if (!m_matrix)
        kodi::Log(ADDON_LOG_FATAL, "kodi::gui::CMatrix can't create shader class from Kodi !!!");
    }

    virtual ~CMatrix()
    {
      if (m_matrix)
        m_interface->kodi_gui->gl_shader_matrix->matrix_destroy(m_interface->kodiBase, m_matrix);
    }

    operator const float*() const
    {
      return m_interface->kodi_gui->gl_shader_matrix->matrix_m_pMatrix(m_interface->kodiBase, m_matrix);
    }

    void LoadIdentity()
    {
      m_interface->kodi_gui->gl_shader_matrix->matrix_load_identity(m_interface->kodiBase, m_matrix);
    }

    void Ortho(float l, float r, float b, float t, float n, float f)
    {
      m_interface->kodi_gui->gl_shader_matrix->matrix_ortho(m_interface->kodiBase, m_matrix, l, r, b, t, n, f);
    }

    void Ortho2D(float l, float r, float b, float t)
    {
      m_interface->kodi_gui->gl_shader_matrix->matrix_ortho2d(m_interface->kodiBase, m_matrix, l, r, b, t);
    }

    void Frustum(float l, float r, float b, float t, float n, float f)
    {
      m_interface->kodi_gui->gl_shader_matrix->matrix_frustum(m_interface->kodiBase, m_matrix, l, r, b, t, n, f);
    }

    void Translatef(float x, float y, float z)
    {
      m_interface->kodi_gui->gl_shader_matrix->matrix_translatef(m_interface->kodiBase, m_matrix, x, y, z);
    }

    void Scalef(float x, float y, float z)
    {
      m_interface->kodi_gui->gl_shader_matrix->matrix_scalef(m_interface->kodiBase, m_matrix, x, y, z);
    }

    void Rotatef(float angle, float x, float y, float z)
    {
      m_interface->kodi_gui->gl_shader_matrix->matrix_rotatef(m_interface->kodiBase, m_matrix, angle, x, y, z);
    }

    void MultMatrixf(const CMatrix &matrix)
    {
      m_interface->kodi_gui->gl_shader_matrix->matrix_mult_matrixf(m_interface->kodiBase, m_matrix, matrix.m_matrix);
    }

    void LookAt(float eyex, float eyey, float eyez, float centerx, float centery, float centerz, float upx, float upy, float upz)
    {
      m_interface->kodi_gui->gl_shader_matrix->matrix_look_at(m_interface->kodiBase, m_matrix, eyex, eyey, eyez, centerx, centery, centerz, upx, upy, upz);
    }

    inline static bool Project(float objx, float objy, float objz, const float modelMatrix[16],
                               const float projMatrix[16], const int viewport[4], float* winx, float* winy, float* winz)
    {
      using namespace ::kodi::addon;
      return CAddonBase::m_interface->toKodi->kodi_gui->gl_shader_matrix->matrix_project(CAddonBase::m_interface->toKodi->kodiBase,
                                                                                         objx, objy, objz, modelMatrix, projMatrix,
                                                                                         viewport, winx, winy, winz);
    }

  private:
    friend class CMatrixStack;

    /*
     * Constructor used from CMatrixStack to override construction with stacked
     * matrix from Kodi.
     */
    CMatrix(void* matrix) : m_matrix(matrix) { }

    void* m_matrix;
    AddonToKodiFuncTable_Addon* m_interface;
  };


  class ATTRIBUTE_HIDDEN CMatrixStack
  {
  public:
    static CMatrixStack& GetGLMatrixModview()
    {
      static CMatrixStack glMatrixModview(AddonToKodiFuncTable_kodi_gui_gl_matrix::MATRIX_STACK_TYPE_MODVIEW);
      return glMatrixModview;
    }

    static CMatrixStack& GetGLMatrixProject()
    {
      static CMatrixStack glMatrixProject(AddonToKodiFuncTable_kodi_gui_gl_matrix::MATRIX_STACK_TYPE_PROJECT);
      return glMatrixProject;
    }

    static CMatrixStack& GetGLMatrixTexture()
    {
      static CMatrixStack glMatrixTexture(AddonToKodiFuncTable_kodi_gui_gl_matrix::MATRIX_STACK_TYPE_TEXTURE);
      return glMatrixTexture;
    }

    void Push()
    {
      if (m_type < 0)
        m_stack.push(m_current);
      else
        m_interface->kodi_gui->gl_shader_matrix->matrix_stack_push(m_interface->kodiBase, m_type);
    }

    void Clear()
    {
      if (m_type < 0)
        m_stack = std::stack<CMatrix>();
      else
        m_interface->kodi_gui->gl_shader_matrix->matrix_stack_clear(m_interface->kodiBase, m_type);
    }

    void Pop()
    {
      if (m_type < 0)
      {
        if(!m_stack.empty())
        {
          m_current = m_stack.top();
          m_stack.pop();
        }
      }
      else
        m_interface->kodi_gui->gl_shader_matrix->matrix_stack_pop(m_interface->kodiBase, m_type);
    }

    void Load()
    {
      if (m_type >= 0)
        m_interface->kodi_gui->gl_shader_matrix->matrix_stack_load(m_interface->kodiBase, m_type);
    }

    void PopLoad()
    {
      if (m_type < 0)
      {
        Pop(); Load();
      }
      else
        m_interface->kodi_gui->gl_shader_matrix->matrix_stack_pop_load(m_interface->kodiBase, m_type);
    }

    CMatrix& Get()
    {
      if (m_type >= 0)
        m_current.m_matrix = m_interface->kodi_gui->gl_shader_matrix->matrix_stack_get_current(m_interface->kodiBase, m_type);

      return m_current;
    }

    CMatrix* operator->()
    {
      if (m_type >= 0)
        m_current.m_matrix = m_interface->kodi_gui->gl_shader_matrix->matrix_stack_get_current(m_interface->kodiBase, m_type);

      return &m_current;
    }

  private:
    CMatrixStack(int type)
      : m_type(type),
        m_interface(::kodi::addon::CAddonBase::m_interface->toKodi),
        m_current(m_interface->kodi_gui->gl_shader_matrix->matrix_stack_get_current(m_interface->kodiBase, m_type))
    {

    }

    int m_type = -1;
    AddonToKodiFuncTable_Addon* m_interface;
    std::stack<CMatrix> m_stack;
    CMatrix m_current;
  };

} /* namespace gl */
} /* namespace gui */
} /* namespace kodi */
