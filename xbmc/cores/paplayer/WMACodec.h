#ifndef WMA_CODEC_H_
#define WMA_CODEC_H_

#include "ICodec.h"
#include "FileReader.h"

struct WMAInfo
{
  CFile fileReader;
  char buffer[16384];
  DWORD iStartOfBuffer;
};


class WMACodec : public ICodec
{
public:
  WMACodec();
  virtual ~WMACodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();
private:
  __int64 m_iDataPos;
  
  XWmaFileMediaObject* m_pWMA;                         
  WMAInfo m_info;
  char m_buffer[16384];
  char* m_startOfBuffer; // not allocated
  DWORD m_iDataInBuffer;
};

#endif