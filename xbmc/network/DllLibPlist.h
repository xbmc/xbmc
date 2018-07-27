/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DynamicDll.h"

#include <plist/plist.h>

class DllLibPlistInterface
{
public:
  virtual ~DllLibPlistInterface() = default;

  virtual void        plist_from_bin        (const char *plist_bin,   uint32_t length, plist_t * plist  )=0;
  virtual plist_t     plist_new_dict        (void                                                       )=0;
  virtual uint32_t    plist_dict_get_size   (plist_t node                                               )=0;
  virtual void        plist_get_string_val  (plist_t node,            char **val                        )=0;
  virtual void        plist_get_real_val    (plist_t node,            double *val                       )=0;
  virtual plist_t     plist_dict_get_item   (plist_t node,            const char* key                   )=0;
  virtual void        plist_free            (plist_t plist                                              )=0;
  virtual void        plist_to_xml          (plist_t plist,           char **plist_xml, uint32_t * length)=0;
  virtual void        plist_dict_new_iter   (plist_t node,            plist_dict_iter *iter             )=0;
  virtual void        plist_dict_next_item  (plist_t node,            plist_dict_iter iter, char **key, plist_t *val) = 0;

};

class DllLibPlist : public DllDynamic, DllLibPlistInterface
{
  DECLARE_DLL_WRAPPER(DllLibPlist, DLL_PATH_LIBPLIST)
  DEFINE_METHOD0(plist_t,       plist_new_dict)
  DEFINE_METHOD1(uint32_t,      plist_dict_get_size,  (plist_t p1))
  DEFINE_METHOD1(void,          plist_free,           (plist_t p1))
  DEFINE_METHOD2(void,          plist_get_string_val, (plist_t p1,      char **p2))
  DEFINE_METHOD2(void,          plist_get_real_val,   (plist_t p1,      double *p2))
  DEFINE_METHOD2(void,          plist_dict_new_iter,  (plist_t p1,      plist_dict_iter* p2))
  DEFINE_METHOD2(plist_t,       plist_dict_get_item,  (plist_t p1,      const char* p2))
  DEFINE_METHOD3(void,          plist_from_bin,       (const char *p1,  uint32_t p2, plist_t *p3))
  DEFINE_METHOD3(void,          plist_to_xml,         (plist_t p1,      char **p2, uint32_t *p3));
  DEFINE_METHOD4(void,          plist_dict_next_item, (plist_t p1, plist_dict_iter p2, char **p3, plist_t *p4))


  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(plist_new_dict,         plist_new_dict)
    RESOLVE_METHOD_RENAME(plist_free,             plist_free)
    RESOLVE_METHOD_RENAME(plist_dict_get_size,    plist_dict_get_size)
    RESOLVE_METHOD_RENAME(plist_from_bin,         plist_from_bin)
    RESOLVE_METHOD_RENAME(plist_get_real_val,     plist_get_real_val)
    RESOLVE_METHOD_RENAME(plist_get_string_val,   plist_get_string_val)
    RESOLVE_METHOD_RENAME(plist_dict_get_item,    plist_dict_get_item)
    RESOLVE_METHOD_RENAME(plist_dict_new_iter,    plist_dict_new_iter)
    RESOLVE_METHOD_RENAME(plist_dict_next_item,   plist_dict_next_item)
    RESOLVE_METHOD_RENAME(plist_to_xml,           plist_to_xml)

  END_METHOD_RESOLVE()
};

