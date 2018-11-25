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
   * Related add-on header is "./xbmc/addons/kodi-addon-dev-kit/include/kodi/gui/shaders/Shader.h"
   */
  struct Interface_GUIShaderGL
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
    static void present_enable(void* kodiBase, int method);
    static void present_disable(void* kodiBase);
    static int present_get_pos(void* kodiBase);
    static int present_get_col(void* kodiBase);
    static int present_get_coord0(void* kodiBase);
    static int present_get_coord1(void* kodiBase);
    static int present_get_uni_col(void* kodiBase);
    static int present_get_model(void* kodiBase);

    static void* shader_create(void* kodiBase, int type);
    static void shader_destroy(void* kodiBase, void* handle);
    static bool shader_compile(void* kodiBase, void* handle);
    static void shader_free(void* kodiBase, void* handle);
    static unsigned int shader_handle(void* kodiBase, void* handle);
    static void shader_set_source(void* kodiBase, void* handle, const char* src);
    static bool shader_load_source(void* kodiBase, void* handle, const char* filename, const char* prefix, const char* base_path);
    static bool shader_append_source(void* kodiBase, void* handle, const char* filename, const char* base_path);
    static bool shader_insert_source(void* kodiBase, void* handle, const char* filename, const char* loc, const char* base_path);
    static bool shader_ok(void* kodiBase, void* handle);

    static void* shader_program_create(void* kodiBase, void* clienthandle,
                                       void (*OnCompiledAndLinked)(void*),
                                       bool (*OnEnabled)(void*),
                                       void (*OnDisabled)(void*),
                                       const char* vert, const char* frag,
                                       const char* base_path);
    static void shader_program_destroy(void* kodiBase, void* handle);
    static bool shader_program_enable(void* kodiBase, void* handle);
    static void shader_program_disable(void* kodiBase, void* handle);
    static bool shader_program_ok(void* kodiBase, void* handle);
    static void* shader_program_vertex_shader(void* kodiBase, void* handle);
    static void* shader_program_pixel_shader(void* kodiBase, void* handle);
    static bool shader_program_compile_and_link(void* kodiBase, void* handle);
    static unsigned int shader_program_program_handle(void* kodiBase, void* handle);
    static void shader_program_free(void* kodiBase, void* handle);
    //@}
  };

} /* namespace ADDON */
} /* extern "C" */
