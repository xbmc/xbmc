#ifndef CUBE_CODEC_H_
#define CUBE_CODEC_H_

#include "ICodec.h"
#include "DllCube.h"

class CubeCodec : public ICodec
{
public:
  CubeCodec();
  virtual ~CubeCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();
  static bool IsSupportedFormat(const CStdString& strExt);

private:
  DllCube m_dll;
  int m_adx;
//  int m_iTrack;
  __int64 m_iDataPos;
};

#endif