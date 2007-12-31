#pragma once
#include "../../../../DynamicDll.h"
#include "libmad/mad.h"

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

class DllLibMad : public DllDynamic, DllLibMadInterface
{
#ifndef _LINUX
  DECLARE_DLL_WRAPPER(DllLibMad, Q:\\system\\players\\dvdplayer\\libmad.dll)
#elif defined(__APPLE__)
  DECLARE_DLL_WRAPPER(DllLibMad, Q:\\system\\players\\dvdplayer\\libmad-osx.so)
#else
  DECLARE_DLL_WRAPPER(DllLibMad, Q:\\system\\players\\dvdplayer\\libmad-i486-linux.so)
#endif
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
