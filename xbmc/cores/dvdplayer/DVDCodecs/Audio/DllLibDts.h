#pragma once
#include "../../../../DynamicDll.h"

#ifndef _LINUX
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8  uint8_t;
typedef __int32          int32_t;
typedef __int16          int16_t;
#endif

#include "libdts/dts.h"

#ifdef LIBDTS_DOUBLE
typedef float convert_t;
#else
typedef sample_t convert_t;
#endif


class DllLibDtsInterface
{
public:
  virtual ~DllLibDtsInterface() {} 
  virtual dts_state_t * dts_init (uint32_t mm_accel)=0;
  virtual int dts_syncinfo (dts_state_t *state, uint8_t * buf, int * flags, int * sample_rate, int * bit_rate, int *frame_length)=0;
  virtual int dts_frame (dts_state_t * state, uint8_t * buf, int * flags, level_t * level, sample_t bias)=0;
  virtual void dts_dynrng (dts_state_t * state, level_t (* call) (level_t, void *), void * data)=0;
  virtual int dts_blocks_num (dts_state_t * state)=0;
  virtual int dts_block (dts_state_t * state)=0;
  virtual sample_t * dts_samples (dts_state_t * state)=0;
  virtual void dts_free (dts_state_t * state)=0;
};

class DllLibDts : public DllDynamic, DllLibDtsInterface
{
#ifndef _LINUX
  DECLARE_DLL_WRAPPER(DllLibDts, Q:\\system\\players\\dvdplayer\\libdts.dll)
#else
  DECLARE_DLL_WRAPPER(DllLibDts, Q:\\system\\players\\dvdplayer\\libdts-i486-linux.so)
#endif
  DEFINE_METHOD1(dts_state_t *, dts_init, (uint32_t p1))
  DEFINE_METHOD6(int, dts_syncinfo, (dts_state_t *p1, uint8_t * p2, int * p3, int * p4, int * p5, int *p6))
  DEFINE_METHOD5(int, dts_frame, (dts_state_t * p1, uint8_t * p2, int * p3, level_t * p4, sample_t p5))
  DEFINE_METHOD3(void, dts_dynrng, (dts_state_t * p1, level_t (* p2) (level_t, void *), void * p3))
  DEFINE_METHOD1(int, dts_blocks_num ,(dts_state_t * p1))
  DEFINE_METHOD1(int, dts_block, (dts_state_t * p1))
  DEFINE_METHOD1(sample_t *, dts_samples, (dts_state_t * p1))
  DEFINE_METHOD1(void, dts_free, (dts_state_t * p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(dts_init)
    RESOLVE_METHOD(dts_syncinfo)
    RESOLVE_METHOD(dts_frame )
    RESOLVE_METHOD(dts_dynrng)
    RESOLVE_METHOD(dts_blocks_num)
    RESOLVE_METHOD(dts_block)
    RESOLVE_METHOD(dts_samples)
    RESOLVE_METHOD(dts_free)
  END_METHOD_RESOLVE()
};
