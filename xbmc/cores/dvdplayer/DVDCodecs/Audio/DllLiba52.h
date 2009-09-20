#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
extern "C" {
#if (defined USE_EXTERNAL_LIBA52)
  #include <a52dec/a52.h>
  #include <a52dec/mm_accel.h>
#else
  #include "liba52/a52.h"
#endif
}
#include "DynamicDll.h"
#include "utils/log.h"
#ifndef _LINUX
typedef unsigned __int32 uint32_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
#endif

#if (defined USE_EXTERNAL_LIBA52)

class DllLiba52Interface
{
public:
  virtual ~DllLiba52Interface() {}
  virtual a52_state_t * a52_init (uint32_t mm_accel)=0;
  virtual sample_t * a52_samples (a52_state_t * state)=0;
  virtual int a52_syncinfo (uint8_t * buf, int * flags,
          int * sample_rate, int * bit_rate)=0;
  virtual int a52_frame (a52_state_t * state, uint8_t * buf, int * flags,
           sample_t * level, sample_t bias)=0;
  virtual void a52_dynrng (a52_state_t * state,
         sample_t (* call) (sample_t, void *), void * data)=0;
  virtual int a52_block (a52_state_t * state)=0;
  virtual void a52_free (a52_state_t * state)=0;
};

class DllLiba52 : public DllDynamic, DllLiba52Interface
{
public:
    virtual ~DllLiba52() {}
    virtual a52_state_t * a52_init (uint32_t mm_accel)
        { return ::a52_init (mm_accel); }
    virtual sample_t * a52_samples (a52_state_t * state)
        { return ::a52_samples (state); }
    virtual int a52_syncinfo (uint8_t * buf, int * flags,
            int * sample_rate, int * bit_rate)
        { return ::a52_syncinfo (buf, flags, sample_rate, bit_rate); }
    virtual int a52_frame (a52_state_t * state, uint8_t * buf, int * flags,
            sample_t * level, sample_t bias)
        { return ::a52_frame (state, buf, flags, level, bias); }
    virtual void a52_dynrng (a52_state_t * state,
            sample_t (* call) (sample_t, void *), void * data)
        { return ::a52_dynrng (state, call, data); }
    virtual int a52_block (a52_state_t * state)
        { return ::a52_block (state); }
    virtual void a52_free (a52_state_t * state)
        { return ::a52_free (state); }

    // DLL faking.
    virtual bool ResolveExports() { return true; }
    virtual bool Load() {
        CLog::Log(LOGDEBUG, "DllLiba52: Using liba52 system library");
        return true;
    }
    virtual void Unload() {}
};

#else

class DllLiba52Interface
{
public:
  virtual ~DllLiba52Interface() {}
  virtual a52_state_t * a52_init (uint32_t mm_accel)=0;
  virtual sample_t * a52_samples (a52_state_t * state)=0;
  virtual int a52_syncinfo (uint8_t * buf, int * flags, int * sample_rate, int * bit_rate)=0;
  virtual int a52_frame (a52_state_t * state, uint8_t * buf, int * flags, level_t * level, sample_t bias)=0;
  virtual void a52_dynrng (a52_state_t * state, level_t (* call) (level_t, void *), void * data)=0;
  virtual int a52_block (a52_state_t * state)=0;
  virtual void a52_free (a52_state_t * state)=0;
};

class DllLiba52 : public DllDynamic, DllLiba52Interface
{
  DECLARE_DLL_WRAPPER(DllLiba52, DLL_PATH_LIBA52)
  DEFINE_METHOD1(a52_state_t *, a52_init, (uint32_t p1))
  DEFINE_METHOD1(sample_t *, a52_samples, (a52_state_t *p1))
  DEFINE_METHOD4(int, a52_syncinfo, (uint8_t * p1, int * p2, int * p3, int * p4))
  DEFINE_METHOD5(int, a52_frame, (a52_state_t * p1, uint8_t * p2, int * p3, level_t * p4, sample_t p5))
  DEFINE_METHOD3(void, a52_dynrng, (a52_state_t * p1, level_t (* p2) (level_t, void *), void * p3))
  DEFINE_METHOD1(int, a52_block, (a52_state_t * p1))
  DEFINE_METHOD1(void, a52_free, (a52_state_t * p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(a52_init)
    RESOLVE_METHOD(a52_samples)
    RESOLVE_METHOD(a52_syncinfo)
    RESOLVE_METHOD(a52_frame)
    RESOLVE_METHOD(a52_dynrng)
    RESOLVE_METHOD(a52_block)
    RESOLVE_METHOD(a52_free)
  END_METHOD_RESOLVE()
};

#endif
