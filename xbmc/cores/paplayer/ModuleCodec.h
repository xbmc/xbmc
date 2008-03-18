#ifndef MODULE_CODEC_H_
#define MODULE_CODEC_H_

#include "ICodec.h"
#include "DllDumb.h"

class ModuleCodec : public ICodec
{
public:
  ModuleCodec();
  virtual ~ModuleCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();

  static bool IsSupportedFormat(const CStdString& strExt);

private:
  long m_iDataLen;
  int m_module;
  int m_renderID;
  int m_iFilePos;

  DllDumb m_dll;
};

#endif

