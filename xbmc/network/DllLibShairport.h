#pragma once

/*
 *      Copyright (C) 2011-2012 Team XBMC
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

#include <shairport/shairport.h>

class DllLibShairportInterface
{
public:
  virtual ~DllLibShairportInterface() {}

  virtual int   shairport_main(int argc, char **argv    )=0;
  virtual void  shairport_exit(void                     )=0;
  virtual int   shairport_loop(void                     )=0;
  virtual int   shairport_is_running(void               )=0;
  virtual void  shairport_set_ao(struct AudioOutput *ao        )=0;
  virtual void  shairport_set_printf(struct printfPtr *funcPtr)=0;
};

class DllLibShairport : public DllDynamic, DllLibShairportInterface
{
  DECLARE_DLL_WRAPPER(DllLibShairport, DLL_PATH_LIBSHAIRPORT)
  DEFINE_METHOD0(void,  shairport_exit)
  DEFINE_METHOD0(int,   shairport_loop)
  DEFINE_METHOD0(int,   shairport_is_running)
  DEFINE_METHOD1(void,  shairport_set_ao, (struct AudioOutput *p1))
  DEFINE_METHOD1(void,  shairport_set_printf, (struct printfPtr *p1))  
  DEFINE_METHOD2(int,   shairport_main, (int p1, char **p2))


  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(shairport_exit,         shairport_exit)    
    RESOLVE_METHOD_RENAME(shairport_loop,         shairport_loop)
    RESOLVE_METHOD_RENAME(shairport_is_running,   shairport_is_running)
    RESOLVE_METHOD_RENAME(shairport_main,         shairport_main)
    RESOLVE_METHOD_RENAME(shairport_set_ao,       shairport_set_ao)
    RESOLVE_METHOD_RENAME(shairport_set_printf,   shairport_set_printf)
  END_METHOD_RESOLVE()
};
