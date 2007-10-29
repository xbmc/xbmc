#pragma once

#include "ICodec.h"
#include "../../FileSystem/File.h"

class CachingCodec : public ICodec
{
public:
  virtual int GetCacheLevel(){ return m_file.GetCacheLevel(); };

protected:
  XFILE::CFile m_file;
};
