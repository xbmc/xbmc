#pragma once
#include "ICodec.h"
#include "../../cores/DllLoader/dll.h"

#define MPC_DLL "Q:\\system\\players\\PAPlayer\\MPCCodec.dll"

// stuff from dll we need
#define FRAMELEN (36 * 32)
#include "mpc/in_mpc.h"

class MPCCodec : public ICodec
{
  struct MPCdll
  {
    bool (__cdecl * Open)(const char *, StreamInfo::BasicData *data, double *timeinseconds);
    void (__cdecl * Close)();
    int (__cdecl * Read)(float *, int);
    int (__cdecl * Seek)(double);
  };

public:
  MPCCodec();
  virtual ~MPCCodec();

  virtual bool Init(const CStdString &strFile);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool HandlesType(const char *type);
private:
  float sample_buffer[FRAMELEN * 2 * 2];
  // Our dll
  DllLoader *m_pDll;
  bool LoadDLL();
  bool m_bDllLoaded;
  MPCdll m_dll;
};
