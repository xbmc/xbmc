#pragma once

#include "directory.h"
using namespace DIRECTORY;
namespace DIRECTORY
{
  class CFactoryDirectory
  {
  public:
    CFactoryDirectory(void);
    virtual ~CFactoryDirectory(void);
    CDirectory* Create(const CStdString& strPath);
  };
}
