#ifndef ASAP_CODEC_H_
#define ASAP_CODEC_H_

#include "ICodec.h"
#include "DllASAP.h"

class ASAPCodec : public ICodec
{
public:
  ASAPCodec();
  virtual ~ASAPCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();

  static bool IsSupportedFormat(const CStdString &strExt);

private:
  DllASAP m_dll;
};

#endif
