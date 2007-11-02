#pragma once
#include "../../../../DynamicDll.h"
#include "libfaad/faad.h"

class DllLibFaadInterface
{
public:
  virtual faacDecHandle faacDecOpen(void)=0;
  virtual faacDecConfigurationPtr faacDecGetCurrentConfiguration(faacDecHandle hDecoder)=0;
  virtual unsigned char faacDecSetConfiguration(faacDecHandle hDecoder, faacDecConfigurationPtr config)=0;
  virtual void faacDecClose(faacDecHandle hDecoder)=0;
  virtual void* faacDecDecode(faacDecHandle hDecoder, faacDecFrameInfo *hInfo, unsigned char *buffer, unsigned long buffer_size)=0;
  virtual long faacDecInit(faacDecHandle hDecoder, unsigned char *buffer, unsigned long buffer_size, unsigned long *samplerate, unsigned char *channels)=0;
  virtual char faacDecInit2(faacDecHandle hDecoder, unsigned char *pBuffer, unsigned long SizeOfDecoderSpecificInfo, unsigned long *samplerate, unsigned char *channels)=0;
  virtual char* faacDecGetErrorMessage(unsigned char errcode)=0;
  virtual void faacDecPostSeekReset(faacDecHandle hDecoder, long frame)=0;
};

class DllLibFaad : public DllDynamic, DllLibFaadInterface
{
  DECLARE_DLL_WRAPPER(DllLibFaad, Q:\\system\\players\\dvdplayer\\libfaad.dll)
  DEFINE_METHOD0(faacDecHandle, faacDecOpen)
  DEFINE_METHOD1(faacDecConfigurationPtr, faacDecGetCurrentConfiguration, (faacDecHandle p1))
  DEFINE_METHOD2(unsigned char, faacDecSetConfiguration, (faacDecHandle p1, faacDecConfigurationPtr p2))
  DEFINE_METHOD1(void, faacDecClose, (faacDecHandle p1))
  DEFINE_METHOD4(void*, faacDecDecode, (faacDecHandle p1, faacDecFrameInfo *p2, unsigned char *p3, unsigned long p4))
  DEFINE_METHOD5(long, faacDecInit, (faacDecHandle p1, unsigned char *p2, unsigned long p3, unsigned long *p4, unsigned char *p5))
  DEFINE_METHOD5(char, faacDecInit2, (faacDecHandle p1, unsigned char *p2, unsigned long p3, unsigned long *p4, unsigned char *p5))
  DEFINE_METHOD1(char*, faacDecGetErrorMessage, (unsigned char p1))
  DEFINE_METHOD2(void, faacDecPostSeekReset, (faacDecHandle p1, long p2))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(faacDecOpen)
    RESOLVE_METHOD(faacDecGetCurrentConfiguration)
    RESOLVE_METHOD(faacDecSetConfiguration)
    RESOLVE_METHOD(faacDecClose)
    RESOLVE_METHOD(faacDecDecode)
    RESOLVE_METHOD(faacDecInit)
    RESOLVE_METHOD(faacDecInit2)
    RESOLVE_METHOD(faacDecGetErrorMessage)
    RESOLVE_METHOD(faacDecPostSeekReset)
  END_METHOD_RESOLVE()
};
