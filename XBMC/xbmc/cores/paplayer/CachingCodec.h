#pragma once

#include "ICodec.h"
#include "../../FileSystem/File.h"
#include "../../FileSystem/CacheStrategy.h"

class CachingCodec : public ICodec
{
public:
  virtual int GetCacheLevel(){ if(m_file.GetCache()) return m_file.GetCache()->GetCacheLevel();return -1; };

protected:
  XFILE::CFile m_file;
};
