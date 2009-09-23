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
/* undefine byte from PlatformDefs.h since it's used in mad.h */
#undef byte
#if (defined USE_EXTERNAL_LIBMAD)
  #include <mad.h>
#else
  #include "libmad/mad.h"
#endif
#include "DynamicDll.h"
#include "utils/log.h"

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
};

#if (defined USE_EXTERNAL_LIBMAD)

class DllLibMad : public DllDynamic, DllLibMadInterface
{
public:
    virtual ~DllLibMad() {}
    virtual void mad_synth_init(struct mad_synth *synth)
        { return ::mad_synth_init(synth); }
    virtual void mad_stream_init(struct mad_stream *stream)
        { return ::mad_stream_init(stream); }
    virtual void mad_frame_init(struct mad_frame *frame)
        { return ::mad_frame_init(frame); }
    virtual void mad_stream_finish(struct mad_stream *stream)
        { return ::mad_stream_finish(stream); }
    virtual void mad_frame_finish(struct mad_frame *frame)
        { return ::mad_frame_finish(frame); }
    virtual void mad_stream_buffer(struct mad_stream *stream, unsigned char const *buffer, unsigned long size)
        { return ::mad_stream_buffer(stream, buffer, size); }
    virtual void mad_synth_frame(struct mad_synth *synth, struct mad_frame const *frame)
        { return ::mad_synth_frame(synth, frame); }
    virtual int mad_frame_decode(struct mad_frame *frame, struct mad_stream *stream)
        { return ::mad_frame_decode(frame, stream); }

    // DLL faking.
    virtual bool ResolveExports() { return true; }
    virtual bool Load() {
        CLog::Log(LOGDEBUG, "DllLibMad: Using libmad system library");
        return true;
    }
    virtual void Unload() {}
};

#else

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
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(mad_synth_init)
    RESOLVE_METHOD(mad_stream_init)
    RESOLVE_METHOD(mad_frame_init)
    RESOLVE_METHOD(mad_stream_finish)
    RESOLVE_METHOD(mad_frame_finish)
    RESOLVE_METHOD(mad_stream_buffer)
    RESOLVE_METHOD(mad_synth_frame)
    RESOLVE_METHOD(mad_frame_decode)
  END_METHOD_RESOLVE()
};

#endif
