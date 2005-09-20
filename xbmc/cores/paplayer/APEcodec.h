#pragma once
#include "ICodec.h"
#include "DllMACDll.h"

class APECodec : public ICodec
{
public:
  APECodec();
  virtual ~APECodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();

protected:
  int m_BytesPerBlock;

private:
  DllMACDll m_dll;
  APE_DECOMPRESS_HANDLE m_handle;
};
