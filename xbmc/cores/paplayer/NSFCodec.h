#ifndef NSF_CODEC_H_
#define NSF_CODEC_H_

#include "ICodec.h"
#include "DllNosefart.h"

class NSFCodec : public ICodec
{
public:
  NSFCodec();
  virtual ~NSFCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();

private:
  int m_iTrack;
  int m_nsf;
  bool m_bIsPlaying;

  DllNosefart m_dll;
  char* m_szBuffer;
  char* m_szStartOfBuffer; // never allocated
  int m_iDataInBuffer;
  __int64 m_iDataPos;
};

#endif

