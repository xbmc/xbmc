#pragma once
#include "ICodec.h"
#include "FileReader.h"
#include "Dllmpccodec.h"

class MPCCodec : public ICodec
{
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

  DllMPCCodec m_dll;
};
