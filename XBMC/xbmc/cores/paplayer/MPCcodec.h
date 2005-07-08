#pragma once
#include "ICodec.h"
#include "../../cores/DllLoader/dll.h"
#include "FileReader.h"

// stuff from dll we need
#define FRAMELEN 1152

#include "mpc/MPCReader.h"
#include "mpc/in_mpc.h"

struct MpcPlayFileStream {
	MpcPlayStream vtbl;
	CFileReader *file;
};

class MPCCodec : public ICodec
{
  struct MPCdll
  {
    bool (__cdecl * Open)(MpcPlayState **state, MpcPlayStream *stream, StreamInfo::BasicData *data, double *timeinseconds);
    void (__cdecl * Close)(MpcPlayState *state);
    int (__cdecl * Read)(MpcPlayState *state, float *buffer, int size);
    int (__cdecl * Seek)(MpcPlayState *state, double timeinseconds);
  };

public:
  MPCCodec();
  virtual ~MPCCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();
private:
  float m_sampleBuffer[FRAMELEN * 2 * 2];
  int m_sampleBufferSize;

  CFileReader m_file;
  MpcPlayFileStream m_stream;
  MpcPlayState *m_handle;

  // Our dll
  bool LoadDLL();
  bool m_bDllLoaded;
  MPCdll m_dll;
};
