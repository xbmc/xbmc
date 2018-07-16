/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DynamicDll.h"

#include <shairplay/raop.h>

struct raop_s;

class DllLibShairplayInterface
{
public:
  virtual ~DllLibShairplayInterface() = default;


  virtual raop_t *    raop_init(int max_clients, raop_callbacks_t *callbacks, const char *pemkey)=0;
  virtual raop_t *    raop_init_from_keyfile(int max_clients, raop_callbacks_t *callbacks, const char *keyfile)=0;
  virtual void        raop_set_log_level(raop_t *raop, int level)=0;
  virtual void        raop_set_log_callback(raop_t *raop, raop_log_callback_t callback, void *cls)=0;
  virtual int         raop_is_running(raop_t *raop)=0;
  virtual int         raop_start(raop_t *raop, unsigned short *port, const char *hwaddr, int hwaddrlen, const char *password)=0;
  virtual void        raop_stop(raop_t *raop)=0;
  virtual void        raop_destroy(raop_t *raop)=0;
};

class DllLibShairplay : public DllDynamic, DllLibShairplayInterface
{
  DECLARE_DLL_WRAPPER(DllLibShairplay, DLL_PATH_LIBSHAIRPLAY)
  DEFINE_METHOD3(raop_t *,  raop_init,              (int p1, raop_callbacks_t *p2, const char *p3))
  DEFINE_METHOD3(raop_t *,  raop_init_from_keyfile, (int p1, raop_callbacks_t *p2, const char *p3))
  DEFINE_METHOD2(void,      raop_set_log_level,     (raop_t *p1, int p2))
  DEFINE_METHOD3(void,      raop_set_log_callback,  (raop_t *p1, raop_log_callback_t p2, void *p3))
  DEFINE_METHOD1(int,       raop_is_running,        (raop_t *p1))
  DEFINE_METHOD5(int,       raop_start,             (raop_t *p1, unsigned short *p2, const char *p3, int p4, const char *p5))
  DEFINE_METHOD1(void,      raop_stop,              (raop_t *p1))
  DEFINE_METHOD1(void,      raop_destroy,           (raop_t *p1))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(raop_init,              raop_init)
    RESOLVE_METHOD_RENAME(raop_init_from_keyfile, raop_init_from_keyfile)
    RESOLVE_METHOD_RENAME(raop_set_log_level,     raop_set_log_level)
    RESOLVE_METHOD_RENAME(raop_set_log_callback,  raop_set_log_callback)
    RESOLVE_METHOD_RENAME(raop_is_running,        raop_is_running)
    RESOLVE_METHOD_RENAME(raop_start,             raop_start)
    RESOLVE_METHOD_RENAME(raop_stop,              raop_stop)
    RESOLVE_METHOD_RENAME(raop_destroy,           raop_destroy)
  END_METHOD_RESOLVE()
};

