#ifndef ADPLUG_CODEC_H_
#define ADPLUG_CODEC_H_

#include "ICodec.h"
#include "DllAdplug.h"

class AdplugCodec : public ICodec
{
public:
  AdplugCodec();
  virtual ~AdplugCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();
  static bool IsSupportedFormat(const CStdString& strExt);

private:
  DllAdplug m_dll;
  int m_adl;
  int m_iTrack;
  __int64 m_iDataPos;
};

#endif