#ifndef WMA_CODEC_H_
#define WMA_CODEC_H_

#ifdef HAS_WMA_CODEC

#include "ICodec.h"
#include "FileReader.h"

#ifdef _LINUX
#include "DllWMA.h"
#endif

struct WMAInfo
{
  CFileReader fileReader;
  char buffer[65536];
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
  
#ifndef _LINUX
  XWmaFileMediaObject* m_pWMA;                         
  WMAInfo m_info;
#else
  DllWMA m_dll;
  void*  m_hnd;
#endif
  char m_buffer[2048*2*6]; // max 5.1
  char* m_startOfBuffer; // not allocated
  DWORD m_iDataInBuffer;
};

#endif
#endif

