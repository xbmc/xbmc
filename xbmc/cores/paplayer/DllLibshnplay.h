#pragma once
#include "../../DynamicDll.h"
#include "shn/shnplay.h"

// forward
class CFileReader;

struct ShnPlayFileStream {
	ShnPlayStream vtbl;
	CFileReader *file;
};

typedef struct ShnPlayFileStream ShnPlayFileStream;

class DllLibShnPlayInterface
{
public:
    virtual ~DllLibShnPlayInterface() {}
    virtual int OpenStream(ShnPlay ** pstate, ShnPlayStream * stream, unsigned int flags)=0;
    virtual int Close(ShnPlay * state)=0;
    virtual int GetInfo(ShnPlay * state, ShnPlayInfo * info)=0;
    virtual int Read(ShnPlay * state, void * buffer, int samples, int * samples_read)=0;
    virtual int Seek(ShnPlay * state, int position)=0;
    virtual const char * ErrorMessage(ShnPlay * state)=0;
};

class DllLibShnPlay : public DllDynamic, DllLibShnPlayInterface
{
  DECLARE_DLL_WRAPPER(DllLibShnPlay, Q:\\system\\players\\PAPlayer\\libshnplay.dll)
  DEFINE_METHOD3(int, OpenStream, (ShnPlay ** p1, ShnPlayStream * p2, unsigned int p3))
  DEFINE_METHOD1(int, Close, (ShnPlay * p1))
  DEFINE_METHOD2(int, GetInfo, (ShnPlay * p1, ShnPlayInfo * p2))
  DEFINE_METHOD4(int, Read, (ShnPlay * p1, void * p2, int p3, int * p4))
  DEFINE_METHOD2(int, Seek, (ShnPlay * p1, int p2))
  DEFINE_METHOD1(const char *, ErrorMessage, (ShnPlay * p1))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(ShnPlay_OpenStream, OpenStream)
    RESOLVE_METHOD_RENAME(ShnPlay_Close, Close)
    RESOLVE_METHOD_RENAME(ShnPlay_GetInfo, GetInfo)
    RESOLVE_METHOD_RENAME(ShnPlay_Read, Read)
    RESOLVE_METHOD_RENAME(ShnPlay_Seek, Seek)
    RESOLVE_METHOD_RENAME(ShnPlay_ErrorMessage, ErrorMessage)
  END_METHOD_RESOLVE()
};
