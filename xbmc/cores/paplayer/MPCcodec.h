#pragma once
#include "ICodec.h"
#include "FileSystem/File.h"
#include "DllMpcCodec.h"

class MPCCodec : public ICodec
{
public:
  MPCCodec();
  virtual ~MPCCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual int ReadSamples(float *pBuffer, int numsamples, int *actualsamples);
  virtual bool CanInit();
  virtual bool HasFloatData() const { return true; };
private:
  float m_sampleBuffer[FRAMELEN * 2 * 2];
  int m_sampleBufferSize;

  mpc_reader m_reader;
  mpc_decoder *m_handle;

  DllMPCCodec m_dll;
};
