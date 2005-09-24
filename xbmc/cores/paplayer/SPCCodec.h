#ifndef SPC_CODEC_H_
#define SPC_CODEC_H_

#include "ICodec.h"
#include "DllSnes9xApu.h"

class SPCCodec : public ICodec
{
public:
  SPCCodec();
  virtual ~SPCCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();

private:
  DllSnes9xApu m_dll;
  char* m_szBuffer;
  char* m_szStartOfBuffer; // never allocated
  int m_iDataInBuffer;
  int m_iBufferSize;
  int m_spc;
  __int64 m_iDataPos;
  CStdString m_strFile;
};

#endif