#pragma once

#include "ICodec.h"
#include "FileReader.h"

class CachingCodec : public ICodec
{
public:
  virtual int GetCacheLevel(){ return m_file.GetCacheLevel(); };

protected:
  CFileReader m_file;
};
