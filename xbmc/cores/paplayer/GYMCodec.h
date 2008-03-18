#ifndef GYM_CODEC_H_
#define GYM_CODEC_H_

#include "ICodec.h"
#include "DllGensApu.h"

class GYMCodec : public ICodec
{
public:
  GYMCodec();
  virtual ~GYMCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();

private:
  DllGensApu m_dll;
  char* m_szBuffer;
  char* m_szStartOfBuffer; // never allocated
  int m_iDataInBuffer;
  int m_iBufferSize;
  int m_gym;
  __int64 m_iDataPos;
};

#endif

