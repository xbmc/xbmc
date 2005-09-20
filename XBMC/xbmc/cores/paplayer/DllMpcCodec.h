#pragma once
#include "../../DynamicDll.h"

#include "mpc/MPCReader.h"
#include "mpc/in_mpc.h"

// stuff from dll we need
#define FRAMELEN 1152

//  forward
class CFileReader;

struct MpcPlayFileStream {
	MpcPlayStream vtbl;
	CFileReader *file;
};

class DllMPCCodecInterface
{
public:
    virtual bool Open(MpcPlayState **state, MpcPlayStream *stream, StreamInfo::BasicData *data, double *timeinseconds)=0;
    virtual void Close(MpcPlayState *state)=0;
    virtual int Read(MpcPlayState *state, float *buffer, int size)=0;
    virtual int Seek(MpcPlayState *state, double timeinseconds)=0;
};

class DllMPCCodec : public DllDynamic, DllMPCCodecInterface
{
  DECLARE_DLL_WRAPPER(DllMPCCodec, Q:\\system\\players\\PAPlayer\\MPCcodec.dll)
  DEFINE_METHOD4(bool, Open, (MpcPlayState **p1, MpcPlayStream *p2, StreamInfo::BasicData *p3, double *p4))
  DEFINE_METHOD1(void, Close, (MpcPlayState *p1))
  DEFINE_METHOD3(int, Read, (MpcPlayState *p1, float *p2, int p3))
  DEFINE_METHOD2(int, Seek, (MpcPlayState *p1, double p2))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(Open)
    RESOLVE_METHOD(Close)
    RESOLVE_METHOD(Read)
    RESOLVE_METHOD(Seek)
  END_METHOD_RESOLVE()
};
