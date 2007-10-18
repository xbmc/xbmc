#ifndef WMA_CODEC_H_
#define WMA_CODEC_H_

#ifdef HAS_WMA_CODEC

#include "ICodec.h"
#include "FileSystem/File.h"

#include "DllWMA.h"

struct WMAInfo
{
  XFILE::CFile fileReader;
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
  
  DllWMA m_dll;
  void*  m_hnd;
};

#endif
#endif

