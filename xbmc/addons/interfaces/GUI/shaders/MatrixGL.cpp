/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MatrixGL.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/shaders/MatrixGL.h"

#include "addons/binary-addons/AddonDll.h"
#if defined(HAS_GL) || HAS_GLES >= 2
#include "rendering/MatrixGL.h"
#endif
#include "utils/log.h"
#include "ServiceBroker.h"

extern "C"
{
namespace ADDON
{

void Interface_GUIMatrixGL::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->gl_shader_matrix = static_cast<AddonToKodiFuncTable_kodi_gui_gl_matrix*>(malloc(sizeof(AddonToKodiFuncTable_kodi_gui_gl_matrix)));

  addonInterface->toKodi->kodi_gui->gl_shader_matrix->matrix_create = matrix_create;
  addonInterface->toKodi->kodi_gui->gl_shader_matrix->matrix_create_a = matrix_create_a;
  addonInterface->toKodi->kodi_gui->gl_shader_matrix->matrix_destroy = matrix_destroy;
  addonInterface->toKodi->kodi_gui->gl_shader_matrix->matrix_m_pMatrix = matrix_m_pMatrix;
  addonInterface->toKodi->kodi_gui->gl_shader_matrix->matrix_load_identity = matrix_load_identity;
  addonInterface->toKodi->kodi_gui->gl_shader_matrix->matrix_ortho = matrix_ortho;
  addonInterface->toKodi->kodi_gui->gl_shader_matrix->matrix_ortho2d = matrix_ortho2d;
  addonInterface->toKodi->kodi_gui->gl_shader_matrix->matrix_frustum = matrix_frustum;
  addonInterface->toKodi->kodi_gui->gl_shader_matrix->matrix_translatef = matrix_translatef;
  addonInterface->toKodi->kodi_gui->gl_shader_matrix->matrix_scalef = matrix_scalef;
  addonInterface->toKodi->kodi_gui->gl_shader_matrix->matrix_rotatef = matrix_rotatef;
  addonInterface->toKodi->kodi_gui->gl_shader_matrix->matrix_mult_matrixf = matrix_mult_matrixf;
  addonInterface->toKodi->kodi_gui->gl_shader_matrix->matrix_look_at = matrix_look_at;
  addonInterface->toKodi->kodi_gui->gl_shader_matrix->matrix_project = matrix_project;

  addonInterface->toKodi->kodi_gui->gl_shader_matrix->matrix_stack_push = matrix_stack_push;
  addonInterface->toKodi->kodi_gui->gl_shader_matrix->matrix_stack_clear = matrix_stack_clear;
  addonInterface->toKodi->kodi_gui->gl_shader_matrix->matrix_stack_pop = matrix_stack_pop;
  addonInterface->toKodi->kodi_gui->gl_shader_matrix->matrix_stack_load = matrix_stack_load;
  addonInterface->toKodi->kodi_gui->gl_shader_matrix->matrix_stack_pop_load = matrix_stack_pop_load;
  addonInterface->toKodi->kodi_gui->gl_shader_matrix->matrix_stack_get_current = matrix_stack_get_current;
}

void Interface_GUIMatrixGL::DeInit(AddonGlobalInterface* addonInterface)
{
  free(addonInterface->toKodi->kodi_gui->gl_shader_matrix);
}

void* Interface_GUIMatrixGL::matrix_create(void* kodiBase)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  return new CMatrixGL();
#else
  return nullptr;
#endif
}

void* Interface_GUIMatrixGL::matrix_create_a(void* kodiBase,
                                             float x0, float x1, float x2, float x3,
                                             float x4, float x5, float x6, float x7,
                                             float x8, float x9, float x10, float x11,
                                             float x12, float x13, float x14, float x15)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  return new CMatrixGL(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15);
#else
  return nullptr;
#endif
}

void Interface_GUIMatrixGL::matrix_destroy(void* kodiBase, void* handle)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  delete static_cast<CMatrixGL*>(handle);
#endif
}

const float* Interface_GUIMatrixGL::matrix_m_pMatrix(void* kodiBase, void* handle)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  return *static_cast<CMatrixGL*>(handle);
#else
  return nullptr;
#endif
}

void Interface_GUIMatrixGL::matrix_load_identity(void* kodiBase, void* handle)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  static_cast<CMatrixGL*>(handle)->LoadIdentity();
#endif
}

void Interface_GUIMatrixGL::matrix_ortho(void* kodiBase, void* handle, float l, float r, float b, float t, float n, float f)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  static_cast<CMatrixGL*>(handle)->Ortho(l, r, b, t, n, f);
#endif
}

void Interface_GUIMatrixGL::matrix_ortho2d(void* kodiBase, void* handle, float l, float r, float b, float t)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  static_cast<CMatrixGL*>(handle)->Ortho2D(l, r, b, t);
#endif
}

void Interface_GUIMatrixGL::matrix_frustum(void* kodiBase, void* handle, float l, float r, float b, float t, float n, float f)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  static_cast<CMatrixGL*>(handle)->Frustum(l, r, b, t, n, f);
#endif
}

void Interface_GUIMatrixGL::matrix_translatef(void* kodiBase, void* handle, float x, float y, float z)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  static_cast<CMatrixGL*>(handle)->Translatef(x, y, z);
#endif
}

void Interface_GUIMatrixGL::matrix_scalef(void* kodiBase, void* handle, float x, float y, float z)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  static_cast<CMatrixGL*>(handle)->Scalef(x, y, z);
#endif
}

void Interface_GUIMatrixGL::matrix_rotatef(void* kodiBase, void* handle, float angle, float x, float y, float z)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  static_cast<CMatrixGL*>(handle)->Rotatef(angle, x, y, z);
#endif
}

void Interface_GUIMatrixGL::matrix_mult_matrixf(void* kodiBase, void* handle, const void* matrix)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  static_cast<CMatrixGL*>(handle)->MultMatrixf(*static_cast<const CMatrixGL*>(matrix));
#endif
}

void Interface_GUIMatrixGL::matrix_look_at(void* kodiBase, void* handle, float eyex, float eyey, float eyez, float centerx, float centery,
                                           float centerz, float upx, float upy, float upz)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  static_cast<CMatrixGL*>(handle)->LookAt(eyex, eyey, eyez, centerx, centery, centerz, upx, upy, upz);
#endif
}

bool Interface_GUIMatrixGL::matrix_project(void* kodiBase, float objx, float objy, float objz, const float* modelMatrix,
                                           const float* projMatrix, const int* viewport, float* winx, float* winy, float* winz)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  return CMatrixGL::Project(objx, objy, objz, modelMatrix, projMatrix, viewport, winx, winy, winz);
#else
  return false;
#endif
}

//------------------------------------------------------------------------------

void Interface_GUIMatrixGL::matrix_stack_push(void* kodiBase, int matrix_type)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  switch (matrix_type)
  {
    case AddonToKodiFuncTable_kodi_gui_gl_matrix::MATRIX_STACK_TYPE_MODVIEW:
      glMatrixModview.Push();
      break;
    case AddonToKodiFuncTable_kodi_gui_gl_matrix::MATRIX_STACK_TYPE_PROJECT:
      glMatrixProject.Push();
      break;
    case AddonToKodiFuncTable_kodi_gui_gl_matrix::MATRIX_STACK_TYPE_TEXTURE:
      glMatrixTexture.Push();
      break;
  }
#endif
}

void Interface_GUIMatrixGL::matrix_stack_clear(void* kodiBase, int matrix_type)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  switch (matrix_type)
  {
    case AddonToKodiFuncTable_kodi_gui_gl_matrix::MATRIX_STACK_TYPE_MODVIEW:
      glMatrixModview.Clear();
      break;
    case AddonToKodiFuncTable_kodi_gui_gl_matrix::MATRIX_STACK_TYPE_PROJECT:
      glMatrixProject.Clear();
      break;
    case AddonToKodiFuncTable_kodi_gui_gl_matrix::MATRIX_STACK_TYPE_TEXTURE:
      glMatrixTexture.Clear();
      break;
  }
#endif
}

void Interface_GUIMatrixGL::matrix_stack_pop(void* kodiBase, int matrix_type)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  switch (matrix_type)
  {
    case AddonToKodiFuncTable_kodi_gui_gl_matrix::MATRIX_STACK_TYPE_MODVIEW:
      glMatrixModview.Pop();
      break;
    case AddonToKodiFuncTable_kodi_gui_gl_matrix::MATRIX_STACK_TYPE_PROJECT:
      glMatrixProject.Pop();
      break;
    case AddonToKodiFuncTable_kodi_gui_gl_matrix::MATRIX_STACK_TYPE_TEXTURE:
      glMatrixTexture.Pop();
      break;
  }
#endif
}

void Interface_GUIMatrixGL::matrix_stack_load(void* kodiBase, int matrix_type)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  switch (matrix_type)
  {
    case AddonToKodiFuncTable_kodi_gui_gl_matrix::MATRIX_STACK_TYPE_MODVIEW:
      glMatrixModview.Load();
      break;
    case AddonToKodiFuncTable_kodi_gui_gl_matrix::MATRIX_STACK_TYPE_PROJECT:
      glMatrixProject.Load();
      break;
    case AddonToKodiFuncTable_kodi_gui_gl_matrix::MATRIX_STACK_TYPE_TEXTURE:
      glMatrixTexture.Load();
      break;
  }
#endif
}

void Interface_GUIMatrixGL::matrix_stack_pop_load(void* kodiBase, int matrix_type)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  switch (matrix_type)
  {
    case AddonToKodiFuncTable_kodi_gui_gl_matrix::MATRIX_STACK_TYPE_MODVIEW:
      glMatrixModview.PopLoad();
      break;
    case AddonToKodiFuncTable_kodi_gui_gl_matrix::MATRIX_STACK_TYPE_PROJECT:
      glMatrixProject.PopLoad();
      break;
    case AddonToKodiFuncTable_kodi_gui_gl_matrix::MATRIX_STACK_TYPE_TEXTURE:
      glMatrixTexture.PopLoad();
      break;
  }
#endif
}

void* Interface_GUIMatrixGL::matrix_stack_get_current(void* kodiBase, int matrix_type)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  switch (matrix_type)
  {
    case AddonToKodiFuncTable_kodi_gui_gl_matrix::MATRIX_STACK_TYPE_MODVIEW:
      return &glMatrixModview.Get();
    case AddonToKodiFuncTable_kodi_gui_gl_matrix::MATRIX_STACK_TYPE_PROJECT:
      return &glMatrixProject.Get();
    case AddonToKodiFuncTable_kodi_gui_gl_matrix::MATRIX_STACK_TYPE_TEXTURE:
      return &glMatrixTexture.Get();
  }
#endif
  return nullptr;
}

} /* namespace ADDON */
} /* extern "C" */
