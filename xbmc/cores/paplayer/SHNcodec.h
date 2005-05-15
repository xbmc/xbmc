#pragma once
#include "ICodec.h"
#include "../../cores/DllLoader/dll.h"
#include "shn/shnplay.h"
#include "FileReader.h"

#define SHN_DLL "Q:\\system\\players\\PAPlayer\\libshnplay.dll"

struct ShnPlayFileStream {
	ShnPlayStream vtbl;
	CFileReader *file;
};

typedef struct ShnPlayFileStream ShnPlayFileStream;

class SHNCodec : public ICodec
{
  struct SHNdll
  {
    int (__cdecl * OpenStream)(ShnPlay ** pstate, ShnPlayStream * stream, unsigned int flags);
    int (__cdecl * Close)(ShnPlay * state);
    int (__cdecl * GetInfo)(ShnPlay * state, ShnPlayInfo * info);
    int (__cdecl * Read)(ShnPlay * state, void * buffer, int samples, int * samples_read);
    int (__cdecl * Seek)(ShnPlay * state, int position);
    const char * (__cdecl * ErrorMessage)(ShnPlay * state);
  };

public:
  SHNCodec();
  virtual ~SHNCodec();

  virtual bool Init(const CStdString &strFile);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool HandlesType(const char *type);
private:

  CFileReader m_fileSHN;
  ShnPlayFileStream m_stream;
  ShnPlay *m_handle;
  // Our dll
  DllLoader *m_pDll;
  bool LoadDLL();
  bool m_bDllLoaded;
  SHNdll m_dll;
};
