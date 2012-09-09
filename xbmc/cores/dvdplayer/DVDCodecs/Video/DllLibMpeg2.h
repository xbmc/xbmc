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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
extern "C" {
#if defined(WIN32)
  #include "libmpeg2/mpeg2.h"
  #include "libmpeg2/mpeg2convert.h"
#else
  #include <mpeg2dec/mpeg2.h>
  #include <mpeg2dec/mpeg2convert.h>
#endif
}
#include "DynamicDll.h"
#include "utils/log.h"

class DllLibMpeg2Interface
{
public:
  virtual ~DllLibMpeg2Interface() {}
  virtual uint32_t mpeg2_accel (uint32_t accel)=0;
  virtual mpeg2dec_t * mpeg2_init (void)=0;
  virtual const mpeg2_info_t * mpeg2_info (mpeg2dec_t * mpeg2dec)=0;
  virtual void mpeg2_close (mpeg2dec_t * mpeg2dec)=0;
  virtual void mpeg2_buffer (mpeg2dec_t * mpeg2dec, uint8_t * start, uint8_t * end)=0;
  virtual void mpeg2_tag_picture (mpeg2dec_t * mpeg2dec, uint32_t tag, uint32_t tag2)=0;
  virtual mpeg2_state_t mpeg2_parse (mpeg2dec_t * mpeg2dec)=0;
  virtual void mpeg2_reset (mpeg2dec_t * mpeg2dec, int full_reset)=0;
  virtual void mpeg2_set_buf (mpeg2dec_t * mpeg2dec, uint8_t * buf[3], void * id)=0;
  virtual void mpeg2_custom_fbuf (mpeg2dec_t * mpeg2dec, int custom_fbuf)=0;
  virtual int mpeg2_convert (mpeg2dec_t * mpeg2dec, mpeg2_convert_t convert, void * arg)=0;
  virtual void mpeg2_skip(mpeg2dec_t * mpeg2dec, int skip)=0;
};

class DllLibMpeg2 : public DllDynamic, DllLibMpeg2Interface
{
  DECLARE_DLL_WRAPPER(DllLibMpeg2, DLL_PATH_LIBMPEG2)
  DEFINE_METHOD1(uint32_t, mpeg2_accel, (uint32_t p1))
  DEFINE_METHOD0(mpeg2dec_t *, mpeg2_init)
  DEFINE_METHOD1(const mpeg2_info_t *, mpeg2_info, (mpeg2dec_t * p1))
  DEFINE_METHOD1(void, mpeg2_close, (mpeg2dec_t * p1))
  DEFINE_METHOD3(void, mpeg2_buffer, (mpeg2dec_t * p1, uint8_t * p2, uint8_t * p3))
  DEFINE_METHOD3(void, mpeg2_tag_picture, (mpeg2dec_t * p1, uint32_t p2, uint32_t p3))
  DEFINE_METHOD1(mpeg2_state_t, mpeg2_parse, (mpeg2dec_t * p1))
  DEFINE_METHOD2(void, mpeg2_reset, (mpeg2dec_t * p1, int p2))
  DEFINE_METHOD3(void, mpeg2_set_buf, (mpeg2dec_t * p1, uint8_t * p2[3], void * p3))
  DEFINE_METHOD2(void, mpeg2_custom_fbuf, (mpeg2dec_t * p1, int p2))
  DEFINE_METHOD3(int, mpeg2_convert, (mpeg2dec_t * p1, mpeg2_convert_t p2, void * p3))
  DEFINE_METHOD2(void,mpeg2_skip, (mpeg2dec_t * p1, int p2))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(mpeg2_accel)
    RESOLVE_METHOD(mpeg2_init)
    RESOLVE_METHOD(mpeg2_info)
    RESOLVE_METHOD(mpeg2_close)
    RESOLVE_METHOD(mpeg2_buffer)
    RESOLVE_METHOD(mpeg2_tag_picture)
    RESOLVE_METHOD(mpeg2_parse)
    RESOLVE_METHOD(mpeg2_reset)
    RESOLVE_METHOD(mpeg2_set_buf)
    RESOLVE_METHOD(mpeg2_custom_fbuf)
    RESOLVE_METHOD(mpeg2_convert)
    RESOLVE_METHOD(mpeg2_skip)
  END_METHOD_RESOLVE()
};
