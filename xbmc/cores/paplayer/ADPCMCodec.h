#ifndef ADPCM_CODEC_H_
#define ADPCM_CODEC_H_

#include "ICodec.h"
#include "DllAdpcm.h"

class ADPCMCodec : public ICodec
{
public:
  ADPCMCodec();
  virtual ~ADPCMCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();

private:
  int m_adpcm;
  bool m_bIsPlaying;

  DllADPCM m_dll;
  __int64 m_iDataPos;
};

#endif

