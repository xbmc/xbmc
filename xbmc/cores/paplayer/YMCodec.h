#ifndef YM_CODEC_H_
#define YM_CODEC_H_

#include "ICodec.h"
#include "DllStSound.h"

class YMCodec : public ICodec
{
public:
  YMCodec();
  virtual ~YMCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();

private:
  DllStSound m_dll;
  int m_ym;
  __int64 m_iDataPos;
};

#endif

