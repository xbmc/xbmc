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
/* undefine byte from PlatformDefs.h since it's used in mad.h */
#undef byte
#if defined(_LINUX) || defined(TARGET_DARWIN)
  #include <mad.h>
#else
  #include "libmad/mad.h"
#endif
#include "DynamicDll.h"

class DllLibMadInterface
{
public:
  virtual ~DllLibMadInterface() {}
  virtual void mad_synth_init(struct mad_synth *)=0;
  virtual void mad_stream_init(struct mad_stream *)=0;
  virtual void mad_frame_init(struct mad_frame *)=0;
  virtual void mad_stream_finish(struct mad_stream *)=0;
  virtual void mad_frame_finish(struct mad_frame *)=0;
  virtual void mad_stream_buffer(struct mad_stream *, unsigned char const *, unsigned long)=0;
  virtual void mad_synth_frame(struct mad_synth *, struct mad_frame const *)=0;
  virtual int mad_frame_decode(struct mad_frame *, struct mad_stream *)=0;
  virtual int mad_stream_sync(struct mad_stream *) = 0;
  virtual char const* mad_stream_errorstr(struct mad_stream const *) = 0;
  virtual void mad_frame_mute(struct mad_frame *) = 0;
  virtual void mad_synth_mute(struct mad_synth *) = 0;
  virtual void mad_timer_add(mad_timer_t *, mad_timer_t) = 0;
  virtual mad_timer_t Get_mad_timer_zero() = 0;
};

class DllLibMad : public DllDynamic, DllLibMadInterface
{
  DECLARE_DLL_WRAPPER(DllLibMad, DLL_PATH_LIBMAD)
  DEFINE_METHOD1(void, mad_synth_init, (struct mad_synth * p1))
  DEFINE_METHOD1(void, mad_stream_init, (struct mad_stream * p1))
  DEFINE_METHOD1(void, mad_frame_init, (struct mad_frame * p1))
  DEFINE_METHOD1(void, mad_stream_finish, (struct mad_stream * p1))
  DEFINE_METHOD1(void, mad_frame_finish, (struct mad_frame * p1))
  DEFINE_METHOD3(void, mad_stream_buffer, (struct mad_stream * p1, unsigned char const *p2, unsigned long p3))
  DEFINE_METHOD2(void, mad_synth_frame, (struct mad_synth *p1, struct mad_frame const *p2))
  DEFINE_METHOD2(int, mad_frame_decode, (struct mad_frame *p1, struct mad_stream *p2))
  DEFINE_METHOD1(int, mad_stream_sync, (struct mad_stream *p1))
  DEFINE_METHOD1(void, mad_frame_mute, (struct mad_frame *p1))
  DEFINE_METHOD1(void, mad_synth_mute, (struct mad_synth *p1))
  DEFINE_METHOD2(void, mad_timer_add, (mad_timer_t *p1, mad_timer_t p2))
  DEFINE_METHOD1(char const*, mad_stream_errorstr, (struct mad_stream const *p1))
  DEFINE_GLOBAL(mad_timer_t, mad_timer_zero)
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(mad_synth_init)
    RESOLVE_METHOD(mad_stream_init)
    RESOLVE_METHOD(mad_frame_init)
    RESOLVE_METHOD(mad_stream_finish)
    RESOLVE_METHOD(mad_frame_finish)
    RESOLVE_METHOD(mad_stream_buffer)
    RESOLVE_METHOD(mad_synth_frame)
    RESOLVE_METHOD(mad_frame_decode)
    RESOLVE_METHOD(mad_stream_sync)
    RESOLVE_METHOD(mad_frame_mute)
    RESOLVE_METHOD(mad_synth_mute)
    RESOLVE_METHOD(mad_timer_add)
    RESOLVE_METHOD(mad_stream_errorstr)
    RESOLVE_METHOD(mad_timer_zero)
  END_METHOD_RESOLVE()
};

