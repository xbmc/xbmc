/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

extern "C"
{

struct AddonGlobalInterface;

namespace ADDON
{

  /*!
   * @brief OpenGL gui shader Add-on to Kodi callback functions
   *
   * To hold functions not related to a instance type and usable for
   * every add-on type.
   *
   * Related add-on header is "./xbmc/addons/kodi-addon-dev-kit/include/kodi/gui/shaders/GLMatrix.h"
   */
  struct Interface_GUIMatrixGL
  {
    static void Init(AddonGlobalInterface* addonInterface);
    static void DeInit(AddonGlobalInterface* addonInterface);

    /*!
     * @brief callback functions from add-on to kodi
     *
     * @note To add a new function use the "_" style to directly identify an
     * add-on callback function. Everything with CamelCase is only to be used
     * in Kodi.
     *
     * The parameter `kodiBase` is used to become the pointer for a `CAddonDll`
     * class.
     */
    //@{
    static void* matrix_create(void* kodiBase);
    static void* matrix_create_a(void* kodiBase,
                                 float x0, float x1, float x2, float x3,
                                 float x4, float x5, float x6, float x7,
                                 float x8, float x9, float x10, float x11,
                                 float x12, float x13, float x14, float x15);
    static void matrix_destroy(void* kodiBase, void* handle);
    static const float* matrix_m_pMatrix(void* kodiBase, void* handle);
    static void matrix_load_identity(void* kodiBase, void* handle);
    static void matrix_ortho(void* kodiBase, void* handle, float l, float r, float b, float t, float n, float f);
    static void matrix_ortho2d(void* kodiBase, void* handle, float l, float r, float b, float t);
    static void matrix_frustum(void* kodiBase, void* handle, float l, float r, float b, float t, float n, float f);
    static void matrix_translatef(void* kodiBase, void* handle, float x, float y, float z);
    static void matrix_scalef(void* kodiBase, void* handle, float x, float y, float z);
    static void matrix_rotatef(void* kodiBase, void* handle, float angle, float x, float y, float z);
    static void matrix_mult_matrixf(void* kodiBase, void* handle, const void* matrix);
    static void matrix_look_at(void* kodiBase, void* handle, float eyex, float eyey, float eyez, float centerx, float centery,
                               float centerz, float upx, float upy, float upz);
    static bool matrix_project(void* kodiBase, float objx, float objy, float objz, const float* modelMatrix,
                               const float* projMatrix, const int* viewport, float* winx, float* winy, float* winz);

    static void matrix_stack_push(void* kodiBase, int matrix_type);
    static void matrix_stack_clear(void* kodiBase, int matrix_type);
    static void matrix_stack_pop(void* kodiBase, int matrix_type);
    static void matrix_stack_load(void* kodiBase, int matrix_type);
    static void matrix_stack_pop_load(void* kodiBase, int matrix_type);
    static void* matrix_stack_get_current(void* kodiBase, int matrix_type);
    //@}
  };

} /* namespace ADDON */
} /* extern "C" */
