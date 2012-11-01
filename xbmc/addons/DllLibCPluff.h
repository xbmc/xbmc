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

#include "DynamicDll.h"

extern "C" {
#include "lib/cpluff/libcpluff/cpluff.h"
}

class DllLibCPluffInterface
{
public:
  virtual ~DllLibCPluffInterface() {}
  virtual const char *get_version(void) =0;
  virtual void set_fatal_error_handler(cp_fatal_error_func_t error_handler) =0;
  virtual cp_status_t init(void) =0;
  virtual void destroy(void) =0;
  virtual cp_context_t * create_context(cp_status_t *status) =0;
  virtual void destroy_context(cp_context_t *ctx) =0;
  virtual cp_status_t register_pcollection(cp_context_t *ctx, const char *dir) =0;
  virtual void unregister_pcollection(cp_context_t *ctx, const char *dir) =0;
  virtual void unregister_pcollections(cp_context_t *ctx) =0;
  virtual cp_status_t register_logger(cp_context_t *ctx, cp_logger_func_t logger, void *user_data, cp_log_severity_t min_severity) =0;
  virtual void unregister_logger(cp_context_t *ctx, cp_logger_func_t logger) =0;
  virtual cp_status_t scan_plugins(cp_context_t *ctx, int flags) =0;
  virtual cp_plugin_info_t * get_plugin_info(cp_context_t *ctx, const char *id, cp_status_t *status) =0;
  virtual cp_plugin_info_t ** get_plugins_info(cp_context_t *ctx, cp_status_t *status, int *num) =0;
  virtual cp_extension_t ** get_extensions_info(cp_context_t *ctx, const char *extpt_id, cp_status_t *status, int *num) =0;
  virtual void release_info(cp_context_t *ctx, void *info) =0;
  virtual cp_cfg_element_t * lookup_cfg_element(cp_cfg_element_t *base, const char *path) =0;
  virtual char * lookup_cfg_value(cp_cfg_element_t *base, const char *path) =0;
  virtual cp_status_t define_symbol(cp_context_t *ctx, const char *name, void *ptr) =0;
  virtual void *resolve_symbol(cp_context_t *ctx, const char *id, const char *name, cp_status_t *status) =0;
  virtual void release_symbol(cp_context_t *ctx, const void *ptr) =0;
  virtual cp_plugin_info_t *load_plugin_descriptor(cp_context_t *ctx, const char *path, cp_status_t *status) =0;
  virtual cp_plugin_info_t *load_plugin_descriptor_from_memory(cp_context_t *ctx, const char *buffer, unsigned int buffer_len, cp_status_t *status) =0;
  virtual cp_status_t uninstall_plugin(cp_context_t *ctx, const char *id)=0;
};

class DllLibCPluff : public DllDynamic, DllLibCPluffInterface
{
  DECLARE_DLL_WRAPPER(DllLibCPluff, DLL_PATH_CPLUFF)
  DEFINE_METHOD0(const char*,         get_version)
  DEFINE_METHOD1(void,                set_fatal_error_handler,  (cp_fatal_error_func_t p1))
  DEFINE_METHOD0(cp_status_t,         init)
  DEFINE_METHOD0(void,                destroy)
  DEFINE_METHOD1(cp_context_t*,       create_context,           (cp_status_t *p1))
  DEFINE_METHOD1(void,                destroy_context,          (cp_context_t *p1))

  DEFINE_METHOD2(cp_status_t,         register_pcollection,     (cp_context_t *p1, const char *p2))
  DEFINE_METHOD2(void,                unregister_pcollection,   (cp_context_t *p1, const char *p2))
  DEFINE_METHOD1(void,                unregister_pcollections,  (cp_context_t *p1))

  DEFINE_METHOD4(cp_status_t,         register_logger,          (cp_context_t *p1, cp_logger_func_t p2, void *p3, cp_log_severity_t p4))
  DEFINE_METHOD2(void,                unregister_logger,        (cp_context_t *p1, cp_logger_func_t p2))
  DEFINE_METHOD2(cp_status_t,         scan_plugins,             (cp_context_t *p1, int p2))
  DEFINE_METHOD3(cp_plugin_info_t*,   get_plugin_info,          (cp_context_t *p1, const char *p2, cp_status_t *p3))
  DEFINE_METHOD3(cp_plugin_info_t**,  get_plugins_info,         (cp_context_t *p1, cp_status_t *p2, int *p3))
  DEFINE_METHOD4(cp_extension_t**,    get_extensions_info,      (cp_context_t *p1, const char *p2, cp_status_t *p3, int *p4))
  DEFINE_METHOD2(void,                release_info,             (cp_context_t *p1, void *p2))

  DEFINE_METHOD2(cp_cfg_element_t*,   lookup_cfg_element,       (cp_cfg_element_t *p1, const char *p2))
  DEFINE_METHOD2(char*,               lookup_cfg_value,         (cp_cfg_element_t *p1, const char *p2))

  DEFINE_METHOD3(cp_status_t,         define_symbol,            (cp_context_t *p1, const char *p2, void *p3))
  DEFINE_METHOD4(void*,               resolve_symbol,           (cp_context_t *p1, const char *p2, const char *p3, cp_status_t *p4))
  DEFINE_METHOD2(void,                release_symbol,           (cp_context_t *p1, const void *p2))
  DEFINE_METHOD3(cp_plugin_info_t*,   load_plugin_descriptor,   (cp_context_t *p1, const char *p2, cp_status_t *p3))
  DEFINE_METHOD4(cp_plugin_info_t*,   load_plugin_descriptor_from_memory, (cp_context_t *p1, const char *p2, unsigned int p3, cp_status_t *p4))
  DEFINE_METHOD2(cp_status_t,         uninstall_plugin,         (cp_context_t *p1, const char *p2))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(cp_get_version, get_version)
    RESOLVE_METHOD_RENAME(cp_set_fatal_error_handler, set_fatal_error_handler)
    RESOLVE_METHOD_RENAME(cp_init, init)
    RESOLVE_METHOD_RENAME(cp_destroy, destroy)
    RESOLVE_METHOD_RENAME(cp_create_context, create_context)
    RESOLVE_METHOD_RENAME(cp_destroy_context, destroy_context)
    RESOLVE_METHOD_RENAME(cp_register_pcollection, register_pcollection)
    RESOLVE_METHOD_RENAME(cp_unregister_pcollection, unregister_pcollection)
    RESOLVE_METHOD_RENAME(cp_unregister_pcollections, unregister_pcollections)
    RESOLVE_METHOD_RENAME(cp_register_logger, register_logger)
    RESOLVE_METHOD_RENAME(cp_unregister_logger, unregister_logger)
    RESOLVE_METHOD_RENAME(cp_scan_plugins, scan_plugins)
    RESOLVE_METHOD_RENAME(cp_get_plugin_info, get_plugin_info)
    RESOLVE_METHOD_RENAME(cp_get_plugins_info, get_plugins_info)
    RESOLVE_METHOD_RENAME(cp_get_extensions_info, get_extensions_info)
    RESOLVE_METHOD_RENAME(cp_release_info, release_info)
    RESOLVE_METHOD_RENAME(cp_lookup_cfg_element, lookup_cfg_element)
    RESOLVE_METHOD_RENAME(cp_lookup_cfg_value, lookup_cfg_value)
    RESOLVE_METHOD_RENAME(cp_define_symbol, define_symbol)
    RESOLVE_METHOD_RENAME(cp_resolve_symbol, resolve_symbol)
    RESOLVE_METHOD_RENAME(cp_release_symbol, release_symbol)
    RESOLVE_METHOD_RENAME(cp_load_plugin_descriptor, load_plugin_descriptor)
    RESOLVE_METHOD_RENAME(cp_load_plugin_descriptor_from_memory, load_plugin_descriptor_from_memory)
    RESOLVE_METHOD_RENAME(cp_uninstall_plugin, uninstall_plugin)
  END_METHOD_RESOLVE()
};
