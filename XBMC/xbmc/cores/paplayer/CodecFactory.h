
#pragma once
#include "ICodec.h"

class CodecFactory
{
public:
  CodecFactory() {}
  virtual ~CodecFactory() {}

  static ICodec* CreateCodec(const CStdString& strFileType);
};