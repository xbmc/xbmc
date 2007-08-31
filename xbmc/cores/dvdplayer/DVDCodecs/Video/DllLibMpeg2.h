#pragma once
#include "../../../../DynamicDll.h"

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef signed __int64 int64_t;
typedef unsigned __int64 uint64_t;

#ifdef __cplusplus
extern "C"
{
#endif
 #include "libmpeg2/mpeg2.h"
 #include "libmpeg2/mpeg2convert.h"
#ifdef __cplusplus
}
#endif

class DllLibMpeg2Interface
{
public:
  virtual uint32_t mpeg2_accel (uint32_t accel)=0;
  virtual mpeg2dec_t * mpeg2_init (void)=0;
  virtual const mpeg2_info_t * mpeg2_info (mpeg2dec_t * mpeg2dec)=0;
  virtual void mpeg2_close (mpeg2dec_t * mpeg2dec)=0;
  virtual void mpeg2_buffer (mpeg2dec_t * mpeg2dec, uint8_t * start, uint8_t * end)=0;
  virtual mpeg2_state_t mpeg2_parse (mpeg2dec_t * mpeg2dec)=0;
  virtual void mpeg2_reset (mpeg2dec_t * mpeg2dec, int full_reset)=0;
  virtual void mpeg2_set_buf (mpeg2dec_t * mpeg2dec, uint8_t * buf[3], void * id)=0;
  virtual void mpeg2_custom_fbuf (mpeg2dec_t * mpeg2dec, int custom_fbuf)=0;
  virtual int mpeg2_convert (mpeg2dec_t * mpeg2dec, mpeg2_convert_t convert, void * arg)=0;
  virtual void mpeg2_skip(mpeg2dec_t * mpeg2dec, int skip)=0;
};

class DllLibMpeg2 : public DllDynamic, DllLibMpeg2Interface
{
  DECLARE_DLL_WRAPPER(DllLibMpeg2, Q:\\system\\players\\dvdplayer\\libmpeg2.dll)
  DEFINE_METHOD1(uint32_t, mpeg2_accel, (uint32_t p1))
  DEFINE_METHOD0(mpeg2dec_t *, mpeg2_init)
  DEFINE_METHOD1(const mpeg2_info_t *, mpeg2_info, (mpeg2dec_t * p1))
  DEFINE_METHOD1(void, mpeg2_close, (mpeg2dec_t * p1))
  DEFINE_METHOD3(void, mpeg2_buffer, (mpeg2dec_t * p1, uint8_t * p2, uint8_t * p3))
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
    RESOLVE_METHOD(mpeg2_parse)
    RESOLVE_METHOD(mpeg2_reset)
    RESOLVE_METHOD(mpeg2_set_buf)
    RESOLVE_METHOD(mpeg2_custom_fbuf)
    RESOLVE_METHOD(mpeg2_convert)
    RESOLVE_METHOD(mpeg2_skip)
  END_METHOD_RESOLVE()
};

