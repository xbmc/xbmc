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

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif
#include <xkbcommon/xkbcommon.h>
#include "utils/log.h"
#include "DynamicDll.h"

class IDllXKBCommon
{
public:
  virtual ~IDllXKBCommon() {}
  
  virtual struct xkb_context * xkb_context_new(enum xkb_context_flags) = 0;
  virtual void xkb_context_unref(struct xkb_context *) = 0;
  virtual struct xkb_keymap * xkb_keymap_new_from_string(struct xkb_context *,
                                                         const char *,
                                                         enum xkb_keymap_format,
                                                         enum xkb_keymap_compile_flags) = 0;
  virtual struct xkb_keymap * xkb_keymap_new_from_names(struct xkb_context *,
                                                        const struct xkb_rule_names *,
                                                        enum xkb_keymap_compile_flags) = 0;
  virtual xkb_mod_index_t xkb_keymap_mod_get_index(struct xkb_keymap *, 
                                                   const char *) = 0;
  virtual void xkb_keymap_unref(struct xkb_keymap *) = 0;
  virtual struct xkb_state * xkb_state_new(struct xkb_keymap *) = 0;
  virtual xkb_mod_mask_t xkb_state_serialize_mods(struct xkb_state *,
                                                  enum xkb_state_component) = 0;
  virtual enum xkb_state_component xkb_state_update_mask (struct xkb_state *,
                                                          xkb_mod_mask_t,
                                                          xkb_mod_mask_t,
                                                          xkb_mod_mask_t,
                                                          xkb_layout_index_t,
                                                          xkb_layout_index_t,
                                                          xkb_layout_index_t) = 0;
  virtual uint32_t xkb_state_key_get_syms(struct xkb_state *,
                                          uint32_t,
                                          const xkb_keysym_t **) = 0;
  virtual void xkb_state_unref(struct xkb_state *) = 0;
};

class DllXKBCommon : public DllDynamic, public IDllXKBCommon
{
  DECLARE_DLL_WRAPPER(DllXKBCommon, DLL_PATH_XKBCOMMON)
  
  DEFINE_METHOD1(struct xkb_context *, xkb_context_new, (enum xkb_context_flags p1));
  DEFINE_METHOD1(void, xkb_context_unref, (struct xkb_context *p1));
  DEFINE_METHOD4(struct xkb_keymap *, xkb_keymap_new_from_string, (struct xkb_context *p1, const char *p2, enum xkb_keymap_format p3, enum xkb_keymap_compile_flags p4));
  DEFINE_METHOD3(struct xkb_keymap *, xkb_keymap_new_from_names, (struct xkb_context *p1, const struct xkb_rule_names *p2, enum xkb_keymap_compile_flags p3));
  DEFINE_METHOD2(xkb_mod_index_t, xkb_keymap_mod_get_index, (struct xkb_keymap *p1, const char *p2));
  DEFINE_METHOD1(void, xkb_keymap_unref, (struct xkb_keymap *p1));
  DEFINE_METHOD1(struct xkb_state *, xkb_state_new, (struct xkb_keymap *p1));
  DEFINE_METHOD2(xkb_mod_mask_t, xkb_state_serialize_mods, (struct xkb_state *p1, enum xkb_state_component p2));
  DEFINE_METHOD7(enum xkb_state_component, xkb_state_update_mask, (struct xkb_state *p1, xkb_mod_mask_t p2, xkb_mod_mask_t p3, xkb_mod_mask_t p4, xkb_layout_index_t p5, xkb_layout_index_t p6, xkb_layout_index_t p7));
  DEFINE_METHOD3(uint32_t, xkb_state_key_get_syms, (struct xkb_state *p1, uint32_t p2, const xkb_keysym_t **p3));
  DEFINE_METHOD1(void, xkb_state_unref, (struct xkb_state *p1));
  
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(xkb_context_new)
    RESOLVE_METHOD(xkb_context_unref)
    RESOLVE_METHOD(xkb_keymap_new_from_string)
    RESOLVE_METHOD(xkb_keymap_new_from_names)
    RESOLVE_METHOD(xkb_keymap_mod_get_index)
    RESOLVE_METHOD(xkb_keymap_unref)
    RESOLVE_METHOD(xkb_state_new)
    RESOLVE_METHOD(xkb_state_serialize_mods)
    RESOLVE_METHOD(xkb_state_update_mask)
    RESOLVE_METHOD(xkb_state_key_get_syms)
    RESOLVE_METHOD(xkb_state_unref)
  END_METHOD_RESOLVE()
};
