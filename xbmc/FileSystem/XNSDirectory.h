#pragma once
#include "directory.h"

using namespace DIRECTORY;
namespace DIRECTORY
{
  class CXNSDirectory :
    public IDirectory
  {
  public:
    CXNSDirectory(void);
    virtual ~CXNSDirectory(void);
    virtual bool  GetDirectory(const CStdString& strPath,VECFILEITEMS &items);
  };
}