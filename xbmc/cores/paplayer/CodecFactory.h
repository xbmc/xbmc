
#pragma once
#include "ICodec.h"

class CodecFactory
{
public:
  CodecFactory() {}
  virtual ~CodecFactory() {}

  static ICodec* CreateCodec(const CStdString& strFileType);
  static ICodec* CreateCodecDemux(const CStdString& strFile, const CStdString& strContent,unsigned int filecache);
};

